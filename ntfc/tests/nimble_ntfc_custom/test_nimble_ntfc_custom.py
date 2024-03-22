# ntfc/tests/nimble_ntfc_custom/test_nimble_ntfc_custom.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC tests for descriptor-defined generic NimBLE custom services."""

from __future__ import annotations

import asyncio
import struct
import time
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd
from bleak import BleakClient, BleakScanner
from dawnpy.descriptor.client import load_client_descriptor
from dawnpy_ble.profile import BleTransportProfile

DESCRIPTOR = (
    Path(__file__).resolve().parents[3] / "descriptors/ntfc/ntfc_nimble_custom.yaml"
)
CLIENT_DESCRIPTOR = load_client_descriptor(str(DESCRIPTOR))
GAP_NAME = str(CLIENT_DESCRIPTOR.protocols[0].config["gap_name"])
assert len(GAP_NAME) <= 10
_CUSTOM_CFG = CLIENT_DESCRIPTOR.protocols[0].config["services"]["custom"][0]
UUID_CUSTOM_SERVICE = str(_CUSTOM_CFG["uuid"]).lower()
UUID_CUSTOM_CHAR = str(_CUSTOM_CFG["characteristics"][0]["uuid"]).lower()
ADV_SETTLE_S = 3.0


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


async def _connect_client():
    device = await _find_device()
    client = BleakClient(device)
    await client.connect()
    return device, client


async def _get_services(client):
    get_services = getattr(client, "get_services", None)
    if callable(get_services):
        return await get_services()
    return getattr(client, "services", None)


def _uuid(value) -> str:
    return str(value).lower()


def _iter_services(services):
    if isinstance(services, dict):
        return services.values()
    registry = getattr(services, "services", None)
    if isinstance(registry, dict):
        return registry.values()
    return services or []


class TestNimbleNtfcCustom:
    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NIMBLE"),
    ]

    @pytest.fixture(scope="class", autouse=True)
    def _start_dawn(self):
        product = pytest.products[0].core(0)
        product.sendCommand("dawn &", timeout=2)
        time.sleep(ADV_SETTLE_S)
        TestNimbleNtfcCustom._product = product
        TestNimbleNtfcCustom._profile = BleTransportProfile.from_descriptor(
            CLIENT_DESCRIPTOR
        )
        yield

    def test_dawn_started_and_shell_is_alive(self):
        ret = shell_cmd(self._product, "ps", timeout=2)
        assert "dawn" in ret.output, ret.output

    def test_profile_exposes_custom_bindings(self):
        customs = [
            b
            for b in self._profile.iter_bindings()
            if getattr(b, "source", "") == "custom"
        ]
        assert len(customs) == 1

    def test_custom_service_is_present(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                for service in _iter_services(services):
                    if _uuid(service.uuid) != UUID_CUSTOM_SERVICE:
                        continue
                    chars = list(getattr(service, "characteristics", []) or [])
                    return chars
                raise AssertionError("custom service not found")
            finally:
                await client.disconnect()

        chars = _run(go())
        assert any(_uuid(char.uuid) == UUID_CUSTOM_CHAR for char in chars)

    def test_custom_characteristic_rw_roundtrip(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                for service in _iter_services(services):
                    if _uuid(service.uuid) != UUID_CUSTOM_SERVICE:
                        continue
                    chars = list(getattr(service, "characteristics", []) or [])
                    target = next(
                        char for char in chars if _uuid(char.uuid) == UUID_CUSTOM_CHAR
                    )
                    break
                else:
                    raise AssertionError("custom characteristic not found")

                payload = struct.pack("<I", 4321)
                await client.write_gatt_char(target, payload)
                data = bytes(await client.read_gatt_char(target))
                return data
            finally:
                await client.disconnect()

        data = _run(go())
        assert struct.unpack("<I", data[:4])[0] == 4321
