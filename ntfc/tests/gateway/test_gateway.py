############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for Serial/CAN gateway program data flow."""

import struct
import time
from contextlib import contextmanager

import pytest
from _can_common import (
    assert_can_bus_idle,
    open_can_socket,
    read_u64_via_can_seg,
    recv_frame_filtered,
)
from _descriptor_common import can_layout, io_objid, protocol_config_value
from _ntfc_common import start_dawn
from dawnpy_can.can import build_isotp_frames, create_can_frame
from dawnpy_serial.serial import DawnSerialProtocol

DESC_PATH = "descriptors/examples/gateway_serial_can.yaml"
# Descriptor path is the device-side simulator endpoint. The host-side NTFC
# client must use the paired PTY exposed by sim.
SERIAL_PORT = "/tmp/ttyNX0"
SERIAL_BAUD = int(protocol_config_value(DESC_PATH, "serial", "baudrate"))
CAN_LAYOUT = can_layout(DESC_PATH)
CAN_READ_START = CAN_LAYOUT["read"]["gw_can_rx_virt"]
CAN_WRITE_START = CAN_LAYOUT["write"]["gw_can_tx_virt"]
CAN_READ_SEG_START = CAN_LAYOUT["read_seg"]["gw_can_u64_rx_virt"]
CAN_WRITE_SEG_START = CAN_LAYOUT["write_seg"]["gw_can_u64_tx_virt"]
CAN_NODE_ID = int(protocol_config_value(DESC_PATH, "can", "node_id"))
SERIAL_READ_OBJID = io_objid(DESC_PATH, "gw_serial_virt")
SERIAL_WRITE_OBJID = io_objid(DESC_PATH, "gw_serial_rx_virt")
SERIAL_READ_U64_OBJID = io_objid(DESC_PATH, "gw_serial_u64_virt")
SERIAL_WRITE_U64_OBJID = io_objid(DESC_PATH, "gw_serial_u64_rx_virt")


@pytest.fixture(scope="module", autouse=True)
def _require_idle_can_bus():
    assert_can_bus_idle()


@contextmanager
def open_serial_client():
    client = DawnSerialProtocol(SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=1.5)
    for _ in range(20):
        if client.connect():
            for _ in range(20):
                if client.ping():
                    break
                time.sleep(0.1)
            else:
                client.disconnect()
                time.sleep(0.2)
                continue
            try:
                yield client
            finally:
                client.disconnect()
            return
        time.sleep(0.2)
    pytest.fail(f"Could not connect to serial port {SERIAL_PORT}")


class TestGateway:
    """Validate gateway forwarding for scalar and segmented U64 data."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROG_GATEWAY"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN"),
    ]

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
    def test_gateway_serial_to_can(self):
        """Write via serial and read the same value through CAN RTR."""

        start_dawn(settle_s=0.3)

        with open_serial_client() as serial_client, open_can_socket() as can_sock:
            test_value = 0x12345678
            assert serial_client.write_io(
                SERIAL_READ_OBJID, struct.pack("<I", test_value)
            )

            time.sleep(0.1)

            can_id = CAN_NODE_ID + CAN_READ_START
            can_sock.send(create_can_frame(can_id, data=None, extended=True, rtr=True))
            parsed = recv_frame_filtered(can_sock, can_id)
            assert parsed is not None, "No CAN read response for gateway value"
            assert parsed["dlc"] == 4
            can_value = struct.unpack("<I", parsed["data"][:4])[0]
            assert can_value == test_value

    def test_gateway_can_to_serial(self):
        """Write via CAN and read the same value through serial IO."""

        start_dawn(settle_s=0.3)

        with open_serial_client() as serial_client, open_can_socket() as can_sock:
            test_value = 0x01020304
            can_id = CAN_NODE_ID + CAN_WRITE_START
            can_sock.send(
                create_can_frame(
                    can_id,
                    struct.pack("<I", test_value),
                    extended=True,
                )
            )

            time.sleep(0.1)

            data = serial_client.read_io(SERIAL_WRITE_OBJID)
            assert data is not None
            assert len(data) >= 4
            serial_value = struct.unpack("<I", data[:4])[0]
            assert serial_value == test_value

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_gateway_serial_u64_to_can_segmented(self):
        """Write U64 via serial and read segmented U64 from CAN."""

        start_dawn(settle_s=0.3)

        with open_serial_client() as serial_client, open_can_socket() as can_sock:
            test_value = 0x0102030405060708
            assert serial_client.write_io(
                SERIAL_READ_U64_OBJID, struct.pack("<Q", test_value)
            )
            time.sleep(0.1)

            can_value = read_u64_via_can_seg(can_sock, CAN_NODE_ID, CAN_READ_SEG_START)
            assert (
                can_value is not None
            ), "No segmented CAN response for u64 gateway value"
            assert can_value == test_value

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_gateway_can_segmented_u64_to_serial(self):
        """Write segmented U64 via CAN and read the value from serial."""

        start_dawn(settle_s=0.3)

        with open_serial_client() as serial_client, open_can_socket() as can_sock:
            test_value = 0x8877665544332211
            can_id = CAN_NODE_ID + CAN_WRITE_SEG_START
            payload = struct.pack("<Q", test_value)
            for frame in build_isotp_frames(can_id, payload, extended=True):
                can_sock.send(frame)
                time.sleep(0.02)
            time.sleep(0.1)

            data = serial_client.read_io(SERIAL_WRITE_U64_OBJID)
            assert data is not None
            assert len(data) >= 8
            serial_value = struct.unpack("<Q", data[:8])[0]
            assert serial_value == test_value
