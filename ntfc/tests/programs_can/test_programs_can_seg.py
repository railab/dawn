############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for segmented CAN U64 sample transport."""

import pytest
from _can_common import (
    read_u64_via_can_seg,
    wait_for_value,
    write_u64_via_can_seg,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    READSEG_SAMPLED_U64,
    WRITESEG_SRC_SAMPLE_U64,
)

pytestmark = COMMON_PYTESTMARK


def _check_programs_sampling_u64_segmented_can(can_sock, node_id):
    test_value = 0x8877665544332211
    write_u64_via_can_seg(can_sock, node_id, WRITESEG_SRC_SAMPLE_U64, test_value)
    observed = wait_for_value(
        read_u64_via_can_seg,
        can_sock,
        node_id,
        READSEG_SAMPLED_U64,
        test_value,
        timeout_s=2.5,
    )
    assert observed == test_value, f"Expected {test_value:#x}, got {observed!r}"


def _check_programs_sampling_u64_latest_write_wins(can_sock, node_id):
    first = 0x0102030405060708
    second = 0xFFEEDDCCBBAA0099

    write_u64_via_can_seg(can_sock, node_id, WRITESEG_SRC_SAMPLE_U64, first)
    write_u64_via_can_seg(can_sock, node_id, WRITESEG_SRC_SAMPLE_U64, second)

    observed = wait_for_value(
        read_u64_via_can_seg,
        can_sock,
        node_id,
        READSEG_SAMPLED_U64,
        second,
        timeout_s=2.5,
    )
    assert observed == second, f"Expected latest {second:#x}, got {observed!r}"


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_SEG")
def test_programs_sampling_u64_segmented_suite(can_sock, node_id):
    """Run segmented U64 sampling checks, including latest-write behavior."""

    _check_programs_sampling_u64_segmented_can(can_sock, node_id)
    _check_programs_sampling_u64_latest_write_wins(can_sock, node_id)
