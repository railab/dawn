############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for file-based descriptor loading.

Writes a binary descriptor file via NSH ``dd`` + ``echo -ne`` so that
null bytes (common in the 32-bit descriptor words) are handled correctly.
"""

import subprocess
import tempfile
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd

DESC_PATH = "/tmp/desc.bin"
PATCH_PATH = "/tmp/p.bin"

DESC_YAML = """\
metadata:
  no_idle_quit: true
ios:
  - &d0
    id: dummy0
    type: dummy
    dtype: uint32
    config:
      init_value: 42
protocols:
  - id: shell0
    type: shell
    bindings:
      - *d0
    config:
      prompt: "dawn> "
"""


def _gen_descriptor_binary(yaml_text: str) -> bytes:
    """Generate a binary descriptor from a YAML string via dawnpy."""
    with tempfile.TemporaryDirectory(prefix="ntfc_descbin_") as tmp:
        yaml_path = Path(tmp) / "desc.yaml"
        out_bin = Path(tmp) / "desc.bin"
        yaml_path.write_text(yaml_text, encoding="utf-8")

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
        binary = out_bin.read_bytes()
        assert len(binary) > 0
        return binary


def _non_null_runs(binary: bytes) -> list[tuple[int, bytes]]:
    """Return list of ``(offset, bytes)`` for each contiguous non-null run."""
    runs: list[tuple[int, bytes]] = []
    start: int | None = None
    for i, b in enumerate(binary):
        if b != 0:
            if start is None:
                start = i
        else:
            if start is not None:
                runs.append((start, binary[start:i]))
                start = None
    if start is not None:
        runs.append((start, binary[start:]))
    return runs


def _echo_hex(bs: bytes) -> str:
    """Build an ``echo -ne`` hex-escape string for a byte sequence.

    Uses literal backslash-x sequences (e.g. ``\\x02``) so that the PTY
    passes the escapes through to NuttX's echo without interpreting
    control characters such as 0x0A / 0x0D.
    """
    return "".join("\\x%02X" % b for b in bs)


def _resolve_dummy_id_from_list(output: str) -> int:
    """Extract the dummy IO object ID from 'list io' output."""
    for line in output.splitlines():
        if "dummy" in line.lower():
            for p in line.split():
                p = p.strip("(),")
                if p.startswith("0x"):
                    try:
                        return int(p, 16)
                    except ValueError:
                        pass
    raise RuntimeError("Could not find dummy IO ID in list io output:\n%s" % output)


class TestDescFile:
    """Validate file-based descriptor loading via NSH-written binary."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_APPS_DESC_SOURCE_FILE"),
        pytest.mark.dep_config("CONFIG_FS_TMPFS"),
    ]

    def test_desc_file_load_and_verify(self):
        """Write a binary descriptor via NSH dd+echo, load it, verify."""

        product = pytest.products[0].core(0)

        binary = _gen_descriptor_binary(DESC_YAML)
        runs = _non_null_runs(binary)

        # Mount writable tmpfs (may report a spurious error but works).

        shell_cmd(product, "mount -t tmpfs /tmp")

        # Create a zero-filled template file.

        ret = shell_cmd(
            product, "dd if=/dev/zero of=%s bs=1 count=%d" % (DESC_PATH, len(binary))
        )
        assert "nsh:" not in ret.output, "dd failed: %s" % ret.output

        # Patch each run of non-null bytes into the template.

        for offset, run_bytes in runs:
            hex_str = _echo_hex(run_bytes)
            ret = shell_cmd(product, "echo -ne '%s' > %s" % (hex_str, PATCH_PATH))
            assert "nsh:" not in ret.output, "echo failed: %s" % ret.output
            ret = shell_cmd(
                product,
                "dd if=%s of=%s bs=1 seek=%d conv=notrunc"
                % (PATCH_PATH, DESC_PATH, offset),
            )
            assert "nsh:" not in ret.output, "dd patch failed: %s" % ret.output

        # Verify the file was written correctly.

        ret = shell_cmd(product, "ls -l %s" % DESC_PATH)
        assert DESC_PATH in ret.output
        ret = shell_cmd(product, "hexdump %s" % DESC_PATH)
        # The magic bytes 0x0D0A0302 in little-endian: 02 03 0A 0D
        assert "02 03 0a 0d" in ret.output.lower(), (
            "hexdump missing magic bytes:\n%s" % ret.output
        )

        # Start dawn with the file descriptor.

        product.sendCommand("dawn -f %s" % DESC_PATH, timeout=5)

        # Resolve the dummy IO ID from info output and verify value.

        ret = shell_cmd(product, "info")
        dummy_id = _resolve_dummy_id_from_list(ret.output)
        ret = shell_cmd(product, "getio 0x%08x" % dummy_id)
        assert ("IO 0x%08x data:" % dummy_id) in ret.output
        assert "\t42" in ret.output

        # Exit dawn shell.

        ret = shell_cmd(product, "exit", timeout=3)
        assert "nsh>" in ret.output
