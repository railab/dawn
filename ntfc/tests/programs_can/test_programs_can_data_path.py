############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN program data-path processing chains."""

import math
import time

import pytest
from _can_common import (
    read_f32_via_can_rtr,
    read_u8_via_can_rtr,
    read_u32_via_can_rtr,
    wait_for_non_none,
    wait_for_value,
    wait_until,
    write_f32_via_can,
    write_u8_via_can,
    write_u32_via_can,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    READ_ADJUST_OUT_U32,
    READ_AVG_U32,
    READ_COUNT_U32,
    READ_IIR_F32,
    READ_LATEST_U32,
    READ_MAX_U32,
    READ_MIN_U32,
    READ_MOVAVG_F32,
    READ_REDIRECT_DUMMY_U32,
    READ_RMS_F32,
    READ_RMS_U32,
    READ_SUM_U32,
    READ_THRESHOLD_BOOL,
    READ_THRESHOLD_VALUE_F32,
    WRITE_RMS_RESET_TRIG,
    WRITE_SRC_ADJUST_U32,
    WRITE_SRC_DRIVE_F32,
)

pytestmark = COMMON_PYTESTMARK


def _check_programs_u32_pipeline_stats_and_latest(can_sock, node_id):
    baseline_count = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_COUNT_U32, timeout_s=5.0
    )
    if baseline_count is None:
        pytest.fail("Failed to read baseline count over CAN RTR")

    first_latest = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_LATEST_U32, timeout_s=2.0
    )
    first_min = read_u32_via_can_rtr(can_sock, node_id, READ_MIN_U32)
    first_max = read_u32_via_can_rtr(can_sock, node_id, READ_MAX_U32)

    count = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value is not None and value > baseline_count,
        timeout_s=2.0,
    )
    latest = read_u32_via_can_rtr(can_sock, node_id, READ_LATEST_U32)
    min_v = read_u32_via_can_rtr(can_sock, node_id, READ_MIN_U32)
    max_v = read_u32_via_can_rtr(can_sock, node_id, READ_MAX_U32)

    assert latest is not None, "No latest_u32 response"
    assert count is not None, "No count_u32 response"
    assert min_v is not None, "No min_u32 response"
    assert max_v is not None, "No max_u32 response"
    assert first_latest is not None and first_min is not None and first_max is not None

    assert count > baseline_count
    assert min_v <= latest <= max_v
    assert min_v <= first_min
    assert max_v >= first_max
    assert latest != first_latest or count >= baseline_count + 2

    pre_reset_count = count
    pre_reset_min = min_v
    pre_reset_max = max_v

    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    post_count = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value is not None and value < pre_reset_count,
        timeout_s=2.0,
    )
    post_latest = read_u32_via_can_rtr(can_sock, node_id, READ_LATEST_U32)
    post_min = read_u32_via_can_rtr(can_sock, node_id, READ_MIN_U32)
    post_max = read_u32_via_can_rtr(can_sock, node_id, READ_MAX_U32)

    assert post_latest is not None and post_count is not None
    assert post_min is not None and post_max is not None

    assert post_count < pre_reset_count
    assert post_min <= post_latest <= post_max
    assert post_min >= pre_reset_min
    assert post_max >= pre_reset_max


def _check_programs_statsrms_float_can(can_sock, node_id):
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 12.0)
    time.sleep(0.12)
    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    high = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_RMS_F32,
        12.0,
        timeout_s=2.5,
        tol=0.6,
    )
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 3.0)
    time.sleep(0.12)
    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    low_after_reset = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_RMS_F32,
        3.0,
        timeout_s=2.5,
        tol=0.25,
    )

    assert high is not None
    assert low_after_reset is not None
    assert math.isfinite(high)
    assert math.isfinite(low_after_reset)
    assert high >= 0.0 and low_after_reset >= 0.0
    assert abs(high - 12.0) <= 0.6
    assert abs(low_after_reset - 3.0) <= 0.25
    assert low_after_reset + 1.0 < high


def _check_programs_redirect_to_dummy_u32(can_sock, node_id):
    first_latest = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_LATEST_U32, timeout_s=5.0
    )
    first_redirect = wait_for_non_none(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_REDIRECT_DUMMY_U32,
        timeout_s=5.0,
    )
    assert first_latest is not None
    assert first_redirect is not None

    latest_before = read_u32_via_can_rtr(can_sock, node_id, READ_LATEST_U32)
    redirected = read_u32_via_can_rtr(can_sock, node_id, READ_REDIRECT_DUMMY_U32)
    latest_after = read_u32_via_can_rtr(can_sock, node_id, READ_LATEST_U32)

    assert (
        latest_before is not None
        and redirected is not None
        and latest_after is not None
    )

    lo = min(latest_before, latest_after)
    hi = max(latest_before, latest_after)
    slack = 400000
    assert lo - slack <= redirected <= hi + slack

    redirected_2 = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_REDIRECT_DUMMY_U32,
        lambda value: value is not None and value != first_redirect,
        timeout_s=1.5,
    )
    latest_2 = read_u32_via_can_rtr(can_sock, node_id, READ_LATEST_U32)
    assert redirected_2 is not None and latest_2 is not None
    assert latest_2 != first_latest or redirected_2 != first_redirect


def _check_programs_statsrms_u32_can(can_sock, node_id):
    first = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_RMS_U32, timeout_s=5.0
    )
    second = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_RMS_U32,
        lambda value: value is not None and value != first,
        timeout_s=2.0,
    )

    assert first is not None
    assert second is not None
    assert second > 0
    assert first != second


def _check_programs_sum_and_avg_can(can_sock, node_id):
    sum_1 = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_SUM_U32, timeout_s=5.0
    )
    avg_1 = wait_for_non_none(
        read_u32_via_can_rtr, can_sock, node_id, READ_AVG_U32, timeout_s=5.0
    )
    count_1 = read_u32_via_can_rtr(can_sock, node_id, READ_COUNT_U32)

    count_2 = wait_until(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_COUNT_U32,
        lambda value: value is not None and value > count_1,
        timeout_s=2.0,
    )
    sum_2 = read_u32_via_can_rtr(can_sock, node_id, READ_SUM_U32)
    avg_2 = read_u32_via_can_rtr(can_sock, node_id, READ_AVG_U32)
    min_2 = read_u32_via_can_rtr(can_sock, node_id, READ_MIN_U32)
    max_2 = read_u32_via_can_rtr(can_sock, node_id, READ_MAX_U32)

    assert sum_1 is not None and sum_2 is not None
    assert avg_1 is not None and avg_2 is not None
    assert count_1 is not None and count_2 is not None
    assert min_2 is not None and max_2 is not None

    assert count_2 > count_1
    assert sum_2 != sum_1
    assert 0 < avg_2 <= sum_2
    assert min_2 <= avg_2 <= max_2


def _check_programs_adjust_can(can_sock, node_id):
    write_u32_via_can(can_sock, node_id, WRITE_SRC_ADJUST_U32, 10)
    first = wait_for_value(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_ADJUST_OUT_U32,
        23,
        timeout_s=2.0,
    )
    write_u32_via_can(can_sock, node_id, WRITE_SRC_ADJUST_U32, 0)
    second = wait_for_value(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_ADJUST_OUT_U32,
        3,
        timeout_s=2.0,
    )

    assert first == 23
    assert second == 3


def _check_programs_filters_float_can(can_sock, node_id):
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 8.0)
    time.sleep(0.04)
    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)

    high_mov = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_MOVAVG_F32,
        8.0,
        timeout_s=2.5,
        tol=0.25,
    )
    high_iir = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_IIR_F32,
        8.0,
        timeout_s=2.5,
        tol=0.4,
    )

    assert high_mov is not None and high_iir is not None

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)

    low_mov = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_MOVAVG_F32,
        0.0,
        timeout_s=3.0,
        tol=0.2,
    )
    low_iir = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_IIR_F32,
        0.0,
        timeout_s=3.0,
        tol=0.4,
    )

    assert low_mov is not None
    assert low_iir is not None
    assert math.isfinite(low_mov) and math.isfinite(low_iir)
    assert high_mov > low_mov
    assert high_iir > low_iir


def _check_programs_threshold_bool_hysteresis_can(can_sock, node_id):
    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_low = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        0,
        timeout_s=0.8,
    )
    assert v_low == 0

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 4.5)
    v_high = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        1,
        timeout_s=0.8,
    )
    assert v_high == 1

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 3.0)
    v_mid_hold = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        1,
        timeout_s=0.8,
    )
    assert v_mid_hold == 1

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_clear = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        0,
        timeout_s=0.8,
    )
    assert v_clear == 0

    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_reset_low = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        0,
        timeout_s=0.8,
    )
    assert v_reset_low == 0

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 3.0)
    v_mid_after_reset = wait_for_value(
        read_u8_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_BOOL,
        0,
        timeout_s=0.8,
    )
    assert v_mid_after_reset == 0


def _check_programs_threshold_value_hysteresis_can(can_sock, node_id):
    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_low = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        0.0,
        timeout_s=0.8,
        tol=0.05,
    )
    assert v_low is not None and abs(v_low - 0.0) <= 0.05

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 4.5)
    v_high = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        4.5,
        timeout_s=0.8,
        tol=0.35,
    )
    assert v_high is not None

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 3.0)
    v_mid_hold = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        3.0,
        timeout_s=0.8,
        tol=0.3,
    )
    assert v_mid_hold is not None

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_clear = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        0.0,
        timeout_s=0.8,
        tol=0.08,
    )
    assert v_clear is not None

    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 0.0)
    v_reset_low = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        0.0,
        timeout_s=0.8,
        tol=0.05,
    )
    assert v_reset_low is not None

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 3.0)
    v_mid_after_reset = wait_for_value(
        read_f32_via_can_rtr,
        can_sock,
        node_id,
        READ_THRESHOLD_VALUE_F32,
        0.0,
        timeout_s=0.8,
        tol=0.05,
    )
    assert v_mid_after_reset is not None


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_rtr_data_path_suite(can_sock, node_id):
    """Run RTR-based data-path checks for stats, filters, and thresholds."""

    _check_programs_u32_pipeline_stats_and_latest(can_sock, node_id)
    _check_programs_statsrms_float_can(can_sock, node_id)
    _check_programs_redirect_to_dummy_u32(can_sock, node_id)
    _check_programs_statsrms_u32_can(can_sock, node_id)
    _check_programs_sum_and_avg_can(can_sock, node_id)
    _check_programs_adjust_can(can_sock, node_id)
    _check_programs_filters_float_can(can_sock, node_id)
    _check_programs_threshold_bool_hysteresis_can(can_sock, node_id)
    _check_programs_threshold_value_hysteresis_can(can_sock, node_id)
