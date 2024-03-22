############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN-exposed program control channels."""

import pytest
from _can_common import (
    read_u32_via_can_rtr,
    read_u64_via_can_seg,
    wait_for_non_none,
    wait_for_value,
    wait_until,
    write_u8_via_can,
    write_u64_via_can_seg,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    CONTROL_WRITE,
    READ_COUNT_U32,
    READSEG_SAMPLED_U64,
    WRITE_CTRL_SAMPLING2,
    WRITE_CTRL_STATSCOUNT1,
    WRITESEG_SRC_SAMPLE_U64,
    ensure_control_state,
    wait_control_state,
)

pytestmark = COMMON_PYTESTMARK

ALL_CONTROL_CHANNELS = tuple(CONTROL_WRITE.items())


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
def test_programs_control_sampling_start_stop(can_sock, node_id):
    """Stop/start sampling program and verify segmented output freezes/resumes."""

    v1 = 0x1111222233334444
    v2 = 0xAAAABBBBCCCCDDDD

    write_u64_via_can_seg(can_sock, node_id, WRITESEG_SRC_SAMPLE_U64, v1)
    observed = wait_for_value(
        read_u64_via_can_seg,
        can_sock,
        node_id,
        READSEG_SAMPLED_U64,
        v1,
        timeout_s=6.0,
    )
    assert observed == v1

    write_u8_via_can(can_sock, node_id, WRITE_CTRL_SAMPLING2, 0)
    assert wait_control_state(can_sock, node_id, "sampling2", 0, timeout_s=2.0) == 0

    write_u64_via_can_seg(can_sock, node_id, WRITESEG_SRC_SAMPLE_U64, v2)
    stopped_val = read_u64_via_can_seg(can_sock, node_id, READSEG_SAMPLED_U64)
    assert stopped_val == v1

    write_u8_via_can(can_sock, node_id, WRITE_CTRL_SAMPLING2, 1)
    assert wait_control_state(can_sock, node_id, "sampling2", 1, timeout_s=2.0) == 1
    resumed = wait_for_value(
        read_u64_via_can_seg,
        can_sock,
        node_id,
        READSEG_SAMPLED_U64,
        v2,
        timeout_s=3.0,
    )
    assert resumed == v2


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_control_process_start_stop(can_sock, node_id):
    """Stop/start process program and verify counter progression behavior."""

    c1 = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_COUNT_U32, timeout_s=5.0
    )
    assert c1 is not None
    c2 = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value is not None and value > c1,
        timeout_s=2.0,
    )
    assert c2 is not None
    assert c2 > c1

    write_u8_via_can(can_sock, node_id, WRITE_CTRL_STATSCOUNT1, 0)
    assert wait_control_state(can_sock, node_id, "statscount1", 0, timeout_s=2.0) == 0
    cs1 = read_u32_via_can_rtr(can_sock, node_id, READ_COUNT_U32)
    cs2 = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value == cs1,
        timeout_s=0.3,
    )
    assert cs1 is not None and cs2 is not None
    assert cs2 == cs1

    write_u8_via_can(can_sock, node_id, WRITE_CTRL_STATSCOUNT1, 1)
    assert wait_control_state(can_sock, node_id, "statscount1", 1, timeout_s=2.0) == 1
    cr1 = read_u32_via_can_rtr(can_sock, node_id, READ_COUNT_U32)
    assert cr1 is not None
    cr2 = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value is not None and value > cr1,
        timeout_s=2.0,
    )
    assert cr2 is not None
    assert cr2 > cr1


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_all_control_channels_toggle(can_sock, node_id):
    """Toggle all exposed control channels and verify resulting states."""

    for control_name, write_offset in ALL_CONTROL_CHANNELS:
        assert (
            ensure_control_state(can_sock, node_id, control_name, write_offset, 0) == 0
        )
        assert (
            ensure_control_state(can_sock, node_id, control_name, write_offset, 1) == 1
        )
