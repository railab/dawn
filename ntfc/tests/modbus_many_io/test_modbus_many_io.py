############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC validation for a broad IO set exposed over Modbus RTU."""

import time
from contextlib import contextmanager

import pytest
from _descriptor_common import modbus_wire_starts
from _modbus_common import ModbusAdapter, regs_from_response
from _ntfc_common import start_dawn
from dawnpy_modbus.client import ModbusClient

DESC_PATH = "descriptors/ntfc/ntfc_modbus_many_io.yaml"
MODBUS_UNIT = 0x01
RTU_BAUD = 115200
RTU_PARITY = "N"
SERIAL_PORT = "/tmp/ttyNX0"


@contextmanager
def _open_rtu_adapter():
    for _ in range(20):
        client = ModbusClient(
            port=SERIAL_PORT,
            baudrate=RTU_BAUD,
            parity=RTU_PARITY,
            stopbits=1,
            timeout=1.5,
        )
        if client.connect():
            try:
                yield ModbusAdapter.for_rtu(client, MODBUS_UNIT)
            finally:
                client.close()
            return

        time.sleep(0.2)

    pytest.fail(f"Could not connect to Modbus RTU port {SERIAL_PORT}")


@pytest.fixture
def modbus_many_io_case():
    starts = modbus_wire_starts(DESC_PATH, "modbus_rtu")
    start_dawn(settle_s=0.5)
    with _open_rtu_adapter() as adapter:
        yield adapter, starts


class TestModbusManyIo:
    """Validate representative IOs from the many-IO Modbus map."""

    pytestmark = [pytest.mark.cmd_check("dawn_main")]

    def test_rgbled_holding_register_read_write(self, modbus_many_io_case):
        adapter, starts = modbus_many_io_case
        holding_start = starts["holding"]

        resp = adapter.read_holding(holding_start, 2)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[:2] == [0x0000, 0x0000]

        red_green_blue = [0x00FF, 0x0040]
        assert adapter.write_registers(holding_start, red_green_blue)

        resp = adapter.read_holding(holding_start, 2)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[:2] == red_green_blue

        assert adapter.write_registers(holding_start, [0x0000, 0x0000])

    def test_dummy_holding_register_read_write(self, modbus_many_io_case):
        adapter, starts = modbus_many_io_case
        # Layout of the first holding block:
        # rgbled0: 2 regs, dummy_u16: 1 reg, dummy_u32: 2 regs.
        dummy_u16 = starts["holding"] + 2
        dummy_u32 = starts["holding"] + 3

        assert adapter.write_registers(dummy_u16, [0x4567])
        resp = adapter.read_holding(dummy_u16, 1)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[:1] == [0x4567]

        assert adapter.write_registers(dummy_u32, [0x89AB, 0xCDEF])
        resp = adapter.read_holding(dummy_u32, 2)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[:2] == [0x89AB, 0xCDEF]

    def test_input_register_block_reads(self, modbus_many_io_case):
        adapter, starts = modbus_many_io_case
        resp = adapter.read_input(starts["input"], 8)
        assert resp is not None
        regs = regs_from_response(resp)
        assert len(regs) >= 8
        assert all(isinstance(value, int) for value in regs[:8])
