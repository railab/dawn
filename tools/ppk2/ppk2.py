#!/usr/bin/env python3
"""PPK2 all-in-one control tool (nxscope over CDC/ACM serial).

Single entry point for every PPK2 feature:

    info                     device state: ranges, pots, calibration
    on [VOLTS]               SMU output on at the given voltage (default 3.0)
    off                      SMU output off (loads off, rails down)
    volt VOLTS               change output voltage while on
    load {100k,10k,1k,100,off}   switch an onboard calibration load
    get NAME|all             read an IO (sw1..4, pots) via GET_IO
    set NAME VALUE           raw write to any writable IO
    plot [--raw] [--window S]    live rolling plot (amps when calibrated)
    measure [-t S]           mean/std current over S seconds
    csv FILE [-t S]          capture the stream to CSV
    monitor [-t S]           host-averaged current + on-device over-level trigger
    cal [...]                run the self-calibration sweep (see ppk2_cal)

Calibration data is read from ppk2-cal.json (see the `cal` subcommand);
without it, current is shown in raw ADC counts.
"""

import argparse
import collections
import json
import struct
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

import nxslib.intf.serial as sermod  # noqa: E402
from nxslib.comm import AckMode  # noqa: E402
from nxslib.nxscope import NxscopeHandler  # noqa: E402
from nxslib.proto.parse import Parser  # noqa: E402

from ppk2_cal import (  # noqa: E402
    CALEE_CHUNK,
    CALEE_MAGIC,
    CALEE_OFFSET,
    CALEE_SIZE,
    OBJ,
    VBB_V_TABLE,
    wiper_volts,
)

SET_IO = 8
GET_IO = 10
GET_IO_SEEK = 11

READABLE = ("sw1", "sw2", "sw3", "sw4", "vbb", "vldo", "iaoff")
WRITABLE = ("vext_en", "vldo_en", "ana_en", "reg_en", "vout_en",
            "cal100k", "cal10k", "cal1k", "cal100", "vbb", "vldo", "iaoff")


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
    return lo if abs(wiper_volts(lo) - volts) < abs(wiper_volts(hi) - volts) \
        else hi


class CalModel:
    def __init__(self, path):
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
        """Prefer the calibration blob stored in the board's EEPROM -
        it follows the hardware, not the host."""
        import zlib
        try:
            blob = dev.read_seek("calmem", CALEE_OFFSET, CALEE_SIZE)
        except (RuntimeError, KeyError):
            return
        if len(blob) < CALEE_SIZE:
            return
        magic, ver = struct.unpack_from("<II", blob, 0)
        crc = struct.unpack_from("<I", blob, CALEE_SIZE - 4)[0]
        if magic != CALEE_MAGIC or ver != 1:
            return
        if zlib.crc32(blob[:CALEE_SIZE - 4]) != crc:
            return
        vals = struct.unpack_from("<5f5f", blob, 8)
        self.gains = list(vals[:5])
        self.offsets = list(vals[5:])
        self.source = "device EEPROM"

    @property
    def valid(self):
        return self.gains is not None

    def amps(self, adc, rng):
        return self.gains[rng] * (adc - self.offsets[rng])


class Ppk2:
    def __init__(self, port):
        self.intf = sermod.SerialDevice(port)
        self.nxs = NxscopeHandler(self.intf, Parser())
        self._resp = []
        self._streaming = False

    def connect(self, stream=False, div=0):
        self.nxs.connect()
        self.nxs.add_user_frame_listener(self._on_user, [GET_IO])
        if stream:
            self.nxs.ch_enable([0])
            self.nxs.ch_divider([0], div)
            self.q = self.nxs.stream_sub(0)
            self.nxs.stream_start()
            self._streaming = True

    def channel_index(self, name):
        """Resolve an NxScope channel index by its name."""
        dev = self.nxs.dev
        for c in range(24):
            data = dev.channel_get(c).data
            if data.is_valid and data.name == name:
                return c
        raise RuntimeError(f"channel '{name}' not found")

    def stream_channels(self, names):
        """Enable and subscribe a set of named channels, returning a
        {name: queue} map. Must be called before any drain."""
        idx = {n: self.channel_index(n) for n in names}
        self.nxs.ch_enable(list(idx.values()))
        queues = {n: self.nxs.stream_sub(i) for n, i in idx.items()}
        self.nxs.stream_start()
        self._streaming = True
        return queues

    @staticmethod
    def drain_scalar(q):
        """Drain a scalar (dim-1) channel queue to a list of ints."""
        out = []
        while True:
            try:
                batch = q.get_nowait()
            except Exception:
                break
            for s in batch:
                d = s.data
                vals = d.tolist() if hasattr(d, "tolist") else [d]
                for v in vals:
                    out.append(int(v[0] if isinstance(v, (list, tuple))
                                   else v))
        return out

    def disconnect(self):
        if self._streaming:
            self.nxs.stream_stop()
        self.nxs.disconnect()

    def _on_user(self, frame):
        self._resp.append(bytes(getattr(frame, "data", frame)))
        return True

    def setio(self, name, value, fast=False):
        payload = struct.pack("<IH", OBJ[name], 4) + \
            int(value).to_bytes(4, "little", signed=int(value) < 0)
        if fast:
            # Fire-and-forget: skip the ACK round-trip, which costs tens of
            # ms under the 100 kS/s stream and would stall a live render loop.
            self.nxs.send_user_frame(SET_IO, payload,
                                     ack_mode=AckMode.DISABLED)
            return
        ack = self.nxs.send_user_frame(SET_IO, payload,
                                       ack_mode=AckMode.ENABLED)
        if not getattr(ack, "state", True):
            raise RuntimeError(f"set {name}={value} rejected by device")

    def getio(self, name):
        self._resp.clear()
        self.nxs.send_user_frame(GET_IO, struct.pack("<I", OBJ[name]),
                                 ack_mode=AckMode.ENABLED)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 1.0:
            for r in self._resp:
                objid, size = struct.unpack_from("<IH", r, 0)
                if objid == OBJ[name] and size == 4:
                    return struct.unpack_from("<I", r, 6)[0]
            time.sleep(0.01)
        raise RuntimeError(f"get {name}: no response")

    def read_range(self):
        return sum(self.getio(f"sw{i}") for i in (1, 2, 3, 4))

    def _read_chunk(self, name, offset, size):
        self._resp.clear()
        self.nxs.send_user_frame(
            GET_IO_SEEK, struct.pack("<IIH", OBJ[name], offset, size),
            ack_mode=AckMode.ENABLED)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 1.0:
            for r in self._resp:
                objid, rsize = struct.unpack_from("<IH", r, 0)
                if objid == OBJ[name]:
                    return r[6:6 + rsize]
            time.sleep(0.01)
        raise RuntimeError(f"read_seek {name}: no response")

    def read_seek(self, name, offset, size):
        out = b""
        while len(out) < size:
            n = min(CALEE_CHUNK, size - len(out))
            out += self._read_chunk(name, offset + len(out), n)
        return out

    # SMU sequences (correct order and polarity semantics: 1 = on/closed)

    def _vbb_headroom(self, volts):
        """Raise the VBB rail when the requested output needs headroom
        (the LDO input sits at ~4.4 V with the default wiper)."""
        need = volts + 0.4
        best = None
        for w, v in VBB_V_TABLE:
            if v >= need:
                best = w
                break
        self.setio("vbb", best if best is not None else 256)

    def smu_on(self, volts):
        self.setio("ana_en", 1)
        self.setio("reg_en", 1)
        self._vbb_headroom(volts)

        # Soft start: connect the output at the lowest voltage and ramp the
        # wiper up - closing the switch at full voltage produces an inrush
        # spike that latches the analog auto-range into the highest range.

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
        """Ampere meter: the DUT's external supply flows through the
        meter (VEXT path in, output switch to the DUT); the internal
        source stays disconnected. Connect the supply at low/zero
        voltage or expect the auto-range to latch high (run 'off' to
        release it)."""
        for n in ("cal100k", "cal10k", "cal1k", "cal100",
                  "vldo_en", "reg_en"):
            self.setio(n, 0)
        self.setio("ana_en", 1)
        self.setio("vext_en", 1)
        self.setio("vout_en", 1)

    def smu_off(self):
        for n in ("cal100k", "cal10k", "cal1k", "cal100",
                  "vout_en", "vldo_en", "vext_en", "reg_en", "ana_en"):
            self.setio(n, 0)
        self.setio("vbb", 128)

    def range_reset(self):
        """Release a latched-high auto-range: blink the analog rails -
        the comparators reset and re-acquire the correct range for the
        present current (~50 ms measurement gap)."""
        self.setio("ana_en", 0)
        time.sleep(0.05)
        self.setio("ana_en", 1)

    def set_load(self, which, fast=False):
        for n in ("cal100k", "cal10k", "cal1k", "cal100"):
            self.setio(n, 0, fast=fast)
        if which != "off":
            self.setio(f"cal{which}", 1, fast=fast)

    def drain(self):
        """Return pending stream samples as raw uint16 words.

        The device tags every sample: bits 0..11 = ADC counts, bits
        12..15 = range switch state (use split_sample). Queue items are
        lists of either per-sample tuples or decoded batch blocks.
        """
        out = []
        while True:
            try:
                batch = self.q.get_nowait()
            except Exception:
                break
            for s in batch:
                d = s.data
                if isinstance(d, tuple):
                    out.append(int(d[0]) & 0xFFFF)
                    continue
                vals = d.tolist() if hasattr(d, "tolist") else list(d)
                for v in vals:
                    v = v[0] if isinstance(v, (list, tuple)) else v
                    out.append(int(v) & 0xFFFF)
        return out



def split_sample(raw):
    """Split a tagged sample word into (adc counts, range index)."""
    return raw & 0xFFF, bin(raw >> 12).count("1")


def decimate(samples, rate):
    """Block-average raw samples down to the requested rate.

    The wire always carries the full 100 kS/s; averaging N = 100k/rate
    samples per point trades bandwidth for resolution (like the Nordic
    app). Returns (adc float, range) tuples; a block's range is the
    majority range and only samples from it enter the average.
    """
    n = max(1, round(100_000 / max(1.0, rate)))
    if n == 1:
        return [split_sample(s) for s in samples]
    out = []
    for i in range(0, len(samples) - n + 1, n):
        block = [split_sample(s) for s in samples[i:i + n]]
        counts = {}
        for _, r in block:
            counts[r] = counts.get(r, 0) + 1
        rng = max(counts, key=counts.get)
        adcs = [a for a, r in block if r == rng]
        out.append((sum(adcs) / len(adcs), rng))
    return out


def fmt_amps(a):
    for unit, mul in (("A", 1.0), ("mA", 1e3), ("uA", 1e6), ("nA", 1e9)):
        if abs(a) >= 1.0 / mul or unit == "nA":
            return f"{a * mul:8.3f} {unit}"
    return f"{a:.3e} A"


def cmd_info(dev, cal, args):
    rng = dev.read_range()
    print(f"range switches : sw1..4 = "
          f"{[dev.getio(f'sw{i}') for i in (1, 2, 3, 4)]}  (range {rng})")
    for p in ("vbb", "vldo", "iaoff"):
        w = dev.getio(p)
        extra = f"  (~{wiper_volts(w):.2f} V)" if p == "vldo" else ""
        print(f"pot {p:6s}    : wiper {w}{extra}")
    print(f"calibration    : "
          f"{'loaded from ' + cal.source if cal.valid else 'NOT FOUND (raw counts only) - run: ppk2.py cal'}")


def cmd_on(dev, cal, args):
    dev.smu_on(args.volts)
    print(f"output ON at ~{wiper_volts(volts_wiper(args.volts)):.2f} V "
          f"(wiper {volts_wiper(args.volts)})")


def cmd_off(dev, cal, args):
    dev.smu_off()
    print("output OFF, loads off, rails down")


def cmd_mode(dev, cal, args):
    if args.which == "ampere":
        dev.ampere_mode()
        print("ampere meter mode: external supply path enabled")
    else:
        dev.smu_off()
        print("modes disabled - use 'on VOLTS' for source meter")


def cmd_volt(dev, cal, args):
    dev._vbb_headroom(args.volts)
    dev.setio("vldo", volts_wiper(args.volts))
    print(f"VLDO -> ~{wiper_volts(volts_wiper(args.volts)):.2f} V")


def cmd_load(dev, cal, args):
    dev.set_load(args.which)
    print(f"load: {args.which}")


def cmd_get(dev, cal, args):
    names = READABLE if args.name == "all" else [args.name]
    for n in names:
        print(f"{n:8s} = {dev.getio(n)}")


def cmd_set(dev, cal, args):
    dev.setio(args.name, args.value)
    print(f"{args.name} = {args.value}")


def _capture(dev, seconds):
    dev.drain()
    t0 = time.monotonic()
    samples = []
    while time.monotonic() - t0 < seconds:
        time.sleep(0.05)
        samples.extend(dev.drain())
    return samples


def cmd_measure(dev, cal, args):
    samples = _capture(dev, args.time)
    if not samples:
        print("no samples received", file=sys.stderr)
        return 1
    groups = {}
    for adc, rng in decimate(samples, args.rate):
        groups.setdefault(rng, []).append(adc)
    print(f"samples: {len(samples)}")
    total = 0.0
    for rng in sorted(groups):
        g = groups[rng]
        mean = sum(g) / len(g)
        std = (sum((s - mean) ** 2 for s in g) / len(g)) ** 0.5
        line = (f"range {rng}: n={len(g):7d}  adc mean {mean:7.1f} "
                f"std {std:5.1f}")
        if cal.valid:
            amps = cal.amps(mean, rng)
            total += amps * len(g)
            line += f"  current {fmt_amps(amps)}"
        print(line)
    if cal.valid:
        print(f"weighted mean: {fmt_amps(total / len(samples))}")
    return 0


def cmd_csv(dev, cal, args):
    samples = _capture(dev, args.time)
    rows = decimate(samples, args.rate)
    with open(args.file, "w") as f:
        f.write("sample,adc,range" + (",amps" if cal.valid else "") + "\n")
        for i, (adc, rng) in enumerate(rows):
            row = f"{i},{adc:.2f},{rng}"
            if cal.valid:
                row += f",{cal.amps(adc, rng):.9e}"
            f.write(row + "\n")
    print(f"{len(rows)} rows at {args.rate:g} S/s -> {args.file}")


def _median_filter(ys, w):
    """Edge-preserving sliding-window median (numpy). Removes runs up to
    (w-1)/2 blocks wide while leaving step edges and sustained levels
    exactly in place."""
    import numpy as np

    if w < 3 or len(ys) < w:
        return ys
    h = w // 2
    a = np.asarray(ys, dtype=float)
    padded = np.concatenate([a[:h], a, a[-h:]])
    win = np.lib.stride_tricks.sliding_window_view(padded, w)
    return np.median(win, axis=1).tolist()


# The current stream is tagged with the auto-range that was live when each
# sample was taken, but the range status bits reach the firmware through a
# debounced GPIO, so after a fast multi-range transition the tag stays stale
# for up to ~15 ms while the ADC already reads the new shunt. Block-averaging
# turns that into a sustained, badly mis-scaled level (a 3 mA reading tagged
# as ~0.5 A). A median window wider than twice that lag rejects it; validated
# against captured hardware streams, 47 ms clears every residual spike.
_DEGLITCH_MS = 47.0
_BLOCK_MS = 1.5


class _Roller:
    """Streaming decimator for the live plots.

    Incoming samples are bucketed into fixed-size blocks at ABSOLUTE
    boundaries (a persistent pending buffer), so a given block is computed
    exactly once and never re-bucketed as the window scrolls - the earlier
    approach recomputed the whole window each frame with boundaries measured
    from the (moving) window start, which made historical points visibly
    change value as they scrolled left. Each finished block is immutable;
    only the newest ~half-median-window of points is still settling."""

    def __init__(self, cal, rate, use_amps, scale, span_s, head=25.0):
        self.cal = cal
        self.rate = rate
        self.use_amps = use_amps
        self.floor = 1e-6 if scale == "log" else 0.0
        self.head = head
        self.bs = max(1, int(rate * _BLOCK_MS * 1e-3))       # samples/block
        self.block_dt = self.bs / rate                        # seconds/block
        self.mw = max(3, int(round(_DEGLITCH_MS / _BLOCK_MS)) | 1)
        self.h = self.mw // 2
        span_blocks = int(span_s / self.block_dt)
        # raw holds forward-filled block values (immutable once appended);
        # disp holds their finalized medians, appended once each block has h
        # future neighbours so a displayed value can never change again.
        self.raw = collections.deque(maxlen=span_blocks + self.mw + 2)
        self.disp = collections.deque(maxlen=span_blocks)
        self.pending = []
        self.fill = self.floor
        self.total = 0        # raw blocks ever appended
        self.final = 0        # raw blocks finalized into disp
        self.range = 0

    def add(self, raws):
        self.pending.extend(raws)
        bs = self.bs
        while len(self.pending) >= bs:
            v = self._block(self.pending[:bs])
            del self.pending[:bs]
            if v is None:
                v = self.fill      # blink / mis-tag: hold the last good level
            else:
                self.fill = v
            self.raw.append(v)
            self.total += 1
        self._finalize()

    def _block(self, blk):
        pts = [split_sample(r) for r in blk]
        if self.use_amps:
            good = [(a, b) for a, b in pts if a - self.cal.offsets[b] >= self.head]
            if not good:
                return None
            ranges = [b for _, b in good]
            rr = max(set(ranges), key=ranges.count)
            self.range = rr
            adcs = [a for a, b in good if b == rr]
            return max(self.floor, self.cal.amps(sum(adcs) / len(adcs), rr))
        self.range = pts[len(pts) // 2][1]
        return sum(a for a, _ in pts) / len(pts)

    def _finalize(self):
        h = self.h
        if self.final >= self.total - h:
            return
        arr = list(self.raw)
        base = self.total - len(arr)      # absolute index of arr[0]
        while self.final < self.total - h:
            pos = self.final - base
            lo = max(0, pos - h)
            hi = pos + h + 1
            win = arr[lo:hi]
            self.disp.append(sorted(win)[len(win) // 2])
            self.final += 1

    def trace(self):
        """Return (xs, ys) scrolling left, x=0 at the newest finalised block.
        Every value in disp is immutable - it was median-finalised once and
        only scrolls; the live edge (newest h blocks) is simply not drawn yet
        (~23 ms lag), so historical points never change under the user."""
        ys = list(self.disp)
        if not ys:
            return None
        n = len(ys)
        xs = [(i - n + 1) * self.block_dt for i in range(n)]
        return xs, ys


def _pqg_setup(title, ylabel, yunit, window_s, scale):
    """Create the shared dark pyqtgraph plot window."""
    import pyqtgraph as pg

    pg.setConfigOptions(antialias=True, background="#0b0f14",
                        foreground="#c8d0d8")
    app = pg.mkQApp(title)
    pw = pg.PlotWidget()
    pw.resize(1040, 560)
    pw.setWindowTitle(title)
    pw.setLabel("bottom", "time", "s")
    pw.setLabel("left", ylabel, yunit)
    pw.showGrid(x=True, y=True, alpha=0.25)
    pw.setXRange(-window_s, 0)
    if scale == "log":
        pw.setLogMode(y=True)
    return app, pw


def cmd_plot(dev, cal, args):
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore

    use_amps = cal.valid and not args.raw
    scale = "log" if (use_amps and args.scale == "log") else "linear"
    rate = 100_000
    roller = _Roller(cal, rate, use_amps, scale, args.window)

    app, pw = _pqg_setup("PPK2 current",
                         "current" if use_amps else "ADC",
                         "A" if use_amps else "counts", args.window, scale)
    if scale != "log":
        pw.enableAutoRange(axis="y")
    curve = pw.plot(pen=pg.mkPen("#00e5a0", width=1.2))
    pw.show()

    def update():
        roller.add(dev.drain())
        res = roller.trace()
        if res is None:
            return
        xs, ys = res
        curve.setData(xs, ys)
        pw.setTitle(f"range {roller.range}   "
                    f"{fmt_amps(ys[-1]).strip() if use_amps else f'{ys[-1]:.0f} cnt'}",
                    color="#ffd166", size="11pt")

    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(50)
    print("plotting... close the window to exit")
    pg.exec()
    return 0


def cmd_demo(dev, cal, args):
    """Choreographed load sequence emulating an IoT device duty cycle,
    rendered as a calibrated live pyqtgraph plot. Uses only the onboard
    calibration loads - nothing external needed."""
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore

    if not cal.valid:
        print("calibration required for the demo - run: ppk2.py cal",
              file=sys.stderr)
        return 1

    # (phase name, load, duration s), looped
    script = [
        ("deep sleep", "100k", 1.8),
        ("wake up", "1k", 0.45),
        ("sensor read", "10k", 0.35),
        ("radio TX", "100", 0.14),
        ("radio TX", "100", 0.14),
        ("radio TX", "100", 0.14),
        ("processing", "1k", 0.5),
        ("deep sleep", "100k", 1.8),
    ]
    ohms = {"100k": 1e5, "10k": 1e4, "1k": 1e3, "100": 1e2}

    dev.smu_on(args.volts)
    dev.set_load(script[0][1])

    rate = 100_000
    roller = _Roller(cal, rate, True, args.scale, args.window)
    seq = {"i": 0, "t": time.monotonic(), "phase": script[0][0]}

    app, pw = _pqg_setup("PPK2 on Dawn - IoT profile demo", "current", "A",
                         args.window, args.scale)
    if args.scale == "log":
        pw.setYRange(-5.3, -1.0)      # 5 uA .. 0.1 A (log10 units)
    else:
        pw.setYRange(0, 0.04)
    curve = pw.plot(pen=pg.mkPen("#00e5a0", width=1.4))
    pw.show()

    def safe(fn, *a):
        try:
            fn(*a)
        except RuntimeError:
            pass

    def update():
        now = time.monotonic()
        name, load, dur = script[seq["i"]]
        if now - seq["t"] >= dur:
            prev = load
            seq["i"] = (seq["i"] + 1) % len(script)
            name, load, dur = script[seq["i"]]
            # All device writes are fire-and-forget (fast=True): the ACK
            # round-trip costs tens of ms under the 100 kS/s stream and would
            # stall the render loop, freezing and jumping the plot.
            safe(dev.set_load, load, True)
            if ohms[load] > ohms[prev]:
                # Stepping down in current: the analog auto-range latches
                # high, so blink the rails to re-acquire - drop ana_en now,
                # raise it 60 ms later off a timer, never blocking the loop.
                safe(dev.setio, "ana_en", 0, True)
                QtCore.QTimer.singleShot(60, lambda: safe(dev.setio,
                                                          "ana_en", 1, True))
            seq["t"] = now
            seq["phase"] = name

        roller.add(dev.drain())
        res = roller.trace()
        if res is None:
            return
        xs, ys = res
        curve.setData(xs, ys)
        pw.setTitle(f"{seq['phase']}   ·   range {roller.range}   ·   "
                    f"now {fmt_amps(ys[-1]).strip()}   ·   "
                    "100 kS/s self-calibrated, auto-range tagged per sample",
                    color="#ffd166", size="12pt")

    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(40)
    print("demo running - record the window; close it to stop", flush=True)
    try:
        pg.exec()
    finally:
        dev.set_load("off")
        dev.smu_off()
    return 0


def cmd_monitor(dev, cal, args):
    """Print an averaged current computed HOST-side from the full tagged
    stream - each sample is de-ranged and calibrated individually, so the
    average is true amps even across auto-range switches (a device raw-ADC
    average would be meaningless there). The over-level trigger stays on the
    device (thresholdvalue), carrying raw ADC that the live range scales."""
    queues = dev.stream_channels(["cur", "trig"])
    print("time   avg current    trigger (raw>thr)")
    t0 = time.monotonic()
    try:
        while time.monotonic() - t0 < args.time:
            time.sleep(args.interval)
            rng = dev.read_range()
            cur = dev.drain_scalar(queues["cur"])
            trig = dev.drain_scalar(queues["trig"])
            if not cur:
                continue
            # decimate for the readout, then per-sample de-range + cal
            sub = cur[:: max(1, len(cur) // 2000)]
            pts = [split_sample(s & 0xffff) for s in sub]
            if cal.valid:
                amps = [cal.amps(a, r) for a, r in pts]
                curstr = fmt_amps(sum(amps) / len(amps)).strip()
            else:
                curstr = f"{sum(a for a, _ in pts) / len(pts):.0f} cnt"
            fired = [x for x in trig if x != 0]
            if fired:
                tmax = fmt_amps(cal.amps(max(fired), rng)).strip() \
                    if cal.valid else f"{max(fired)} cnt"
                trg = f"{len(fired)} samples, peak {tmax}"
            else:
                trg = "-"
            print(f"{time.monotonic() - t0:5.1f}  {curstr:>12}   {trg}")
    except KeyboardInterrupt:
        pass
    return 0


def cmd_cal(dev, cal, args):
    raise SystemExit("run calibration with its own connection: "
                     "this is dispatched in main()")


def main():
    ap = argparse.ArgumentParser(
        prog="ppk2.py",
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="\n".join(__doc__.splitlines()[2:]))
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--cal-file", default="ppk2-cal.json")
    sub = ap.add_subparsers(dest="cmd", required=True)

    sub.add_parser("info")
    p = sub.add_parser("on")
    p.add_argument("volts", type=float, nargs="?", default=3.0)
    sub.add_parser("off")
    p = sub.add_parser("volt")
    p.add_argument("volts", type=float)
    p = sub.add_parser("load")
    p.add_argument("which", choices=["100k", "10k", "1k", "100", "off"])
    p = sub.add_parser("get")
    p.add_argument("name", choices=list(READABLE) + ["all"])
    p = sub.add_parser("set")
    p.add_argument("name", choices=WRITABLE)
    p.add_argument("value", type=int)
    p = sub.add_parser("mode")
    p.add_argument("which", choices=["ampere", "off"])
    p = sub.add_parser("measure")
    p.add_argument("-t", "--time", type=float, default=1.0)
    p.add_argument("--rate", type=float, default=100000)
    p = sub.add_parser("csv")
    p.add_argument("file")
    p.add_argument("-t", "--time", type=float, default=1.0)
    p.add_argument("--rate", type=float, default=100000)
    p = sub.add_parser("plot")
    p.add_argument("--raw", action="store_true",
                   help="plot raw ADC counts even when calibrated")
    p.add_argument("--window", type=float, default=2.0,
                   help="rolling window in seconds (default 2)")
    p.add_argument("--scale", choices=["log", "linear"], default="linear")
    p = sub.add_parser("monitor",
                       help="averaged current (host-side) + on-device over-level trigger")
    p.add_argument("-t", "--time", type=float, default=10.0)
    p.add_argument("--interval", type=float, default=0.5,
                   help="print interval in seconds (default 0.5)")
    p = sub.add_parser("demo")
    p.add_argument("volts", type=float, nargs="?", default=3.0)
    p.add_argument("--window", type=float, default=6.0)
    p.add_argument("--scale", choices=["log", "linear"], default="log")
    p = sub.add_parser("cal", add_help=False,
                       help="run self-calibration (extra args passed through)")
    p.add_argument("calargs", nargs=argparse.REMAINDER)

    args = ap.parse_args()

    if args.cmd == "cal":
        import ppk2_cal
        sys.argv = ["ppk2_cal.py", "--port", args.port,
                    "--out", args.cal_file] + args.calargs
        return ppk2_cal.main()

    handlers = {
        "info": cmd_info, "on": cmd_on, "off": cmd_off, "volt": cmd_volt,
        "mode": cmd_mode, "load": cmd_load, "get": cmd_get, "set": cmd_set,
        "measure": cmd_measure, "csv": cmd_csv, "plot": cmd_plot,
        "demo": cmd_demo, "monitor": cmd_monitor,
    }
    needs_stream = args.cmd in ("measure", "csv", "plot", "demo")

    cal = CalModel(args.cal_file)
    dev = Ppk2(args.port)
    dev.connect(stream=needs_stream)
    cal.load_device(dev)
    try:
        return handlers[args.cmd](dev, cal, args) or 0
    finally:
        dev.disconnect()


if __name__ == "__main__":
    sys.exit(main())
