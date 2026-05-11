############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC test for Dawn protocol writes published as NuttX user sensors."""

import math
import re
import socket
import time

import pytest
from _can_common import (
    assert_can_bus_idle,
    open_can_socket,
    write_f32_pair_via_can,
    write_f32_via_can,
)
from _descriptor_common import can_layout
from _ntfc_common import start_dawn
from dawnpy_can.can import parse_can_frame

DESC_PATH = "descriptors/examples/can_sensor_producer.yaml"
CAN_LAYOUT = can_layout(DESC_PATH)

PUSH_START = CAN_LAYOUT["push"]["can_timestamp_io1"]
WRITE_TEMP_START = CAN_LAYOUT["write"]["can_temp_sensor_pub"]
WRITE_HUMI_START = CAN_LAYOUT["write"]["can_humi_sensor_pub"]
WRITE_BARO_START = CAN_LAYOUT["write"]["can_baro_sensor_pub"]
WRITE_LIGHT_START = CAN_LAYOUT["write"]["can_light_sensor_pub"]
READER_POLL_TIMEOUT_MS = 30000


@pytest.fixture(scope="module", autouse=True)
def _require_idle_can_bus():
    assert_can_bus_idle()


def get_runtime_node_id(sock, timeout_s=2.5):
    old_timeout = sock.gettimeout()
    sock.settimeout(0.25)
    deadline = time.monotonic() + timeout_s

    try:
        while time.monotonic() < deadline:
            try:
                frame = sock.recv(16)
            except socket.timeout:
                continue

            parsed = parse_can_frame(frame)
            if parsed.get("rtr"):
                continue
            if parsed.get("dlc") != 8:
                continue

            return parsed["can_id"] - PUSH_START
    finally:
        sock.settimeout(old_timeout)

    pytest.fail("Could not discover CAN node ID from push notifications")


def start_reader(product, path, count):
    product.sendCommand(
        f"usensor_reader {path} {count} {READER_POLL_TIMEOUT_MS} &",
        timeout=1,
    )


def expect_reader_values(product, label, expected):
    ret = product.readUntilPattern(f"usensor_reader {label}=".encode(), timeout=8)
    match = re.search(
        rf"usensor_reader {label}=([-+0-9.,]+)",
        ret.output,
    )
    assert match is not None, ret.output

    observed = [float(value) for value in match.group(1).split(",")]
    assert len(observed) == len(expected), ret.output
    for obs, exp in zip(observed, expected):
        assert math.isclose(obs, exp, rel_tol=0.0, abs_tol=0.001)


@pytest.mark.cmd_check("dawn_main")
@pytest.mark.dep_config("CONFIG_DAWN_IO_SENSOR_PRODUCER")
@pytest.mark.dep_config("CONFIG_EXAMPLES_USENSOR_READER")
@pytest.mark.dep_config("CONFIG_DAWN_PROTO_CAN")
def test_can_writes_are_read_by_parallel_usensor_apps():
    updates = (
        {
            "temp": (23.5,),
            "humi": (45.0,),
            "baro": (1001.25, 22.75),
            "light": (320.0, 12.5),
        },
        {
            "temp": (24.25,),
            "humi": (50.5,),
            "baro": (1003.75, 23.5),
            "light": (280.25, 10.0),
        },
        {
            "temp": (19.75,),
            "humi": (41.25,),
            "baro": (998.5, 18.25),
            "light": (120.0, 4.75),
        },
    )
    product = pytest.products[0].core(0)
    count = len(updates)

    start_reader(product, "/dev/uorb/sensor_temp10", count)
    start_reader(product, "/dev/uorb/sensor_humi11", count)
    start_reader(product, "/dev/uorb/sensor_baro12", count)
    start_reader(product, "/dev/uorb/sensor_light13", count)

    start_dawn()

    with open_can_socket() as sock:
        node_id = get_runtime_node_id(sock)

        for update in updates:
            write_f32_via_can(sock, node_id, WRITE_TEMP_START, update["temp"][0])
            expect_reader_values(product, "temp", update["temp"])

            write_f32_via_can(sock, node_id, WRITE_HUMI_START, update["humi"][0])
            expect_reader_values(product, "humi", update["humi"])

            write_f32_pair_via_can(
                sock,
                node_id,
                WRITE_BARO_START,
                update["baro"][0],
                update["baro"][1],
            )
            expect_reader_values(product, "baro", update["baro"])

            write_f32_pair_via_can(
                sock,
                node_id,
                WRITE_LIGHT_START,
                update["light"][0],
                update["light"][1],
            )
            expect_reader_values(product, "light", update["light"])
