############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for CAN push-notification program outputs."""

import math
import socket
import struct
import time

from _can_common import (
    drain_frames,
    read_u8_via_can_rtr,
    wait_for_value,
    write_f32_via_can,
    write_u8_via_can,
)
from _programs_can_common import (
    COMMON_PYTESTMARK,
    PUSH_THRESHOLD_BOOL,
    PUSH_THRESHOLD_VALUE_F32,
    READ_THRESHOLD_BOOL,
    WRITE_RMS_RESET_TRIG,
    WRITE_SRC_DRIVE_F32,
)
from dawnpy_can.can import parse_can_frame

pytestmark = COMMON_PYTESTMARK


def test_programs_threshold_push_notifications_can(can_sock, node_id):
    """Verify threshold push notifications after driving input above limit."""

    bool_push_id = node_id + PUSH_THRESHOLD_BOOL
    value_push_id = node_id + PUSH_THRESHOLD_VALUE_F32

    write_u8_via_can(can_sock, node_id, WRITE_RMS_RESET_TRIG, 0)
    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 1.0)
    assert (
        wait_for_value(
            read_u8_via_can_rtr,
            can_sock,
            node_id,
            READ_THRESHOLD_BOOL,
            0,
            timeout_s=0.4,
        )
        == 0
    )
    drain_frames(can_sock)

    write_f32_via_can(can_sock, node_id, WRITE_SRC_DRIVE_F32, 4.5)
    assert (
        wait_for_value(
            read_u8_via_can_rtr,
            can_sock,
            node_id,
            READ_THRESHOLD_BOOL,
            1,
            timeout_s=0.4,
        )
        == 1
    )

    deadline = time.monotonic() + 0.4
    bool_seen = None
    value_seen = None
    while time.monotonic() < deadline and (bool_seen != 1 or value_seen is None):
        try:
            parsed = parse_can_frame(can_sock.recv(16))
        except socket.timeout:
            continue

        if parsed.get("rtr"):
            continue
        if parsed["can_id"] == bool_push_id and parsed.get("dlc", 0) >= 1:
            bool_seen = struct.unpack("<B", parsed["data"][:1])[0]
            continue
        if parsed["can_id"] == value_push_id and parsed.get("dlc", 0) >= 4:
            value = struct.unpack("<f", parsed["data"][:4])[0]
            if math.isfinite(value) and abs(value - 4.5) <= 0.5:
                value_seen = value

    assert bool_seen == 1
    assert value_seen is not None
