############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for runtime descriptor upload and slot switching."""

import struct
import subprocess
import tempfile
import time
from pathlib import Path

import pytest
from _descriptor_common import io_objid
from _ntfc_common import start_dawn
from _proto_client_common import connect_client
from dawnpy.objectid import ObjectIdDecoder
from dawnpy_serial.serial import DawnSerialProtocol

SERIAL_PORT = "/tmp/ttyNX0"
SERIAL_BAUD = 115200
DESC_PATH = "descriptors/examples/dynamic_desc_slot0.yaml"
SLOT1_DESC_PATH = "descriptors/examples/dynamic_desc_slot1.yaml"

DESCRIPTOR0_ID = io_objid(DESC_PATH, "descriptor0")
DESCRIPTOR1_ID = io_objid(DESC_PATH, "descriptor1")
DUMMY0_ID = io_objid(DESC_PATH, "dummy0")


def _dtype_id(dtype_name: str) -> int:
    decoder = ObjectIdDecoder()
    target = dtype_name.strip().lower()
    for dtype_id, info in decoder.dtype_info.items():
        if str(info.get("type", "")).strip().lower() == target:
            return int(dtype_id)
    raise RuntimeError(f"Unsupported dtype mapping: {dtype_name}")


DESCSELECTOR0_ID = ObjectIdDecoder().encode(
    obj_type=1,  # OBJTYPE_IO
    cls=54,  # IO_CLASS_DESC_SELECTOR
    dtype=_dtype_id("uint32"),
    flags=0,
    priv=0,
)


def _new_serial_client():
    return DawnSerialProtocol(SERIAL_PORT, baudrate=SERIAL_BAUD, timeout=1.5)


def _connect_serial(timeout_s=5.0):
    return connect_client(_new_serial_client, timeout_s=timeout_s)


def _read_selector_slot(client: DawnSerialProtocol) -> int | None:
    """Return current selector slot, or ``None`` if the read failed.

    ``None`` lets the poll loop treat a transient serial blip during a
    slot-switch reboot as "not ready yet" without an extra try/except.
    """
    selector_data = client.read_io(DESCSELECTOR0_ID)
    if selector_data is None or len(selector_data) != 4:
        return None
    return struct.unpack("<I", selector_data)[0]


def _wait_selector_slot(expected: int, timeout_s: float = 12.0) -> None:
    deadline = time.monotonic() + timeout_s
    last = None

    while time.monotonic() < deadline:
        client = _connect_serial(timeout_s=3.0)
        try:
            last = _read_selector_slot(client)
            if last == expected:
                return
        finally:
            client.disconnect()
        time.sleep(0.25)

    pytest.fail(f"Selector did not reach expected slot {expected}, last={last}")


def _gen_descriptor_binary(yaml_rel_path: str) -> bytes:
    repo_root = Path(__file__).resolve().parents[3]
    yaml_path = repo_root / yaml_rel_path
    if not yaml_path.exists():
        raise RuntimeError(f"Descriptor YAML not found: {yaml_path}")

    with tempfile.TemporaryDirectory(prefix="ntfc_descbin_") as tmp_dir:
        out_bin = Path(tmp_dir) / "descriptor.bin"
        cmd = [
            "python",
            "-m",
            "dawnpy",
            "desc-bin",
            str(yaml_path),
            "-o",
            str(out_bin),
        ]
        result = subprocess.run(
            cmd,
            cwd=str(repo_root),
            capture_output=True,
            text=True,
            check=False,
        )
        if result.returncode != 0:
            raise RuntimeError(
                "Failed to generate descriptor binary:\n"
                f"stdout:\n{result.stdout}\n"
                f"stderr:\n{result.stderr}"
            )
        return out_bin.read_bytes()


def _read_u32_io(client: DawnSerialProtocol, objid: int) -> int:
    data = client.read_io(objid)
    if data is None or len(data) != 4:
        raise RuntimeError(f"Failed to read uint32 IO 0x{objid:08X}")
    return struct.unpack("<I", data)[0]


class TestDynamicDescriptor:
    """Validate descriptor upload, switch, reconnect, and rollback behavior."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_DESC_SWITCH"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_DESCRIPTOR"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_DESC_SELECTOR"),
    ]

    def test_dynamic_desc_upload_switch_and_rollback(self):
        """Upload slot-1 descriptor, switch active slot, then roll back."""

        start_dawn(settle_s=0.3)
        client = _connect_serial(timeout_s=8.0)

        try:
            assert _read_selector_slot(client) == 0
            assert _read_u32_io(client, DUMMY0_ID) == 1

            slot1_bin = _gen_descriptor_binary(SLOT1_DESC_PATH)
            assert len(slot1_bin) > 32
            assert slot1_bin[:4] == b"\x02\x03\x0a\x0d"

            # Empty slot-1 must not be switchable.
            assert client.write_io(DESCSELECTOR0_ID, struct.pack("<I", 1)) is False

            chunk_size = 128
            for offset in range(0, len(slot1_bin), chunk_size):
                chunk = slot1_bin[offset : offset + chunk_size]
                assert client.write_io_seek(DESCRIPTOR1_ID, offset, chunk) is True

            slot1_readback = client.read_io_seek(DESCRIPTOR1_ID)
            assert slot1_readback is not None
            assert slot1_readback == slot1_bin

            assert client.write_io(DESCSELECTOR0_ID, struct.pack("<I", 1)) is True

            client.disconnect()
            _wait_selector_slot(1, timeout_s=12.0)
            client = _connect_serial(timeout_s=6.0)
            assert _read_u32_io(client, DUMMY0_ID) == 77

            assert client.write_io(DESCSELECTOR0_ID, struct.pack("<I", 0)) is True

            client.disconnect()
            _wait_selector_slot(0, timeout_s=12.0)
            client = _connect_serial(timeout_s=6.0)
            assert _read_u32_io(client, DUMMY0_ID) == 1
        finally:
            client.disconnect()
