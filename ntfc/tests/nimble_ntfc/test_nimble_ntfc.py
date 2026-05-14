# ntfc/tests/nimble_ntfc/test_nimble_ntfc.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC tests for the board-agnostic all-services NimBLE target."""

from __future__ import annotations

import asyncio
import math
import struct
import time
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd
from bleak import BleakClient, BleakScanner
from dawnpy.descriptor.client import load_client_descriptor
from dawnpy_ble.ble import DawnBleProtocol
from dawnpy_ble.profile import BleTransportProfile
from dawnpy_ble.services.ots import (
    PROP_READ,
    PROP_TRUNC,
    PROP_WRITE,
    RES_SUCCESS,
    UUID_FEATURE,
    UUID_OACP,
    UUID_OBJ_ID,
    UUID_OBJ_NAME,
    UUID_OBJ_PROPS,
    UUID_OBJ_SIZE,
    UUID_OBJ_TYPE,
    UUID_OLCP,
    OtsClient,
)

GAP_NAME = "ntfc-nimble"
assert len(GAP_NAME) <= 16
DESCRIPTOR = (
    Path(__file__).resolve().parents[3] / "descriptors/ntfc/ntfc_nimble_all.yaml"
)
RO_OBJECT = "some_file_ro"
RW_OBJECT = "some_file_rw"
RW_PATH = "/tmp/some_file_rw.txt"
INITIAL_CONTENT = b"initial data"
ADV_SETTLE_S = 3.0

UUID_GAP = "00001800-0000-1000-8000-00805f9b34fb"
UUID_DIS = "0000180a-0000-1000-8000-00805f9b34fb"
UUID_TPS = "00001804-0000-1000-8000-00805f9b34fb"
UUID_BAS = "0000180f-0000-1000-8000-00805f9b34fb"
UUID_AIOS = "00001815-0000-1000-8000-00805f9b34fb"
UUID_ESS = "0000181a-0000-1000-8000-00805f9b34fb"
UUID_OTS = "00001825-0000-1000-8000-00805f9b34fb"
UUID_IMDS = "0000185a-0000-1000-8000-00805f9b34fb"

UUID_GAP_NAME = "00002a00-0000-1000-8000-00805f9b34fb"
UUID_GAP_APPEARANCE = "00002a01-0000-1000-8000-00805f9b34fb"
UUID_TX_POWER = "00002a07-0000-1000-8000-00805f9b34fb"
UUID_BATTERY_LEVEL = "00002a19-0000-1000-8000-00805f9b34fb"
UUID_DIGITAL = "00002a56-0000-1000-8000-00805f9b34fb"
UUID_ANALOG = "00002a58-0000-1000-8000-00805f9b34fb"
UUID_AGGREGATE = "00002a5a-0000-1000-8000-00805f9b34fb"
UUID_TEMPERATURE = "00002a6e-0000-1000-8000-00805f9b34fb"
UUID_EXTENDED_PROPERTIES_DSC = "00002900-0000-1000-8000-00805f9b34fb"
UUID_USER_DESCRIPTION_DSC = "00002901-0000-1000-8000-00805f9b34fb"
UUID_PRESENTATION_FORMAT_DSC = "00002904-0000-1000-8000-00805f9b34fb"
UUID_VALID_RANGE_DSC = "00002906-0000-1000-8000-00805f9b34fb"
UUID_NUMBER_OF_DIGITALS_DSC = "00002909-0000-1000-8000-00805f9b34fb"
UUID_VALUE_TRIGGER_SETTING_DSC = "0000290a-0000-1000-8000-00805f9b34fb"
UUID_ES_CONFIGURATION_DSC = "0000290b-0000-1000-8000-00805f9b34fb"
UUID_ES_MEASUREMENT_DSC = "0000290c-0000-1000-8000-00805f9b34fb"
UUID_ES_TRIGGER_SETTING_DSC = "0000290d-0000-1000-8000-00805f9b34fb"
UUID_TIME_TRIGGER_SETTING_DSC = "0000290e-0000-1000-8000-00805f9b34fb"
UUID_DIS_STRINGS = {
    "00002a24-0000-1000-8000-00805f9b34fb",
    "00002a25-0000-1000-8000-00805f9b34fb",
    "00002a26-0000-1000-8000-00805f9b34fb",
    "00002a27-0000-1000-8000-00805f9b34fb",
    "00002a28-0000-1000-8000-00805f9b34fb",
    "00002a29-0000-1000-8000-00805f9b34fb",
}

EXPECTED_SERVICES = {
    UUID_GAP,
    UUID_DIS,
    UUID_TPS,
    UUID_BAS,
    UUID_AIOS,
    UUID_ESS,
    UUID_IMDS,
    UUID_OTS,
}
EXPECTED_OTS_CHARS = {
    UUID_FEATURE,
    UUID_OBJ_NAME,
    UUID_OBJ_TYPE,
    UUID_OBJ_SIZE,
    UUID_OBJ_ID,
    UUID_OBJ_PROPS,
    UUID_OACP,
    UUID_OLCP,
}
EXPECTED_VALUES = {
    "bas_battery": 42,
    "aios_analog": 1.5,
    "ess_humidity": 55.5,
    "ess_temperature": 23.25,
    "ess_pressure": 101325.0,
    "ess_gas": 12345.0,
    "imds_humidity": 60.0,
    "imds_temperature": 25.5,
    "imds_pressure": 100000.0,
    "imds_gas": 22222.0,
}


def _run(coro):
    return asyncio.run(coro)


def _uuid(value) -> str:
    return str(value).lower()


def _services_by_uuid(services) -> dict[str, object]:
    items = services.values() if isinstance(services, dict) else services
    registry = getattr(services, "services", None)
    if isinstance(registry, dict):
        items = registry.values()
    return {_uuid(service.uuid): service for service in items}


def _characteristics_by_key(services) -> dict[tuple[str, str], list[object]]:
    result: dict[tuple[str, str], list[object]] = {}
    for service_uuid, service in _services_by_uuid(services).items():
        for char in getattr(service, "characteristics", []):
            result.setdefault((service_uuid, _uuid(char.uuid)), []).append(char)
    return result


def _all_characteristics(services) -> list[tuple[str, object]]:
    chars = []
    for service_uuid, service in _services_by_uuid(services).items():
        for char in getattr(service, "characteristics", []):
            chars.append((service_uuid, char))
    return chars


def _props(char) -> set[str]:
    return {str(prop).lower() for prop in (getattr(char, "properties", []) or [])}


def _decode_raw(dtype: str, raw: bytes):
    if dtype == "bool":
        return bool(raw[0])
    if dtype == "uint8":
        return raw[0]
    if dtype == "float":
        return struct.unpack("<f", raw[:4])[0]
    raise AssertionError(f"unsupported test dtype {dtype}")


def _assert_float(actual: float, expected: float) -> None:
    assert math.isclose(actual, expected, rel_tol=0.01, abs_tol=0.05)


def _assert_expected_binding_value(binding, value) -> None:
    if binding.io_id in EXPECTED_VALUES:
        expected = EXPECTED_VALUES[binding.io_id]
        if isinstance(expected, float):
            _assert_float(value, expected)
        else:
            assert value == expected
    elif binding.io_id.startswith("aios_gpi"):
        assert value in (False, True)
    elif binding.io_id.startswith("aios_gpo"):
        assert value in (False, True)
    elif binding.io_id == "aios_digital_rw":
        assert value in (False, True)
    elif binding.io_id == "aios_analog_rw":
        _assert_float(value, 0.0)
    else:
        raise AssertionError(f"unexpected BLE binding {binding.io_id}")


async def _find_characteristic_by_description(
    client, chars: list[object], description: str
):
    for char in chars:
        descriptors = {
            _uuid(desc.uuid): desc for desc in getattr(char, "descriptors", [])
        }
        description_descriptor = descriptors.get(UUID_USER_DESCRIPTION_DSC)
        if description_descriptor is None:
            continue

        actual = bytes(
            await client.read_gatt_descriptor(description_descriptor.handle)
        ).decode("utf-8")
        if actual == description:
            return char
    return None


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


class TestNimbleNtfcAllCharacteristics:
    """Validate every exposed characteristic on the all-services target."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NIMBLE_OTS"),
    ]

    @pytest.fixture(scope="class", autouse=True)
    def _start_dawn(self):
        """Launch Dawn on any NSH-backed ``nimble_ntfc`` hardware target."""
        product = pytest.products[0].core(0)
        product.sendCommand("dawn &", timeout=2)
        time.sleep(ADV_SETTLE_S)
        TestNimbleNtfcAllCharacteristics._product = product
        descriptor = load_client_descriptor(str(DESCRIPTOR))
        TestNimbleNtfcAllCharacteristics._descriptor = descriptor
        TestNimbleNtfcAllCharacteristics._profile = BleTransportProfile.from_descriptor(
            descriptor
        )
        yield

    def test_dawn_started_and_shell_is_alive(self):
        ret = shell_cmd(self._product, "ps", timeout=2)
        assert "dawn" in ret.output, ret.output

    def test_gatt_inventory_has_all_expected_characteristics(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                service_map = _services_by_uuid(services)
                chars = _characteristics_by_key(services)
                return service_map, chars
            finally:
                await client.disconnect()

        service_map, chars = _run(go())
        assert EXPECTED_SERVICES <= set(service_map)

        assert (UUID_GAP, UUID_GAP_NAME) in chars
        assert (UUID_GAP, UUID_GAP_APPEARANCE) in chars
        assert any(key[0] == UUID_DIS for key in chars)
        assert (UUID_TPS, UUID_TX_POWER) in chars
        assert (UUID_BAS, UUID_BATTERY_LEVEL) in chars

        for binding in self._profile.iter_bindings():
            key = (binding.service_uuid, binding.characteristic_uuid)
            assert key in chars
            assert binding.characteristic_index < len(chars[key])

        for char_uuid in EXPECTED_OTS_CHARS:
            assert (UUID_OTS, char_uuid) in chars

    def test_every_non_ots_readable_characteristic_can_be_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                reads = {}
                for service_uuid, char in _all_characteristics(services):
                    char_uuid = _uuid(char.uuid)
                    if service_uuid == UUID_OTS:
                        continue
                    if "read" not in _props(char):
                        continue
                    data = bytes(await client.read_gatt_char(char))
                    assert data, f"{service_uuid}/{char_uuid} read empty payload"
                    reads.setdefault((service_uuid, char_uuid), []).append(data)
                return reads
            finally:
                await client.disconnect()

        reads = _run(go())
        assert reads[(UUID_GAP, UUID_GAP_NAME)][0].decode() == GAP_NAME
        assert len(reads[(UUID_GAP, UUID_GAP_APPEARANCE)][0]) == 2
        assert reads[(UUID_BAS, UUID_BATTERY_LEVEL)][0] == bytes([42])
        tx_power = struct.unpack("<b", reads[(UUID_TPS, UUID_TX_POWER)][0][:1])[0]
        assert -127 <= tx_power <= 20

        for key, payloads in reads.items():
            if key[0] == UUID_DIS and key[1] in UUID_DIS_STRINGS:
                for payload in payloads:
                    assert payload.decode("utf-8", errors="strict")

    def test_descriptor_bound_characteristics_read_expected_fake_values(self):
        device = _run(_find_device())
        protocol = DawnBleProtocol(
            device.address,
            self._profile,
            client_factory=lambda _identifier, timeout=10.0: BleakClient(
                device, timeout=timeout
            ),
        )

        try:
            assert protocol.connect()
            for binding in self._profile.iter_bindings():
                if binding.writable:
                    continue
                raw = protocol.read_io(binding.objid)
                assert raw is not None, binding.io_id
                value = _decode_raw(binding.dtype, raw)

                if binding.io_id == "aios_analog":
                    continue
                _assert_expected_binding_value(binding, value)
        finally:
            protocol.disconnect()

    def test_aios_analog_input_reads_expected_fake_value(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                analog_chars = chars.get((UUID_AIOS, UUID_ANALOG), [])
                assert analog_chars

                target = await _find_characteristic_by_description(
                    client, analog_chars, "AIOS analog in"
                )
                assert target is not None
                assert "read" in _props(target)

                raw = bytes(await client.read_gatt_char(target))[:4]
                return struct.unpack("<f", raw)[0]
            finally:
                await client.disconnect()

        _assert_float(_run(go()), 1.5)

    def test_ess_temperature_descriptors_can_be_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                temp_chars = chars.get((UUID_ESS, UUID_TEMPERATURE), [])
                assert temp_chars
                temp_char = temp_chars[0]
                descriptors = {
                    _uuid(desc.uuid): desc
                    for desc in getattr(temp_char, "descriptors", [])
                }
                assert UUID_USER_DESCRIPTION_DSC in descriptors
                assert UUID_VALID_RANGE_DSC in descriptors
                assert UUID_ES_CONFIGURATION_DSC in descriptors
                assert UUID_ES_MEASUREMENT_DSC in descriptors
                assert UUID_ES_TRIGGER_SETTING_DSC in descriptors

                description = bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_USER_DESCRIPTION_DSC].handle
                    )
                ).decode("utf-8")
                valid_range = bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_VALID_RANGE_DSC].handle
                    )
                )
                configuration = bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_ES_CONFIGURATION_DSC].handle
                    )
                )
                measurement = bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_ES_MEASUREMENT_DSC].handle
                    )
                )
                trigger_setting = bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_ES_TRIGGER_SETTING_DSC].handle
                    )
                )
                return (
                    description,
                    valid_range,
                    configuration,
                    measurement,
                    trigger_setting,
                )
            finally:
                await client.disconnect()

        description, valid_range, configuration, measurement, trigger_setting = _run(
            go()
        )
        assert description == "ESS temp"
        assert valid_range == struct.pack("<hh", -4000, 8500)
        assert configuration == bytes([1])
        assert measurement == (
            struct.pack("<HB", 0, 1)
            + (60).to_bytes(3, "little")
            + (10).to_bytes(3, "little")
            + struct.pack("<BB", 0, 2)
        )
        assert trigger_setting == bytes([0])

    def test_imds_temperature_descriptors_can_be_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                temp_chars = chars.get((UUID_IMDS, UUID_TEMPERATURE), [])
                assert temp_chars
                temp_char = temp_chars[0]
                descriptors = {
                    _uuid(desc.uuid): desc
                    for desc in getattr(temp_char, "descriptors", [])
                }
                assert UUID_USER_DESCRIPTION_DSC in descriptors

                return bytes(
                    await client.read_gatt_descriptor(
                        descriptors[UUID_USER_DESCRIPTION_DSC].handle
                    )
                ).decode("utf-8")
            finally:
                await client.disconnect()

        assert _run(go()) == "IMDS temp"

    def test_aios_digital_descriptors_can_be_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                digital_chars = chars.get((UUID_AIOS, UUID_DIGITAL), [])
                assert digital_chars

                for char in digital_chars:
                    descriptors = {
                        _uuid(desc.uuid): desc
                        for desc in getattr(char, "descriptors", [])
                    }
                    description_descriptor = descriptors.get(UUID_USER_DESCRIPTION_DSC)
                    number_descriptor = descriptors.get(UUID_NUMBER_OF_DIGITALS_DSC)
                    value_trigger_descriptor = descriptors.get(
                        UUID_VALUE_TRIGGER_SETTING_DSC
                    )
                    time_trigger_descriptor = descriptors.get(
                        UUID_TIME_TRIGGER_SETTING_DSC
                    )
                    extended_properties_descriptor = descriptors.get(
                        UUID_EXTENDED_PROPERTIES_DSC
                    )
                    presentation_descriptor = descriptors.get(
                        UUID_PRESENTATION_FORMAT_DSC
                    )
                    if description_descriptor is None:
                        continue
                    value = bytes(
                        await client.read_gatt_descriptor(description_descriptor.handle)
                    ).decode("utf-8")
                    if (
                        value == "AIOS gpi1"
                        and number_descriptor is not None
                        and value_trigger_descriptor is not None
                        and time_trigger_descriptor is not None
                        and extended_properties_descriptor is not None
                        and presentation_descriptor is not None
                    ):
                        number = bytes(
                            await client.read_gatt_descriptor(number_descriptor.handle)
                        )
                        value_trigger = bytes(
                            await client.read_gatt_descriptor(
                                value_trigger_descriptor.handle
                            )
                        )
                        time_trigger = bytes(
                            await client.read_gatt_descriptor(
                                time_trigger_descriptor.handle
                            )
                        )
                        presentation = bytes(
                            await client.read_gatt_descriptor(
                                presentation_descriptor.handle
                            )
                        )
                        extended_properties = bytes(
                            await client.read_gatt_descriptor(
                                extended_properties_descriptor.handle
                            )
                        )
                        return (
                            value,
                            number,
                            value_trigger,
                            time_trigger,
                            extended_properties,
                            presentation,
                        )

                return None
            finally:
                await client.disconnect()

        assert _run(go()) == (
            "AIOS gpi1",
            bytes([1]),
            bytes([0]),
            bytes([0]),
            struct.pack("<H", 0),
            struct.pack("<BbHBH", 1, 0, 0x2700, 1, 0),
        )

    def test_aios_writable_outputs_can_be_written(self):
        device = _run(_find_device())
        protocol = DawnBleProtocol(
            device.address,
            self._profile,
            client_factory=lambda _identifier, timeout=10.0: BleakClient(
                device, timeout=timeout
            ),
        )

        try:
            assert protocol.connect()
            output_bindings = [
                binding
                for binding in self._profile.iter_bindings()
                if binding.io_id.startswith("aios_gpo")
            ]
            assert output_bindings
            for binding in output_bindings:
                assert binding.writable
                for state in (1, 0):
                    assert protocol.write_io(binding.objid, bytes([state]))
        finally:
            protocol.disconnect()

    def test_aios_rw_digital_roundtrip(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                digital_chars = chars.get((UUID_AIOS, UUID_DIGITAL), [])
                assert digital_chars

                target = await _find_characteristic_by_description(
                    client, digital_chars, "AIOS rw digital"
                )
                assert target is not None
                assert "write" in _props(target)
                assert "read" in _props(target)

                values = []
                for state in (1, 0, 1):
                    await client.write_gatt_char(target, bytes([state]))
                    values.append(bytes(await client.read_gatt_char(target))[:1])
                return values
            finally:
                await client.disconnect()

        assert _run(go()) == [bytes([1]), bytes([0]), bytes([1])]

    def test_aios_rw_analog_roundtrip(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                analog_chars = chars.get((UUID_AIOS, UUID_ANALOG), [])
                assert analog_chars

                target = await _find_characteristic_by_description(
                    client, analog_chars, "AIOS rw analog"
                )
                assert target is not None
                assert "write" in _props(target)
                assert "read" in _props(target)

                values = []
                for value in (12.5, -3.25, 0.75):
                    await client.write_gatt_char(target, struct.pack("<f", value))
                    raw = bytes(await client.read_gatt_char(target))[:4]
                    values.append(struct.unpack("<f", raw)[0])
                return values
            finally:
                await client.disconnect()

        values = _run(go())
        for actual, expected in zip(values, (12.5, -3.25, 0.75)):
            _assert_float(actual, expected)

    def test_aios_aggregate_can_be_read(self):
        async def go():
            _device, client = await _connect_client()
            try:
                services = await _get_services(client)
                chars = _characteristics_by_key(services)
                aggregate_chars = chars.get((UUID_AIOS, UUID_AGGREGATE), [])
                if not aggregate_chars:
                    discovered = sorted(
                        char_uuid
                        for service_uuid, char_uuid in chars
                        if service_uuid == UUID_AIOS
                    )
                    raise AssertionError(
                        f"AIOS Aggregate missing; discovered AIOS chars: {discovered}"
                    )
                return bytes(await client.read_gatt_char(aggregate_chars[0]))
            finally:
                await client.disconnect()

        value = _run(go())
        assert len(value) >= 5

    def test_ots_all_characteristics_and_object_metadata(self):
        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                feature = await ots.read_feature()
                objects = []
                result = await ots.olcp_first()
                assert result == RES_SUCCESS
                while result == RES_SUCCESS:
                    name = await ots.read_object_name()
                    obj_type = bytes(await ots.client.read_gatt_char(UUID_OBJ_TYPE))
                    size = await ots.read_object_size()
                    obj_id = bytes(await ots.client.read_gatt_char(UUID_OBJ_ID))
                    props = await ots.read_object_props()
                    objects.append((name, obj_type, size, obj_id, props))
                    result = await ots.olcp_next()
                return feature, objects
            finally:
                await ots.disconnect()

        (feat_oacp, feat_olcp), objects = _run(go())
        assert feat_oacp & (1 << 4)
        assert feat_oacp & (1 << 5)
        assert feat_oacp & (1 << 7)
        assert feat_oacp & (1 << 9)
        assert feat_olcp & (1 << 0)

        by_name = {item[0]: item for item in objects}
        assert set(by_name) == {RO_OBJECT, RW_OBJECT}
        object_ids = set()
        for name, obj_type, (cur_size, alloc_size), obj_id, props in objects:
            assert struct.unpack("<H", obj_type)[0] == 0x2ACA
            assert len(obj_id) == 6
            object_ids.add(obj_id)
            assert cur_size == len(INITIAL_CONTENT)
            assert alloc_size == cur_size
            assert props & PROP_READ
            if name == RO_OBJECT:
                assert not (props & PROP_WRITE)
            else:
                assert props & PROP_WRITE
                assert props & PROP_TRUNC
        decoded_ids = {int.from_bytes(obj_id, "little") for obj_id in object_ids}
        assert decoded_ids == {256, 257}

    def test_ots_read_write_control_points_and_l2cap(self):
        marker = b"ntfc-nimble-all-marker"

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                assert await ots.read_object(RO_OBJECT) == INITIAL_CONTENT
                assert await ots.read_object(RW_OBJECT) == INITIAL_CONTENT

                new_size = await ots.write_object(RW_OBJECT, marker)
                assert new_size == len(marker)
                assert await ots.read_object(RW_OBJECT) == marker
            finally:
                await ots.disconnect()

        _run(go())

        time.sleep(0.5)
        ret = shell_cmd(self._product, f"cat {RW_PATH}", timeout=2)
        assert marker.decode() in ret.output, ret.output

    def test_ots_write_to_read_only_object_rejected(self):
        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                meta = await ots.select_by_name(RO_OBJECT)
                assert meta is not None
                await ots.open_l2cap()
                try:
                    return await ots.oacp_write(0, 4, mode=0x02)
                finally:
                    await ots.close_l2cap()
            finally:
                await ots.disconnect()

        result = _run(go())
        assert result != RES_SUCCESS
