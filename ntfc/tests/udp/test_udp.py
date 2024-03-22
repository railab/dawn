############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for UDP protocol IO and notification paths."""

import struct

import pytest
from _descriptor_common import io_objid, protocol_config_value
from _ntfc_common import start_dawn
from _proto_client_common import (
    exchange_status,
    exchange_write,
    wait_for_notification,
)
from dawnpy_udp.udp import DawnUdpProtocol

DESC_PATH = "descriptors/examples/udp_basic.yaml"
UDP_HOST = "192.168.8.104"
UDP_PORT = int(protocol_config_value(DESC_PATH, "udp", "port"))

DUMMYIO1_ID = io_objid(DESC_PATH, "dummyio1")
DUMMYIO2_ID = io_objid(DESC_PATH, "dummyio2")
NOTIFYIO1_ID = io_objid(DESC_PATH, "notifyio1")


@pytest.fixture
def udp_client():
    client = DawnUdpProtocol(UDP_HOST, UDP_PORT, timeout=2.0)
    client.connect()
    yield client
    client.disconnect()


def _cleanup_udp_file(path):
    product = pytest.products[0].core(0)
    product.sendCommand(f"rm {path}", timeout=1)


class TestUdp:
    """Validate UDP ping, IO access, notifications, and FileIO behavior."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_UDP"),
    ]

    def test_udp_ping(self, udp_client):
        """Test basic UDP ping."""
        start_dawn()

        assert udp_client.ping() is True

    def test_udp_get_io_list(self, udp_client):
        """Test getting IO list via UDP."""
        start_dawn()

        io_list = udp_client.get_io_list()
        assert len(io_list) >= 2
        assert DUMMYIO1_ID in io_list
        assert DUMMYIO2_ID in io_list

    def test_udp_get_set_io_uint16(self, udp_client):
        """Test basic UDP get/set IO (uint16)."""
        start_dawn()

        # Write value
        test_val = 0x1234
        data = struct.pack("<H", test_val)
        assert udp_client.write_io(DUMMYIO1_ID, data) is True

        # Read back
        read_data = udp_client.read_io(DUMMYIO1_ID)
        assert read_data is not None
        assert len(read_data) == 2
        assert struct.unpack("<H", read_data)[0] == test_val

    def test_udp_get_set_io_float(self, udp_client):
        """Test basic UDP get/set IO (float)."""
        start_dawn()

        # Write value
        test_val = 123.456
        data = struct.pack("<f", test_val)
        assert udp_client.write_io(DUMMYIO2_ID, data) is True

        # Read back
        read_data = udp_client.read_io(DUMMYIO2_ID)
        assert read_data is not None
        assert len(read_data) == 4
        # Float precision comparison
        assert pytest.approx(struct.unpack("<f", read_data)[0]) == test_val

    def test_udp_subscribe_notify_unsubscribe(self, udp_client):
        """Test UDP subscribe/notify/unsubscribe flow."""
        start_dawn()

        assert exchange_status(
            udp_client,
            udp_client.CMD_SUBSCRIBE,
            NOTIFYIO1_ID,
        )

        notification = wait_for_notification(udp_client, NOTIFYIO1_ID)
        assert notification is not None
        assert len(notification) == 4

        assert exchange_status(
            udp_client,
            udp_client.CMD_UNSUBSCRIBE,
            NOTIFYIO1_ID,
        )

    def test_udp_fileio_read_write(self, udp_client):
        """Test UDP FileIO read/write roundtrip."""
        _cleanup_udp_file("/tmp/dawn_udp_fileio.bin")
        start_dawn()
        fileio_id = io_objid(DESC_PATH, "udp_fileio1")

        payload = b"udp-fileio-roundtrip"
        assert udp_client.write_io(fileio_id, payload) is True

        readback = udp_client.read_io_seek(fileio_id)
        assert readback == payload

    def test_udp_fileio_write_once(self, udp_client):
        """Test UDP FileIO write-once enforcement."""
        _cleanup_udp_file("/tmp/dawn_udp_fileonce.bin")
        start_dawn()
        fileonce_id = io_objid(DESC_PATH, "udp_fileonce1")

        first = exchange_write(udp_client, fileonce_id, b"once")
        assert first is not None
        assert first[0] == udp_client.CMD_SET_IO
        assert len(first[1]) >= 1
        assert first[1][0] == udp_client.STATUS_OK

        second = exchange_write(udp_client, fileonce_id, b"twice")
        assert second is not None
        assert second[0] == udp_client.CMD_SET_IO
        assert len(second[1]) >= 1
        assert second[1][0] == udp_client.STATUS_ERROR
