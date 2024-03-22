############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for qemu NXScope UDP streaming."""

import queue
import struct
import time

import pytest
from _descriptor_common import io_objid, protocol_config_value
from _ntfc_common import start_dawn

UdpDevice = pytest.importorskip(
    "nxslib.intf.udp",
    reason="nxslib is required for NXScope host tests",
).UdpDevice
NxscopeHandler = pytest.importorskip(
    "nxslib.nxscope",
    reason="nxslib is required for NXScope host tests",
).NxscopeHandler
Parser = pytest.importorskip(
    "nxslib.proto.parse",
    reason="nxslib is required for NXScope host tests",
).Parser

DESC_PATH = "descriptors/examples/qemu_nxscope_udp.yaml"
NXSCOPE_HOST = "192.168.8.104"
NXSCOPE_PORT = int(protocol_config_value(DESC_PATH, "nxscope_udp", "port"))
NXSCOPE_DUMMY_NOTIFY_ID = io_objid(DESC_PATH, "nx_dummy_notify1")
NXSCOPE_DUMMY_CHAN = 0
DAWN_NXSCOPE_SET_IO = 8


def _open_nxscope(timeout_s=5.0):
    deadline = time.monotonic() + timeout_s
    last_exc = None

    while time.monotonic() < deadline:
        try:
            nxscope = NxscopeHandler(
                UdpDevice(NXSCOPE_HOST, NXSCOPE_PORT, timeout=1.0),
                Parser(),
            )
            nxscope.connect()
            return nxscope
        except Exception as exc:
            last_exc = exc
            time.sleep(0.1)

    pytest.fail(
        "Could not connect NXScope UDP host on "
        f"{NXSCOPE_HOST}:{NXSCOPE_PORT}: {last_exc!r}"
    )


def _wait_for_nxscope_value(subq, expected, timeout_s=2.0):
    deadline = time.monotonic() + timeout_s
    seen = []

    while time.monotonic() < deadline:
        left = deadline - time.monotonic()
        try:
            samples = subq.get(block=True, timeout=min(0.2, left))
        except queue.Empty:
            continue

        for sample in samples:
            if len(sample.data) < 1:
                continue
            first = sample.data[0]
            scalar = first[0] if hasattr(first, "__len__") else first
            value = int(scalar) & 0xFFFFFFFF
            seen.append(value)
            if expected is None or value == expected:
                return value

    pytest.fail(
        f"Timeout waiting for NXScope value {expected}, " f"received: {seen[-10:]}"
    )


def _drain_nxscope_queue(subq):
    while True:
        try:
            subq.get_nowait()
        except queue.Empty:
            return


def _assert_no_nxscope_samples(subq, timeout_s=0.8):
    try:
        subq.get(block=True, timeout=timeout_s)
    except queue.Empty:
        return

    pytest.fail("Unexpected NXScope samples while disabled")


def _configure_dummy_channel(nxscope, chan):
    nxscope.channels_default_cfg()
    nxscope.ch_enable(chan)
    nxscope.ch_divider(chan, 1)


def _nxscope_set_u32(nxscope, objid, value):
    payload = struct.pack("<IH", objid, 4) + struct.pack("<I", value)
    ack = nxscope.send_user_frame(DAWN_NXSCOPE_SET_IO, payload)
    assert ack.state is True


class TestNxscopeUdp:
    """Validate NXScope UDP stream control and Dawn set-IO extension."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NXSCOPE"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NXSCOPE_UDP"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_DUMMY_NOTIFY"),
    ]

    def test_nxscope_udp_stream_with_set_io(self):
        """Ensure UDP stream is active and writable through user frames."""

        start_dawn(settle_s=0.3)
        nxscope = _open_nxscope()
        subq = None

        try:
            _configure_dummy_channel(nxscope, NXSCOPE_DUMMY_CHAN)
            subq = nxscope.stream_sub(NXSCOPE_DUMMY_CHAN)
            nxscope.stream_start()
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            _drain_nxscope_queue(subq)
            nxscope.stream_stop()
            _assert_no_nxscope_samples(subq, timeout_s=0.8)
            nxscope.stream_start()
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            _drain_nxscope_queue(subq)
            nxscope.ch_disable(NXSCOPE_DUMMY_CHAN, writenow=True)
            assert nxscope.dev_channel_get(NXSCOPE_DUMMY_CHAN).data.en is False
            _assert_no_nxscope_samples(subq, timeout_s=0.8)

            nxscope.ch_enable(NXSCOPE_DUMMY_CHAN, writenow=True)
            nxscope.ch_divider(NXSCOPE_DUMMY_CHAN, 1, writenow=True)
            assert nxscope.dev_channel_get(NXSCOPE_DUMMY_CHAN).data.en is True
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            _nxscope_set_u32(nxscope, NXSCOPE_DUMMY_NOTIFY_ID, 0x11)
            sampled = _wait_for_nxscope_value(
                subq,
                expected=0x11,
                timeout_s=2.0,
            )
            assert sampled == 0x11

            _nxscope_set_u32(nxscope, NXSCOPE_DUMMY_NOTIFY_ID, 0x77)
            sampled = _wait_for_nxscope_value(
                subq,
                expected=0x77,
                timeout_s=2.0,
            )
            assert sampled == 0x77
        finally:
            if subq is not None:
                nxscope.stream_unsub(subq)
            nxscope.disconnect()
