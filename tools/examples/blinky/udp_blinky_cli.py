#!/usr/bin/env python3
"""UDP blinky demo helper.

Simple host-side helper for descriptors/examples/blinky_udp_demo.yaml.

Usage examples:
  python3 tools/examples/blinky/udp_blinky_cli.py start
  python3 tools/examples/blinky/udp_blinky_cli.py status
  python3 tools/examples/blinky/udp_blinky_cli.py --host 192.168.8.104 period 250000
  python3 tools/examples/blinky/udp_blinky_cli.py --host 192.168.8.104 interactive

Requires:
  pip install -e tools/dawnpy -e tools/dawnpy-udp
"""

from __future__ import annotations

import argparse
import struct
import sys
from pathlib import Path

try:
    from dawnpy.descriptor.client import load_client_descriptor
    from dawnpy.descriptor.definitions.summary import ObjectIdResolver
    from dawnpy_udp.udp import DawnUdpProtocol
except Exception as exc:  # pragma: no cover - import-time dependency guard
    print(
        "Missing dependency: dawnpy / dawnpy-udp.\n"
        "Install with: pip install -e tools/dawnpy -e tools/dawnpy-udp",
        file=sys.stderr,
    )
    raise SystemExit(1) from exc


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_DESCRIPTOR = str(REPO_ROOT / "descriptors/examples/blinky_udp_demo.yaml")
DEFAULT_HOST = "192.168.8.104"
DEFAULT_PORT = 50000
DEFAULT_TIMEOUT = 1.0

OUTPUT_ID = "led1"
CONTROL_ID = "ctrl_blinky"
START_INDEX_ID = "cfg_seq_start"
DWELL_OFF_ID = "cfg_dwell_off"
DWELL_ON_ID = "cfg_dwell_on"

CMD_STOP = 0
CMD_START = 1


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


class BlinkyUdp:
    def __init__(
        self,
        descriptor_path: str,
        host: str,
        port: int | None,
        timeout: float,
        debug: bool,
    ) -> None:
        self.timeout = timeout
        self.desc = load_client_descriptor(descriptor_path)
        self.proto_desc = self.desc.get_protocol("udp")
        if self.proto_desc is None:
            raise RuntimeError("descriptor does not define a UDP protocol")

        self.host = host or DEFAULT_HOST
        proto_port = self.proto_desc.config.get("port")
        self.port = port or (int(proto_port) if proto_port is not None else DEFAULT_PORT)

        resolver = ObjectIdResolver()
        self.output_objid = self._resolve_objid(resolver, OUTPUT_ID)
        self.control_objid = self._resolve_objid(resolver, CONTROL_ID)
        self.start_index_objid = self._resolve_objid(resolver, START_INDEX_ID)
        self.dwell_off_objid = self._resolve_objid(resolver, DWELL_OFF_ID)
        self.dwell_on_objid = self._resolve_objid(resolver, DWELL_ON_ID)

        self.client = DawnUdpProtocol(
            self.host,
            port=self.port,
            timeout=timeout,
            verbose=debug,
        )

    def _resolve_objid(self, resolver: ObjectIdResolver, io_id: str) -> int:
        io = self.desc.get_io(io_id)
        if io is None:
            raise RuntimeError(f"descriptor missing IO '{io_id}'")
        objid = resolver.io_objid(io)
        if objid is None:
            raise RuntimeError(f"failed to resolve ObjectID for '{io_id}'")
        return objid

    def __enter__(self) -> "BlinkyUdp":
        if not self.client.connect():
            raise RuntimeError(f"failed to open UDP socket for {self.host}:{self.port}")
        if not self.client.ping():
            self.client.disconnect()
            raise RuntimeError("device did not respond to ping")
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.client.disconnect()
        return False

    def read(self, objid: int) -> bytes | None:
        return self.client.read_io(objid)

    def write(self, objid: int, payload: bytes) -> None:
        if not self.client.write_io(objid, payload):
            raise RuntimeError(f"write failed for 0x{objid:08X}")

    def start(self) -> None:
        self.write(self.control_objid, bytes([CMD_START]))
        print("  Blinky started.")

    def stop(self) -> None:
        self.write(self.control_objid, bytes([CMD_STOP]))
        print("  Blinky stopped.")

    def period(self, off_us: int, on_us: int) -> None:
        self.write(self.dwell_off_objid, struct.pack("<I", off_us))
        self.write(self.dwell_on_objid, struct.pack("<I", on_us))
        print(f"  Period set: off={fmt_us(off_us)} on={fmt_us(on_us)}")

    def read_running(self) -> bool | None:
        value = unpack_u8(self.read(self.control_objid))
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
        output = unpack_u32(self.read(self.output_objid))
        start_index = unpack_u32(self.read(self.start_index_objid))
        dwell_off = unpack_u32(self.read(self.dwell_off_objid))
        dwell_on = unpack_u32(self.read(self.dwell_on_objid))

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
        print(
            f"  LED value: {output}"
            if output is not None
            else "  LED value: <timeout>"
        )
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
        print("  Descriptor:            Blinky UDP Demo")
        print(f"  Host:                  {self.host}")
        print(f"  UDP port:              {self.port}")
        print(f"  Output IO ObjectID:    0x{self.output_objid:08X}")
        print(f"  Control IO ObjectID:   0x{self.control_objid:08X}")
        print(f"  Start index ObjectID:  0x{self.start_index_objid:08X}")
        print(f"  Dwell off ObjectID:    0x{self.dwell_off_objid:08X}")
        print(f"  Dwell on ObjectID:     0x{self.dwell_on_objid:08X}")


HELP = """Commands:
  start              Start blinking
  stop               Stop blinking
  toggle             Toggle start/stop
  period <us>        Set symmetric dwell
  period <off> <on>  Set asymmetric dwell
  status             Read state and dwell values
  info               Show descriptor-derived ObjectIDs
  help               Show this text
  quit               Exit
"""


def run_interactive(cli: BlinkyUdp) -> None:
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
        except Exception as exc:  # pragma: no cover
            print(f"  Error: {exc}", file=sys.stderr)


def run_command(cli: BlinkyUdp, command: str, args: list[str]) -> None:
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


def main() -> int:
    parser = argparse.ArgumentParser(description="Dawn UDP blinky helper")
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
    parser.add_argument("--debug", action="store_true")

    args = parser.parse_args()

    try:
        with BlinkyUdp(
            args.descriptor,
            args.host,
            args.port,
            args.timeout,
            args.debug,
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
