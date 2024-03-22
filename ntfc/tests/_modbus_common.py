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


def check_seekable_window(adapter, descriptor_group):
    """Validate seekable holding window offset control semantics."""
    seekable_start = descriptor_group["start"]
    window_regs = descriptor_group["window_regs"]
    ctrl_reg = descriptor_group["ctrl"]

    assert adapter.write_registers(ctrl_reg, [0])

    regs_at_0 = read_holding_regs_chunked(adapter, seekable_start, window_regs)
    assert len(regs_at_0) == window_regs

    resp = adapter.read_holding(ctrl_reg, 1)
    assert resp is not None
    regs = regs_from_response(resp)
    assert regs[0] == window_regs * 2

    assert adapter.write_registers(ctrl_reg, [4])
    resp = adapter.read_holding(ctrl_reg, 1)
    assert resp is not None
    regs = regs_from_response(resp)
    assert regs[0] == 4

    sample_regs = min(16, window_regs - 2)
    assert sample_regs > 0
    regs_at_4 = read_holding_regs_chunked(adapter, seekable_start, sample_regs)
    assert len(regs_at_4) == sample_regs
    # Offset=4 bytes means two 16-bit registers shift.
    assert regs_at_4 == regs_at_0[2 : 2 + sample_regs]

    # Partial read must not auto-increment offset.
    resp = adapter.read_holding(seekable_start, 1)
    assert resp is not None

    resp = adapter.read_holding(ctrl_reg, 1)
    assert resp is not None
    regs = regs_from_response(resp)
    assert regs[0] == 4

    # Restore default offset for following tests.
    adapter.write_registers(ctrl_reg, [0])


def check_seekable_ios(
    adapter, seekable_groups, io_ids=("descriptor1", "capabilities1")
):
    """Validate seekable windows for descriptor/capabilities IOs."""
    for io_id in io_ids:
        group = seekable_groups[io_id]
        start = group["start"]
        ctrl = group["ctrl"]
        window_regs = group["window_regs"]

        assert adapter.write_registers(ctrl, [0])

        regs_at_0 = read_holding_regs_chunked(adapter, start, window_regs)
        assert len(regs_at_0) == window_regs

        resp = adapter.read_holding(ctrl, 1)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[0] == window_regs * 2

        assert adapter.write_registers(ctrl, [4])
        resp = adapter.read_holding(ctrl, 1)
        assert resp is not None
        regs = regs_from_response(resp)
        assert regs[0] == 4


def check_seekable_fileio(adapter, fileio_group):
    """Write via seekable FileIO and read back."""
    start = fileio_group["start"]
    ctrl = fileio_group["ctrl"]

    assert adapter.write_registers(ctrl, [0])

    write_regs = [0x1234, 0xABCD, 0x00FF, 0x5501]
    assert adapter.write_registers(start, write_regs)

    # Re-seek to start and read back exact payload.
    adapter.write_registers(ctrl, [0])
    resp = adapter.read_holding(start, len(write_regs))
    assert resp is not None
    regs = regs_from_response(resp)
    assert regs[: len(write_regs)] == write_regs
