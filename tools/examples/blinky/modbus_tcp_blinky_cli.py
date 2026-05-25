#!/usr/bin/env python3
"""Modbus TCP blinky demo helper.

Simple host-side helper for descriptors/examples/blinky_modbus_tcp_demo.yaml.

Usage examples:
  python3 tools/examples/blinky/modbus_tcp_blinky_cli.py start
  python3 tools/examples/blinky/modbus_tcp_blinky_cli.py stop
  python3 tools/examples/blinky/modbus_tcp_blinky_cli.py status
  python3 tools/examples/blinky/modbus_tcp_blinky_cli.py --host 192.168.8.104 period 250000
  python3 tools/examples/blinky/modbus_tcp_blinky_cli.py --host 192.168.8.104 interactive

Requires:
  pip install -e tools/dawnpy -e tools/dawnpy-modbus
"""

from __future__ import annotations

import argparse
import sys
import time
from dataclasses import dataclass
from pathlib import Path
from typing import Any

DEFAULT_HOST = "192.168.8.104"
DEFAULT_PORT = 5020
DEFAULT_TIMEOUT = 1.0
DEFAULT_UNIT = 1
DEFAULT_CONNECT_ATTEMPTS = 10
DEFAULT_RETRY_S = 0.5

REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_DESCRIPTOR = str(
    REPO_ROOT / "descriptors/examples/blinky_modbus_tcp_demo.yaml"
)

CONTROL_ID = "ctrl_blinky"
LED_ID = "led1"
START_INDEX_ID = "cfg_seq_start"
DWELL_OFF_ID = "cfg_dwell_off"
DWELL_ON_ID = "cfg_dwell_on"

CMD_STOP = 0
CMD_START = 1

try:  # pragma: no branch
    from dawnpy.descriptor.client import load_client_descriptor
    from dawnpy.descriptor.support.utils import resolve_references
    from dawnpy_modbus.client import (
        BIT_REGISTER_TYPES,
        registers_from_value,
        registers_needed,
        value_from_registers,
    )
    from pymodbus.client import ModbusTcpClient
except Exception as exc:  # pragma: no cover - import-time dependency guard
    print(
        "Missing dependency: dawnpy / dawnpy-modbus / pymodbus.\n"
        "Install with: pip install -e tools/dawnpy -e tools/dawnpy-modbus",
        file=sys.stderr,
    )
    raise SystemExit(1) from exc


@dataclass
class ModbusBinding:
    io_id: str
    dtype: str
    rw: bool
    group_type: str
    start_address: int
    register_count: int


class ModbusTcpDescriptorInfo:
    def __init__(self, descriptor) -> None:
        self.descriptor = descriptor
        self.protocol = descriptor.get_protocol("modbus_tcp")
        if self.protocol is None:
            raise ValueError("descriptor has no modbus_tcp protocol entry")

        self.config = self.protocol.config or {}
        self.port = int(self.config.get("port", DEFAULT_PORT))
        self.binding_map = self._build_binding_map()

    def _build_binding_map(self) -> dict[str, ModbusBinding]:
        registers = self.config.get("registers")
        if not registers:
            raise ValueError("modbus_tcp protocol must define config.registers")

        binding_map: dict[str, ModbusBinding] = {}
        for reg in registers:
            reg_type = str(reg.get("type", "")).lower()
            start = int(reg.get("start", 0))
            offset = 0

            for io_id in resolve_references(reg.get("bindings", [])):
                io = self.descriptor.get_io(io_id)
                if io is None:
                    raise ValueError(f"unknown IO '{io_id}' in modbus registers")

                dtype = io.dtype or "uint16"
                dim = max(1, int(getattr(io, "config", {}).get("dim", 1)))
                if reg_type in BIT_REGISTER_TYPES:
                    count = dim
                else:
                    count = registers_needed(dtype) * dim

                binding_map[io_id] = ModbusBinding(
                    io_id=io_id,
                    dtype=dtype,
                    rw=bool(io.rw),
                    group_type=reg_type,
                    start_address=start + offset,
                    register_count=count,
                )
                offset += count

        return binding_map

    def get_binding(self, io_id: str) -> ModbusBinding | None:
        return self.binding_map.get(io_id)


def wire_address(start_address: int) -> int:
    return max(0, start_address - 1)


def wire_addresses(start_address: int) -> tuple[int, ...]:
    wire = wire_address(start_address)
    if wire == start_address:
        return (start_address,)
    return (start_address, wire)


def parse_int(raw: str) -> int:
    try:
        return int(raw, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"invalid integer: {raw}") from exc


def fmt_us(value: int) -> str:
    if value >= 1_000_000:
        return f"{value / 1_000_000:.1f}s"
    if value >= 1000:
        return f"{value / 1000:.0f}ms"
    return f"{value}us"


def decode_binding(binding: ModbusBinding, response: Any) -> tuple[Any, str | None]:
    if response is None:
        return None, "No response"

    if reg_type_is_bit(binding.group_type):
        bits = list(getattr(response, "bits", []))
        if not bits:
            return None, "No bits"
        return bool(bits[0]), None

    registers = list(getattr(response, "registers", []))
    if not registers:
        return None, "No registers"

    if len(registers) > 1:
        registers = list(reversed(registers))

    try:
        return value_from_registers(binding.dtype, registers), None
    except Exception:
        return registers, None


def reg_type_is_bit(reg_type: str) -> bool:
    return reg_type in BIT_REGISTER_TYPES


class BlinkyModbusTcp:
    def __init__(
        self,
        descriptor_path: str,
        host: str,
        port: int | None,
        timeout: float,
        unit: int,
        connect_attempts: int,
        retry_s: float,
        control_id: str = CONTROL_ID,
        led_id: str = LED_ID,
        start_index_id: str = START_INDEX_ID,
        dwell_off_id: str = DWELL_OFF_ID,
        dwell_on_id: str = DWELL_ON_ID,
    ) -> None:
        self.host = host
        self.unit = unit
        self.connect_attempts = max(1, int(connect_attempts))
        self.retry_s = max(0.0, float(retry_s))
        self.desc = load_client_descriptor(descriptor_path)
        self.info = ModbusTcpDescriptorInfo(self.desc)
        self.port = port or self.info.port

        self.control_id = control_id
        self.led_id = led_id
        self.start_index_id = start_index_id
        self.dwell_off_id = dwell_off_id
        self.dwell_on_id = dwell_on_id

        self.control = self._require_binding(control_id)
        self.led = self.info.get_binding(led_id)
        self.start_index = self._require_binding(start_index_id)
        self.dwell_off = self.info.get_binding(dwell_off_id)
        self.dwell_on = self.info.get_binding(dwell_on_id)

        self.timeout = timeout
        self.client = ModbusTcpClient(host=self.host, port=self.port, timeout=timeout)

    def _require_binding(self, io_id: str) -> ModbusBinding:
        binding = self.info.get_binding(io_id)
        if binding is None:
            raise RuntimeError(f"missing IO '{io_id}' in descriptor")
        return binding

    def __enter__(self) -> "BlinkyModbusTcp":
        for _ in range(self.connect_attempts):
            self.client.close()
            self.client = ModbusTcpClient(
                host=self.host,
                port=self.port,
                timeout=self.timeout,
            )

            if self.client.connect() and self._probe_ready():
                return self

            self.client.close()
            time.sleep(self.retry_s)

        raise RuntimeError(
            "failed to connect to ready Modbus TCP server at "
            f"{self.host}:{self.port} after {self.connect_attempts} attempts"
        )

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.client.close()
        return False

    def _call(self, method: str, *args: Any, **kwargs: Any) -> Any:
        fn = getattr(self.client, method)
        try:
            response = fn(*args, **kwargs)
        except Exception:
            return None
        if response is None:
            return None
        if getattr(response, "isError", lambda: False)():
            return None
        return response

    def _probe_ready(self) -> bool:
        response = self._read_binding(self.control)
        return response is not None

    def _read_binding(self, binding: ModbusBinding) -> Any:
        for addr in wire_addresses(binding.start_address):
            if reg_type_is_bit(binding.group_type):
                response = self._call(
                    "read_coils",
                    addr,
                    count=binding.register_count,
                    device_id=self.unit,
                )
            elif binding.group_type == "holding":
                response = self._call(
                    "read_holding_registers",
                    addr,
                    count=binding.register_count,
                    device_id=self.unit,
                )
            else:
                response = self._call(
                    "read_input_registers",
                    addr,
                    count=binding.register_count,
                    device_id=self.unit,
                )

            if response is not None:
                return response
        return None

    def _write_binding(self, binding: ModbusBinding, value: Any) -> bool:
        for addr in wire_addresses(binding.start_address):
            if reg_type_is_bit(binding.group_type):
                response = self._call(
                    "write_coil", addr, bool(value), device_id=self.unit
                )
                if response is not None:
                    return True
                continue

            if binding.group_type != "holding":
                return False

            try:
                registers = registers_from_value(binding.dtype, value)
            except Exception:
                return False

            if len(registers) > 1:
                registers = list(reversed(registers))

            response = self._call(
                "write_registers", addr, registers, device_id=self.unit
            )
            if response is not None:
                return True

        return False

    def read(self, io_id: str) -> tuple[Any, str | None]:
        binding = self.info.get_binding(io_id)
        if binding is None:
            raise RuntimeError(f"unknown IO '{io_id}'")
        return decode_binding(binding, self._read_binding(binding))

    def write(self, io_id: str, value: Any) -> bool:
        binding = self.info.get_binding(io_id)
        if binding is None:
            raise RuntimeError(f"unknown IO '{io_id}'")
        if not binding.rw and io_id != self.control_id:
            raise RuntimeError(f"IO '{io_id}' is not writable")
        return self._write_binding(binding, value)

    def start(self) -> None:
        if not self.write(self.control_id, CMD_START):
            raise RuntimeError("failed to send START command")
        print("  Blinky started.")

    def stop(self) -> None:
        if not self.write(self.control_id, CMD_STOP):
            raise RuntimeError("failed to send STOP command")
        print("  Blinky stopped.")

    def toggle(self) -> None:
        running, _ = self.read(self.control_id)
        if running is None:
            raise RuntimeError("could not read current control state")
        command = CMD_STOP if bool(running) else CMD_START
        if not self.write(self.control_id, command):
            raise RuntimeError("failed to send TOGGLE command")
        print("  Blinky started." if command == CMD_START else "  Blinky stopped.")

    def period(self, off_us: int, on_us: int) -> None:
        if self.dwell_off is None or self.dwell_on is None:
            raise RuntimeError("this descriptor does not expose dwell fields")
        ok = self.write(self.dwell_off_id, int(off_us))
        ok = ok and self.write(self.dwell_on_id, int(on_us))
        if not ok:
            raise RuntimeError("failed to write dwell values")
        print(f"  Period set: off={fmt_us(off_us)} on={fmt_us(on_us)}")

    def status(self) -> None:
        print("\nBlinky status:")

        running, _ = self.read(self.control_id)
        print(f"  Running:   {'yes' if bool(running) else 'no'}")

        start_idx, _ = self.read(self.start_index_id)
        print(
            f"  Start idx: {int(start_idx)}"
            if start_idx is not None
            else "  Start idx: <unreadable>"
        )

        if self.led is not None:
            led, _ = self.read(self.led_id)
            print(
                f"  LED value: {int(led)}"
                if led is not None
                else "  LED value: <unreadable>"
            )

        if self.dwell_off is not None and self.dwell_on is not None:
            off_val, _ = self.read(self.dwell_off_id)
            on_val, _ = self.read(self.dwell_on_id)
            print(
                f"  Dwell off: {fmt_us(int(off_val))}"
                if off_val is not None
                else "  Dwell off: <unreadable>"
            )
            print(
                f"  Dwell on:  {fmt_us(int(on_val))}"
                if on_val is not None
                else "  Dwell on:  <unreadable>"
            )

    def info(self) -> None:
        title = getattr(getattr(self.desc, "metadata", None), "title", None)
        print(f"  Descriptor:            {title or 'Blinky Modbus TCP Demo'}")
        print(f"  Host:                  {self.host}")
        print(f"  TCP port:              {self.port}")
        print(f"  Modbus unit:           {self.unit}")
        print(f"  Control IO:            {self.control_id}")
        print(f"  LED IO:                {self.led_id if self.led else 'n/a'}")
        print(f"  Start index IO:        {self.start_index_id}")
        print(f"  Dwell off IO:          {self.dwell_off_id if self.dwell_off else 'n/a'}")
        print(f"  Dwell on  IO:          {self.dwell_on_id if self.dwell_on else 'n/a'}")


HELP = """Commands:
  start              Start blinking
  stop               Stop blinking
  toggle             Toggle start/stop
  period <us>        Set symmetric dwell
  period <off> <on>  Set asymmetric dwell
  status             Read state and dwell values
  info               Show connection details
  help               Show this text
  quit               Exit
"""


def run_command(cli: BlinkyModbusTcp, command: str, args: list[str]) -> None:
    if command == "start":
        cli.start()
    elif command == "stop":
        cli.stop()
    elif command == "toggle":
        cli.toggle()
    elif command == "status":
        cli.status()
    elif command == "info":
        cli.info()
    elif command == "period":
        if len(args) == 1:
            value = parse_int(args[0])
            cli.period(value, value)
        elif len(args) == 2:
            cli.period(parse_int(args[0]), parse_int(args[1]))
        else:
            raise RuntimeError("usage: period <us> | period <off_us> <on_us>")
    elif command == "help":
        print(HELP)
    elif command in {"quit", "exit", "q"}:
        raise SystemExit(0)
    else:
        raise RuntimeError(f"unknown command: {command}")


def run_interactive(cli: BlinkyModbusTcp) -> None:
    print("  Type 'help' for commands.\n")
    while True:
        try:
            line = input("blinky> ").strip()
        except EOFError:
            print()
            return
        if not line:
            continue

        parts = line.split()
        command = parts[0].lower()
        args = parts[1:]

        try:
            run_command(cli, command, args)
        except SystemExit:
            raise
        except Exception as exc:  # pragma: no cover - interactive shell
            print(f"  Error: {exc}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Dawn Modbus TCP blinky helper")
    parser.add_argument(
        "command",
        nargs="?",
        choices=[
            "start",
            "stop",
            "toggle",
            "period",
            "status",
            "info",
            "interactive",
        ],
        default="interactive",
    )
    parser.add_argument("args", nargs="*")
    parser.add_argument("--descriptor", "-d", default=DEFAULT_DESCRIPTOR)
    parser.add_argument("--host", default=DEFAULT_HOST)
    parser.add_argument("--port", "-p", type=int, default=None)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    parser.add_argument("--unit", "-u", type=int, default=DEFAULT_UNIT)
    parser.add_argument(
        "--connect-attempts",
        type=int,
        default=DEFAULT_CONNECT_ATTEMPTS,
        help="Retries while the target is still booting after reset.",
    )
    parser.add_argument(
        "--retry-s",
        type=float,
        default=DEFAULT_RETRY_S,
        help="Delay between connection/readiness retries in seconds.",
    )

    args = parser.parse_args()

    try:
        with BlinkyModbusTcp(
            args.descriptor,
            args.host,
            args.port,
            args.timeout,
            args.unit,
            args.connect_attempts,
            args.retry_s,
        ) as cli:
            if args.command == "interactive":
                run_interactive(cli)
            else:
                run_command(cli, args.command, args.args)
    except KeyboardInterrupt:
        print()
    except SystemExit as exc:
        return int(exc.code or 0)
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
