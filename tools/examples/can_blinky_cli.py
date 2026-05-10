#!/usr/bin/env python3
"""CAN Blinky Demo CLI.

Simple host-side helper for descriptors/examples/blinky_can_demo.yaml.

Usage examples:
  python3 tools/examples/can_blinky_cli.py start
  python3 tools/examples/can_blinky_cli.py stop
  python3 tools/examples/can_blinky_cli.py status
  python3 tools/examples/can_blinky_cli.py period 250000
  python3 tools/examples/can_blinky_cli.py period 100000 500000
  python3 tools/examples/can_blinky_cli.py toggle
  python3 tools/examples/can_blinky_cli.py interactive

Requires:
  pip install -e tools/dawnpy -e tools/dawnpy-can
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    from dawnpy_can.client import CanClient
    from dawnpy_can.descriptor import CanAccess, load_can_descriptor
except Exception as exc:  # pragma: no cover - import-time dependency guard
    print(
        "Missing dependency: dawnpy-can.\n"
        "Install with: pip install -e tools/dawnpy -e tools/dawnpy-can",
        file=sys.stderr,
    )
    raise SystemExit(1) from exc


REPO_ROOT = Path(__file__).resolve().parents[2]
DEFAULT_DESCRIPTOR = str(REPO_ROOT / "descriptors/examples/blinky_can_demo.yaml")
DEFAULT_IFNAME = "can0"
DEFAULT_TIMEOUT = 1.0

CONTROL_ID = "ctrl_blinky"
TRIGGER_ID = "trig_blinky"
LED_ID = "led1"
START_INDEX_ID = "cfg_seq_start"
DWELL_OFF_ID = "cfg_dwell_off"
DWELL_ON_ID = "cfg_dwell_on"

CMD_STOP = 0
CMD_START = 1
CMD_RESET = 0


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


def unpack_u8(data: bytes | None) -> int | None:
    if data is None or len(data) < 1:
        return None
    return data[0]


def unpack_u32(data: bytes | None) -> int | None:
    if data is None or len(data) < 4:
        return None
    return struct.unpack("<I", data[:4])[0]


class BlinkyCan:
    def __init__(
        self,
        descriptor_path: str,
        ifname: str,
        timeout: float,
        extended: bool | None,
    ) -> None:
        self.timeout = timeout
        self.desc = load_can_descriptor(descriptor_path)
        self.extended = (
            self.desc.uses_extended_ids() if extended is None else extended
        )
        self.client = CanClient(self.desc, ifname=ifname, extended=self.extended)

        self.control_read = self._access(CONTROL_ID, "read")
        self.control_write = self._access(CONTROL_ID, "write")
        self.trigger_write = self._access(TRIGGER_ID, "write")
        self.led_read = self._access(LED_ID, "read")
        self.start_index_read = self._access(START_INDEX_ID, "read")
        self.dwell_off_read = self._access(DWELL_OFF_ID, "read")
        self.dwell_on_read = self._access(DWELL_ON_ID, "read")
        self.dwell_off_write = self._access(DWELL_OFF_ID, "write")
        self.dwell_on_write = self._access(DWELL_ON_ID, "write")

    def _access(self, io_id: str, method: str) -> CanAccess:
        for access in self.desc.get_access(io_id):
            if access.method == method:
                return access
        raise RuntimeError(f"missing CAN {method} access for {io_id}")

    def __enter__(self) -> "BlinkyCan":
        self.client.start()
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.client.close()
        return False

    def read(self, access: CanAccess) -> bytes | None:
        if access.method != "read":
            raise RuntimeError(f"{access.method} access cannot use simple read")
        return self.client.read_simple(access, timeout=self.timeout)

    def write(self, access: CanAccess, payload: bytes) -> None:
        if access.method != "write":
            raise RuntimeError(f"{access.method} access cannot use simple write")
        if len(payload) > 8:
            raise RuntimeError("simple CAN write payload exceeds 8 bytes")
        self.client.write_simple(access, payload)

    def write_segmented(self, access: CanAccess, payload: bytes) -> None:
        if access.method != "write_seg":
            raise RuntimeError(f"{access.method} access cannot use segmented write")
        self.client.write_segmented(access, payload)

    def start(self) -> None:
        self.write(self.control_write, bytes([CMD_START]))
        print("  Blinky started.")

    def stop(self) -> None:
        self.write(self.control_write, bytes([CMD_STOP]))
        print("  Blinky stopped.")

    def reset(self) -> None:
        self.write(self.trigger_write, bytes([CMD_RESET]))
        print("  Blinky reset.")

    def period(self, off_us: int, on_us: int) -> None:
        self.write(self.dwell_off_write, struct.pack("<I", off_us))
        self.write(self.dwell_on_write, struct.pack("<I", on_us))
        print(f"  Period set: off={fmt_us(off_us)} on={fmt_us(on_us)}")

    def read_running(self) -> bool | None:
        value = unpack_u8(self.read(self.control_read))
        if value is None:
            return None
        return value == CMD_START

    def toggle(self) -> None:
        running = self.read_running()
        if running is None:
            raise RuntimeError("control state read timed out")
        if running:
            self.stop()
        else:
            self.start()

    def status(self) -> None:
        running = self.read_running()
        start_index = unpack_u32(self.read(self.start_index_read))
        led = unpack_u32(self.read(self.led_read))
        dwell_off = unpack_u32(self.read(self.dwell_off_read))
        dwell_on = unpack_u32(self.read(self.dwell_on_read))

        print("\nBlinky status:")
        if running is None:
            print("  Running:   <timeout>")
        else:
            print(f"  Running:   {'yes' if running else 'no'}")
        print(
            f"  Start idx: {start_index}"
            if start_index is not None
            else "  Start idx: <timeout>"
        )
        print(f"  LED value: {led}" if led is not None else "  LED value: <timeout>")
        print(
            f"  Dwell off: {fmt_us(dwell_off)}"
            if dwell_off is not None
            else "  Dwell off: <timeout>"
        )
        print(
            f"  Dwell on:  {fmt_us(dwell_on)}"
            if dwell_on is not None
            else "  Dwell on:  <timeout>"
        )

    def info(self) -> None:
        print("  CAN mapping:")
        self._print_access("led read", self.led_read)
        self._print_access("control read", self.control_read)
        self._print_access("control write", self.control_write)
        self._print_access("trigger write", self.trigger_write)
        self._print_access("start idx read", self.start_index_read)
        self._print_access("dwell off read", self.dwell_off_read)
        self._print_access("dwell off write", self.dwell_off_write)
        self._print_access("dwell on read", self.dwell_on_read)
        self._print_access("dwell on write", self.dwell_on_write)

    @staticmethod
    def _print_access(label: str, access: CanAccess) -> None:
        print(f"    {label + ':':17s} 0x{access.can_id:X}")


HELP = """Commands:
  start              Start blinking
  stop               Stop blinking
  reset              Reset sequencer
  toggle             Toggle start/stop
  period <us>        Set symmetric dwell
  period <off> <on>  Set asymmetric dwell
  status             Read state and dwell values
  info               Show descriptor-derived CAN IDs
  help               Show this text
  quit               Exit
"""


def run_interactive(cli: BlinkyCan) -> None:
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


def run_command(cli: BlinkyCan, command: str, args: list[str]) -> None:
    if command == "start":
        cli.start()
    elif command == "stop":
        cli.stop()
    elif command == "reset":
        cli.reset()
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Dawn CAN blinky helper")
    parser.add_argument(
        "command",
        nargs="?",
        choices=[
            "start",
            "stop",
            "reset",
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
    parser.add_argument("--ifname", "-i", default=DEFAULT_IFNAME)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    parser.add_argument("--extended", action="store_true")

    args = parser.parse_args()
    extended = True if args.extended else None

    try:
        with BlinkyCan(
            args.descriptor,
            args.ifname,
            args.timeout,
            extended,
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
