############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

"""Shared Modbus helpers and test bodies for Dawn NTFC.

Centralizes the ~80% overlap between RTU and TCP integration tests.
``ModbusAdapter`` normalizes the two pymodbus call styles (positional
unit-first for dawnpy-modbus's RTU client vs kwarg ``device_id`` for
pymodbus TCP client) so the shared ``check_*`` test bodies stay
transport-agnostic.
"""

import pytest
from dawnpy_modbus.files import SeekableFile
from pymodbus.pdu.file_message import FileRecord

MAX_HOLDING_READ_REGS = 125


def bits_from_response(resp):
    bits = getattr(resp, "bits", None)
    if bits is None:
        pytest.fail(f"Modbus coil response has no bits field: {resp!r}")
    return list(bits)


def regs_from_response(resp):
    regs = getattr(resp, "registers", None)
    if regs is None:
        pytest.fail(f"Modbus register response has no registers field: {resp!r}")
    return list(regs)


class ModbusAdapter:
    """Transport-neutral Modbus client wrapper.

    Construct via ``for_rtu`` or ``for_tcp``. Binds unit id at
    construction so callers use address-based calls only.
    """

    def __init__(self, client, unit, flavor):
        self._client = client
        self._unit = unit
        self._flavor = flavor

    @classmethod
    def for_rtu(cls, client, unit):
        return cls(client, unit, "rtu")

    @classmethod
    def for_tcp(cls, client, unit):
        return cls(client, unit, "tcp")

    def read_coils(self, addr, count):
        if self._flavor == "rtu":
            return self._client.read_coils(self._unit, addr, count)
        return self._client.read_coils(addr, count=count, device_id=self._unit)

    def read_holding(self, addr, count):
        if self._flavor == "rtu":
            return self._client.read_holding_registers(self._unit, addr, count)
        return self._client.read_holding_registers(
            addr, count=count, device_id=self._unit
        )

    def read_input(self, addr, count):
        if self._flavor == "rtu":
            return self._client.read_input_registers(self._unit, addr, count)
        return self._client.read_input_registers(
            addr, count=count, device_id=self._unit
        )

    def write_coil(self, addr, value):
        if self._flavor == "rtu":
            return bool(self._client.write_coil(self._unit, addr, value))
        resp = self._client.write_coil(addr, value, device_id=self._unit)
        return resp is not None and not resp.isError()

    def write_registers(self, addr, values):
        if self._flavor == "rtu":
            return bool(self._client.write_registers(self._unit, addr, values))
        resp = self._client.write_registers(addr, values, device_id=self._unit)
        return resp is not None and not resp.isError()

    def read_file_records(self, unit, number, record, count):
        """Read one record group using the FileClient error contract."""
        if self._flavor == "rtu":
            return self._client.read_file_records(unit, number, record, count)
        req = FileRecord(
            file_number=number, record_number=record, record_length=count * 2
        )
        response = self._client.read_file_record([req], device_id=unit)
        if response is None or response.isError():
            raise OSError("failed FC20 response")
        records = response.records
        if len(records) != 1 or len(records[0].record_data) != count * 2:
            raise OSError("invalid FC20 response")
        return bytes(records[0].record_data)

    def write_file_records(self, unit, number, record, data):
        """Write one record group and validate the echoed request."""
        if self._flavor == "rtu":
            return self._client.write_file_records(unit, number, record, data)
        req = FileRecord(file_number=number, record_number=record, record_data=data)
        response = self._client.write_file_record([req], device_id=unit)
        if response is None or response.isError():
            raise OSError("failed FC21 response")
        if response.records != [req]:
            raise OSError("invalid FC21 response")


def read_holding_regs_chunked(adapter, start, count):
    regs = []
    addr = start
    remain = count

    while remain > 0:
        chunk = min(remain, MAX_HOLDING_READ_REGS)
        resp = adapter.read_holding(addr, chunk)
        assert resp is not None
        part = regs_from_response(resp)
        assert len(part) >= chunk
        regs.extend(part[:chunk])
        addr += chunk
        remain -= chunk

    return regs


def check_coil_read_write(adapter, coil_packed_start):
    """Read and toggle packed coil bits, then restore original state."""
    resp = adapter.read_coils(coil_packed_start, 16)
    assert resp is not None
    bits = bits_from_response(resp)
    assert len(bits) >= 16

    original = bool(bits[0])
    toggled = not original

    assert adapter.write_coil(coil_packed_start, toggled)

    resp = adapter.read_coils(coil_packed_start, 16)
    assert resp is not None
    bits = bits_from_response(resp)
    assert bool(bits[0]) is toggled

    adapter.write_coil(coil_packed_start, original)


def check_holding_read_write(adapter, holding_start):
    """Read/write holding register map and restore writable values."""
    resp = adapter.read_holding(holding_start, 6)
    assert resp is not None
    regs = regs_from_response(resp)
    assert len(regs) >= 6

    original_u16 = regs[0]
    original_u64_regs = regs[1:5]

    new_u16 = 0x1234
    new_u64_regs = [0x0102, 0x0304, 0x0506, 0x0708]
    write_vals = [new_u16] + new_u64_regs

    assert adapter.write_registers(holding_start, write_vals)

    resp = adapter.read_holding(holding_start, 6)
    assert resp is not None
    regs = regs_from_response(resp)
    assert regs[0] == new_u16
    assert regs[1:5] == new_u64_regs

    # Restore writable values only (dummy20 + dummy21)
    restore_vals = [original_u16] + original_u64_regs
    adapter.write_registers(holding_start, restore_vals)


def check_input_read(adapter, input_start):
    """Read input-register map and validate returned register payload."""
    resp = adapter.read_input(input_start, 6)
    assert resp is not None
    regs = regs_from_response(resp)
    assert len(regs) >= 6
    assert all(isinstance(v, int) for v in regs[:6])


def check_seekable_window(adapter, group):
    """Repeated FC20 reads have no implicit cursor or byte swapping."""
    number = group["start"] + 1
    first = adapter.read_file_records(1, number, 0, 4)
    assert adapter.read_file_records(1, number, 0, 4) == first
    assert adapter.read_file_records(1, number, 1, 2) == first[2:6]


def check_seekable_ios(adapter, groups, io_ids=("descriptor1", "capabilities1")):
    """Read the existing windows with explicit, repeatable byte offsets."""
    for io_id in io_ids:
        group = groups[io_id]
        file = SeekableFile(adapter, 1, group["start"] + 1, group["window_regs"] + 1)
        width = group["window_regs"] * 2
        data = file.read(0, width)
        assert len(data) == width and any(data)
        assert file.read(0, width) == data
        assert file.read(1, width - 1) == data[1:]
        if io_id.startswith("capabilities"):
            assert file.read(512, 2) == bytes(2)
        with pytest.raises(OSError):
            file.write(0, b"xx")


def check_seekable_fileio(adapter, group):
    """Write records across file boundaries and verify replacement at zero."""
    file = SeekableFile(adapter, 1, group["start"] + 1, group["window_regs"] + 1)
    for data in (bytes(range(256)) * 79 + b"end!", b"short!"):
        file.write(0, data)
        assert file.read(0, len(data)) == data
        assert file.read(0, len(data)) == data
        assert file.read(len(data), 4) == bytes(4)
    file.write(2, b"AB")
    assert file.read(0, 6) == b"shABt!"
