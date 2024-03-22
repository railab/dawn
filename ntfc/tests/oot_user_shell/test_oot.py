############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for the out-of-tree demo.

Exercised by both ntfc sessions ``sim-oot-user-shell`` (C++ descriptor
path) and ``sim-oot-user-shell-yaml`` (YAML descriptor path generated
by dawnpy). Both must produce identical runtime behavior.

The tests verify:

- User-defined IO/PROG/PROTO factories ran (the ``mydummy`` IO from
  ``CIOMyDummy`` shows up in the inspector list).
- A user-defined ConfigField (``init_value``) reached the device - the

  IO returns the descriptor-baked value on the first ``getio`` call.
"""

import pytest
from _ntfc_common import (
    open_dawn_shell,
    shell_get_io_scalar,
    shell_list_io_entries,
)

CFG_INIT_VALUE = 0xDEADBEEF


class TestOotUserShell:
    """Validate the OOT extension surface end-to-end."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL"),
    ]

    def test_user_io_present(self):
        """Inspector should list the user-defined CIOMyDummy instance."""
        product = open_dawn_shell()
        entries = shell_list_io_entries(product)
        names = {e["name"] for e in entries}
        assert any(
            n.startswith("mydummy") for n in names
        ), f"user IO 'mydummy' not in inspector list: {sorted(names)}"

    def test_init_value_applied(self):
        """ConfigField init_value=0xDEADBEEF must reach CIOMyDummy."""
        product = open_dawn_shell()
        entries = shell_list_io_entries(product)
        objid = next(
            (e["objid"] for e in entries if e["name"].startswith("mydummy")),
            None,
        )
        assert objid is not None, "no mydummy IO found"
        assert shell_get_io_scalar(product, objid) & 0xFFFFFFFF == CFG_INIT_VALUE
