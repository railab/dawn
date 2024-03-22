# ntfc/tests/nimble_ntfc_buffer/test_nimble_ntfc_buffer.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC tests for chunked buffer transfer over custom NimBLE."""

from __future__ import annotations

import asyncio
import struct
import time
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd
from bleak import BleakClient, BleakScanner
from dawnpy.descriptor.client import load_client_descriptor

DESCRIPTOR = (
    Path(__file__).resolve().parents[3] / "descriptors/ntfc/ntfc_nimble_buffer.yaml"
)
CLIENT_DESCRIPTOR = load_client_descriptor(str(DESCRIPTOR))
GAP_NAME = str(CLIENT_DESCRIPTOR.protocols[0].config["gap_name"])
assert len(GAP_NAME) <= 10

UUID_BUFFER_SERVICE = "12345678-1234-5678-1234-56789abcdf00"
UUID_BUFFER_CHUNK = "12345678-1234-5678-1234-56789abcdf01"
UUID_BUFFER_SELECTOR = "12345678-1234-5678-1234-56789abcdf02"
UUID_BUFFER_STATUS = "12345678-1234-5678-1234-56789abcdf03"
UUID_BUFFER_TRIGGER = "12345678-1234-5678-1234-56789abcdf04"

ADV_SETTLE_S = 3.0
BUFFER_DEPTH = 1024
CHUNK_SAMPLES = 32
CHUNK_BYTES = CHUNK_SAMPLES * 4
STATUS_BYTES = 8 * 4
CAPTURE_TIMEOUT_S = 20.0

STAT_COUNT = 0
STAT_DEPTH = 1
STAT_RUNTIME_FLAGS = 5
RUNTIME_CAPTURE_ACTIVE = 1 << 1

CMD_TRIGGER1_START_CAPTURE = 1
CMD_TRIGGER2_STOP_CAPTURE = 2


def _run(coro):
    return asyncio.run(coro)


def _uuid(value) -> str:
    return str(value).lower()


def _iter_services(services):
    if isinstance(services, dict):
        return services.values()
    registry = getattr(services, "services", None)
    if isinstance(registry, dict):
        return registry.values()
    return services or []


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


async def _buffer_characteristics(client):
    services = await _get_services(client)
    for service in _iter_services(services):
        if _uuid(service.uuid) != UUID_BUFFER_SERVICE:
            continue
        chars = {
            _uuid(char.uuid): char
            for char in (getattr(service, "characteristics", []) or [])
        }
        return {
            "chunk": chars[UUID_BUFFER_CHUNK],
            "selector": chars[UUID_BUFFER_SELECTOR],
            "status": chars[UUID_BUFFER_STATUS],
            "trigger": chars[UUID_BUFFER_TRIGGER],
        }
    raise AssertionError("buffer custom service not found")


async def _read_status(client, status_char) -> tuple[int, ...]:
    payload = bytes(await client.read_gatt_char(status_char))
    assert len(payload) >= STATUS_BYTES
    return struct.unpack("<8I", payload[:STATUS_BYTES])


class TestNimbleNtfcBuffer:
    """Validate chunked captured-buffer transfer over custom BLE."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROG_BUFFER"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NIMBLE"),
    ]

    @pytest.fixture(scope="class", autouse=True)
    def _start_dawn(self):
        product = pytest.products[0].core(0)
        product.sendCommand("dawn &", timeout=2)
        time.sleep(ADV_SETTLE_S)
        TestNimbleNtfcBuffer._product = product
        yield

    def test_dawn_started_and_shell_is_alive(self):
        ret = shell_cmd(self._product, "ps", timeout=2)
        assert "dawn" in ret.output, ret.output

    def test_buffer_service_is_present(self):
        async def go():
            _device, client = await _connect_client()
            try:
                chars = await _buffer_characteristics(client)
                return set(chars)
            finally:
                await client.disconnect()

        assert _run(go()) == {"chunk", "selector", "status", "trigger"}

    def test_buffer_chunked_bulk_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                chars = await _buffer_characteristics(client)

                deadline = time.monotonic() + CAPTURE_TIMEOUT_S
                status = await _read_status(client, chars["status"])
                while status[STAT_COUNT] < BUFFER_DEPTH:
                    if time.monotonic() >= deadline:
                        raise AssertionError(
                            f"buffer captured {status[STAT_COUNT]} "
                            f"of {BUFFER_DEPTH} samples"
                        )
                    await asyncio.sleep(0.05)
                    status = await _read_status(client, chars["status"])

                assert status[STAT_DEPTH] == BUFFER_DEPTH
                await client.write_gatt_char(
                    chars["trigger"],
                    struct.pack("<B", CMD_TRIGGER2_STOP_CAPTURE),
                    response=True,
                )
                status = await _read_status(client, chars["status"])
                assert status[STAT_COUNT] == BUFFER_DEPTH
                assert (status[STAT_RUNTIME_FLAGS] & RUNTIME_CAPTURE_ACTIVE) == 0

                values = []
                for offset in range(0, BUFFER_DEPTH, CHUNK_SAMPLES):
                    await client.write_gatt_char(
                        chars["selector"], struct.pack("<I", offset), response=True
                    )
                    payload = bytes(await client.read_gatt_char(chars["chunk"]))
                    assert len(payload) == CHUNK_BYTES
                    values.extend(struct.unpack("<32I", payload))

                await client.write_gatt_char(
                    chars["trigger"],
                    struct.pack("<B", CMD_TRIGGER1_START_CAPTURE),
                    response=True,
                )
                return values
            finally:
                await client.disconnect()

        values = _run(go())
        assert len(values) == BUFFER_DEPTH
        assert any(value != 0 for value in values)
        assert all(left >= right for left, right in zip(values, values[1:]))
