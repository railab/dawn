#!/usr/bin/env python3
"""Host helper for the Dawn Thingy:91 Wakaama/LwM2M demo with Leshan sandbox

The Thingy:91 demo registers with an Eclipse Leshan LwM2M server over LTE and
exposes its two RGB LEDs as custom objects: the **lightwell** LED (object
33002) and the **sense** LED (object 33003), each as a writable packed
0xRRGGBB colour on resource 0. This CLI drives them through the Leshan REST
API (the same server the device registers with), mirroring the Thingy:53
NimBLE demo CLI but over the network instead of BLE.

Uses only the Python standard library (urllib) - no extra packages.

The device must be registered with the Leshan server and reachable. Note that
on a single radio the modem shares time between LTE and GNSS, so a write may
occasionally time out (HTTP 504) while GNSS holds priority; just retry.

Examples:
  python3 tools/examples/thingy91_demo_cli.py rgb get
  python3 tools/examples/thingy91_demo_cli.py rgb set '#ff4000'
  python3 tools/examples/thingy91_demo_cli.py rgb set red --led sense
  python3 tools/examples/thingy91_demo_cli.py rgb fade --cycles 2
  python3 tools/examples/thingy91_demo_cli.py \\
      --server https://leshan.eclipseprojects.io --endpoint my-ep rgb set blue
"""

from __future__ import annotations

import argparse
import colorsys
import json
import ssl
import sys
import time
import urllib.error
import urllib.request

# Defaults match descriptors/examples/thingy91_wakaama_lte.yaml.
DEFAULT_SERVER = "https://23.97.187.154"
DEFAULT_ENDPOINT = "dawn-thingy91-leshan"
REQUEST_TIMEOUT_S = 20.0
# LTE-M is a constrained, lossy link: individual server->device requests
# occasionally time out (HTTP 504) or drop. Retry transparently so a single
# transient hiccup does not fail the whole operation.
REQUEST_RETRIES = 4
RETRY_BACKOFF_S = 0.5

# Custom RGB LED objects (resource 0 = packed 0xRRGGBB colour).
LED_OBJECTS = {
    "lightwell": 33002,
    "sense": 33003,
}

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


def format_rgb(rgb: int) -> str:
    return f"#{rgb & 0x00FFFFFF:06x}"


def palette_color(index: int, steps: int) -> int:
    """Return one color from a full HSV hue wheel."""
    hue = (index % steps) / float(steps)
    red, green, blue = colorsys.hsv_to_rgb(hue, 1.0, 1.0)
    return (round(red * 255) << 16) | (round(green * 255) << 8) | round(blue * 255)


def _ssl_context() -> ssl.SSLContext:
    """The Leshan sandbox uses a self-signed cert; don't verify it (like curl -k)."""
    ctx = ssl.create_default_context()
    ctx.check_hostname = False
    ctx.verify_mode = ssl.CERT_NONE
    return ctx


def _request(method: str, url: str, body: bytes | None) -> dict:
    headers = {"Accept": "application/json"}
    if body is not None:
        headers["Content-Type"] = "application/json"

    last_exc: RuntimeError | None = None
    for attempt in range(REQUEST_RETRIES):
        req = urllib.request.Request(
            url, data=body, headers=headers, method=method
        )
        try:
            with urllib.request.urlopen(
                req, timeout=REQUEST_TIMEOUT_S, context=_ssl_context()
            ) as resp:
                raw = resp.read().decode("utf-8")
            return json.loads(raw) if raw else {}
        except urllib.error.HTTPError as exc:
            # 504 = the device did not answer in time (asleep / GNSS holds the
            # radio / transient LTE-M loss). Transient -> retry. Other HTTP
            # errors are not transient.
            if exc.code != 504:
                raise RuntimeError(f"HTTP {exc.code} from {url}") from exc
            last_exc = RuntimeError(
                "device did not respond (HTTP 504) - asleep / GNSS holds the "
                "radio / transient LTE-M loss"
            )
        except urllib.error.URLError as exc:
            # Network/timeout reaching the server -> retry.
            last_exc = RuntimeError(
                f"cannot reach Leshan server {url}: {exc.reason}"
            )

        if attempt < REQUEST_RETRIES - 1:
            time.sleep(RETRY_BACKOFF_S * (attempt + 1))

    assert last_exc is not None
    raise last_exc


def _resource_url(server: str, endpoint: str, obj: int, res: int) -> str:
    return f"{server.rstrip('/')}/api/clients/{endpoint}/{obj}/0/{res}"


def leshan_read(server: str, endpoint: str, obj: int, res: int) -> int:
    result = _request("GET", _resource_url(server, endpoint, obj, res), None)
    if not result.get("success"):
        raise RuntimeError(f"read failed: {result.get('status', 'unknown error')}")
    content = result.get("content", {})
    value = content.get("value")
    # Custom objects have no model registered with Leshan, so it returns the
    # value as OPAQUE (a big-endian hex string, e.g. "00ff0000"); a known
    # INTEGER comes back as a number or decimal string.
    if isinstance(value, str):
        base = 16 if str(content.get("type", "")).upper() == "OPAQUE" else 10
        return int(value, base) & 0x00FFFFFF
    return int(value) & 0x00FFFFFF


def leshan_write(server: str, endpoint: str, obj: int, res: int, rgb: int) -> None:
    payload = {
        "kind": "singleResource",
        "id": res,
        "value": rgb & 0x00FFFFFF,
        "type": "INTEGER",
    }
    body = json.dumps(payload).encode("utf-8")
    result = _request("PUT", _resource_url(server, endpoint, obj, res), body)
    if not result.get("success"):
        raise RuntimeError(f"write failed: {result.get('status', 'unknown error')}")


def cmd_get(args: argparse.Namespace, obj: int) -> None:
    rgb = leshan_read(args.server, args.endpoint, obj, 0)
    print(format_rgb(rgb))


def cmd_set(args: argparse.Namespace, obj: int) -> None:
    leshan_write(args.server, args.endpoint, obj, 0, args.color)
    print(f"{args.led} LED set to {format_rgb(args.color)}")


def cmd_fade(args: argparse.Namespace, obj: int) -> None:
    if args.steps <= 0:
        raise ValueError("steps must be greater than zero")
    if args.period < 0:
        raise ValueError("period must be non-negative")
    if args.cycles < 0:
        raise ValueError("cycles must be zero or greater")

    total = None if args.cycles == 0 else args.steps * args.cycles
    label = "forever" if total is None else f"{total} update(s)"
    print(
        f"Fading {args.led} LED palette: steps={args.steps} period={args.period:g}s "
        f"cycles={args.cycles} ({label})"
    )
    print("Press Ctrl-C to stop.")

    index = 0
    skipped = 0
    try:
        while total is None or index < total:
            rgb = palette_color(index, args.steps)
            try:
                leshan_write(args.server, args.endpoint, obj, 0, rgb)
                print(f"  {format_rgb(rgb)}     ", end="\r", flush=True)
            except RuntimeError as exc:
                # A step still failed after retries - skip it and keep fading
                # rather than aborting the whole run on one LTE-M hiccup.
                skipped += 1
                print(f"  {format_rgb(rgb)} (skipped: {exc})", flush=True)
            index += 1
            if args.period > 0:
                time.sleep(args.period)
    finally:
        try:
            leshan_write(args.server, args.endpoint, obj, 0, 0x000000)
        except RuntimeError:
            pass  # best-effort LED-off on exit

    note = f" ({skipped} step(s) skipped)" if skipped else ""
    print(f"\nFade complete; RGB LED off.{note}")


def main_dispatch(args: argparse.Namespace) -> None:
    obj = LED_OBJECTS[args.led]
    if args.rgb_command == "get":
        cmd_get(args, obj)
    elif args.rgb_command == "set":
        cmd_set(args, obj)
    elif args.rgb_command == "fade":
        cmd_fade(args, obj)
    else:
        raise RuntimeError("Unsupported command")


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(
        description="Control the Dawn Thingy:91 RGB LEDs over the Leshan LwM2M API."
    )
    parser.add_argument(
        "--server",
        default=DEFAULT_SERVER,
        help=f"Leshan server base URL (default: {DEFAULT_SERVER}).",
    )
    parser.add_argument(
        "--endpoint",
        default=DEFAULT_ENDPOINT,
        help=f"LwM2M endpoint/registration name (default: {DEFAULT_ENDPOINT}).",
    )
    parser.add_argument(
        "--led",
        choices=sorted(LED_OBJECTS),
        default="lightwell",
        help="Which RGB LED to control (default: lightwell).",
    )

    subparsers = parser.add_subparsers(dest="command", required=True)
    rgb_parser = subparsers.add_parser("rgb", help="Control an RGB LED.")
    rgb_subparsers = rgb_parser.add_subparsers(dest="rgb_command", required=True)

    rgb_subparsers.add_parser("get", help="Read the current RGB value.")

    set_parser = rgb_subparsers.add_parser("set", help="Set one RGB color.")
    set_parser.add_argument("color", type=parse_rgb, help="Color name or #RRGGBB.")

    fade_parser = rgb_subparsers.add_parser(
        "fade", help="Walk gradually around the RGB palette."
    )
    fade_parser.add_argument(
        "--steps", type=int, default=64, help="Number of colors per palette cycle."
    )
    fade_parser.add_argument(
        "--period",
        type=float,
        default=0.2,
        help="Delay between color updates in seconds (network round-trips are "
        "slower than BLE).",
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
        main_dispatch(args)
    except KeyboardInterrupt:
        print("\nInterrupted.")
        return 130
    except Exception as exc:
        print(f"Error: {exc}", file=sys.stderr)
        return 1

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
