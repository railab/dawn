############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

import time

import pytest
from _can_common import (
    drain_frames,
    read_u8_via_can_rtr,
    read_u32_via_can_rtr,
    setup_can_socket,
    wait_for_value,
    write_u8_via_can,
)
from _descriptor_common import can_layout, load_descriptor_spec

NODE_ID_POLL_INTERVAL_S = 0.01

DESC_PATH = "descriptors/ntfc/ntfc_programs_can.yaml"

CAN_LAYOUT = can_layout(DESC_PATH, check_collisions=True)
PUSH = CAN_LAYOUT["push"]
READ = CAN_LAYOUT["read"]
WRITE = CAN_LAYOUT["write"]
READ_SEG = CAN_LAYOUT["read_seg"]
WRITE_SEG = CAN_LAYOUT["write_seg"]

PUSH_START = min(PUSH.values())
PUSH_THRESHOLD_BOOL = PUSH["thresh_bool"]
PUSH_THRESHOLD_VALUE_F32 = PUSH["thresh_value_f32"]

READ_LATEST_U32 = READ["latest_u32"]
READ_COUNT_U32 = READ["count_u32"]
READ_MIN_U32 = READ["min_u32"]
READ_MAX_U32 = READ["max_u32"]
READ_SUM_U32 = READ["sum_u32"]
READ_AVG_U32 = READ["avg_u32"]
READ_RMS_U32 = READ["rms_u32"]
READ_RMS_F32 = READ["rms_f32"]
READ_REDIRECT_DUMMY_U32 = READ["redirect_dummy_u32"]
READ_MOVAVG_F32 = READ["movavg_f32"]
READ_IIR_F32 = READ["iir_f32"]
READ_THRESHOLD_BOOL = READ["thresh_bool"]
READ_THRESHOLD_VALUE_F32 = READ["thresh_value_f32"]
READ_BUFFER_OUT_U32 = READ["buffer_out_u32"]
READ_BUFFER_SEL_U32 = READ["buffer_sel_u32"]
READ_ADJUST_OUT_U32 = READ["adjust_out_u32"]
READ_BITPACK_OUT_U32 = READ["bitpack_out_u32"]
READ_BITSPLIT_BIT0_BOOL = READ["bitsplit_bit0_bool"]
READ_BITSPLIT_BIT1_BOOL = READ["bitsplit_bit1_bool"]
READ_COUNTER_OUT_U32 = READ["counter_out_u32"]
READ_EXPRESSION_OUT_U32 = READ["expression_out_u32"]
READ_SELECTOR_OUT_U32 = READ["selector_out_u32"]
READ_SWITCH_OUT_U32 = READ["switch_out_u32"]
READ_TOGGLE_OUT_U32 = READ["toggle_out_u32"]

WRITE_SRC_DRIVE_F32 = WRITE["src_drive_f32"]
WRITE_RMS_RESET_TRIG = WRITE["rms_reset_trig"]
WRITE_CTRL_SAMPLING2 = WRITE["ctrl_sampling2"]
WRITE_CTRL_STATSCOUNT1 = WRITE["ctrl_statscount1"]
WRITE_CTRL_SAMPLING1 = WRITE["ctrl_sampling1"]
WRITE_CTRL_LATEST1 = WRITE["ctrl_latest1"]
WRITE_CTRL_REDIRECT1 = WRITE["ctrl_redirect1"]
WRITE_CTRL_STATSMIN1 = WRITE["ctrl_statsmin1"]
WRITE_CTRL_STATSMAX1 = WRITE["ctrl_statsmax1"]
WRITE_CTRL_STATSSUM1 = WRITE["ctrl_statssum1"]
WRITE_CTRL_STATSAVG1 = WRITE["ctrl_statsavg1"]
WRITE_CTRL_STATSRMS_U32_1 = WRITE["ctrl_statsrms_u32_1"]
WRITE_CTRL_STATSRMS_F32_1 = WRITE["ctrl_statsrms_f32_1"]
WRITE_CTRL_MOVINGAVG_F32_1 = WRITE["ctrl_movingavg_f32_1"]
WRITE_CTRL_IIR_F32_1 = WRITE["ctrl_iir_f32_1"]
WRITE_CTRL_THRESHOLD_BOOL_F32_1 = WRITE["ctrl_threshold_bool_f32_1"]
WRITE_CTRL_THRESHOLD_VALUE_F32_1 = WRITE["ctrl_threshold_value_f32_1"]
WRITE_CTRL_BUFFER1 = WRITE["ctrl_buffer1"]
WRITE_CTRL_ADJUST1 = WRITE["ctrl_adjust1"]
WRITE_BUFFER_SEL_U32 = WRITE["buffer_sel_u32"]
WRITE_SRC_ADJUST_U32 = WRITE["src_adjust_u32"]
WRITE_BITPACK_SRC0_BOOL = WRITE["bitpack_src0_bool"]
WRITE_BITPACK_SRC1_BOOL = WRITE["bitpack_src1_bool"]
WRITE_BITSPLIT_SRC_U32 = WRITE["bitsplit_src_u32"]
WRITE_COUNTER_SRC_BOOL = WRITE["counter_src_bool"]
WRITE_EXPRESSION_SRC_U32 = WRITE["expression_src_u32"]
WRITE_SELECTOR_CTRL_U32 = WRITE["selector_ctrl_u32"]
WRITE_SELECTOR_DATA0_U32 = WRITE["selector_data0_u32"]
WRITE_SELECTOR_DATA1_U32 = WRITE["selector_data1_u32"]
WRITE_SWITCH_SRC0_U32 = WRITE["switch_src0_u32"]
WRITE_SWITCH_SRC1_U32 = WRITE["switch_src1_u32"]
WRITE_TOGGLE_SRC_BOOL = WRITE["toggle_src_bool"]

READSEG_SAMPLED_U64 = READ_SEG["sampled_u64"]
READSEG_BUFFER_STAT_U32 = READ_SEG["buffer_stat_u32"]
WRITESEG_SRC_SAMPLE_U64 = WRITE_SEG["src_sample_u64"]

CONFIG_NODE_ID = 0
CONTROL_STATE_READ = {
    "sampling2": READ["ctrl_sampling2"],
    "statscount1": READ["ctrl_statscount1"],
    "sampling1": READ["ctrl_sampling1"],
    "latest1": READ["ctrl_latest1"],
    "redirect1": READ["ctrl_redirect1"],
    "statsmin1": READ["ctrl_statsmin1"],
    "statsmax1": READ["ctrl_statsmax1"],
    "statssum1": READ["ctrl_statssum1"],
    "statsavg1": READ["ctrl_statsavg1"],
    "statsrms_u32_1": READ["ctrl_statsrms_u32_1"],
    "statsrms_f32_1": READ["ctrl_statsrms_f32_1"],
    "movingavg_f32_1": READ["ctrl_movingavg_f32_1"],
    "iir_f32_1": READ["ctrl_iir_f32_1"],
    "threshold_bool_f32_1": READ["ctrl_threshold_bool_f32_1"],
    "threshold_value_f32_1": READ["ctrl_threshold_value_f32_1"],
    "buffer1": READ["ctrl_buffer1"],
    "adjust1": READ["ctrl_adjust1"],
}

CONTROL_WRITE = {
    "sampling2": WRITE_CTRL_SAMPLING2,
    "statscount1": WRITE_CTRL_STATSCOUNT1,
    "sampling1": WRITE_CTRL_SAMPLING1,
    "latest1": WRITE_CTRL_LATEST1,
    "redirect1": WRITE_CTRL_REDIRECT1,
    "statsmin1": WRITE_CTRL_STATSMIN1,
    "statsmax1": WRITE_CTRL_STATSMAX1,
    "statssum1": WRITE_CTRL_STATSSUM1,
    "statsavg1": WRITE_CTRL_STATSAVG1,
    "statsrms_u32_1": WRITE_CTRL_STATSRMS_U32_1,
    "statsrms_f32_1": WRITE_CTRL_STATSRMS_F32_1,
    "movingavg_f32_1": WRITE_CTRL_MOVINGAVG_F32_1,
    "iir_f32_1": WRITE_CTRL_IIR_F32_1,
    "threshold_bool_f32_1": WRITE_CTRL_THRESHOLD_BOOL_F32_1,
    "threshold_value_f32_1": WRITE_CTRL_THRESHOLD_VALUE_F32_1,
    "buffer1": WRITE_CTRL_BUFFER1,
    "adjust1": WRITE_CTRL_ADJUST1,
}


def _get_program_config_value(program_id, key, default=None):
    desc = load_descriptor_spec(DESC_PATH)
    for prog in desc.get("programs", []):
        if prog.get("id") == program_id:
            return prog.get("config", {}).get(key, default)
    return default


BUFFER1_DEPTH = int(_get_program_config_value("buffer1", "depth", 16))


def get_runtime_node_id(sock, timeout_s=6.0):
    """Verify fixed descriptor-defined node ID responds on latest_u32 RTR."""
    deadline = time.monotonic() + timeout_s
    while time.monotonic() < deadline:
        drain_frames(sock, max_frames=300)
        if read_u32_via_can_rtr(sock, CONFIG_NODE_ID, READ_LATEST_U32) is not None:
            return CONFIG_NODE_ID
        time.sleep(NODE_ID_POLL_INTERVAL_S)
    pytest.fail("Could not read latest_u32 for configured CAN node_id=0")


def wait_control_state(sock, node_id, control_name, expected, timeout_s=2.0):
    can_offset = CONTROL_STATE_READ[control_name]
    return wait_for_value(
        read_u8_via_can_rtr, sock, node_id, can_offset, expected, timeout_s=timeout_s
    )


def ensure_control_state(sock, node_id, control_name, write_offset, expected):
    current = read_u8_via_can_rtr(sock, node_id, CONTROL_STATE_READ[control_name])
    if current == expected:
        return expected

    write_u8_via_can(sock, node_id, write_offset, expected)
    return wait_control_state(sock, node_id, control_name, expected, timeout_s=2.0)


COMMON_PYTESTMARK = [
    pytest.mark.cmd_check("dawn_main"),
    pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN"),
    pytest.mark.dep_config("CONFIG_DAWN_IO_DUMMY_NOTIFY"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_SAMPLING"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_LATEST"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_REDIRECT"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_MOVING_AVG"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_IIR_FILTER"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_COUNT"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_MIN"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_MAX"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_SUM"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_AVG"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_STATS_RMS"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_ADJUST"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_THRESHOLD"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_THRESHOLD_VALUE"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_BUFFER"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_BITPACK"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_BITSPLIT"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_COUNTER"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_EXPRESSION"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_SELECTOR"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_SWITCH"),
    pytest.mark.dep_config("CONFIG_DAWN_PROG_TOGGLE"),
    pytest.mark.dep_config("CONFIG_DAWN_IO_TRIGGER"),
    pytest.mark.dep_config("CONFIG_DAWN_IO_CONTROL"),
]


def start_dawn_and_socket():
    from _ntfc_common import start_dawn

    start_dawn(settle_s=2.0)
    return setup_can_socket()
