# ntfc/tests/tests/test_dawntest.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC integration test that executes dawntest built-in suite."""

import pytest


class TestDawntest:
    """Validate the aggregate `dawntest` command succeeds."""

    pytestmark = [
        pytest.mark.cmd_check("dawntest_main"),
        pytest.mark.dep_config("CONFIG_DAWN_TESTS"),
    ]

    def test_dawn_tests(self):
        """Run `dawntest` and assert no failures are reported."""

        product = pytest.products[0].core(0)
        ret = product.sendCommandReadUntilPattern(
            "dawntest", pattern=b"Test result:", timeout=200
        )
        assert "FAILED" not in ret.output
