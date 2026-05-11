############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC test for NimBLE writes published as NuttX user sensors."""

from __future__ import annotations

import asyncio
import math
import re
import struct
import time
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd
from bleak import BleakClient, BleakScanner
from dawnpy.descriptor.client import load_client_descriptor

DESCRIPTOR = (
    Path(__file__).resolve().parents[3]
    / "descriptors/examples/nimble_sensor_producer.yaml"
)
CLIENT_DESCRIPTOR = load_client_descriptor(str(DESCRIPTOR))
GAP_NAME = str(CLIENT_DESCRIPTOR.protocols[0].config["gap_name"])
CUSTOM_CFG = CLIENT_DESCRIPTOR.protocols[0].config["services"]["custom"][0]
CHAR_UUIDS = {
    "temp": str(CUSTOM_CFG["characteristics"][0]["uuid"]).lower(),
    "humi": str(CUSTOM_CFG["characteristics"][1]["uuid"]).lower(),
    "baro": str(CUSTOM_CFG["characteristics"][2]["uuid"]).lower(),
    "light": str(CUSTOM_CFG["characteristics"][3]["uuid"]).lower(),
}
ADV_SETTLE_S = 3.0
READER_POLL_TIMEOUT_MS = 45000


def _run(coro):
    return asyncio.run(coro)


async def _find_device():
    device = await BleakScanner.find_device_by_name(GAP_NAME, timeout=10.0)
    if device is not None:
        return device

    devices = await BleakScanner.discover(timeout=10.0, return_adv=True)
    seen = []
    for candidate, adv in devices.values():
        names = {
            getattr(candidate, "name", None),
            getattr(adv, "local_name", None),
        }
        seen.extend(str(name) for name in names if name)
        if GAP_NAME in names:
            return candidate

    seen_names = ", ".join(sorted(set(seen))) or "none"
    raise RuntimeError(
        f"BLE device '{GAP_NAME}' not found; discovered names: {seen_names}"
    )


async def _write_sensor_updates(updates, expect):
    device = await _find_device()
    client = BleakClient(device)
    await client.connect()
    try:
        for update in updates:
            await client.write_gatt_char(
                CHAR_UUIDS["temp"], struct.pack("<f", update["temp"][0])
            )
            expect("temp", update["temp"])

            await client.write_gatt_char(
                CHAR_UUIDS["humi"], struct.pack("<f", update["humi"][0])
            )
            expect("humi", update["humi"])

            await client.write_gatt_char(
                CHAR_UUIDS["baro"], struct.pack("<ff", *update["baro"])
            )
            expect("baro", update["baro"])

            await client.write_gatt_char(
                CHAR_UUIDS["light"], struct.pack("<ff", *update["light"])
            )
            expect("light", update["light"])
    finally:
        await client.disconnect()


def start_reader(product, path, count):
    product.sendCommand(
        f"usensor_reader {path} {count} {READER_POLL_TIMEOUT_MS} &",
        timeout=1,
    )


def expect_reader_values(product, label, expected):
    ret = product.readUntilPattern(f"usensor_reader {label}=".encode(), timeout=10)
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
@pytest.mark.dep_config("CONFIG_DAWN_PROTO_NIMBLE")
def test_nimble_writes_are_read_by_parallel_usensor_apps():
    updates = (
        {
            "temp": (23.5,),
            "humi": (45.0,),
            "baro": (1001.25, 22.75),
            "light": (320.0, 12.5),
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

    product.sendCommand("dawn &", timeout=2)
    time.sleep(ADV_SETTLE_S)

    ret = shell_cmd(product, "ps", timeout=2)
    assert "dawn" in ret.output, ret.output

    _run(
        _write_sensor_updates(
            updates,
            lambda label, expected: expect_reader_values(product, label, expected),
        )
    )
