############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN buffer program behavior."""

import struct
import time

import pytest
from _can_common import (
    read_bytes_via_can_seg,
    read_u32_via_can_rtr,
    wait_for_non_none,
    wait_for_value,
    wait_until,
    write_u8_via_can,
    write_u32_via_can,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    READ_BUFFER_OUT_U32,
    READ_BUFFER_SEL_U32,
    READSEG_BUFFER_STAT_U32,
    WRITE_BUFFER_SEL_U32,
    WRITE_CTRL_BUFFER1,
    WRITE_RMS_RESET_TRIG,
    ensure_control_state,
)


class TestProgramsCanBuffer:
    """Validate buffer selection, control, and statistics over CAN."""

    pytestmark = COMMON_PYTESTMARK

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
    def test_programs_buffer_select_and_reset(self, can_sock, node_id):
        """Buffer select path: write sel over CAN, read selected sample, then reset."""
        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 1)
            == 1
        )

        first = wait_for_non_none(
            read_u32_via_can_rtr, can_sock, node_id, READ_BUFFER_OUT_U32, timeout_s=5.0
        )
        assert first is not None, "No initial buffer_out_u32 response"

        # Ensure there are multiple captured samples before freezing.
        second = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v is not None and v != first,
            timeout_s=3.0,
        )
        assert second is not None and second != first
        third = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v is not None and v != second,
            timeout_s=3.0,
        )
        assert third is not None and third != second

        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 1)
        older = wait_for_non_none(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            timeout_s=0.5,
        )

        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 0)
        floor = older if older is not None else 0
        newest = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v is not None and v > floor,
            timeout_s=1.0,
        )

        assert older is not None and newest is not None
        assert older < newest

        # Verify selector value is visible on CAN read path.
        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 1)
        sel_val = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_SEL_U32,
            1,
            timeout_s=2.0,
        )
        assert sel_val == 1

        # Reset path: command should be accepted and data remains readable.
        write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 0)
        after_reset = wait_for_non_none(
            read_u32_via_can_rtr, can_sock, node_id, READ_BUFFER_OUT_U32, timeout_s=2.0
        )
        assert after_reset is not None

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
    def test_programs_buffer_control_start_stop(self, can_sock, node_id):
        """Buffer control path: stop freezes output, start resumes updates."""
        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 0)
        baseline = wait_for_non_none(
            read_u32_via_can_rtr, can_sock, node_id, READ_BUFFER_OUT_U32, timeout_s=5.0
        )
        assert baseline is not None

        moving = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v is not None and v != baseline,
            timeout_s=2.0,
        )
        assert moving is not None and moving != baseline

        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 0)
            == 0
        )

        frozen = wait_for_non_none(
            read_u32_via_can_rtr, can_sock, node_id, READ_BUFFER_OUT_U32, timeout_s=1.0
        )
        assert frozen is not None
        stable = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v == frozen,
            timeout_s=0.3,
        )
        assert stable == frozen

        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 1)
            == 1
        )

        resumed = wait_until(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_OUT_U32,
            lambda v: v is not None and v > frozen,
            timeout_s=3.0,
        )
        assert resumed is not None and resumed > frozen

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
    def test_programs_buffer_selector_write_echo_can(self, can_sock, node_id):
        """CAN write access: selector write should be visible on CAN read path."""
        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 1)
            == 1
        )

        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 2)
        sel_val = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BUFFER_SEL_U32,
            2,
            timeout_s=2.0,
        )
        assert sel_val == 2

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_programs_buffer_stat_segmented_read_can(self, can_sock, node_id):
        """CAN segmented read access: fetch buffer stat array via read_seg."""
        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 1)
            == 1
        )

        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 1)
        assert (
            wait_for_value(
                read_u32_via_can_rtr,
                can_sock,
                node_id,
                READ_BUFFER_SEL_U32,
                1,
                timeout_s=2.0,
            )
            == 1
        )

        payload = read_bytes_via_can_seg(
            can_sock, node_id, READSEG_BUFFER_STAT_U32, min_len=32
        )
        assert payload is not None, "No segmented buffer_stat_u32 payload"
        assert len(payload) >= 32

        words = struct.unpack("<8I", payload[:32])
        count, depth, _head, _overflow, _snapshot_seq, _runtime_flags, selected, _ = (
            words
        )
        assert depth > 0
        assert count <= depth
        assert count > 0
        assert selected == 1

    @pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
    def test_programs_buffer_overflow_counter_increments(self, can_sock, node_id):
        """Reset buffer, wait for overflow, then confirm the counter increments."""
        assert (
            ensure_control_state(can_sock, node_id, "buffer1", WRITE_CTRL_BUFFER1, 1)
            == 1
        )

        write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
        write_u32_via_can(can_sock, node_id, WRITE_BUFFER_SEL_U32, 0)
        assert (
            wait_for_value(
                read_u32_via_can_rtr,
                can_sock,
                node_id,
                READ_BUFFER_SEL_U32,
                0,
                timeout_s=2.0,
            )
            == 0
        )

        deadline = time.monotonic() + 0.8
        overflow = 0
        while time.monotonic() < deadline and overflow == 0:
            payload = read_bytes_via_can_seg(
                can_sock, node_id, READSEG_BUFFER_STAT_U32, min_len=32
            )
            assert payload is not None, "No segmented buffer_stat_u32 payload"
            assert len(payload) >= 32

            words = struct.unpack("<8I", payload[:32])
            _count, _depth, _head, overflow, _seq, _flags, _selected, _ = words
            if overflow > 0:
                break
            time.sleep(0.01)

        assert overflow > 0
