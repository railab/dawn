############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for dynamic descriptor capabilities blob IO."""

import copy
import struct

import pytest
from _descriptor_common import io_objid, load_descriptor_spec
from _ntfc_common import start_dawn
from _proto_client_common import connect_client, parse_caps_blob
from dawnpy.descriptor.reports.graph import build_descriptor_graph
from dawnpy.descriptor.validation.compat import (
    check_compatibility,
    format_report,
)
from dawnpy.objectid import ObjectIdDecoder
from dawnpy_serial.serial import DawnSerialProtocol

SERIAL_PORT = "/tmp/ttyNX0"
SERIAL_BAUD = 115200
DESC_PATH = "descriptors/examples/dynamic_desc_slot0.yaml"
SLOT1_DESC_PATH = "descriptors/examples/dynamic_desc_slot1.yaml"

CAP_IO_ID = io_objid(DESC_PATH, "capabilities_io")

# Capabilities blob header is 8 bytes; the IO class bitmap follows it.
CAPS_HEADER_SIZE = 8


def _new_serial_client():
    return DawnSerialProtocol(SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=1.5)


def _read_caps_blob(client: DawnSerialProtocol, objid: int) -> bytes:
    payload = client.read_io_seek(objid)
    if payload is None:
        raise RuntimeError(f"Failed to read capabilities IO 0x{objid:08X}")
    return payload


def _descriptor_objids(rel_path: str) -> dict:
    """Return {object id: ObjectID} for every object in a descriptor."""
    spec = copy.deepcopy(load_descriptor_spec(rel_path))
    graph = build_descriptor_graph(spec)
    return {node.id: node.objid for node in graph.nodes if node.objid is not None}


def _disable_io_class(blob: bytes, cls_id: int) -> bytes:
    """Return a copy of ``blob`` with one IO class bit cleared."""
    patched = bytearray(blob)
    patched[CAPS_HEADER_SIZE + (cls_id // 8)] &= ~(1 << (cls_id % 8)) & 0xFF
    return bytes(patched)


def _fetch_caps_blob() -> bytes:
    """Start Dawn and read the capabilities blob off the target."""
    start_dawn(settle_s=0.3)
    client = connect_client(_new_serial_client, timeout_s=8.0)
    try:
        return _read_caps_blob(client, CAP_IO_ID)
    finally:
        client.disconnect()


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


class TestDescriptorCompatibility:
    """Check switchable descriptors against the target's advertised caps."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_CAPABILITIES"),
    ]

    def test_slot1_descriptor_is_compatible(self):
        """The descriptor this target switches to must be loadable."""
        blob = _fetch_caps_blob()

        objids = _descriptor_objids(SLOT1_DESC_PATH)
        assert objids, "slot-1 descriptor produced no decodable objects"

        result = check_compatibility(objids, blob, slot=1)

        assert result.compatible, "\n".join(format_report(result))

    def test_disabled_io_class_is_reported(self):
        """Clearing a class the descriptor needs must block the upload."""
        blob = _fetch_caps_blob()

        objids = _descriptor_objids(SLOT1_DESC_PATH)
        decoder = ObjectIdDecoder()
        target = next(
            (
                (name, decoder.decode(objid))
                for name, objid in sorted(objids.items())
                if decoder.decode(objid).type_name == "IO"
            ),
            None,
        )
        assert target is not None, "no IO object in slot-1 descriptor"
        name, decoded = target

        result = check_compatibility(
            objids, _disable_io_class(blob, decoded.cls), slot=1
        )

        assert not result.compatible
        assert any(
            issue.kind == "class" and issue.object_id == name for issue in result.issues
        )

    def test_slot_beyond_advertised_count_is_rejected(self):
        """A slot index the target does not have must be rejected."""
        blob = _fetch_caps_blob()

        result = check_compatibility(_descriptor_objids(SLOT1_DESC_PATH), blob, slot=99)

        assert not result.compatible
        assert any(issue.kind == "slot" for issue in result.issues)
