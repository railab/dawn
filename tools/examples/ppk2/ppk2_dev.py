#!/usr/bin/env python3
"""Shared PPK2 device class: Dawn nxscope over CDC/ACM serial.

Object IDs are resolved by nxscope channel name from the board descriptor
(dawnpy desc-graph), IO access goes through the dawn-nxscope plugin
(SET_IO/GET_IO user frames) and the current stream through nxslib
stream_sub.
"""

import json
import struct
import sys
import time
import zlib
from pathlib import Path

from dawn_nxscope.plugin import DawnNxscopePlugin
from dawnpy.descriptor.reports.graph import load_descriptor_graph
from nxslib.comm import AckMode
from nxslib.intf.serial import SerialDevice
from nxslib.nxscope import NxscopeHandler
from nxslib.proto.parse import Parser

HERE = Path(__file__).resolve().parent
DESCRIPTOR = HERE.parents[2] / "descriptors/examples/ppk2_nxscope.yaml"
CAL_FILE = HERE / "ppk2-cal.json"

READABLE = ("sw1", "sw2", "sw3", "sw4", "vbb", "vldo", "iaoff")
WRITABLE = (
    "vext_en",
    "vldo_en",
    "ana_en",
    "reg_en",
    "vout_en",
    "cal100k",
    "cal10k",
    "cal1k",
    "cal100",
    "vbb",
    "vldo",
    "iaoff",
)

# Self-calibration blob in the 24CW160 EEPROM tail, far away from the
# Nordic factory data (bytes 0x000..0x0FC). Layout: magic, version,
# 5 gains f32, 5 offsets f32, crc32.

CALEE_OFFSET = 0x7C0
CALEE_MAGIC = 0x434E5744  # "DWNC"
CALEE_SIZE = 52
CALEE_CHUNK = 32  # fits the device cmd RX buffer per transfer

# Onboard calibration loads (0.1% parts on the PPK2)

LOADS = {"cal100k": 100e3, "cal10k": 10e3, "cal1k": 1e3, "cal100": 100.0}

# Wiper -> VLDO volts model, measured on the rails ADC (adc_fetch)
# (VIN = USB 5.00 V assumed for the counts->volts anchor). The pot is
# nonlinear; below wiper 24 the output is unstable and above ~224 VLDO
# saturates at the default VBB rail (raise VBB for outputs above 4.3 V).

WIPER_V_TABLE = [
    (24, 0.852), (32, 1.049), (40, 1.239), (48, 1.393), (56, 1.583),
    (64, 1.699), (72, 1.854), (80, 2.026), (88, 2.129), (96, 2.310),
    (104, 2.460), (112, 2.623), (120, 2.714), (128, 2.869), (136, 2.989),
    (144, 3.122), (152, 3.269), (160, 3.415), (168, 3.570), (176, 3.690),
    (184, 3.836), (192, 3.952), (200, 4.094), (208, 4.236), (216, 4.408),
    (224, 4.434),
]  # fmt: skip

# VBB (LDO input rail) wiper map - needed as headroom for VLDO > 4.2 V.

VBB_V_TABLE = [
    (0, 2.714), (16, 2.976), (32, 3.195), (48, 3.441), (64, 3.643),
    (80, 3.828), (96, 4.021), (112, 4.245), (128, 4.413), (144, 4.610),
    (160, 4.821), (176, 4.993), (192, 5.161), (208, 5.342), (224, 5.557),
    (240, 5.733), (256, 5.944),
]  # fmt: skip

ANCHOR_WIPER = 224


def wiper_volts(w):
    """Interpolate the wiper->VLDO volts table."""
    t = WIPER_V_TABLE
    if w <= t[0][0]:
        lo, hi = t[0], t[1]
    elif w >= t[-1][0]:
        lo, hi = t[-2], t[-1]
    else:
        lo, hi = next((a, b) for a, b in zip(t, t[1:]) if a[0] <= w <= b[0])
    return lo[1] + (hi[1] - lo[1]) * (w - lo[0]) / (hi[0] - lo[0])


def volts_wiper(volts):
    """Inverse of the wiper->volts model (clamped)."""
    lo, hi = 0, 256
    if volts <= wiper_volts(lo):
        return lo
    if volts >= wiper_volts(hi):
        return hi
    while hi - lo > 1:
        mid = (lo + hi) // 2
        if wiper_volts(mid) < volts:
            lo = mid
        else:
            hi = mid
    if abs(wiper_volts(lo) - volts) < abs(wiper_volts(hi) - volts):
        return lo
    return hi


def split_sample(raw):
    """Split a tagged sample word into (adc counts, range index)."""
    return raw & 0xFFF, bin(raw >> 12).count("1")


def load_objids(descriptor=DESCRIPTOR, kconfig=None):
    """Map nxscope channel names to object IDs from the descriptor."""
    doc = load_descriptor_graph(str(descriptor), kconfig_path=kconfig)
    for graph in doc.descriptors:
        ids = {n.id: n.objid for n in graph.nodes}
        for node in graph.nodes:
            if node.kind == "protocol" and node.type.startswith("nxscope"):
                binds = node.config.get("iobind2", [])
                return {b["name"]: ids[b["id"]] for b in binds}
    raise RuntimeError(f"no nxscope protocol in {descriptor}")


class CalModel:
    """Per-range linear model I = G[r] * (adc - O[r]).

    The EEPROM blob is the source of truth; the JSON file (written by the
    calibration run) is only a fallback for boards without one.
    """

    def __init__(self, path=CAL_FILE):
        """Load the JSON fallback if present."""
        self.path = path
        self.source = None
        self.gains = None
        self.offsets = None
        try:
            with open(path) as f:
                d = json.load(f)
            self.gains = d["gains_amps_per_count"]
            self.offsets = d["offsets_counts"]
            self.source = f"json ({path})"
        except (OSError, KeyError, json.JSONDecodeError):
            pass

    def load_device(self, dev):
        """Load the calibration blob stored in the board's EEPROM."""
        try:
            blob = dev.read_seek("calmem", CALEE_OFFSET, CALEE_SIZE)
        except (RuntimeError, KeyError, TimeoutError):
            return
        if len(blob) < CALEE_SIZE:
            return
        magic, ver = struct.unpack_from("<II", blob, 0)
        crc = struct.unpack_from("<I", blob, CALEE_SIZE - 4)[0]
        if magic != CALEE_MAGIC or ver != 1:
            return
        if zlib.crc32(blob[: CALEE_SIZE - 4]) != crc:
            return
        vals = struct.unpack_from("<5f5f", blob, 8)
        self.gains = list(vals[:5])
        self.offsets = list(vals[5:])
        self.source = "device EEPROM"

    @property
    def valid(self):
        """True when a gain/offset set is loaded."""
        return self.gains is not None

    def amps(self, adc, rng):
        """Convert raw ADC counts in the given range to amps."""
        return self.gains[rng] * (adc - self.offsets[rng])


class Ppk2:
    """PPK2 over nxscope: IO access via dawn-nxscope, stream via nxslib."""

    def __init__(self, port, descriptor=DESCRIPTOR, kconfig=None):
        """Resolve object IDs and open the serial transport."""
        self.obj = load_objids(descriptor, kconfig)
        self.intf = SerialDevice(port)
        self.nxs = NxscopeHandler(self.intf, Parser())
        self.ext = DawnNxscopePlugin()
        self.queues = {}
        self.q = None
        self.rate = None

    def connect(self, stream=False):
        """Connect; with stream=True also subscribe the current channel."""
        self.nxs.connect()
        self.nxs.register_plugin(self.ext, [self.ext.extension_ids.get_io])
        if stream:
            self.q = self.stream_channels(["cur"])["cur"]

    def disconnect(self):
        """Stop the stream (if any) and disconnect."""
        if self.queues:
            self.nxs.stream_stop()
        self.nxs.unregister_plugin(self.ext.name)
        self.nxs.disconnect()

    def channel_index(self, name):
        """Resolve an NxScope channel index by its name."""
        dev = self.nxs.dev
        for c in range(dev.data.chmax):
            data = dev.channel_get(c).data
            if data.is_valid and data.name == name:
                return c
        raise RuntimeError(f"channel '{name}' not found")

    def stream_channels(self, names):
        """Enable and subscribe named channels, returning {name: queue}."""
        idx = {n: self.channel_index(n) for n in names}
        self.nxs.ch_enable(list(idx.values()))
        self.nxs.ch_divider(list(idx.values()), 0)
        queues = {n: self.nxs.stream_sub(i) for n, i in idx.items()}
        self.nxs.stream_start()
        self.queues.update(queues)
        return queues

    def measure_rate(self, seconds=0.5):
        """Measure the wire sample rate of the current stream (S/s)."""
        self.drain()
        t0 = time.monotonic()
        time.sleep(seconds)
        n = len(self.drain())
        self.rate = round(n / (time.monotonic() - t0), -3)
        return self.rate

    @staticmethod
    def _unpack(q, mask=None):
        out = []
        while True:
            try:
                batch = q.get_nowait()
            except Exception:
                break
            for s in batch:
                d = s.data
                if isinstance(d, tuple):
                    vals = [d[0]]
                else:
                    vals = d.tolist() if hasattr(d, "tolist") else list(d)
                for v in vals:
                    v = v[0] if isinstance(v, (list, tuple)) else v
                    out.append(int(v) & mask if mask else int(v))
        return out

    def drain(self):
        """Return pending current samples as raw uint16 words.

        The device tags every sample: bits 0..11 = ADC counts, bits
        12..15 = range switch state (use split_sample).
        """
        return self._unpack(self.q, 0xFFFF)

    @staticmethod
    def drain_scalar(q):
        """Drain a scalar (dim-1) channel queue to a list of ints."""
        return Ppk2._unpack(q)

    # IO access

    def setio(self, name, value, fast=False):
        """Write a 32-bit IO.

        fast=True skips the ACK round-trip, which costs tens of ms under
        the 100 kS/s stream.
        """
        value = int(value)
        data = value.to_bytes(4, "little", signed=value < 0)
        mode = AckMode.DISABLED if fast else AckMode.ENABLED
        ack = self.ext.set_io(self.obj[name], data, ack_mode=mode)
        if not fast and not ack.state:
            raise RuntimeError(f"set {name}={value} rejected by device")

    def getio(self, name):
        """Read a 32-bit IO via GET_IO."""
        data = self.ext.get_io(self.obj[name], ack_mode=AckMode.ENABLED)
        return int.from_bytes(data[:4], "little")

    def read_range(self):
        """Active auto-range index from the sw1..4 status inputs."""
        return sum(self.getio(f"sw{i}") for i in (1, 2, 3, 4))

    def read_seek(self, name, offset, size):
        """Read a seekable IO in CALEE_CHUNK pieces."""
        out = b""
        while len(out) < size:
            n = min(CALEE_CHUNK, size - len(out))
            out += self.ext.get_io_seek(
                self.obj[name], offset + len(out), n, ack_mode=AckMode.ENABLED
            )
        return out

    def write_seek(self, name, offset, data):
        """Write a seekable IO in CALEE_CHUNK pieces."""
        for pos in range(0, len(data), CALEE_CHUNK):
            chunk = data[pos : pos + CALEE_CHUNK]
            ack = self.ext.set_io_seek(
                self.obj[name], offset + pos, chunk, ack_mode=AckMode.ENABLED
            )
            if not ack.state:
                raise RuntimeError(
                    f"write_seek {name} rejected (ret {ack.retcode})"
                )

    # SMU sequences (1 = on/closed)

    def _vbb_headroom(self, volts):
        """Raise the VBB rail when the requested output needs headroom.

        The LDO input sits at ~4.4 V with the default wiper.
        """
        need = volts + 0.4
        best = next((w for w, v in VBB_V_TABLE if v >= need), 256)
        self.setio("vbb", best)

    def smu_on(self, volts):
        """Source meter on: soft-start the output at the given voltage."""
        self.setio("ana_en", 1)
        self.setio("reg_en", 1)
        self._vbb_headroom(volts)

        # Connect the output at the lowest voltage and ramp the wiper up -
        # closing the switch at full voltage produces an inrush spike that
        # latches the analog auto-range into the highest range.

        target = volts_wiper(volts)
        self.setio("vldo", 30)
        time.sleep(0.2)
        self.setio("vldo_en", 1)
        self.setio("vout_en", 1)
        time.sleep(0.1)
        w = 30
        while w < target:
            w = min(w + 25, target)
            self.setio("vldo", w)
            time.sleep(0.05)
        if target < 30:
            self.setio("vldo", target)

    def ampere_mode(self):
        """Ampere meter mode.

        The DUT's external supply flows through the meter (VEXT in, output
        switch to the DUT), internal source disconnected. Connect the
        supply at low/zero voltage or expect the auto-range to latch high
        (run 'off' to release it).
        """
        for n in ("cal100k", "cal10k", "cal1k", "cal100", "vldo_en", "reg_en"):
            self.setio(n, 0)
        self.setio("ana_en", 1)
        self.setio("vext_en", 1)
        self.setio("vout_en", 1)

    def smu_off(self):
        """Output off, loads off, rails down."""
        for n in (
            "cal100k",
            "cal10k",
            "cal1k",
            "cal100",
            "vout_en",
            "vldo_en",
            "vext_en",
            "reg_en",
            "ana_en",
        ):
            self.setio(n, 0)
        self.setio("vbb", 128)

    def range_reset(self):
        """Release a latched-high auto-range.

        Blinks the analog rails so the comparators re-acquire the range
        (~50 ms measurement gap).
        """
        self.setio("ana_en", 0)
        time.sleep(0.05)
        self.setio("ana_en", 1)

    def set_load(self, which, fast=False):
        """Switch one onboard calibration load in ('off' = none)."""
        for n in ("cal100k", "cal10k", "cal1k", "cal100"):
            self.setio(n, 0, fast=fast)
        if which != "off":
            self.setio(f"cal{which}", 1, fast=fast)


def open_device(args, stream=False):
    """Connect a Ppk2 from parsed CLI args and load the calibration."""
    dev = Ppk2(args.port, args.descriptor)
    dev.connect(stream=stream)
    cal = CalModel(args.cal_file)
    cal.load_device(dev)
    if stream:
        dev.rate = args.rate or dev.measure_rate()
    return dev, cal


def add_common_args(ap):
    """Add the connection/calibration options shared by the tools."""
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument(
        "--descriptor",
        default=DESCRIPTOR,
        help="board descriptor YAML used to resolve object IDs",
    )
    ap.add_argument(
        "--cal-file",
        default=CAL_FILE,
        help="calibration JSON fallback (EEPROM blob takes precedence)",
    )
    ap.add_argument(
        "--rate",
        type=float,
        default=None,
        help="wire sample rate in S/s (default: measured from the stream)",
    )


if __name__ == "__main__":
    for name, objid in load_objids().items():
        print(f"{name:12s} 0x{objid:08x}")
    sys.exit(0)
