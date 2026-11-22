#!/usr/bin/env python3
"""Serial X-NUCLEO-LED16A1 demo helper.

Simple host-side helper for descriptors/examples/led16a1_serial_demo.yaml.

Usage examples:
  python3 tools/examples/led16a1/serial_led16a1_cli.py set 0 ff0000
  python3 tools/examples/led16a1/serial_led16a1_cli.py set 2 blue
  python3 tools/examples/led16a1/serial_led16a1_cli.py all 202020
  python3 tools/examples/led16a1/serial_led16a1_cli.py off
  python3 tools/examples/led16a1/serial_led16a1_cli.py status
  python3 tools/examples/led16a1/serial_led16a1_cli.py rainbow 10
  python3 tools/examples/led16a1/serial_led16a1_cli.py chase green 10
  python3 tools/examples/led16a1/serial_led16a1_cli.py demo
  python3 tools/examples/led16a1/serial_led16a1_cli.py interactive

Requires:
  pip install -e tools/dawnpy -e tools/dawnpy-serial
"""

from __future__ import annotations

import argparse
import colorsys
import math
import struct
import sys
import time
from pathlib import Path

try:
    from dawnpy.descriptor.client import load_client_descriptor
    from dawnpy.descriptor.definitions.summary import ObjectIdResolver
    from dawnpy_serial.serial import DawnSerialProtocol
except Exception as exc:  # pragma: no cover - import-time dependency guard
    print(
        "Missing dependency: dawnpy / dawnpy-serial.\n"
        "Install with: pip install -e tools/dawnpy -e tools/dawnpy-serial",
        file=sys.stderr,
    )
    raise SystemExit(1) from exc


REPO_ROOT = Path(__file__).resolve().parents[3]
DEFAULT_DESCRIPTOR = str(
    REPO_ROOT / "descriptors/examples/led16a1_serial_demo.yaml"
)
DEFAULT_TIMEOUT = 1.0
DEFAULT_PORT = "/dev/ttyACM0"
DEFAULT_EFFECT_TIME = 10.0
DEMO_FRAME_TIME = 0.03
DEMO_FADE_TIME = 2.0
DEMO_GAMMA = 2.2

LED_IDS = ["rgbled0", "rgbled1", "rgbled2", "rgbled3", "rgbled4"]

COLORS = {
    "off": 0x000000,
    "black": 0x000000,
    "red": 0xFF0000,
    "green": 0x00FF00,
    "blue": 0x0000FF,
    "yellow": 0xFFFF00,
    "cyan": 0x00FFFF,
    "magenta": 0xFF00FF,
    "white": 0xFFFFFF,
}


def parse_color(raw: str) -> int:
    name = raw.lower()
    if name in COLORS:
        return COLORS[name]
    try:
        value = int(name.removeprefix("#").removeprefix("0x"), 16)
    except ValueError as exc:
        raise RuntimeError(f"invalid color: {raw}") from exc
    if not 0 <= value <= 0xFFFFFF:
        raise RuntimeError(f"color out of range: {raw}")
    return value


def hsv_color(hue: float, value: float = 1.0) -> int:
    red, green, blue = colorsys.hsv_to_rgb(hue % 1.0, 1.0, value)
    return (int(red * 255) << 16) | (int(green * 255) << 8) | int(blue * 255)


class Led16a1Serial:
    def __init__(
        self,
        descriptor_path: str,
        port: str | None,
        baudrate: int | None,
        timeout: float,
        debug: bool,
    ) -> None:
        self.timeout = timeout
        self.desc = load_client_descriptor(descriptor_path)
        self.proto_desc = self.desc.get_protocol("serial")
        if self.proto_desc is None:
            raise RuntimeError("descriptor does not define a serial protocol")

        self.port = port or DEFAULT_PORT

        proto_baud = self.proto_desc.config.get("baudrate")
        self.baudrate = baudrate or (
            int(proto_baud) if proto_baud is not None else 115200
        )

        resolver = ObjectIdResolver()
        self.led_objids = [
            self._resolve_objid(resolver, io_id) for io_id in LED_IDS
        ]

        self.client = DawnSerialProtocol(
            self.port,
            baudrate=self.baudrate,
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

    def __enter__(self) -> "Led16a1Serial":
        if not self.client.connect():
            raise RuntimeError(f"failed to open serial port {self.port}")
        if not self.client.ping():
            self.client.disconnect()
            raise RuntimeError("device did not respond to ping")
        return self

    def __exit__(self, exc_type, exc, tb) -> bool:
        self.client.disconnect()
        return False

    def led_index(self, raw: str) -> int:
        try:
            index = int(raw, 0)
        except ValueError as exc:
            raise RuntimeError(f"invalid LED index: {raw}") from exc
        if not 0 <= index < len(self.led_objids):
            raise RuntimeError(
                f"LED index out of range 0-{len(self.led_objids) - 1}: {raw}"
            )
        return index

    def write(self, index: int, color: int) -> None:
        objid = self.led_objids[index]
        if not self.client.write_io(objid, struct.pack("<I", color)):
            raise RuntimeError(f"write failed for 0x{objid:08X}")

    def read(self, index: int) -> int | None:
        data = self.client.read_io(self.led_objids[index])
        if data is None or len(data) < 4:
            return None
        return struct.unpack("<I", data[:4])[0]

    def set(self, index: int, color: int) -> None:
        self.write(index, color)
        print(f"  LED{index} = #{color:06X}")

    def all(self, color: int) -> None:
        for index in range(len(self.led_objids)):
            self.write(index, color)
        print(f"  All LEDs = #{color:06X}")

    def status(self) -> None:
        print("\nLED status:")
        for index in range(len(self.led_objids)):
            color = self.read(index)
            if color is None:
                print(f"  LED{index}: <timeout>")
            else:
                print(f"  LED{index}: #{color:06X}")

    def rainbow(self, duration: float) -> None:
        count = len(self.led_objids)
        start = time.monotonic()
        while (now := time.monotonic() - start) < duration:
            for index in range(count):
                self.write(index, hsv_color(now / 4.0 + index / count))
        self.all(0)

    def demo(self, duration: float | None) -> None:
        """Slow color wave over all LEDs, runs until Ctrl-C by default."""
        count = len(self.led_objids)
        start = time.monotonic()
        try:
            while True:
                now = time.monotonic() - start
                if duration is not None and now >= duration:
                    break
                fade = min(1.0, now / DEMO_FADE_TIME)
                if duration is not None:
                    fade = min(fade, (duration - now) / DEMO_FADE_TIME)
                for index in range(count):
                    phase = index / count
                    hue = now / 20.0 + phase * 0.4
                    breath = 0.75 + 0.25 * math.sin(now / 3.0 - phase * 2.0)
                    value = (fade * breath) ** DEMO_GAMMA
                    self.write(index, hsv_color(hue, value))
                time.sleep(DEMO_FRAME_TIME)
        finally:
            self.all(0)

    def chase(self, color: int, duration: float) -> None:
        count = len(self.led_objids)
        start = time.monotonic()
        step = 0
        while time.monotonic() - start < duration:
            for index in range(count):
                self.write(index, color if index == step % count else 0)
            step += 1
            time.sleep(0.1)
        self.all(0)

    def info(self) -> None:
        print("  Descriptor:       LED16A1 Serial Demo")
        print(f"  Serial path:      {self.port}")
        print(f"  Serial baudrate:  {self.baudrate}")
        for index, objid in enumerate(self.led_objids):
            print(f"  LED{index} ObjectID:    0x{objid:08X}")


HELP = """Commands:
  set <led> <color>       Set one LED (0-4)
  all <color>             Set all LEDs
  off                     Turn all LEDs off
  status                  Read back LED colors
  demo [seconds]          Slow color wave, Ctrl-C to stop
  rainbow [seconds]       Rainbow effect
  chase [color] [seconds] Running light effect
  info                    Show descriptor-derived ObjectIDs
  help                    Show this text
  quit                    Exit

Colors: RRGGBB hex or one of
  red green blue yellow cyan magenta white off
"""


def run_interactive(cli: Led16a1Serial) -> None:
    print("  Type 'help' for commands.\n")
    while True:
        try:
            line = input("led16a1> ").strip()
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
        except KeyboardInterrupt:
            print()
        except Exception as exc:  # pragma: no cover - interactive shell
            print(f"  Error: {exc}", file=sys.stderr)


def run_command(cli: Led16a1Serial, command: str, args: list[str]) -> None:
    if command == "set":
        if len(args) != 2:
            raise RuntimeError("usage: set <led> <color>")
        cli.set(cli.led_index(args[0]), parse_color(args[1]))
    elif command == "all":
        if len(args) != 1:
            raise RuntimeError("usage: all <color>")
        cli.all(parse_color(args[0]))
    elif command == "off":
        cli.all(0)
    elif command == "status":
        cli.status()
    elif command == "demo":
        if len(args) > 1:
            raise RuntimeError("usage: demo [seconds]")
        cli.demo(float(args[0]) if args else None)
    elif command == "rainbow":
        if len(args) > 1:
            raise RuntimeError("usage: rainbow [seconds]")
        cli.rainbow(float(args[0]) if args else DEFAULT_EFFECT_TIME)
    elif command == "chase":
        if len(args) > 2:
            raise RuntimeError("usage: chase [color] [seconds]")
        cli.chase(
            parse_color(args[0]) if args else COLORS["red"],
            float(args[1]) if len(args) > 1 else DEFAULT_EFFECT_TIME,
        )
    elif command == "info":
        cli.info()
    elif command == "help":
        print(HELP)
    elif command in {"quit", "exit", "q"}:
        raise SystemExit(0)
    else:
        raise RuntimeError(f"unknown command: {command}")


def main() -> int:
    parser = argparse.ArgumentParser(
        description="Dawn serial X-NUCLEO-LED16A1 helper"
    )
    parser.add_argument(
        "command",
        nargs="?",
        choices=[
            "set",
            "all",
            "off",
            "status",
            "demo",
            "rainbow",
            "chase",
            "info",
            "interactive",
        ],
        default="interactive",
    )
    parser.add_argument("args", nargs="*")
    parser.add_argument("--descriptor", "-d", default=DEFAULT_DESCRIPTOR)
    parser.add_argument("--port", "-p", default=DEFAULT_PORT)
    parser.add_argument("--baudrate", "-b", type=int, default=None)
    parser.add_argument("--timeout", type=float, default=DEFAULT_TIMEOUT)
    parser.add_argument("--debug", action="store_true")

    args = parser.parse_args()

    try:
        with Led16a1Serial(
            args.descriptor,
            args.port,
            args.baudrate,
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
