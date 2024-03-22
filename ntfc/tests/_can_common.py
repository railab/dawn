############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

"""Shared CAN helpers for Dawn NTFC integration tests.

Consolidates CAN primitives previously duplicated across can/,
programs_can/, and gateway/ test suites:
- socket setup (plain + context manager)
- frame receive with can_id filter and deadline semantics
- drain utility that clears pending frames without blocking
- typed RTR read helpers (u8/u32/f32) and scalar write helpers
- segmented (ISO-TP) read/write helpers for u64 and arbitrary bytes
- generic wait helpers used by programs_can flow control tests

Test-suite-specific pieces (node-id discovery policies, descriptor
constants, push-notification parsing) remain in their respective
modules because they encode per-config semantics.
"""

import socket
import struct
import time
from contextlib import contextmanager

import pytest
from dawnpy_can.can import build_isotp_frames, create_can_frame, parse_can_frame

CAN_IFNAME_DEFAULT = "can0"
CAN_POLL_INTERVAL_S = 0.02
CAN_WRITE_SETTLE_S = 0.1
CAN_IDLE_CHECK_S = 0.3
CAN_EFF_MASK = 0x1FFFFFFF
CAN_ID_FILTER_MASK = CAN_EFF_MASK | socket.CAN_EFF_FLAG


def setup_can_socket(ifname=CAN_IFNAME_DEFAULT, timeout=1.0):
    """Return a bound SocketCAN raw socket with the given timeout."""
    sock = socket.socket(socket.AF_CAN, socket.SOCK_RAW, socket.CAN_RAW)
    sock.bind((ifname,))
    sock.settimeout(timeout)
    return sock


def assert_can_bus_idle(ifname=CAN_IFNAME_DEFAULT, idle_s=CAN_IDLE_CHECK_S):
    """Fail early if host-side CAN traffic is present before Dawn starts."""
    try:
        sock = setup_can_socket(ifname=ifname, timeout=idle_s)
    except OSError as exc:
        pytest.fail(f"Could not open CAN interface {ifname}: {exc}")

    try:
        frame = sock.recv(16)
    except socket.timeout:
        return
    finally:
        sock.close()

    parsed = parse_can_frame(frame)
    pytest.fail(
        f"CAN interface {ifname} is not idle before starting Dawn: "
        f"saw frame can_id=0x{parsed['can_id']:x} dlc={parsed['dlc']} "
        f"rtr={int(parsed['rtr'])} extended={int(parsed['extended'])}. "
        "Stop leftover CAN traffic first, for example stale NuttX simulator, "
        "canplayer, cansend, or bridge processes."
    )


@contextmanager
def open_can_socket(ifname=CAN_IFNAME_DEFAULT, timeout=1.0):
    """Context-managed SocketCAN socket; closes on exit."""
    sock = setup_can_socket(ifname=ifname, timeout=timeout)
    try:
        yield sock
    finally:
        sock.close()


def drain_frames(sock, max_frames=30, quiet_timeout=0.05):
    """Drop up to ``max_frames`` pending frames without blocking.

    Restores the original socket timeout on return.
    """
    old_timeout = sock.gettimeout()
    sock.settimeout(quiet_timeout)
    try:
        for _ in range(max_frames):
            try:
                sock.recv(16)
            except socket.timeout:
                break
    finally:
        sock.settimeout(old_timeout)


@contextmanager
def filter_can_id(sock, can_id):
    """Temporarily receive only extended frames for ``can_id``."""
    filt = struct.pack("=II", can_id | socket.CAN_EFF_FLAG, CAN_ID_FILTER_MASK)
    receive_all = struct.pack("=II", 0, 0)

    sock.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_FILTER, filt)
    try:
        drain_frames(sock)
        yield
    finally:
        sock.setsockopt(socket.SOL_CAN_RAW, socket.CAN_RAW_FILTER, receive_all)


def recv_frame_filtered(sock, expected_can_id, attempts=10):
    """Read frames until one matches ``expected_can_id`` or deadline expires.

    Deadline is ``max(0.2, 0.05 * attempts)`` to keep behavior close to
    the programs_can variant which survives short socket timeouts.
    """
    deadline = time.monotonic() + max(0.2, 0.05 * attempts)
    while time.monotonic() < deadline:
        try:
            frame = sock.recv(16)
        except socket.timeout:
            continue
        parsed = parse_can_frame(frame)
        if parsed["can_id"] == expected_can_id:
            return parsed
    return None


def _read_rtr_payload(sock, can_id, min_len=4, rtr_attempts=4):
    """Send an RTR request and return the response payload (up to ``dlc``).

    Retries up to ``rtr_attempts`` times for slow targets.
    """
    with filter_can_id(sock, can_id):
        for _ in range(rtr_attempts):
            sock.send(create_can_frame(can_id, data=None, extended=True, rtr=True))
            deadline = time.monotonic() + 0.5
            while time.monotonic() < deadline:
                parsed = recv_frame_filtered(sock, can_id, attempts=1)
                if not parsed:
                    continue
                if parsed.get("rtr"):
                    continue
                if parsed.get("dlc", 0) < min_len:
                    continue
                return parsed["data"][: parsed["dlc"]]
    return None


def read_u8_via_can_rtr(sock, node_id, can_offset):
    payload = _read_rtr_payload(sock, node_id + can_offset, min_len=1)
    if payload is None:
        return None
    return struct.unpack("<B", payload[:1])[0]


def read_u32_via_can_rtr(sock, node_id, can_offset):
    payload = _read_rtr_payload(sock, node_id + can_offset, min_len=4)
    if payload is None:
        return None
    return struct.unpack("<I", payload[:4])[0]


def read_f32_via_can_rtr(sock, node_id, can_offset):
    payload = _read_rtr_payload(sock, node_id + can_offset, min_len=4)
    if payload is None:
        return None
    return struct.unpack("<f", payload[:4])[0]


def write_u8_via_can(sock, node_id, can_offset, value):
    sock.send(
        create_can_frame(node_id + can_offset, struct.pack("<B", value), extended=True)
    )
    time.sleep(CAN_WRITE_SETTLE_S)


def write_u32_via_can(sock, node_id, can_offset, value):
    sock.send(
        create_can_frame(node_id + can_offset, struct.pack("<I", value), extended=True)
    )
    time.sleep(CAN_WRITE_SETTLE_S)


def write_f32_via_can(sock, node_id, can_offset, value):
    sock.send(
        create_can_frame(node_id + can_offset, struct.pack("<f", value), extended=True)
    )
    time.sleep(CAN_WRITE_SETTLE_S)


def _collect_two_seg_frames(sock, can_id, timeout_s=1.2, inner_timeout_s=0.25):
    """Request segmented payload repeatedly until exactly two frames arrive."""
    frames = []
    deadline = time.monotonic() + timeout_s
    with filter_can_id(sock, can_id):
        while time.monotonic() < deadline and len(frames) < 2:
            sock.send(create_can_frame(can_id, b"", extended=True))
            t_end = time.monotonic() + inner_timeout_s
            while time.monotonic() < t_end and len(frames) < 2:
                try:
                    parsed = parse_can_frame(sock.recv(16))
                except socket.timeout:
                    continue
                if parsed["can_id"] != can_id:
                    continue
                frames.append(parsed)
    return frames if len(frames) == 2 else None


def read_u64_via_can_seg(sock, node_id, can_offset):
    """Read a 2-frame segmented u64 payload; returns ``int`` or ``None``."""
    can_id = node_id + can_offset
    frames = _collect_two_seg_frames(sock, can_id)
    if frames is None:
        return None

    payload = (
        frames[0]["data"][1 : frames[0]["dlc"]]
        + frames[1]["data"][1 : frames[1]["dlc"]]
    )
    if len(payload) < 8:
        return None
    return struct.unpack("<Q", payload[:8])[0]


def write_u64_via_can_seg(sock, node_id, can_offset, value):
    """Write a u64 using ISO-TP segmented frames."""
    can_id = node_id + can_offset
    for frame in build_isotp_frames(can_id, struct.pack("<Q", value), extended=True):
        sock.send(frame)
        time.sleep(0.02)


def _collect_seg_frame(sock, can_id, assembled):
    try:
        parsed = parse_can_frame(sock.recv(16))
    except socket.timeout:
        return None

    if parsed["can_id"] != can_id:
        return None
    if parsed.get("rtr"):
        return None
    if parsed.get("dlc", 0) == 0:
        return None

    seg = parsed["data"][0]
    idx = seg & 0x7F
    is_last = (seg & 0x80) != 0
    assembled[idx] = parsed["data"][1 : parsed["dlc"]]
    return idx if is_last else None


def _try_assemble_seg_payload(assembled, last_idx, min_len):
    if last_idx is None:
        return False, None
    if not all(i in assembled for i in range(last_idx + 1)):
        return False, None
    payload = b"".join(assembled[i] for i in range(last_idx + 1))
    if len(payload) >= min_len:
        return True, payload
    return True, None


def read_bytes_via_can_seg(sock, node_id, can_offset, min_len=1, timeout_s=1.2):
    """Read an arbitrary-length segmented payload; returns bytes or ``None``."""
    can_id = node_id + can_offset
    assembled = {}
    last_idx = None
    deadline = time.monotonic() + timeout_s

    with filter_can_id(sock, can_id):
        while time.monotonic() < deadline:
            sock.send(create_can_frame(can_id, b"", extended=True))
            t_end = time.monotonic() + 0.25
            while time.monotonic() < t_end:
                new_last = _collect_seg_frame(sock, can_id, assembled)
                if new_last is not None:
                    last_idx = new_last
                done, payload = _try_assemble_seg_payload(assembled, last_idx, min_len)
                if done:
                    return payload
    return None


def wait_for_value(
    read_fn, sock, node_id, can_offset, expected, timeout_s=2.5, tol=None
):
    """Poll ``read_fn`` until observed value matches ``expected``."""
    deadline = time.monotonic() + timeout_s
    observed = None
    while time.monotonic() < deadline:
        observed = read_fn(sock, node_id, can_offset)
        if tol is None:
            if observed == expected:
                return observed
        elif observed is not None and abs(observed - expected) <= tol:
            return observed
        time.sleep(CAN_POLL_INTERVAL_S)
    return observed


def wait_for_non_none(read_fn, sock, node_id, can_offset, timeout_s=4.0):
    """Poll ``read_fn`` until a non-None value is returned."""
    deadline = time.monotonic() + timeout_s
    observed = None
    while time.monotonic() < deadline:
        observed = read_fn(sock, node_id, can_offset)
        if observed is not None:
            return observed
        time.sleep(CAN_POLL_INTERVAL_S)
    return observed


def wait_until(
    read_fn,
    sock,
    node_id,
    can_offset,
    predicate,
    timeout_s=3.0,
    step_s=CAN_POLL_INTERVAL_S,
):
    """Poll ``read_fn`` until ``predicate(observed)`` is true or deadline hits.

    Returns the last observed value regardless of match, mirroring the
    existing ``wait_for_*`` convention so callers can inspect what actually
    arrived on timeout.
    """
    deadline = time.monotonic() + timeout_s
    observed = None
    while time.monotonic() < deadline:
        observed = read_fn(sock, node_id, can_offset)
        if predicate(observed):
            return observed
        time.sleep(step_s)
    return observed
