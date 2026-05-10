#!/usr/bin/env python3
"""Modbus RTU Blinky Demo CLI

Simple host-side helper for the Modbus RTU blinky demo.

Usage examples:
  python3 tools/examples/modbus_blinky_cli.py start
  python3 tools/examples/modbus_blinky_cli.py stop
  python3 tools/examples/modbus_blinky_cli.py status
  python3 tools/examples/modbus_blinky_cli.py period 250000
  python3 tools/examples/modbus_blinky_cli.py period 100000 500000
  python3 tools/examples/modbus_blinky_cli.py toggle
  python3 tools/examples/modbus_blinky_cli.py interactive

Requires:
  pip install -e tools/dawnpy
  pip install -e tools/dawnpy-modbus
"""

from __future__ import annotations

import argparse
import sys
from typing import Any

DEFAULT_DESCRIPTOR = "descriptors/examples/blinky_modbus_rtu_demo.yaml"
DEFAULT_PORT = "/dev/ttyUSB0"
DEFAULT_CONTROL_ID = "ctrl_blinky"
DEFAULT_LED_ID = "led1"
DEFAULT_START_INDEX_ID = "cfg_seq_start"
DEFAULT_DWELL_OFF_ID = "cfg_dwell_off"
DEFAULT_DWELL_ON_ID = "cfg_dwell_on"

# ---------------------------------------------------------------------------
# Optional dependency imports.
# ---------------------------------------------------------------------------

try:  # pragma: no branch
    from dawnpy.descriptor.client import load_client_descriptor
    from dawnpy_modbus.client import (
        BIT_REGISTER_TYPES,
        ModbusClient,
        ModbusDescriptorInfo,
        registers_from_value,
        value_from_registers,
    )
except Exception as exc:  # pragma: no cover - import-time dependency guard
    print(
        "Missing dependency: dawnpy / dawnpy-modbus.\n"
        "Install with: pip install -e tools/dawnpy -e tools/dawnpy-modbus",
        file=sys.stderr,
    )
    raise SystemExit(1) from exc


CMD_START = 1
CMD_STOP = 0


def wire_address(start_address: int) -> int:
    """Convert descriptor register numbering to legacy Modbus wire address."""
    return max(0, start_address - 1)


def wire_addresses(start_address: int) -> tuple[int, ...]:
    """Return candidate addresses for both descriptor and wire conventions."""
    wire = wire_address(start_address)
    if wire == start_address:
        return (start_address,)
    return (start_address, wire)


def decode_binding(binding, response: Any) -> tuple[Any, str | None]:
    """Decode a Modbus response to a python value."""
    if response is None:
        return None, "No response"

    if binding.group_type in BIT_REGISTER_TYPES:
        bits = list(getattr(response, "bits", []))
        if not bits:
            return None, "No bits"
        return bool(bits[0]), None

    registers = list(getattr(response, "registers", []))
    if not registers:
        return None, "No registers"

    # The firmware maps mixed-width fields in low-word-first order.
    if len(registers) > 1:
        registers = list(reversed(registers))

    try:
        return value_from_registers(binding.dtype, registers), None
    except Exception:
        return registers, None


class BlinkyModbus:
    """Small wrapper for Modbus-backed blinky operations."""

    def __init__(
        self,
        descriptor_path: str,
        *,
        port: str | None,
        baudrate: int = 115200,
        parity: str = "E",
        stopbits: int = 1,
        timeout: float = 1.0,
        unit: int = 1,
        control_id: str = DEFAULT_CONTROL_ID,
        led_id: str = DEFAULT_LED_ID,
        start_index_id: str = DEFAULT_START_INDEX_ID,
        dwell_off_id: str = DEFAULT_DWELL_OFF_ID,
        dwell_on_id: str = DEFAULT_DWELL_ON_ID,
    ) -> None:
        self.unit = unit
        self.desc = load_client_descriptor(descriptor_path)
        self.info = ModbusDescriptorInfo(self.desc, unit=unit)

        self.control_id = control_id
        self.led_id = led_id
        self.start_index_id = start_index_id
        self.dwell_off_id = dwell_off_id
        self.dwell_on_id = dwell_on_id

        self.control = self.info.get_binding(control_id)
        self.led = self.info.get_binding(led_id)
        self.start_index = self.info.get_binding(start_index_id)
        self.dwell_off = self.info.get_binding(dwell_off_id)
        self.dwell_on = self.info.get_binding(dwell_on_id)

        if self.control is None:
            raise RuntimeError(f"Missing control IO '{control_id}' in descriptor")
        if self.start_index is None:
            raise RuntimeError(
                f"Missing start-index IO '{start_index_id}' in descriptor"
            )

        serial_port = port or self.info.path
        if not serial_port:
            raise RuntimeError(
                "Modbus path not specified and not present in descriptor"
            )

        self.client = ModbusClient(
            port=serial_port,
            baudrate=baudrate,
            parity=parity,
            stopbits=stopbits,
            timeout=timeout,
        )

    def __enter__(self):
        if not self.client.connect():
            raise RuntimeError(f"Failed to open Modbus serial port {self.client.port}")
        return self

    def __exit__(self, exc_type, exc, tb):
        self.client.close()
        return False

    @staticmethod
    def _fmt_us(us: int) -> str:
        if us >= 1_000_000:
            return f"{us / 1_000_000:.1f}s"
        if us >= 1_000:
            return f"{us / 1000:.0f}ms"
        return f"{us}us"

    def _read_binding(self, binding):
        for addr in wire_addresses(binding.start_address):
            if binding.group_type in BIT_REGISTER_TYPES:
                if binding.group_type.startswith("coil"):
                    response = self.client.read_coils(
                        self.unit, addr, binding.register_count
                    )
                else:
                    response = self.client.read_discrete_inputs(
                        self.unit, addr, binding.register_count
                    )
            elif binding.group_type == "holding":
                response = self.client.read_holding_registers(
                    self.unit, addr, binding.register_count
                )
            else:
                response = self.client.read_input_registers(
                    self.unit, addr, binding.register_count
                )

            if response is None:
                continue
            return response

        return None

    def _write_binding(self, binding, value) -> bool:
        for addr in wire_addresses(binding.start_address):
            if binding.group_type in BIT_REGISTER_TYPES:
                if self.client.write_coil(self.unit, addr, bool(value)):
                    return True
                continue

            if binding.group_type in ("input", "discrete", "discrete_packed"):
                return False

            try:
                regs = registers_from_value(binding.dtype, value)
            except Exception:
                return False

            if len(regs) > 1:
                regs = list(reversed(regs))

            if self.client.write_registers(self.unit, addr, regs):
                return True
        return False

    def read(self, io_name: str):
        binding = self.info.get_binding(io_name)
        if binding is None:
            raise RuntimeError(f"Unknown IO '{io_name}'")
        return decode_binding(binding, self._read_binding(binding))

    def write(self, io_name: str, value) -> bool:
        binding = self.info.get_binding(io_name)
        if binding is None:
            raise RuntimeError(f"Unknown IO '{io_name}'")
        if not binding.rw and io_name != self.control_id:
            raise RuntimeError(f"IO '{io_name}' is not writable")
        return self._write_binding(binding, value)

    def cmd_start(self) -> None:
        if not self.write(self.control_id, CMD_START):
            raise RuntimeError("Failed to send START command")
        print("  Blinky started.")

    def cmd_stop(self) -> None:
        if not self.write(self.control_id, CMD_STOP):
            raise RuntimeError("Failed to send STOP command")
        print("  Blinky stopped.")

    def cmd_toggle(self) -> None:
        running, _ = self.read(self.control_id)
        if running is None:
            raise RuntimeError("Could not read current control state")
        cmd = CMD_START if bool(running) is False else CMD_STOP
        if not self.write(self.control_id, cmd):
            raise RuntimeError("Failed to send TOGGLE command")
        print("  Blinky started." if cmd == CMD_START else "  Blinky stopped.")

    def cmd_period(self, off_us: int, on_us: int) -> None:
        if self.dwell_off is None or self.dwell_on is None:
            raise RuntimeError("This firmware descriptor does not expose dwell fields")

        ok = self.write(self.dwell_off_id, int(off_us))
        ok = ok and self.write(self.dwell_on_id, int(on_us))
        if not ok:
            raise RuntimeError("Failed to write dwell values")
        print(
            "  Period set: "
            f"off={self._fmt_us(int(off_us))} on={self._fmt_us(int(on_us))}"
        )

    def cmd_status(self) -> None:
        print("\nBlinky status:")

        running, _ = self.read(self.control_id)
        print(f"  Running:   {'yes' if bool(running) else 'no'}")

        start_idx, _ = self.read(self.start_index_id)
        if start_idx is None:
            print("  Start idx: <unreadable>")
        else:
            print(f"  Start idx: {int(start_idx)}")

        if self.led is not None:
            led, _ = self.read(self.led_id)
            print(f"  LED value: {int(led)}" if led is not None else "  LED value: <unreadable>")

        if self.dwell_off is not None and self.dwell_on is not None:
            off_val, _ = self.read(self.dwell_off_id)
            on_val, _ = self.read(self.dwell_on_id)
            print(
                f"  Dwell off: {self._fmt_us(int(off_val))}"
                if off_val is not None
                else "  Dwell off: <unreadable>"
            )
            print(
                f"  Dwell on:  {self._fmt_us(int(on_val))}"
                if on_val is not None
                else "  Dwell on:  <unreadable>"
            )

    def cmd_show(self) -> None:
        if self.dwell_off is None or self.dwell_on is None:
            print("  Dwell info not available in this descriptor.")
            return
        off_val, _ = self.read(self.dwell_off_id)
        on_val, _ = self.read(self.dwell_on_id)
        print(
            f"  Dwell off: {self._fmt_us(int(off_val))}"
            if off_val is not None
            else "  Dwell off: <unreadable>"
        )
        print(
            f"  Dwell on:  {self._fmt_us(int(on_val))}"
            if on_val is not None
            else "  Dwell on:  <unreadable>"
        )

    def cmd_info(self) -> None:
        print("  Descriptor:            Blinky Modbus RTU Demo")
        if self.desc.protocols:
            proto = self.desc.protocols[0]
            print(f"  Protocol config path:  {proto.config.get('path', 'n/a')}")
            print(f"  Protocol baudrate:     {proto.config.get('baudrate', 'n/a')}")
            print(f"  Modbus unit:           {self.unit}")
        print(f"  Control IO:            {self.control_id}")
        print(f"  LED IO:                {self.led_id if self.led else 'n/a'}")
        print(f"  Start index IO:        {self.start_index_id}")
        print(f"  Dwell off IO:          {self.dwell_off_id if self.dwell_off else 'n/a'}")
        print(f"  Dwell on  IO:          {self.dwell_on_id if self.dwell_on else 'n/a'}")


def _parse_int(raw: str) -> int:
    try:
        return int(raw, 0)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"Invalid integer: {raw}") from exc


_HELP = """Commands:
  start            Start the blinky
  stop             Stop the blinky
  toggle           Toggle start/stop
  status           Show runtime state + start index + dwell table
  show             Show dwell table
  period <us>      Set symmetric dwell (e.g. period 250000)
  period <off> <on>  Set asymmetric (e.g. period 100000 500000)
  info             Show connection/protocol details
  help             This message
  quit             Exit
"""


def run_interactive(cli: BlinkyModbus) -> None:
    print("  Type 'help' for commands.\n")
    print(_HELP)

    while True:
        try:
            line = input("blinky> ").strip()
        except EOFError:
            print()
            break

        if not line:
            continue

        parts = line.split()
        cmd = parts[0].lower()
        args = parts[1:]

        try:
            if cmd in {"quit", "exit", "q"}:
                break
            if cmd == "help":
                print(_HELP)
            elif cmd == "start":
                cli.cmd_start()
            elif cmd == "stop":
                cli.cmd_stop()
            elif cmd == "toggle":
                cli.cmd_toggle()
            elif cmd == "status":
                cli.cmd_status()
            elif cmd == "show":
                cli.cmd_show()
            elif cmd == "period":
                if len(args) == 1:
                    off = _parse_int(args[0])
                    cli.cmd_period(off, off)
                elif len(args) >= 2:
                    off = _parse_int(args[0])
                    on_ = _parse_int(args[1])
                    cli.cmd_period(off, on_)
                else:
                    print("  Usage: period <us>  |  period <off_us> <on_us>")
            elif cmd == "info":
                cli.cmd_info()
            else:
                print(f"  Unknown command: {cmd}")
        except Exception as exc:  # pragma: no cover - interactive command loop
            print(f"  Error: {exc}", file=sys.stderr)


def main() -> int:
    parser = argparse.ArgumentParser(description="Dawn Modbus RTU Blinky CLI")
    parser.add_argument(
        "command",
        nargs="?",
        choices=[
            "start",
            "stop",
            "toggle",
            "status",
            "show",
            "period",
            "info",
            "interactive",
        ],
        default="interactive",
        help="Command to run",
    )
    parser.add_argument("args", nargs="*", help="Optional command arguments")
    parser.add_argument(
        "--descriptor",
        "-d",
        default=DEFAULT_DESCRIPTOR,
        help="Descriptor path",
    )
    parser.add_argument(
        "--port",
        "-p",
        default=DEFAULT_PORT,
        help=f"Serial port (default: {DEFAULT_PORT})",
    )
    parser.add_argument("--baudrate", "-b", type=int, default=115200)
    parser.add_argument(
        "--parity",
        default="E",
        choices=["N", "O", "E", "n", "o", "e"],
    )
    parser.add_argument("--stopbits", type=int, default=1)
    parser.add_argument("--timeout", type=float, default=1.0)
    parser.add_argument("--unit", "-u", type=int, default=1)

    args = parser.parse_args()

    try:
        with BlinkyModbus(
            descriptor_path=args.descriptor,
            port=args.port,
            baudrate=args.baudrate,
            parity=str(args.parity).upper(),
            stopbits=args.stopbits,
            timeout=args.timeout,
            unit=args.unit,
        ) as cli:
            if args.command == "start":
                cli.cmd_start()
            elif args.command == "stop":
                cli.cmd_stop()
            elif args.command == "toggle":
                cli.cmd_toggle()
            elif args.command == "status":
                cli.cmd_status()
            elif args.command == "show":
                cli.cmd_show()
            elif args.command == "period":
                if not args.args:
                    raise RuntimeError("Usage: period <us> | period <off_us> <on_us>")
                off = _parse_int(args.args[0])
                on_ = _parse_int(args.args[1]) if len(args.args) > 1 else off
                cli.cmd_period(off, on_)
            elif args.command == "info":
                cli.cmd_info()
            elif args.command == "interactive":
                run_interactive(cli)
    except KeyboardInterrupt:
        print()
    except Exception as exc:  # pragma: no cover - CLI surface
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
