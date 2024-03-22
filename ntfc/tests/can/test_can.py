############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN protocol read/write/push paths."""

import socket
import struct
import time

import pytest
from _can_common import (
    assert_can_bus_idle,
    drain_frames,
    open_can_socket,
    recv_frame_filtered,
)
from _descriptor_common import can_layout, io_objid
from _ntfc_common import shell_get_io_scalar, start_dawn
from dawnpy_can.can import (
    build_indexed_request,
    build_isotp_frames,
    create_can_frame,
    parse_can_frame,
)

DESC_PATH = "descriptors/ntfc/ntfc_can_dummy.yaml"
CAN_LAYOUT = can_layout(DESC_PATH)

PUSH_START = CAN_LAYOUT["push"]["can_timestamp_io1"]
WRITE_SIMPLE_START = CAN_LAYOUT["write"]["can_dummyio1"]
READ_SIMPLE_START2 = CAN_LAYOUT["read"]["can_dummyio5"]
READ_SEG_START = CAN_LAYOUT["read_seg"]["can_dummyio8"]
READ_SEG_DESCRIPTOR = CAN_LAYOUT["read_seg"]["descriptor1"]
READ_SEG_FILEIO = CAN_LAYOUT["read_seg"]["can_fileio1"]
WRITE_SEG_START = CAN_LAYOUT["write_seg"]["can_dummyio8"]
WRITE_SEG_FILEIO = CAN_LAYOUT["write_seg"]["can_fileio1"]
READ_INDEXED_START = CAN_LAYOUT["read_indexed"]["can_dummyio1"]
WRITE_INDEXED_START = CAN_LAYOUT["write_indexed"]["can_dummyio1"]
WRITE_INDEX_DUMMY2 = (
    CAN_LAYOUT["write_indexed"]["can_dummyio2"] - WRITE_INDEXED_START + 1
)
WRITE_INDEX_DUMMY3 = (
    CAN_LAYOUT["write_indexed"]["can_dummyio3"] - WRITE_INDEXED_START + 1
)
READ_INDEX_DUMMY1 = CAN_LAYOUT["read_indexed"]["can_dummyio1"] - READ_INDEXED_START + 1

DUMMY1_ID = io_objid(DESC_PATH, "can_dummyio1")
DUMMY2_ID = io_objid(DESC_PATH, "can_dummyio2")
DUMMY3_ID = io_objid(DESC_PATH, "can_dummyio3")
DUMMY5_ID = io_objid(DESC_PATH, "can_dummyio5")
DUMMY8_ID = io_objid(DESC_PATH, "can_dummyio8")
DESCRIPTOR1_ID = io_objid(DESC_PATH, "descriptor1")


@pytest.fixture(scope="module", autouse=True)
def _require_idle_can_bus():
    assert_can_bus_idle()


def recv_segmented_payload(sock, expected_can_id, max_attempts=20):
    """Receive segmented CAN payload bytes for a specific CAN ID."""
    payload = bytearray()
    expected_seg = 0

    for _ in range(max_attempts):
        parsed = recv_frame_filtered(sock, expected_can_id)
        if parsed is None:
            break

        assert parsed["dlc"] >= 1, "Segmented frame missing segment header"

        seg = parsed["data"][0]
        seg_no = seg & 0x7F
        assert (
            seg_no == expected_seg
        ), f"Unexpected seg seq {seg_no}, expected {expected_seg}"

        payload.extend(parsed["data"][1 : parsed["dlc"]])
        expected_seg += 1

        if seg & 0x80:
            return bytes(payload)

    return None


def get_runtime_node_id(sock, timeout_s=2.5):
    """Discover CAN node ID from a timestamp push notification.

    The timestamp IO periodically emits 8-byte push frames at
    ``node_id + PUSH_START``, so the first such frame reveals node_id.
    Rediscovered per call to keep tests independent of each other.
    """
    old_timeout = sock.gettimeout()
    sock.settimeout(0.25)
    deadline = time.monotonic() + timeout_s

    try:
        while time.monotonic() < deadline:
            try:
                frame = sock.recv(16)
            except socket.timeout:
                continue

            parsed = parse_can_frame(frame)
            if parsed.get("rtr"):
                continue
            if parsed.get("dlc") != 8:
                continue

            return parsed["can_id"] - PUSH_START
    finally:
        sock.settimeout(old_timeout)

    pytest.fail("Could not discover CAN node ID from push notifications")


class TestCan:
    """Validate CAN simple, segmented, indexed, push, and FileIO behavior."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN"),
    ]

    def test_can_simple_write_basic(self):
        """Test basic CAN write."""
        product = start_dawn()

        with open_can_socket() as s:
            node_id = get_runtime_node_id(s)

            can_id = node_id + WRITE_SIMPLE_START + 0
            s.send(create_can_frame(can_id, b"\x01", extended=True))
            time.sleep(0.2)

            assert shell_get_io_scalar(product, DUMMY1_ID) == 1

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
    def test_can_simple_read_rtr(self):
        """Test RTR (Remote Transmission Request) read."""
        product = start_dawn()

        with open_can_socket() as s:
            drain_frames(s)
            node_id = get_runtime_node_id(s)

            shell_value = shell_get_io_scalar(product, DUMMY5_ID)

            can_id = node_id + READ_SIMPLE_START2 + 0
            s.send(create_can_frame(can_id, data=None, extended=True, rtr=True))

            parsed = recv_frame_filtered(s, can_id)
            assert parsed is not None, "Did not receive RTR response"
            assert not parsed["rtr"], "Response should not be RTR"
            assert parsed["dlc"] == 2, f"Expected DLC=2, got {parsed['dlc']}"

            can_value = struct.unpack("<H", parsed["data"][:2])[0]
            assert (
                can_value == shell_value
            ), f"CAN value {can_value} != shell value {shell_value}"

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_can_segmented_write_isotp(self):
        """Test ISO-TP segmented write (8 bytes)."""
        product = start_dawn()

        with open_can_socket() as s:
            node_id = get_runtime_node_id(s)

            can_id = node_id + WRITE_SEG_START
            test_value = 0x0807060504030201
            for frame in build_isotp_frames(
                can_id, struct.pack("<Q", test_value), extended=True
            ):
                s.send(frame)
                time.sleep(0.05)
            time.sleep(0.2)

            assert shell_get_io_scalar(product, DUMMY8_ID) == test_value

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_can_segmented_read_isotp(self):
        """Test segmented read (8 bytes) using a non-RTR empty request frame."""
        product = start_dawn()

        with open_can_socket() as s:
            drain_frames(s)
            node_id = get_runtime_node_id(s)

            shell_value = shell_get_io_scalar(product, DUMMY8_ID)

            can_id = node_id + READ_SEG_START
            s.send(create_can_frame(can_id, b"", extended=True))

            frames = [recv_frame_filtered(s, can_id) for _ in range(2)]
            assert all(
                f is not None for f in frames
            ), f"Expected 2 segmented frames, got {frames}"

            data = (
                frames[0]["data"][1 : frames[0]["dlc"]]
                + frames[1]["data"][1 : frames[1]["dlc"]]
            )
            can_value = struct.unpack("<Q", data)[0]
            assert (
                can_value == shell_value
            ), f"CAN value {can_value} != shell value {shell_value}"

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_can_segmented_read_descriptor(self):
        """Test segmented read of DescriptorIO via READ_SEG second binding."""
        product = start_dawn()

        with open_can_socket() as s:
            drain_frames(s)
            node_id = get_runtime_node_id(s)

            # Ensure descriptor IO exists on the target before probing CAN.
            ret = product.sendCommandReadUntilPattern(
                f"getio 0x{DESCRIPTOR1_ID:08x}", timeout=1
            )
            if "failed" in ret.output.lower() or "not found" in ret.output.lower():
                pytest.skip(f"DescriptorIO not available: {ret.output}")

            can_id = node_id + READ_SEG_DESCRIPTOR
            s.send(create_can_frame(can_id, b"", extended=True))

            payload = bytearray()
            seq = 0
            got_last = False

            # Descriptor payload grows as project capabilities/metadata evolve.
            # Keep a generous cap so the test validates framing semantics instead
            # of a historical descriptor size.
            for _ in range(512):
                parsed = recv_frame_filtered(s, can_id, attempts=20)
                if parsed is None:
                    break

                assert parsed["dlc"] >= 2, "Segmented response frame too short"
                seg = parsed["data"][0]
                assert (
                    seg & 0x7F
                ) == seq, f"Unexpected seg seq {seg & 0x7F}, expected {seq}"

                payload.extend(parsed["data"][1 : parsed["dlc"]])

                if seg & 0x80:
                    got_last = True
                    break

                seq += 1

            assert got_last, "Did not receive last segmented frame for descriptor"
            assert len(payload) > 8, f"Descriptor payload too short: {len(payload)}"
            assert any(
                b != 0 for b in payload[: min(len(payload), 32)]
            ), "Descriptor payload appears empty"

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SINGLE_ID")
    def test_can_indexed_write(self):
        """Test indexed write (one CAN ID, multiple IOs)."""
        product = start_dawn()

        with open_can_socket() as s:
            node_id = get_runtime_node_id(s)
            can_id = node_id + WRITE_INDEXED_START

            s.send(
                create_can_frame(
                    can_id, bytes([0x80, WRITE_INDEX_DUMMY2, 0x01]), extended=True
                )
            )
            time.sleep(0.2)
            assert shell_get_io_scalar(product, DUMMY2_ID) == 1

            s.send(
                create_can_frame(
                    can_id, bytes([0x80, WRITE_INDEX_DUMMY3, 0x00]), extended=True
                )
            )
            time.sleep(0.2)
            assert shell_get_io_scalar(product, DUMMY3_ID) == 0

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SINGLE_ID")
    def test_can_indexed_read(self):
        """Test indexed read (one CAN ID, multiple IOs)."""
        product = start_dawn()

        with open_can_socket() as s:
            drain_frames(s)
            node_id = get_runtime_node_id(s)

            shell_value = shell_get_io_scalar(product, DUMMY1_ID)

            can_id = node_id + READ_INDEXED_START
            s.send(
                build_indexed_request(
                    can_id, index=READ_INDEX_DUMMY1, extended=True, seg=0x80
                )
            )

            parsed = recv_frame_filtered(s, can_id)
            assert parsed is not None, "Did not receive indexed read response"

            index = parsed["data"][1]
            value = parsed["data"][2]
            assert (
                index == READ_INDEX_DUMMY1
            ), f"Expected index={READ_INDEX_DUMMY1}, got {index}"
            assert (
                value == shell_value
            ), f"CAN value {value} != shell value {shell_value}"

    def test_can_push_notification(self):
        """Test automatic push notifications (timestamp every 1 second)."""
        start_dawn()

        with open_can_socket(timeout=2.0) as s:
            drain_frames(s)
            node_id = get_runtime_node_id(s)

            expected_can_id = node_id + PUSH_START
            parsed = recv_frame_filtered(s, expected_can_id, attempts=10)
            assert parsed is not None, "Did not receive push notification"
            assert parsed["dlc"] == 8, f"Expected DLC=8, got {parsed['dlc']}"
            timestamp = struct.unpack("<Q", parsed["data"][:8])[0]
            assert timestamp > 0, "Expected positive timestamp"

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_can_fileio_segmented_read_write(self):
        """Test segmented CAN FileIO roundtrip."""
        start_dawn()

        with open_can_socket() as s:
            node_id = get_runtime_node_id(s)

            payload = b"can-fileio-segmented"
            can_id = node_id + WRITE_SEG_FILEIO
            for frame in build_isotp_frames(can_id, payload, extended=True):
                s.send(frame)
                time.sleep(0.05)
            time.sleep(0.2)

            can_id = node_id + READ_SEG_FILEIO
            s.send(create_can_frame(can_id, b"", extended=True))

            readback = recv_segmented_payload(s, can_id)
            assert readback == payload
