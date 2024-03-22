############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN-exposed logic/bit programs."""

import time

import pytest
from _can_common import (
    read_u8_via_can_rtr,
    read_u32_via_can_rtr,
    wait_for_value,
    write_u8_via_can,
    write_u32_via_can,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    READ_BITPACK_OUT_U32,
    READ_BITSPLIT_BIT0_BOOL,
    READ_BITSPLIT_BIT1_BOOL,
    READ_COUNTER_OUT_U32,
    READ_EXPRESSION_OUT_U32,
    READ_SELECTOR_OUT_U32,
    READ_SWITCH_OUT_U32,
    READ_TOGGLE_OUT_U32,
    WRITE_BITPACK_SRC0_BOOL,
    WRITE_BITPACK_SRC1_BOOL,
    WRITE_BITSPLIT_SRC_U32,
    WRITE_COUNTER_SRC_BOOL,
    WRITE_EXPRESSION_SRC_U32,
    WRITE_SELECTOR_CTRL_U32,
    WRITE_SELECTOR_DATA0_U32,
    WRITE_SELECTOR_DATA1_U32,
    WRITE_SWITCH_SRC0_U32,
    WRITE_SWITCH_SRC1_U32,
    WRITE_TOGGLE_SRC_BOOL,
)

pytestmark = COMMON_PYTESTMARK


def _write_bool_edge(can_sock, node_id, can_offset):
    write_u8_via_can(can_sock, node_id, can_offset, 0)
    time.sleep(0.1)
    write_u8_via_can(can_sock, node_id, can_offset, 1)


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_bitpack_bool_inputs(can_sock, node_id):
    """Pack two writable bool inputs into a uint32 bitmask."""

    cases = (
        (0, 0, 0),
        (1, 0, 1),
        (0, 1, 2),
        (1, 1, 3),
    )

    for in0, in1, expected in cases:
        write_u8_via_can(can_sock, node_id, WRITE_BITPACK_SRC0_BOOL, in0)
        write_u8_via_can(can_sock, node_id, WRITE_BITPACK_SRC1_BOOL, in1)
        observed = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_BITPACK_OUT_U32,
            expected,
            timeout_s=1.0,
        )
        assert observed == expected


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_bitsplit_bool_outputs(can_sock, node_id):
    """Split bits 0 and 1 from a uint32 source into bool outputs."""

    cases = (
        (0, 0, 0),
        (1, 1, 0),
        (2, 0, 1),
        (3, 1, 1),
    )

    for src, bit0, bit1 in cases:
        write_u32_via_can(can_sock, node_id, WRITE_BITSPLIT_SRC_U32, src)
        observed0 = wait_for_value(
            read_u8_via_can_rtr,
            can_sock,
            node_id,
            READ_BITSPLIT_BIT0_BOOL,
            bit0,
            timeout_s=1.0,
        )
        observed1 = wait_for_value(
            read_u8_via_can_rtr,
            can_sock,
            node_id,
            READ_BITSPLIT_BIT1_BOOL,
            bit1,
            timeout_s=1.0,
        )
        assert observed0 == bit0
        assert observed1 == bit1


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_counter_rising_edges(can_sock, node_id):
    """Increment counter only on rising edges and wrap at configured max."""

    expected_values = (1, 2, 3, 0)

    write_u8_via_can(can_sock, node_id, WRITE_COUNTER_SRC_BOOL, 0)
    for expected in expected_values:
        _write_bool_edge(can_sock, node_id, WRITE_COUNTER_SRC_BOOL)
        observed = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_COUNTER_OUT_U32,
            expected,
            timeout_s=1.0,
        )
        assert observed == expected

        write_u8_via_can(can_sock, node_id, WRITE_COUNTER_SRC_BOOL, 1)
        held = read_u32_via_can_rtr(can_sock, node_id, READ_COUNTER_OUT_U32)
        assert held == expected

        write_u8_via_can(can_sock, node_id, WRITE_COUNTER_SRC_BOOL, 0)
        time.sleep(0.1)


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_expression_add_constant(can_sock, node_id):
    """Apply deterministic uint32 expression output over CAN."""

    for src, expected in ((0, 10), (5, 15), (0x1000, 0x100A)):
        write_u32_via_can(can_sock, node_id, WRITE_EXPRESSION_SRC_U32, src)
        observed = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_EXPRESSION_OUT_U32,
            expected,
            timeout_s=1.0,
        )
        assert observed == expected


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_selector_routes_data(can_sock, node_id):
    """Route selected data input and ignore out-of-range control values."""

    write_u32_via_can(can_sock, node_id, WRITE_SELECTOR_DATA0_U32, 0xAAAA)
    write_u32_via_can(can_sock, node_id, WRITE_SELECTOR_DATA1_U32, 0xBBBB)

    write_u32_via_can(can_sock, node_id, WRITE_SELECTOR_CTRL_U32, 0)
    observed = wait_for_value(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_SELECTOR_OUT_U32,
        0xAAAA,
        timeout_s=1.0,
    )
    assert observed == 0xAAAA

    write_u32_via_can(can_sock, node_id, WRITE_SELECTOR_CTRL_U32, 1)
    observed = wait_for_value(
        read_u32_via_can_rtr,
        can_sock,
        node_id,
        READ_SELECTOR_OUT_U32,
        0xBBBB,
        timeout_s=1.0,
    )
    assert observed == 0xBBBB

    write_u32_via_can(can_sock, node_id, WRITE_SELECTOR_CTRL_U32, 5)
    time.sleep(0.05)
    assert read_u32_via_can_rtr(can_sock, node_id, READ_SELECTOR_OUT_U32) == 0xBBBB


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_switch_and_gate(can_sock, node_id):
    """Write the on value only when all switch inputs match."""

    cases = (
        (0, 0, 0),
        (1, 0, 0),
        (0, 1, 0),
        (1, 1, 66),
    )

    for in0, in1, expected in cases:
        write_u32_via_can(can_sock, node_id, WRITE_SWITCH_SRC0_U32, in0)
        write_u32_via_can(can_sock, node_id, WRITE_SWITCH_SRC1_U32, in1)
        observed = wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_SWITCH_OUT_U32,
            expected,
            timeout_s=1.0,
        )
        assert observed == expected


@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN_RTR")
def test_programs_toggle_rising_edges(can_sock, node_id):
    """Toggle output on rising edges only."""

    write_u8_via_can(can_sock, node_id, WRITE_TOGGLE_SRC_BOOL, 0)
    assert (
        wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_TOGGLE_OUT_U32,
            0,
            timeout_s=1.0,
        )
        == 0
    )

    _write_bool_edge(can_sock, node_id, WRITE_TOGGLE_SRC_BOOL)
    assert (
        wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_TOGGLE_OUT_U32,
            1,
            timeout_s=1.0,
        )
        == 1
    )

    write_u8_via_can(can_sock, node_id, WRITE_TOGGLE_SRC_BOOL, 1)
    time.sleep(0.05)
    assert read_u32_via_can_rtr(can_sock, node_id, READ_TOGGLE_OUT_U32) == 1

    _write_bool_edge(can_sock, node_id, WRITE_TOGGLE_SRC_BOOL)
    assert (
        wait_for_value(
            read_u32_via_can_rtr,
            can_sock,
            node_id,
            READ_TOGGLE_OUT_U32,
            0,
            timeout_s=1.0,
        )
        == 0
    )
