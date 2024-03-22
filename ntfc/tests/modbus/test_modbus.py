############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""Target-aware NTFC integration tests for Modbus mappings."""

import time
from contextlib import contextmanager
from dataclasses import dataclass

import pytest
from _descriptor_common import (
    modbus_seekable_groups,
    modbus_wire_starts,
    protocol_config_value,
)
from _modbus_common import (
    ModbusAdapter,
    check_coil_read_write,
    check_holding_read_write,
    check_input_read,
    check_seekable_fileio,
    check_seekable_ios,
    check_seekable_window,
)
from _ntfc_common import start_dawn
from dawnpy_modbus.client import ModbusClient
from pymodbus.client import ModbusTcpClient

RTU_DESC_PATH = "descriptors/ntfc/ntfc_modbus_rtu_dummy_map.yaml"
TCP_DESC_PATH = "descriptors/examples/qemu_modbus_tcp_dummy_map.yaml"

MODBUS_UNIT = 0x01
RTU_BAUD = 115200
RTU_PARITY = "E"


@dataclass(frozen=True)
class ModbusTarget:
    device: str
    desc_path: str
    protocol_id: str
    transport: str
    settle_s: float
    connect_attempts: int
    retry_s: float
    serial_port: str = ""
    tcp_host: str = ""
    tcp_port: int = 0
    readiness_probe: bool = False


def _device_type():
    product = pytest.products[0]
    device = product.conf.cfg_core(0).device
    if device is not None:
        return device

    live_device = product.core(0).device
    name = live_device.name
    return name() if callable(name) else name


def _modbus_target():
    device = _device_type()

    if device == "sim":
        return ModbusTarget(
            device=device,
            desc_path=RTU_DESC_PATH,
            protocol_id="modbus_rtu",
            transport="rtu",
            settle_s=0.5,
            connect_attempts=20,
            retry_s=0.2,
            serial_port="/tmp/ttyNX0",
        )

    if device == "serial":
        return ModbusTarget(
            device=device,
            desc_path=RTU_DESC_PATH,
            protocol_id="modbus_rtu",
            transport="rtu",
            settle_s=0.5,
            connect_attempts=30,
            retry_s=0.2,
            serial_port="/dev/ttyUSB0",
            readiness_probe=True,
        )

    if device == "qemu":
        return ModbusTarget(
            device=device,
            desc_path=TCP_DESC_PATH,
            protocol_id="modbus_tcp",
            transport="tcp",
            settle_s=1.0,
            connect_attempts=30,
            retry_s=0.5,
            tcp_host="192.168.8.104",
            tcp_port=int(protocol_config_value(TCP_DESC_PATH, "modbus_tcp", "port")),
        )

    pytest.fail(f"Unsupported Modbus NTFC device type: {device}")


def _open_rtu_client(target):
    client = ModbusClient(
        port=target.serial_port,
        baudrate=RTU_BAUD,
        parity=RTU_PARITY,
        stopbits=1,
        timeout=1.5,
    )
    if client.connect():
        return client
    return None


@contextmanager
def _open_rtu_adapter(target, coil_packed_start):
    for _ in range(target.connect_attempts):
        client = _open_rtu_client(target)
        if client is not None:
            adapter = ModbusAdapter.for_rtu(client, MODBUS_UNIT)
            if target.readiness_probe:
                try:
                    if adapter.read_coils(coil_packed_start, 1) is None:
                        client.close()
                        time.sleep(target.retry_s)
                        continue
                except OSError:
                    client.close()
                    time.sleep(target.retry_s)
                    continue

            try:
                yield adapter
            finally:
                client.close()
            return

        time.sleep(target.retry_s)

    pytest.fail(f"Could not connect to Modbus RTU port {target.serial_port}")


@contextmanager
def _open_tcp_adapter(target):
    client = ModbusTcpClient(host=target.tcp_host, port=target.tcp_port, timeout=2.0)
    for _ in range(target.connect_attempts):
        if client.connect():
            try:
                yield ModbusAdapter.for_tcp(client, MODBUS_UNIT)
            finally:
                client.close()
            return

        time.sleep(target.retry_s)

    pytest.fail(
        f"Could not connect to Modbus TCP at {target.tcp_host}:{target.tcp_port}"
    )


@pytest.fixture
def modbus_case():
    target = _modbus_target()
    starts = modbus_wire_starts(target.desc_path, target.protocol_id)
    seekable_groups = modbus_seekable_groups(target.desc_path, target.protocol_id)

    start_dawn(settle_s=target.settle_s)

    if target.transport == "rtu":
        with _open_rtu_adapter(target, starts["coil_packed"]) as adapter:
            yield adapter, starts, seekable_groups
    else:
        with _open_tcp_adapter(target) as adapter:
            yield adapter, starts, seekable_groups


class TestModbus:
    """Validate Modbus coil, holding, input, and seekable register access."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
    ]

    def test_modbus_coil_read_write(self, modbus_case):
        adapter, starts, _ = modbus_case
        check_coil_read_write(adapter, starts["coil_packed"])

    def test_modbus_holding_registers_read_write(self, modbus_case):
        adapter, starts, _ = modbus_case
        check_holding_read_write(adapter, starts["holding"])

    def test_modbus_input_registers_read(self, modbus_case):
        adapter, starts, _ = modbus_case
        check_input_read(adapter, starts["input"])

    def test_modbus_seekable_register_window_and_offset(self, modbus_case):
        adapter, _, seekable_groups = modbus_case
        check_seekable_window(adapter, seekable_groups["descriptor1"])

    def test_modbus_seekable_ios_read_window_and_offset(self, modbus_case):
        adapter, _, seekable_groups = modbus_case
        check_seekable_ios(adapter, seekable_groups)

    def test_modbus_seekable_fileio_rw(self, modbus_case):
        adapter, _, seekable_groups = modbus_case
        check_seekable_fileio(adapter, seekable_groups["modbus_fileio1"])
