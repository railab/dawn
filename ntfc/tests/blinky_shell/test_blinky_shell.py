# ntfc/tests/blinky_shell/test_blinky_shell.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""Shell-based NTFC validation for sequencer blinky control behavior.

For this demo target, LED meaning is defined by bit 0 of the LED register:
bit0=0 => OFF, bit0=1 => ON. Tests evaluate only this bit.
"""

import time

import pytest
from _descriptor_common import io_objid, load_descriptor_spec
from _ntfc_common import (
    assert_set_ok,
    close_dawn_shell,
    open_dawn_shell,
    shell_get_io_scalar,
    shell_get_io_words,
    shell_get_io_write_count,
    shell_resolve_io_objid_from_expected,
    shell_set_io_scalar,
    shell_set_io_words,
    shell_wait_for_io_state,
)

DESC_PATH = "descriptors/examples/blinky_shell_demo.yaml"
LED1_EXPECTED_ID = io_objid(DESC_PATH, "led1")
CTRL_EXPECTED_ID = io_objid(DESC_PATH, "ctrl_blinky")
TRIG_EXPECTED_ID = io_objid(DESC_PATH, "trig_blinky")
CFG_START_EXPECTED_ID = io_objid(DESC_PATH, "cfg_seq_start")
CFG_STATES_EXPECTED_ID = io_objid(DESC_PATH, "cfg_seq_states")
DESC_SPEC = load_descriptor_spec(DESC_PATH)

STATE_CFG = DESC_SPEC.get("programs", [{}])[0].get("config", {}).get("states", [])
DWELL_US = (
    min(int(state.get("dwell_us", 500000)) for state in STATE_CFG)
    if STATE_CFG
    else 500000
)
BLINK_DWELL_S = DWELL_US / 1_000_000.0
# Sampling period for shell getio polling. We sample at least 8x per dwell
# and never faster than 20ms to avoid shell overload.
POLL_S = max(0.02, BLINK_DWELL_S / 8.0)
# Stability window while stopped/reset: slightly above one dwell interval.
STABLE_WINDOW_FACTOR = 1.3
STABLE_WINDOW_S = BLINK_DWELL_S * STABLE_WINDOW_FACTOR
# Control state deadline after start command.
CTRL_STATE_TIMEOUT_FACTOR = 2.0
CTRL_STATE_TIMEOUT_S = BLINK_DWELL_S * CTRL_STATE_TIMEOUT_FACTOR
# Dwell-relative probe points used to confirm a resumed transition.
TRANSITION_PROBE_FACTORS = (1.2, 1.4, 1.7)


def _read_led(product, led_objid):
    """Read current LED scalar value through shell getio."""

    return shell_get_io_scalar(product, led_objid, timeout=0.2)


def _read_led_bool(product, led_objid):
    """Read LED logical state from bit 0 of LED register."""

    return _read_led(product, led_objid) & 0x1


def _collect_led_states(product, led_objid, duration_s, step_s):
    """Collect unique LED values observed within a time window."""

    states = set()
    deadline = time.monotonic() + duration_s

    while time.monotonic() < deadline:
        states.add(_read_led_bool(product, led_objid))
        time.sleep(step_s)

    return states


def _assert_led_stable(product, led_objid):
    """Assert LED value does not change across the stability window."""

    states = _collect_led_states(product, led_objid, STABLE_WINDOW_S, POLL_S)
    assert len(states) == 1


def _assert_led_activity_fast(product, led_objid):
    """Assert the sequencer resumes LED writes after a start command."""

    base_writes = shell_get_io_write_count(product, led_objid, timeout=2)

    for factor in TRANSITION_PROBE_FACTORS:
        time.sleep(BLINK_DWELL_S * factor)
        cur_writes = shell_get_io_write_count(product, led_objid, timeout=2)
        if cur_writes > base_writes:
            return

    assert False, "LED write count did not increase after restart"


class TestBlinkyShell:
    """Validate sequencer blinky control and runtime reconfiguration via shell."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL"),
    ]

    def test_shell_blinky_end_to_end(self):
        """Verify STOP/RESET/START control path and observable LED behavior.

        Sequence:
        1. Resolve runtime IO IDs from descriptor IDs.
        2. STOP sequencer and assert LED bit0 remains stable.
        3. RESET while stopped and assert LED bit0 remains stable.
        4. START sequencer and assert LED bit0 transitions again.
        """

        product = open_dawn_shell(settle_s=0.05)
        led_objid = shell_resolve_io_objid_from_expected(product, LED1_EXPECTED_ID)
        ctrl_objid = shell_resolve_io_objid_from_expected(product, CTRL_EXPECTED_ID)
        trig_objid = shell_resolve_io_objid_from_expected(product, TRIG_EXPECTED_ID)

        ret = shell_set_io_scalar(product, ctrl_objid, 0)
        assert_set_ok(ret)
        _assert_led_stable(product, led_objid)

        ret = shell_set_io_scalar(product, trig_objid, 0)
        assert_set_ok(ret)
        _assert_led_stable(product, led_objid)

        ret = shell_set_io_scalar(product, ctrl_objid, 1)
        assert_set_ok(ret)
        assert (
            shell_wait_for_io_state(
                product, ctrl_objid, 1, timeout_s=CTRL_STATE_TIMEOUT_S, step_s=POLL_S
            )
            == 1
        )
        _assert_led_activity_fast(product, led_objid)

        ret = close_dawn_shell(product)
        assert "nsh>" in ret.output

    def test_shell_blinky_runtime_update_via_config_io(self):
        """Update sequencer runtime config via ConfigIO and verify behavior.

        Flow:
        1. Stop sequencer.
        2. Write start index and state table via ConfigIO setio.
        3. Read back both ConfigIO values via getio.
        4. Verify LED applies updated first state without external apply trigger.
        """

        product = open_dawn_shell(settle_s=0.05)
        led_objid = shell_resolve_io_objid_from_expected(product, LED1_EXPECTED_ID)
        ctrl_objid = shell_resolve_io_objid_from_expected(product, CTRL_EXPECTED_ID)
        cfg_start_objid = shell_resolve_io_objid_from_expected(
            product, CFG_START_EXPECTED_ID
        )
        cfg_states_objid = shell_resolve_io_objid_from_expected(
            product, CFG_STATES_EXPECTED_ID
        )

        ret = shell_set_io_scalar(product, ctrl_objid, 0)
        assert_set_ok(ret)

        new_start = 0
        new_states = [1, DWELL_US, 0, DWELL_US]
        ret = shell_set_io_scalar(product, cfg_start_objid, new_start)
        assert_set_ok(ret)
        ret = shell_set_io_words(product, cfg_states_objid, new_states)
        assert_set_ok(ret)

        assert shell_get_io_scalar(product, cfg_start_objid) == new_start
        assert shell_get_io_words(product, cfg_states_objid) == new_states

        assert (
            shell_wait_for_io_state(
                product, led_objid, 1, timeout_s=CTRL_STATE_TIMEOUT_S, step_s=POLL_S
            )
            == 1
        )

        ret = close_dawn_shell(product)
        assert "nsh>" in ret.output

    def test_shell_blinky_runtime_update_while_running(self):
        """Update sequencer states while running and verify new behavior.

        Flow:
        1. Ensure sequencer is running.
        2. Write a constant-HIGH state table via ConfigIO while running.
        3. Verify LED remains stable HIGH for a dwell-relative window.
        """

        product = open_dawn_shell(settle_s=0.05)
        led_objid = shell_resolve_io_objid_from_expected(product, LED1_EXPECTED_ID)
        ctrl_objid = shell_resolve_io_objid_from_expected(product, CTRL_EXPECTED_ID)
        cfg_states_objid = shell_resolve_io_objid_from_expected(
            product, CFG_STATES_EXPECTED_ID
        )

        ret = shell_set_io_scalar(product, ctrl_objid, 1)
        assert_set_ok(ret)
        assert (
            shell_wait_for_io_state(
                product, ctrl_objid, 1, timeout_s=CTRL_STATE_TIMEOUT_S, step_s=POLL_S
            )
            == 1
        )

        new_states = [1, DWELL_US, 1, DWELL_US]
        ret = shell_set_io_words(product, cfg_states_objid, new_states)
        assert_set_ok(ret)
        assert shell_get_io_words(product, cfg_states_objid) == new_states

        states = _collect_led_states(product, led_objid, STABLE_WINDOW_S, POLL_S)
        assert states == {1}

        ret = close_dawn_shell(product)
        assert "nsh>" in ret.output
