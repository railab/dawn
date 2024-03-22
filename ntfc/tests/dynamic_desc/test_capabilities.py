############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for dynamic descriptor capabilities blob IO."""

import struct

import pytest
from _descriptor_common import io_objid
from _ntfc_common import start_dawn
from _proto_client_common import connect_client, parse_caps_blob
from dawnpy_serial.serial import DawnSerialProtocol

SERIAL_PORT = "/tmp/ttyNX0"
SERIAL_BAUD = 115200
DESC_PATH = "descriptors/examples/dynamic_desc_slot0.yaml"

CAP_IO_ID = io_objid(DESC_PATH, "capabilities_io")


def _new_serial_client():
    return DawnSerialProtocol(SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=1.5)


def _read_caps_blob(client: DawnSerialProtocol, objid: int) -> bytes:
    payload = client.read_io_seek(objid)
    if payload is None:
        raise RuntimeError(f"Failed to read capabilities IO 0x{objid:08X}")
    return payload


class TestDynamicDescriptorCapabilities:
    """Validate capabilities blob framing and payload layout."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_CAPABILITIES"),
    ]

    def test_capabilities_blob_layout_and_content(self):
        """Verify capabilities blob header fields and payload sections."""

        start_dawn(settle_s=0.3)
        client = connect_client(_new_serial_client, timeout_s=8.0)

        try:
            blob = _read_caps_blob(client, CAP_IO_ID)

            ver, category, reserved, payload_len, payload = parse_caps_blob(blob)
            assert ver == 2
            assert category == 0
            assert reserved == 0
            assert payload_len == 504
            assert len(payload) == 504

            io_payload = payload[0:64]
            prog_payload = payload[64:128]
            proto_payload = payload[128:192]
            meta_payload = payload[192:]
            assert len(meta_payload) == 312

            (
                dtype_bits_lo,
                dtype_bits_hi,
                io_flags_lo,
                _io_flags_hi,
                build_flags_lo,
                _build_flags_hi,
                desc_slots,
                slot_size,
                max_io_cls,
                max_prog_cls,
                max_proto_cls,
            ) = struct.unpack("<IIIIIIIIIII", meta_payload[:44])
            assert dtype_bits_lo != 0
            assert dtype_bits_hi == 0
            assert io_flags_lo >= 0
            assert build_flags_lo != 0
            assert max_io_cls == 0x1FF
            assert max_prog_cls == 0x1FF
            assert max_proto_cls == 0x1FF

            assert desc_slots == 2
            assert slot_size == 4096

            assert any(byte != 0 for byte in io_payload)
            # PROG bitmap may be all-zero if no program classes are enabled
            # in the target firmware config.
            assert len(prog_payload) == 64
            assert any(byte != 0 for byte in proto_payload)
        finally:
            client.disconnect()
