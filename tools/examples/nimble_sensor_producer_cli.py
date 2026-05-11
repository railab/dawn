#!/usr/bin/env python3
"""Host helper for the Dawn NimBLE sensor producer demo.

Requires:
  pip install bleak

Examples:
  python3 tools/examples/nimble_sensor_producer_cli.py scan
  python3 tools/examples/nimble_sensor_producer_cli.py interactive
  python3 tools/examples/nimble_sensor_producer_cli.py write temp 23.5
  python3 tools/examples/nimble_sensor_producer_cli.py write humi 45.0
  python3 tools/examples/nimble_sensor_producer_cli.py write baro 1001.25 22.75
  python3 tools/examples/nimble_sensor_producer_cli.py write light 320.0 12.5
  python3 tools/examples/nimble_sensor_producer_cli.py write-all \
      --temp 23.5 --humi 45.0 --pressure 1001.25 --baro-temp 22.75 \
      --light 320.0 --ir 12.5
"""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys

GAP_NAME = "dawn-sensor-prod"
SCAN_TIMEOUT_S = 10.0

SENSORS = {
    "temp": {
        "uuid": "7f3d4a30-2b58-4bc2-9f31-3c0b2c4d0001",
        "fmt": "<f",
        "fields": ("temperature",),
    },
    "humi": {
        "uuid": "7f3d4a30-2b58-4bc2-9f31-3c0b2c4d0002",
        "fmt": "<f",
        "fields": ("humidity",),
    },
    "baro": {
        "uuid": "7f3d4a30-2b58-4bc2-9f31-3c0b2c4d0003",
        "fmt": "<ff",
        "fields": ("pressure", "temperature"),
    },
    "light": {
        "uuid": "7f3d4a30-2b58-4bc2-9f31-3c0b2c4d0004",
        "fmt": "<ff",
        "fields": ("light", "ir"),
    },
}


async def scan_devices(timeout: float) -> list[object]:
    from bleak import BleakScanner

    print(f"Scanning for BLE devices ({timeout:.0f}s) ...", flush=True)
    devices = await BleakScanner.discover(timeout=timeout)
    devices.sort(key=lambda d: (d.name or "", d.address))

    for device in devices:
        name = device.name or "<no-name>"
        print(f"  {device.address}  {name}")

    return devices


async def resolve_device(identifier: str | None, timeout: float) -> str:
    from bleak import BleakScanner

    target = identifier or GAP_NAME
    print(f"Scanning for '{target}' ({timeout:.0f}s) ...", flush=True)

    devices = await BleakScanner.discover(timeout=timeout)
    for device in devices:
        name = device.name or ""
        if device.address == target or name == target:
            print(f"  Found {device.address} name={name!r}")
            return device.address
        if identifier is None and GAP_NAME in name:
            print(f"  Found {device.address} name={name!r}")
            return device.address

    if identifier:
        return identifier

    raise RuntimeError(f"BLE device '{GAP_NAME}' not found")


def encode_sensor_payload(sensor: str, values: list[float]) -> bytes:
    spec = SENSORS[sensor]
    fields = spec["fields"]
    if len(values) != len(fields):
        names = ", ".join(fields)
        raise ValueError(f"{sensor} expects {len(fields)} value(s): {names}")
    return struct.pack(spec["fmt"], *values)


async def write_sensor(address: str, sensor: str, values: list[float]) -> None:
    from bleak import BleakClient

    spec = SENSORS[sensor]
    payload = encode_sensor_payload(sensor, values)

    async with BleakClient(address, timeout=SCAN_TIMEOUT_S) as client:
        if not client.is_connected:
            raise RuntimeError(f"Failed to connect to {address}")

        await client.write_gatt_char(spec["uuid"], payload, response=True)

    values_str = ", ".join(f"{value:g}" for value in values)
    print(f"Wrote {sensor}: {values_str}")


async def write_many(address: str, updates: list[tuple[str, list[float]]]) -> None:
    from bleak import BleakClient

    async with BleakClient(address, timeout=SCAN_TIMEOUT_S) as client:
        if not client.is_connected:
            raise RuntimeError(f"Failed to connect to {address}")

        for sensor, values in updates:
            spec = SENSORS[sensor]
            payload = encode_sensor_payload(sensor, values)
            await client.write_gatt_char(spec["uuid"], payload, response=True)
            values_str = ", ".join(f"{value:g}" for value in values)
            print(f"Wrote {sensor}: {values_str}")


INTERACTIVE_HELP = """\
Commands:
  temp <value>                       Write temperature
  humi <value>                       Write humidity
  baro <pressure> <temperature>      Write barometer sample
  light <light> <ir>                 Write light sample
  all <temp> <humi> <pressure> <baro_temp> <light> <ir>
                                    Write all demo sensor topics
  write <sensor> <values...>         Write one sensor, same as CLI write
  info                               Show GATT mapping
  help                               This message
  quit                               Disconnect and exit
"""


async def read_input(prompt: str) -> str:
    return await asyncio.get_event_loop().run_in_executor(
        None, lambda: input(prompt).strip()
    )


def parse_floats(values: list[str]) -> list[float]:
    return [float(value) for value in values]


async def write_connected(client: object, sensor: str, values: list[float]) -> None:
    spec = SENSORS[sensor]
    payload = encode_sensor_payload(sensor, values)
    await client.write_gatt_char(spec["uuid"], payload, response=True)
    values_str = ", ".join(f"{value:g}" for value in values)
    print(f"  Wrote {sensor}: {values_str}")


async def interactive(address: str) -> None:
    from bleak import BleakClient

    async with BleakClient(address, timeout=SCAN_TIMEOUT_S) as client:
        if not client.is_connected:
            raise RuntimeError(f"Failed to connect to {address}")

        print("  Type 'help' for commands.\n")

        while True:
            try:
                line = await read_input("sensor> ")
            except EOFError:
                break
            except KeyboardInterrupt:
                print()
                break

            if not line:
                continue

            parts = line.split()
            cmd = parts[0].lower()

            try:
                if cmd in ("quit", "exit", "q"):
                    break
                if cmd == "help":
                    print(INTERACTIVE_HELP)
                elif cmd == "info":
                    print(f"  Device: {address}")
                    for sensor, spec in SENSORS.items():
                        fields = ", ".join(spec["fields"])
                        print(f"  {sensor:5} {spec['uuid']} ({fields})")
                elif cmd == "all":
                    values = parse_floats(parts[1:])
                    if len(values) != 6:
                        print(
                            "  Usage: all <temp> <humi> <pressure> <baro_temp> <light> <ir>"
                        )
                        continue
                    updates = [
                        ("temp", [values[0]]),
                        ("humi", [values[1]]),
                        ("baro", [values[2], values[3]]),
                        ("light", [values[4], values[5]]),
                    ]
                    for sensor, sensor_values in updates:
                        await write_connected(client, sensor, sensor_values)
                elif cmd == "write":
                    if len(parts) < 3:
                        print("  Usage: write <sensor> <values...>")
                        continue
                    sensor = parts[1]
                    if sensor not in SENSORS:
                        print(f"  Unknown sensor: {sensor}")
                        continue
                    await write_connected(client, sensor, parse_floats(parts[2:]))
                elif cmd in SENSORS:
                    await write_connected(client, cmd, parse_floats(parts[1:]))
                else:
                    print(f"  Unknown command: {cmd}. Type 'help'.")
            except Exception as exc:  # pragma: no cover - interactive shell
                print(f"  Error: {exc}", file=sys.stderr)

        print("\n  Disconnecting...")


async def main_async(args: argparse.Namespace) -> None:
    if args.command == "scan":
        await scan_devices(args.timeout)
        return

    address = await resolve_device(args.device, args.timeout)

    if args.command == "write":
        await write_sensor(address, args.sensor, args.values)
        return

    if args.command == "interactive":
        await interactive(address)
        return

    updates = [
        ("temp", [args.temp]),
        ("humi", [args.humi]),
        ("baro", [args.pressure, args.baro_temp]),
        ("light", [args.light, args.ir]),
    ]
    await write_many(address, updates)


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Update Dawn sensor_producer IOs over NimBLE."
    )
    parser.add_argument(
        "--device",
        help=(
            "BLE address or exact advertised name. If omitted, scan for "
            f"{GAP_NAME!r}."
        ),
    )
    parser.add_argument(
        "--timeout",
        type=float,
        default=SCAN_TIMEOUT_S,
        help="BLE scan/connect timeout in seconds.",
    )

    subparsers = parser.add_subparsers(dest="command")
    subparsers.add_parser("scan", help="List nearby BLE devices.")
    subparsers.add_parser("interactive", help="Open an interactive BLE shell.")

    write = subparsers.add_parser("write", help="Write one sensor value.")
    write.add_argument("sensor", choices=sorted(SENSORS))
    write.add_argument("values", nargs="+", type=float)

    write_all = subparsers.add_parser("write-all", help="Write all demo sensor topics.")
    write_all.add_argument("--temp", type=float, required=True)
    write_all.add_argument("--humi", type=float, required=True)
    write_all.add_argument("--pressure", type=float, required=True)
    write_all.add_argument("--baro-temp", type=float, required=True)
    write_all.add_argument("--light", type=float, required=True)
    write_all.add_argument("--ir", type=float, required=True)

    return parser


def main() -> int:
    parser = build_parser()
    args = parser.parse_args()
    if args.command is None:
        args.command = "interactive"

    try:
        asyncio.run(main_async(args))
    except Exception as exc:
        print(f"error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
