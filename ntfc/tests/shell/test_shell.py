# ntfc/tests/shell/test_shell.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC integration tests for Dawn shell command behavior."""

import pytest
from _descriptor_common import io_objid
from _ntfc_common import close_dawn_shell, open_dawn_shell, shell_cmd

DESC_PATH = "descriptors/examples/shell_core_demo.yaml"
DUMMY1_ID = io_objid(DESC_PATH, "dummyio1")
DUMMY2_ID = io_objid(DESC_PATH, "dummyio2")
HOSTNAME1_ID = io_objid(DESC_PATH, "hostname1")
VIRTIO2_ID = io_objid(DESC_PATH, "virtio2")
CFGIO1_ID = io_objid(DESC_PATH, "cfgio1")


class TestShell:
    """Validate shell IO operations and inspector command surface."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL"),
    ]

    def test_shell_tests_init(self):
        """Verify core help/info/getio/setio command behavior."""

        product = open_dawn_shell()

        # help command
        ret = shell_cmd(product, "help")
        assert "Proto SHELL help" in ret.output

        # info command
        ret = shell_cmd(product, "info")
        assert "INFO: shell IO bindings" in ret.output
        assert f"0x{DUMMY1_ID:08x}" in ret.output

        # get dummy IO
        ret = shell_cmd(product, f"getio 0x{DUMMY1_ID:08x}")
        assert f"IO 0x{DUMMY1_ID:08x} data:" in ret.output
        assert "\t1" in ret.output

        # get and set new value for dummy IO
        ret = shell_cmd(product, f"getio 0x{DUMMY2_ID:08x}")
        assert f"IO 0x{DUMMY2_ID:08x} data:" in ret.output
        assert "\t-20" in ret.output
        shell_cmd(product, f"setio 0x{DUMMY2_ID:08x} 0xffffffff")
        ret = shell_cmd(product, f"getio 0x{DUMMY2_ID:08x}")
        assert f"IO 0x{DUMMY2_ID:08x} data:" in ret.output
        assert "\t-1" in ret.output

        # get host name
        ret = shell_cmd(product, f"getio 0x{HOSTNAME1_ID:08x}")
        assert f"IO 0x{HOSTNAME1_ID:08x} data:" in ret.output
        assert "\tdawndemo" in ret.output

        # get virtio2 - output from program
        ret = shell_cmd(product, f"getio 0x{VIRTIO2_ID:08x}")
        assert f"IO 0x{VIRTIO2_ID:08x} data:" in ret.output
        assert "\t1.0000 2.0000 3.0000" in ret.output

        # exit dawn shell
        ret = close_dawn_shell(product)
        assert "nsh>" in ret.output

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL_INSPECT")
    def test_shell_help_lists_inspector_commands(self):
        """Verify inspector commands are present in shell help output."""

        product = open_dawn_shell()

        ret = shell_cmd(product, "help")
        assert "Proto SHELL help" in ret.output
        assert "list [io|prog|proto] [verbose]" in ret.output
        assert "inspect <objectID>" in ret.output
        assert "tree - show object hierarchy" in ret.output
        assert "stats - show runtime statistics" in ret.output
        close_dawn_shell(product)

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL_INSPECT")
    def test_shell_list_inventory(self):
        """Verify inspector list command returns inventory sections."""

        product = open_dawn_shell()

        ret = shell_cmd(product, "list", timeout=2)
        assert "Object Inventory" in ret.output
        assert "IOs:" in ret.output
        assert "PROGs:" in ret.output
        assert "PROTOs:" in ret.output
        assert "Flags: R=Read W=Write N=Notify T=Timestamp C=Config" in ret.output
        close_dawn_shell(product)

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL_INSPECT")
    def test_shell_inspect_known_object(self):
        """Verify inspect command prints details for a known object ID."""

        product = open_dawn_shell()

        ret = shell_cmd(product, f"inspect 0x{DUMMY1_ID:08x}", timeout=2)
        assert "Object Details" in ret.output
        assert f"Object ID:    0x{DUMMY1_ID:08x}" in ret.output
        assert "Handler:      IO" in ret.output
        close_dawn_shell(product)

    def test_shell_invalid_commands_and_objects(self):
        """Verify invalid shell commands/IDs report expected errors."""

        product = open_dawn_shell()

        ret = shell_cmd(product, "getio 0x00000000")
        assert "no IO with objID =" in ret.output
        assert "0x0" in ret.output

        ret = shell_cmd(product, "setio 0x12345678")
        assert "invalid argument:" in ret.output
        assert "0xXXXXXXXX" in ret.output

        ret = shell_cmd(product, "getcfg 0x00000000 0x00000000")
        assert "no object with objID =" in ret.output
        assert "0x0" in ret.output
        close_dawn_shell(product)

    def test_shell_cfgio_limits_enforced(self):
        """Verify CIOLimits validation rejects out-of-range and step-misaligned
        writes through a config IO and accepts in-range step-aligned values.

        Descriptor ``cfgio1`` (uint32, targets ``dummyio1`` init_value=1234)
        has limits min=0, max=2000, step=2.
        """

        product = open_dawn_shell()

        # Initial value mirrors the dummy's init_value.
        ret = shell_cmd(product, f"getio 0x{CFGIO1_ID:08x}")
        assert f"IO 0x{CFGIO1_ID:08x} data:" in ret.output
        assert "\t1234" in ret.output

        # In-range, step-aligned write succeeds.
        ret = shell_cmd(product, f"setio 0x{CFGIO1_ID:08x} 100")
        assert "setio failed" not in ret.output
        ret = shell_cmd(product, f"getio 0x{CFGIO1_ID:08x}")
        assert "\t100" in ret.output

        # Step-misaligned write (odd number) is rejected with -ERANGE (-34).
        ret = shell_cmd(product, f"setio 0x{CFGIO1_ID:08x} 7")
        assert "setio failed -34" in ret.output

        # Above max is rejected.
        ret = shell_cmd(product, f"setio 0x{CFGIO1_ID:08x} 3000")
        assert "setio failed -34" in ret.output

        # Stored value remains the last accepted write.
        ret = shell_cmd(product, f"getio 0x{CFGIO1_ID:08x}")
        assert "\t100" in ret.output

        close_dawn_shell(product)
