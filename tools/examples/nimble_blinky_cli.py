#!/usr/bin/env python3
"""
NimBLE Blinky Demo CLI

Connects to a Dawn NimBLE blinky device and controls the LED over BLE.

Usage:
  python3 tools/examples/nimble_blinky_cli.py start         # Start blinking
  python3 tools/examples/nimble_blinky_cli.py stop          # Stop blinking
  python3 tools/examples/nimble_blinky_cli.py status        # Show current state
  python3 tools/examples/nimble_blinky_cli.py period <us>   # Set dwell (same for on/off)
  python3 tools/examples/nimble_blinky_cli.py period <off_us> <on_us>
  python3 tools/examples/nimble_blinky_cli.py toggle        # Toggle start/stop
  python3 tools/examples/nimble_blinky_cli.py interactive   # Interactive shell
  python3 tools/examples/nimble_blinky_cli.py scan          # List BLE devices nearby

Requires: pip install bleak
"""

from __future__ import annotations

import argparse
import asyncio
import struct
import sys


SVC_UUID    = "12345678-1234-5678-1234-56789abcdef0"
CTRL_UUID   = "12345678-1234-5678-1234-56789abcdef1"
STATE_UUID  = "12345678-1234-5678-1234-56789abcdef2"
DWELLOFF_UUID = "12345678-1234-5678-1234-56789abcdef3"
DWELLON_UUID  = "12345678-1234-5678-1234-56789abcdef4"

GAP_NAME   = "dawn-blink"

CMD_STOP  = 0x00
CMD_START = 0x01


async def find_device(timeout: float = 10) -> str | None:
    """Scan for the Dawn NimBLE blinky device."""
    from bleak import BleakScanner

    print(f"Scanning for '{GAP_NAME}' ({timeout:.0f}s) ...", flush=True)

    devices = await BleakScanner.discover(timeout=timeout)
    for d in devices:
        name = d.name or ""
        if GAP_NAME in name or "dawnblink" in name:
            print(f"  Found: {d.address}  name={name!r}")
            return d.address

    print("  Device not found. Try 'scan' to list all nearby devices.",
          file=sys.stderr)
    return None


async def scan_nearby():
    """List all nearby BLE devices."""
    from bleak import BleakScanner

    print("Scanning (10s) ...", flush=True)
    devices = await BleakScanner.discover(timeout=10)
    devices.sort(key=lambda d: (d.name or "", d.address))
    print(f"\nFound {len(devices)} device(s):\n")
    for d in devices:
        rssi = f"RSSI={d.rssi}" if hasattr(d, 'rssi') and d.rssi is not None else ""
        print(f"  {d.address}  {(d.name or '<no-name>'):30s}  {rssi}")


async def connect(address: str):
    """Connect to the device and return (client, ctrl_char, state_char,
       dwell_off_char, dwell_on_char)."""
    from bleak import BleakClient

    print(f"Connecting to {address} ...")
    client = BleakClient(address)
    await client.connect()

    svc = client.services.get_service(SVC_UUID)
    if svc is None:
        await client.disconnect()
        raise RuntimeError(f"Custom service {SVC_UUID} not found")

    ctrl_char = svc.get_characteristic(CTRL_UUID)
    state_char = svc.get_characteristic(STATE_UUID)
    dwell_off_char = svc.get_characteristic(DWELLOFF_UUID)
    dwell_on_char = svc.get_characteristic(DWELLON_UUID)

    if ctrl_char is None:
        await client.disconnect()
        raise RuntimeError(f"Control characteristic {CTRL_UUID} not found")
    if state_char is None:
        await client.disconnect()
        raise RuntimeError(f"States characteristic {STATE_UUID} not found")

    print("  Connected.")
    return client, ctrl_char, state_char, dwell_off_char, dwell_on_char


def decode_state(data: bytes) -> dict:
    """Decode states table: [val0, dwell0, val1, dwell1] all uint32."""
    if len(data) < 16:
        return {"raw": data.hex()}
    vals = struct.unpack("<4I", data[:16])
    return {
        "state0_value":  vals[0],
        "state0_dwell_us": vals[1],
        "state1_value":  vals[2],
        "state1_dwell_us": vals[3],
    }


def encode_state(off_us: int, on_us: int) -> bytes:
    """Encode states table with val0=0/dwell0=off_us, val1=1/dwell1=on_us."""
    return struct.pack("<4I", 0, off_us, 1, on_us)


def fmt_dwell(us: int) -> str:
    if us >= 1_000_000:
        return f"{us/1_000_000:.1f}s"
    if us >= 1000:
        return f"{us/1000:.0f}ms"
    return f"{us}us"


# ---------------------------------------------------------------------------
# Commands
# ---------------------------------------------------------------------------


async def cmd_status(address: str):
    client, ctrl_char, _, dwell_off_char, dwell_on_char = None, None, None, None, None
    try:
        client, ctrl_char, _, dwell_off_char, dwell_on_char = await connect(address)
        ctrl_data = await client.read_gatt_char(ctrl_char)
        off_data = await client.read_gatt_char(dwell_off_char) if dwell_off_char else None
        on_data = await client.read_gatt_char(dwell_on_char) if dwell_on_char else None

        cmd = ctrl_data[0] if ctrl_data else 0xFF
        dwell_off = struct.unpack("<I", off_data)[0] if off_data and len(off_data) >= 4 else 0
        dwell_on = struct.unpack("<I", on_data)[0] if on_data and len(on_data) >= 4 else 0

        print()
        print(f"  Running:  {'yes' if cmd == CMD_START else 'no'} (cmd=0x{cmd:02x})")
        print(f"  Dwell off: {fmt_dwell(dwell_off)}")
        print(f"  Dwell on:  {fmt_dwell(dwell_on)}")
    finally:
        if client:
            await client.disconnect()


async def cmd_start(address: str):
    client, ctrl_char = None, None
    try:
        client, ctrl_char, _, _, _ = await connect(address)
        await client.write_gatt_char(ctrl_char, bytes([CMD_START]), response=True)
        print("  Blinky started.")
    finally:
        if client:
            await client.disconnect()


async def cmd_stop(address: str):
    client, ctrl_char = None, None
    try:
        client, ctrl_char, _, _, _ = await connect(address)
        await client.write_gatt_char(ctrl_char, bytes([CMD_STOP]), response=True)
        print("  Blinky stopped.")
    finally:
        if client:
            await client.disconnect()


async def cmd_toggle(address: str):
    client, ctrl_char = None, None
    try:
        client, ctrl_char, _, _, _ = await connect(address)
        data = await client.read_gatt_char(ctrl_char)
        current = data[0] if data else CMD_STOP
        new_cmd = CMD_STOP if current == CMD_START else CMD_START
        await client.write_gatt_char(ctrl_char, bytes([new_cmd]), response=True)
        print(f"  {'Started' if new_cmd == CMD_START else 'Stopped'}.")
    finally:
        if client:
            await client.disconnect()


async def cmd_period(address: str, off_us: int, on_us: int):
    client, dwell_off_char, dwell_on_char = None, None, None
    try:
        client, _, _, dwell_off_char, dwell_on_char = await connect(address)
        if dwell_off_char:
            await client.write_gatt_char(dwell_off_char, struct.pack("<I", off_us),
                                         response=True)
        if dwell_on_char:
            await client.write_gatt_char(dwell_on_char, struct.pack("<I", on_us),
                                         response=True)
        print(f"  Period set: off={fmt_dwell(off_us)} on={fmt_dwell(on_us)}")
    finally:
        if client:
            await client.disconnect()


async def cmd_show(address: str):
    client, dwell_off_char, dwell_on_char = None, None, None
    try:
        client, _, _, dwell_off_char, dwell_on_char = await connect(address)
        off_data = await client.read_gatt_char(dwell_off_char) if dwell_off_char else None
        on_data = await client.read_gatt_char(dwell_on_char) if dwell_on_char else None
        dwell_off = struct.unpack("<I", off_data)[0] if off_data and len(off_data) >= 4 else 0
        dwell_on = struct.unpack("<I", on_data)[0] if on_data and len(on_data) >= 4 else 0
        print()
        print(f"  Dwell off: {fmt_dwell(dwell_off)}")
        print(f"  Dwell on:  {fmt_dwell(dwell_on)}")
    finally:
        if client:
            await client.disconnect()


# ---------------------------------------------------------------------------
# Interactive
# ---------------------------------------------------------------------------

_HELP = """\
Commands:
  start         Start the blinky
  stop          Stop the blinky
  toggle        Toggle start/stop
  status        Show running state + dwell table
  show          Show dwell table
  period <us>   Set symmetric dwell (e.g. period 250000)
  period <off> <on>  Set asymmetric (e.g. period 100000 500000)
  info          Show GATT info
  help          This message
  quit          Disconnect and exit
"""


async def interactive(address: str):
    from bleak import BleakClient

    client, ctrl_char, state_char, dwell_off_char, dwell_on_char = \
        await connect(address)

    try:
        print("  Type 'help' for commands.\n")

        while True:
            try:
                line = await asyncio.get_event_loop().run_in_executor(
                    None, lambda: input("blinky> ").strip()
                )
            except EOFError:
                break
            if not line:
                continue

            parts = line.split()
            cmd = parts[0].lower()

            try:
                if cmd in ("quit", "exit", "q"):
                    break
                elif cmd == "help":
                    print(_HELP)
                elif cmd == "start":
                    await client.write_gatt_char(ctrl_char, bytes([CMD_START]), response=True)
                    print("  Started.")
                elif cmd == "stop":
                    await client.write_gatt_char(ctrl_char, bytes([CMD_STOP]), response=True)
                    print("  Stopped.")
                elif cmd == "toggle":
                    data = await client.read_gatt_char(ctrl_char)
                    cur = data[0] if data else CMD_STOP
                    nxt = CMD_STOP if cur == CMD_START else CMD_START
                    await client.write_gatt_char(ctrl_char, bytes([nxt]), response=True)
                    print(f"  {'Started' if nxt == CMD_START else 'Stopped'}.")
                elif cmd == "status":
                    cd = await client.read_gatt_char(ctrl_char)
                    running = cd[0] == CMD_START if cd else False
                    off = struct.unpack("<I", await client.read_gatt_char(dwell_off_char))[0] if dwell_off_char else 0
                    on_ = struct.unpack("<I", await client.read_gatt_char(dwell_on_char))[0] if dwell_on_char else 0
                    print(f"  Running:  {running}")
                    print(f"  Dwell off: {fmt_dwell(off)}")
                    print(f"  Dwell on:  {fmt_dwell(on_)}")
                elif cmd == "show":
                    off = struct.unpack("<I", await client.read_gatt_char(dwell_off_char))[0] if dwell_off_char else 0
                    on_ = struct.unpack("<I", await client.read_gatt_char(dwell_on_char))[0] if dwell_on_char else 0
                    print(f"  Dwell off: {fmt_dwell(off)}")
                    print(f"  Dwell on:  {fmt_dwell(on_)}")
                elif cmd == "period":
                    if len(parts) == 2:
                        d = int(parts[1])
                        if dwell_off_char:
                            await client.write_gatt_char(dwell_off_char, struct.pack("<I", d), response=True)
                        if dwell_on_char:
                            await client.write_gatt_char(dwell_on_char, struct.pack("<I", d), response=True)
                        print(f"  Symmetric dwell: {fmt_dwell(d)}")
                    elif len(parts) >= 3:
                        off = int(parts[1])
                        on_ = int(parts[2])
                        if dwell_off_char:
                            await client.write_gatt_char(dwell_off_char, struct.pack("<I", off), response=True)
                        if dwell_on_char:
                            await client.write_gatt_char(dwell_on_char, struct.pack("<I", on_), response=True)
                        print(f"  dwell: off={fmt_dwell(off)} on={fmt_dwell(on_)}")
                    else:
                        print("  Usage: period <us>  |  period <off_us> <on_us>")
                elif cmd == "info":
                    print(f"  Service:        {SVC_UUID}")
                    print(f"  Control char:   {CTRL_UUID}  (1 byte: 0=stop 1=start)")
                    print(f"  States char:    {STATE_UUID}  (full states table)")
                    print(f"  Dwell-off char: {DWELLOFF_UUID}  (uint32, us)")
                    print(f"  Dwell-on char:  {DWELLON_UUID}  (uint32, us)")
                    print(f"  Device:         {address}")
                else:
                    print(f"  Unknown command: {cmd}. Type 'help'.")
            except Exception as e:
                print(f"  Error: {e}", file=sys.stderr)
    finally:
        print("\n  Disconnecting...")
        await client.disconnect()


# ---------------------------------------------------------------------------
# Main
# ---------------------------------------------------------------------------

async def main():
    parser = argparse.ArgumentParser(
        description="NimBLE Blinky Demo CLI",
    )
    parser.add_argument("command", nargs="?",
                        choices=["start", "stop", "toggle", "status",
                                 "period", "show", "interactive", "scan"],
                        default="interactive")
    parser.add_argument("args", nargs="*", type=str)
    parser.add_argument("--address", "-a", type=str, default=None,
                        help="BLE MAC address (auto-scan if omitted)")
    args_v = parser.parse_args()

    if args_v.command == "scan":
        await scan_nearby()
        return

    address = args_v.address
    if address is None:
        address = await find_device()
        if address is None:
            sys.exit(1)

    try:
        if args_v.command == "start":
            await cmd_start(address)
        elif args_v.command == "stop":
            await cmd_stop(address)
        elif args_v.command == "toggle":
            await cmd_toggle(address)
        elif args_v.command == "status":
            await cmd_status(address)
        elif args_v.command == "show":
            await cmd_show(address)
        elif args_v.command == "period":
            if len(args_v.args) == 0:
                print("Usage: period <us> | period <off_us> <on_us>")
                sys.exit(1)
            off = int(args_v.args[0])
            on_ = int(args_v.args[1]) if len(args_v.args) > 1 else off
            await cmd_period(address, off, on_)
        elif args_v.command == "interactive":
            await interactive(address)
    except KeyboardInterrupt:
        print()
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    asyncio.run(main())
