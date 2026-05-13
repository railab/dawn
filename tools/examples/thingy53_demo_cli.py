#!/usr/bin/env python3
"""Host helper for the Dawn Thingy:53 NimBLE demo.

The current Thingy:53 demo exposes one RGB LED IO over a NimBLE custom
service. This CLI is intentionally structured around feature subcommands so
additional Thingy:53 demo controls can be added later without changing the
entry point.

Requires:
  pip install bleak

Examples:
  python3 tools/examples/thingy53_demo_cli.py scan
  python3 tools/examples/thingy53_demo_cli.py rgb get       # auto-detect dawn-thingy53
  python3 tools/examples/thingy53_demo_cli.py rgb set '#ff4000'
  python3 tools/examples/thingy53_demo_cli.py rgb fade --cycles 2
  python3 tools/examples/thingy53_demo_cli.py rgb fade --steps 96 --period 0.03
"""

from __future__ import annotations

import argparse
import asyncio
import colorsys
import struct
import sys
from collections.abc import AsyncIterator
from contextlib import asynccontextmanager

GAP_NAME = "dawn-thingy53"
SCAN_TIMEOUT_S = 10.0

THINGY53_CUSTOM_SERVICE_UUID = "9f2d2b30-74d3-4f34-9f37-ef7d8f6d3000"
THINGY53_RGB_CHAR_UUID = "9f2d2b30-74d3-4f34-9f37-ef7d8f6d3001"

COLOR_NAMES = {
    "black": 0x000000,
    "white": 0xFFFFFF,
    "red": 0xFF0000,
    "green": 0x00FF00,
    "blue": 0x0000FF,
    "cyan": 0x00FFFF,
    "magenta": 0xFF00FF,
    "yellow": 0xFFFF00,
    "orange": 0xFF4000,
    "purple": 0x8000FF,
}


def parse_rgb(value: str) -> int:
    """Parse a color name, #RRGGBB, RRGGBB, or 0xRRGGBB value."""
    text = value.strip().lower()
    if text in COLOR_NAMES:
        return COLOR_NAMES[text]

    if text.startswith("#"):
        text = text[1:]
    elif text.startswith("0x"):
        text = text[2:]

    if len(text) != 6:
        raise argparse.ArgumentTypeError(
            "RGB color must be a name, #RRGGBB, RRGGBB, or 0xRRGGBB"
        )

    try:
        rgb = int(text, 16)
    except ValueError as exc:
        raise argparse.ArgumentTypeError(f"Invalid RGB color: {value}") from exc

    if rgb < 0 or rgb > 0xFFFFFF:
        raise argparse.ArgumentTypeError(f"RGB color out of range: {value}")

    return rgb


def encode_rgb(rgb: int) -> bytes:
    """Encode Dawn uint32 RGB value as little-endian 0x00RRGGBB."""
    return struct.pack("<I", rgb & 0x00FFFFFF)


def decode_rgb(data: bytes) -> int:
    if len(data) < 4:
        raise RuntimeError(f"RGB characteristic returned {len(data)} byte(s)")
    return struct.unpack("<I", data[:4])[0] & 0x00FFFFFF


def format_rgb(rgb: int) -> str:
    return f"#{rgb & 0x00FFFFFF:06x}"


def palette_color(index: int, steps: int) -> int:
    """Return one color from a full HSV hue wheel."""
    hue = (index % steps) / float(steps)
    red, green, blue = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
    return (round(red * 255) << 16) | (round(green * 255) << 8) | round(blue * 255)


async def scan_devices(timeout: float) -> list[object]:
    from bleak import BleakScanner

    print(f"Scanning for BLE devices ({timeout:.0f}s) ...", flush=True)
    devices = await BleakScanner.discover(timeout=timeout)
    devices.sort(key=lambda d: (d.name or "", d.address))

    for device in devices:
        name = device.name or "<no-name>"
        mark = "*" if GAP_NAME in name else " "
        print(f" {mark} {device.address}  {name}")

    return devices


async def resolve_device(identifier: str | None, timeout: float) -> str:
    from bleak import BleakScanner

    target = identifier or GAP_NAME
    print(f"Auto-detecting BLE device '{target}' ({timeout:.0f}s) ...", flush=True)

    devices = await BleakScanner.discover(timeout=timeout)
    for device in devices:
        name = device.name or ""
        if device.address == target or name == target:
            print(f"  Found {device.address} name={name!r}")
            return str(device.address)
        if identifier is None and GAP_NAME in name:
            print(f"  Found {device.address} name={name!r}")
            return str(device.address)

    if identifier:
        return identifier

    raise RuntimeError(
        f"BLE device '{GAP_NAME}' not found; pass --device with a BLE address "
        "or advertised name to connect explicitly"
    )


@asynccontextmanager
async def connect_device(address: str) -> AsyncIterator[object]:
    from bleak import BleakClient

    async with BleakClient(address, timeout=SCAN_TIMEOUT_S) as client:
        if not client.is_connected:
            raise RuntimeError(f"Failed to connect to {address}")

        yield client


async def get_rgb(address: str) -> int:
    async with connect_device(address) as client:
        data = await client.read_gatt_char(THINGY53_RGB_CHAR_UUID)
    return decode_rgb(data)


async def set_rgb(address: str, rgb: int) -> None:
    async with connect_device(address) as client:
        await client.write_gatt_char(
            THINGY53_RGB_CHAR_UUID,
            encode_rgb(rgb),
            response=True,
        )

    print(f"RGB set to {format_rgb(rgb)}")


async def fade_rgb(address: str, steps: int, period: float, cycles: int) -> None:
    if steps <= 0:
        raise ValueError("steps must be greater than zero")
    if period < 0:
        raise ValueError("period must be non-negative")
    if cycles < 0:
        raise ValueError("cycles must be zero or greater")

    total = None if cycles == 0 else steps * cycles
    label = "forever" if total is None else f"{total} update(s)"
    print(
        f"Fading RGB palette: steps={steps} period={period:g}s cycles={cycles} ({label})"
    )
    print("Press Ctrl-C to stop.")

    async with connect_device(address) as client:
        index = 0
        while total is None or index < total:
            rgb = palette_color(index, steps)
            await client.write_gatt_char(
                THINGY53_RGB_CHAR_UUID,
                encode_rgb(rgb),
                response=True,
            )
            print(f"  {format_rgb(rgb)}", end="\r", flush=True)
            index += 1
            if period > 0:
                await asyncio.sleep(period)

        await client.write_gatt_char(
            THINGY53_RGB_CHAR_UUID,
            encode_rgb(0x000000),
            response=True,
        )

    print("\nFade complete; RGB LED off.")


async def main_async(args: argparse.Namespace) -> None:
    if args.command == "scan":
        await scan_devices(args.timeout)
        return

    address = await resolve_device(args.device, args.timeout)

    if args.command == "rgb" and args.rgb_command == "get":
        rgb = await get_rgb(address)
        print(format_rgb(rgb))
        return

    if args.command == "rgb" and args.rgb_command == "set":
        await set_rgb(address, args.color)
        return

    if args.command == "rgb" and args.rgb_command == "fade":
        await fade_rgb(address, args.steps, args.period, args.cycles)
        return

    raise RuntimeError("Unsupported command")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Control the Dawn Thingy:53 NimBLE demo."
    )
    parser.add_argument(
        "--device",
        help=(
            "BLE address or advertised name. If omitted, auto-detect "
            f"'{GAP_NAME}'."
        ),
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=SCAN_TIMEOUT_S,
        help="BLE scan/connect timeout in seconds.",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)
    subparsers.add_parser("scan", help="List nearby BLE devices.")

    rgb_parser = subparsers.add_parser("rgb", help="Control RGB LED feature.")
    rgb_subparsers = rgb_parser.add_subparsers(dest="rgb_command", required=True)

    rgb_subparsers.add_parser("get", help="Read current RGB value.")

    set_parser = rgb_subparsers.add_parser("set", help="Set one RGB color.")
    set_parser.add_argument("color", type=parse_rgb, help="Color name or #RRGGBB.")

    fade_parser = rgb_subparsers.add_parser(
        "fade",
        help="Walk gradually around the RGB palette.",
    )
    fade_parser.add_argument(
        "--steps",
        type=int,
        default=64,
        help="Number of colors per palette cycle.",
    )
    fade_parser.add_argument(
        "--period",
        type=float,
        default=0.05,
        help="Delay between color updates in seconds.",
    )
    fade_parser.add_argument(
        "--cycles",
        type=int,
        default=1,
        help="Palette cycles to run; 0 means run until Ctrl-C.",
    )

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()

    try:
        asyncio.run(main_async(args))
    except KeyboardInterrupt:
        print("\nInterrupted.")
        return 130
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
