############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

"""NTFC integration tests for Dawn serial protocol behavior.

These tests verify:
- control-path liveness (PING),
- discovery/listing consistency against descriptor serial bindings,
- metadata-only discovery behavior for descriptor/block IOs,
- data-path read/write roundtrips (dummy, fileio),
- notify subscribe/unsubscribe flow,
- seek read/write behavior and error reporting.
"""

import struct
import time

import pytest
from _descriptor_common import io_objid, protocol_spec
from _ntfc_common import start_dawn
from _proto_client_common import (
    exchange_status,
    exchange_write,
    parse_caps_blob,
    wait_for_notification,
)
from dawnpy_serial.serial import DawnSerialProtocol

SERIAL_PORT = "/tmp/ttyNX0"
SERIAL_BAUD = 115200
DESC_PATH = "descriptors/examples/serial_core_demo.yaml"
SERIAL_NOTIFYIO1_ID = io_objid(DESC_PATH, "notifyio1")
SERIAL_DESCRIPTOR1_ID = io_objid(DESC_PATH, "descriptor1")
SERIAL_CAPABILITIES1_ID = io_objid(DESC_PATH, "capabilities1")
VOLATILE_MASK = 0x001FC000  # flags[15:14], dtype[19:16], ext[20]


@pytest.fixture
def serial_client():
    """Create a connected serial protocol client for each test."""
    client = DawnSerialProtocol(SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=1.5)
    for _ in range(20):
        if client.connect():
            try:
                yield client
            finally:
                client.disconnect()
            return
        time.sleep(0.2)
    pytest.fail(f"Could not connect to serial port {SERIAL_PORT}")


def _decoded_class_name(client, objid):
    """Return decoded class name for object ID, or None on decode failure."""
    if not client.objid_decoder:
        return None
    try:
        return client.objid_decoder.decode(objid).cls_name
    except Exception:
        return None


def _find_rw_dummy(client, io_list):
    """Find one scalar read-write dummy IO usable for set/get roundtrip."""
    for objid in io_list:
        if _decoded_class_name(client, objid) != "dummy":
            continue
        info = client.get_io_info(objid)
        if not info:
            continue
        if info["io_type"] != client.IO_TYPE_READ_WRITE:
            continue
        if info["dimension"] != 1:
            continue
        if info["dtype"] not in (6, 7, 9, 10):  # i32/u32/u64/f32
            continue
        return objid, info
    return None, None


def _serial_binding_objids(rel_path):
    """Return object IDs for all IOs bound to serial protocol in descriptor."""
    spec = protocol_spec(rel_path, "serial")
    bindings = spec.get("bindings", [])
    out = []
    for binding in bindings:
        io_id = binding.get("id")
        if io_id is None:
            continue
        out.append(io_objid(rel_path, io_id))
    return out


def _make_test_payload(dtype):
    """Return deterministic payload bytes matching selected scalar dtype."""
    if dtype == 6:  # int32
        return struct.pack("<i", -123456)
    if dtype == 7:  # uint32
        return struct.pack("<I", 0x12345678)
    if dtype == 9:  # uint64
        return struct.pack("<Q", 0x0102030405060708)
    if dtype == 10:  # float
        return struct.pack("<f", 12.5)
    return None


class TestSerial:
    """End-to-end serial protocol integration tests against simulator target."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_CAPABILITIES"),
    ]

    def test_serial_ping(self, serial_client):
        """Device should respond to protocol PING after Dawn starts."""
        start_dawn(settle_s=0.2)

        assert serial_client.ping() is True

    def test_serial_get_io_list(self, serial_client):
        """LIST_IOS should cover all serial descriptor bindings."""
        start_dawn(settle_s=0.2)

        io_list = serial_client.get_io_list()
        expected = _serial_binding_objids(DESC_PATH)
        expected_masked = {objid & ~VOLATILE_MASK for objid in expected}
        actual_masked = {objid & ~VOLATILE_MASK for objid in io_list}
        missing = expected_masked - actual_masked
        extra = actual_masked - expected_masked

        assert len(io_list) >= len(expected)
        assert not missing, (
            "Missing serial-discovered IOs: "
            + ", ".join(f"0x{objid:08X}" for objid in sorted(missing))
            + "; Extra: "
            + ", ".join(f"0x{objid:08X}" for objid in sorted(extra))
        )
        assert any(
            _decoded_class_name(serial_client, objid) == "dummy" for objid in io_list
        )

    def test_serial_discovery_skips_descriptor_get_io(self, serial_client):
        """Discovery must not issue GET_IO for descriptor block objects."""
        start_dawn(settle_s=0.2)

        errors = []
        original_err = serial_client._err
        serial_client._err = lambda message: errors.append(message)
        try:
            io_data = serial_client.discover_all_ios()
        finally:
            serial_client._err = original_err

        assert SERIAL_DESCRIPTOR1_ID in io_data
        assert io_data[SERIAL_DESCRIPTOR1_ID].get("data") is None
        assert not any(
            f"Error reading IO 0x{SERIAL_DESCRIPTOR1_ID:08X}" in message
            for message in errors
        )

    def test_serial_get_capabilities_blob(self, serial_client):
        """Capabilities IO should be bound over serial and return valid blob."""
        start_dawn(settle_s=0.2)

        io_list = serial_client.get_io_list()
        assert SERIAL_CAPABILITIES1_ID in io_list

        blob = serial_client.read_io_seek(SERIAL_CAPABILITIES1_ID)
        assert blob is not None
        version, category, reserved, payload_len, _ = parse_caps_blob(blob)
        assert version == 2
        assert category == 0
        assert reserved == 0
        assert len(blob) == 8 + payload_len
        assert payload_len > 0

    def test_serial_get_set_dummy(self, serial_client):
        """Read/write roundtrip should work for a bound RW dummy IO."""
        start_dawn(settle_s=0.2)

        io_list = serial_client.get_io_list()
        assert io_list

        objid, info = _find_rw_dummy(serial_client, io_list)
        if objid is None or info is None:
            pytest.skip("No writable dummy IO found in serial bindings")

        original = serial_client.read_io(objid)
        assert original is not None

        payload = _make_test_payload(info["dtype"])
        if payload is None:
            pytest.skip(f"Unsupported dtype for test write: {info['dtype']}")

        assert serial_client.write_io(objid, payload) is True
        readback = serial_client.read_io(objid)
        assert readback is not None
        assert readback == payload

        # Restore original value to avoid affecting later tests.
        assert serial_client.write_io(objid, original) is True

    def test_serial_subscribe_notify_unsubscribe(self, serial_client):
        """SUBSCRIBE should deliver notify, then UNSUBSCRIBE should pass."""
        start_dawn(settle_s=0.2)

        assert exchange_status(
            serial_client,
            serial_client.CMD_SUBSCRIBE,
            SERIAL_NOTIFYIO1_ID,
        )

        notification = wait_for_notification(serial_client, SERIAL_NOTIFYIO1_ID)
        assert notification is not None
        assert len(notification) == 4

        assert exchange_status(
            serial_client,
            serial_client.CMD_UNSUBSCRIBE,
            SERIAL_NOTIFYIO1_ID,
        )

    def test_serial_fileio_read_write(self, serial_client):
        """FileIO seek-read should return bytes previously written by SET_IO."""
        start_dawn(settle_s=0.2)
        fileio_id = io_objid(DESC_PATH, "serial_fileio1")

        payload = b"serial-fileio-roundtrip"
        assert serial_client.write_io(fileio_id, payload) is True

        readback = serial_client.read_io_seek(fileio_id)
        assert readback == payload

    def test_serial_fileio_write_once(self, serial_client):
        """Write-once FileIO should accept first write and reject second."""
        start_dawn(settle_s=0.2)
        fileonce_id = io_objid(DESC_PATH, "serial_fileonce1")

        first = exchange_write(serial_client, fileonce_id, b"once")
        assert first is not None
        assert first[0] == serial_client.CMD_SET_IO
        assert len(first[1]) >= 1
        assert first[1][0] == serial_client.STATUS_OK

        second = exchange_write(serial_client, fileonce_id, b"twice")
        assert second is not None
        assert second[0] == serial_client.CMD_SET_IO
        assert len(second[1]) >= 1
        assert second[1][0] == serial_client.STATUS_ERROR

    def test_serial_set_io_seek_fileio_roundtrip(self, serial_client):
        """SET_IO_SEEK must update requested offset range in FileIO payload."""
        start_dawn(settle_s=0.2)
        fileio_id = io_objid(DESC_PATH, "serial_fileio1")

        assert serial_client.write_io(fileio_id, b"abcdefgh") is True
        assert serial_client.write_io_seek(fileio_id, 2, b"XYZ") is True

        readback = serial_client.read_io_seek(fileio_id)
        assert readback == b"abXYZfgh"

    def test_serial_set_io_seek_invalid_obj(self, serial_client):
        """SET_IO_SEEK on invalid object should return CMD_ERROR/INVALID_OBJ."""
        start_dawn(settle_s=0.2)

        payload = struct.pack("<II", 0xFFFFFFFF, 0) + b"x"
        assert serial_client.send_frame(serial_client.CMD_SET_IO_SEEK, payload)

        frame = serial_client.receive_frame()
        assert frame is not None
        cmd, response = frame
        assert cmd == serial_client.CMD_ERROR
        assert len(response) >= 2
        assert response[0] == serial_client.STATUS_INVALID_OBJ
        assert response[1] == serial_client.CMD_SET_IO_SEEK
