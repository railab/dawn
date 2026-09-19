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
    mode {ampere,off}        ampere meter mode (external supply)
    plot [--raw] [--window S]    live rolling plot (amps when calibrated)
    measure [-t S]           mean/std current over S seconds
    csv FILE [-t S]          capture the stream to CSV
    monitor [-t S]           host-averaged current + on-device trigger
    demo [VOLTS]             IoT duty-cycle demo on the onboard loads
    cal [...]                run the self-calibration sweep (see ppk2_cal)

Calibration is read from the board EEPROM (source of truth), with the
JSON written by `cal` as a fallback; without either, current is shown in
raw ADC counts. Object IDs come from the board descriptor YAML.
"""

import argparse
import collections
import sys
import time
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ppk2_dev import (  # noqa: E402
    READABLE,
    WRITABLE,
    add_common_args,
    open_device,
    split_sample,
    volts_wiper,
    wiper_volts,
)


def decimate(samples, rate, wire_rate):
    """Block-average raw samples down to the requested rate.

    The wire always carries the full wire_rate; averaging N = wire/rate
    samples per point trades bandwidth for resolution (like the Nordic
    app). Returns (adc float, range) tuples; a block's range is the
    majority range and only samples from it enter the average.
    """
    n = max(1, round(wire_rate / max(1.0, rate)))
    if n == 1:
        return [split_sample(s) for s in samples]
    out = []
    for i in range(0, len(samples) - n + 1, n):
        block = [split_sample(s) for s in samples[i : i + n]]
        counts = {}
        for _, r in block:
            counts[r] = counts.get(r, 0) + 1
        rng = max(counts, key=counts.get)
        adcs = [a for a, r in block if r == rng]
        out.append((sum(adcs) / len(adcs), rng))
    return out


def fmt_amps(a):
    """Format amps with an auto-scaled unit."""
    for unit, mul in (("A", 1.0), ("mA", 1e3), ("uA", 1e6), ("nA", 1e9)):
        if abs(a) >= 1.0 / mul or unit == "nA":
            return f"{a * mul:8.3f} {unit}"
    return f"{a:.3e} A"


def cmd_info(dev, cal, args):
    rng = dev.read_range()
    sw = [dev.getio(f"sw{i}") for i in (1, 2, 3, 4)]
    print(f"range switches : sw1..4 = {sw}  (range {rng})")
    for p in ("vbb", "vldo", "iaoff"):
        w = dev.getio(p)
        extra = f"  (~{wiper_volts(w):.2f} V)" if p == "vldo" else ""
        print(f"pot {p:6s}    : wiper {w}{extra}")
    if cal.valid:
        print(f"calibration    : loaded from {cal.source}")
    else:
        print("calibration    : NOT FOUND (raw counts) - run: ppk2.py cal")


def cmd_on(dev, cal, args):
    dev.smu_on(args.volts)
    w = volts_wiper(args.volts)
    print(f"output ON at ~{wiper_volts(w):.2f} V (wiper {w})")


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
    w = volts_wiper(args.volts)
    dev.setio("vldo", w)
    print(f"VLDO -> ~{wiper_volts(w):.2f} V")


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
    rows = decimate(samples, args.decimate or dev.rate, dev.rate)
    for adc, rng in rows:
        groups.setdefault(rng, []).append(adc)
    print(f"samples: {len(samples)} at {dev.rate:g} S/s")
    total = 0.0
    for rng in sorted(groups):
        g = groups[rng]
        mean = sum(g) / len(g)
        std = (sum((s - mean) ** 2 for s in g) / len(g)) ** 0.5
        line = (
            f"range {rng}: n={len(g):7d}  adc mean {mean:7.1f} "
            f"std {std:5.1f}"
        )
        if cal.valid:
            amps = cal.amps(mean, rng)
            total += amps * len(g)
            line += f"  current {fmt_amps(amps)}"
        print(line)
    if cal.valid:
        print(f"weighted mean: {fmt_amps(total / len(rows))}")
    return 0


def cmd_csv(dev, cal, args):
    samples = _capture(dev, args.time)
    out_rate = args.decimate or dev.rate
    rows = decimate(samples, out_rate, dev.rate)
    with open(args.file, "w") as f:
        f.write("sample,adc,range" + (",amps" if cal.valid else "") + "\n")
        for i, (adc, rng) in enumerate(rows):
            row = f"{i},{adc:.2f},{rng}"
            if cal.valid:
                row += f",{cal.amps(adc, rng):.9e}"
            f.write(row + "\n")
    print(f"{len(rows)} rows at {out_rate:g} S/s -> {args.file}")


# The current stream is tagged with the auto-range that was live when each
# sample was taken. Captured streams show that after a fast multi-range
# transition the tag can disagree with the shunt the ADC already reads for
# up to ~15 ms (the analog front end settles before the range status pins
# do). Block-averaging turns that into a sustained, badly mis-scaled level
# (a 3 mA reading tagged as ~0.5 A). A median window wider than twice that
# lag rejects it; validated on captured streams, 47 ms clears every spike.
_DEGLITCH_MS = 47.0
_BLOCK_MS = 1.5


class _Roller:
    """Streaming decimator for the live plots.

    Incoming samples are bucketed into fixed-size blocks at ABSOLUTE
    boundaries (a persistent pending buffer), so a given block is computed
    exactly once and never re-bucketed as the window scrolls. Each finished
    block is immutable; only the newest ~half-median-window of points is
    still settling.
    """

    def __init__(self, cal, rate, use_amps, scale, span_s, head=25.0):
        self.cal = cal
        self.rate = rate
        self.use_amps = use_amps
        self.floor = 1e-6 if scale == "log" else 0.0
        self.head = head
        self.bs = max(1, int(rate * _BLOCK_MS * 1e-3))  # samples/block
        self.block_dt = self.bs / rate  # seconds/block
        self.mw = max(3, int(round(_DEGLITCH_MS / _BLOCK_MS)) | 1)
        self.h = self.mw // 2
        span_blocks = int(span_s / self.block_dt)
        # raw holds forward-filled block values (immutable once appended);
        # disp holds their finalized medians, appended once each block has
        # h future neighbours so a displayed value can never change again.
        self.raw = collections.deque(maxlen=span_blocks + self.mw + 2)
        self.disp = collections.deque(maxlen=span_blocks)
        self.pending = []
        self.fill = self.floor
        self.total = 0  # raw blocks ever appended
        self.final = 0  # raw blocks finalized into disp
        self.range = 0

    def add(self, raws):
        self.pending.extend(raws)
        bs = self.bs
        while len(self.pending) >= bs:
            v = self._block(self.pending[:bs])
            del self.pending[:bs]
            if v is None:
                v = self.fill  # blink / mis-tag: hold the last good level
            else:
                self.fill = v
            self.raw.append(v)
            self.total += 1
        self._finalize()

    def _block(self, blk):
        pts = [split_sample(r) for r in blk]
        if self.use_amps:
            good = [
                (a, b) for a, b in pts if a - self.cal.offsets[b] >= self.head
            ]
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
        base = self.total - len(arr)  # absolute index of arr[0]
        # A burst larger than raw's capacity evicts blocks that were never
        # finalized; they are gone, so resume from the oldest one held.
        self.final = max(self.final, base)
        while self.final < self.total - h:
            pos = self.final - base
            lo = max(0, pos - h)
            hi = pos + h + 1
            win = arr[lo:hi]
            self.disp.append(sorted(win)[len(win) // 2])
            self.final += 1

    def trace(self):
        """Return (xs, ys) scrolling left, x=0 at the newest final block."""
        ys = list(self.disp)
        if not ys:
            return None
        n = len(ys)
        xs = [(i - n + 1) * self.block_dt for i in range(n)]
        return xs, ys


def _pqg_setup(title, ylabel, yunit, window_s, scale):
    """Create the shared dark pyqtgraph plot window."""
    import pyqtgraph as pg

    pg.setConfigOptions(
        antialias=True, background="#0b0f14", foreground="#c8d0d8"
    )
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
    roller = _Roller(cal, dev.rate, use_amps, scale, args.window)

    app, pw = _pqg_setup(
        "PPK2 current",
        "current" if use_amps else "ADC",
        "A" if use_amps else "counts",
        args.window,
        scale,
    )
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
        if use_amps:
            now = fmt_amps(ys[-1]).strip()
        else:
            now = f"{ys[-1]:.0f} cnt"
        pw.setTitle(f"range {roller.range}   {now}", color="#ffd166")

    timer = QtCore.QTimer()
    timer.timeout.connect(update)
    timer.start(50)
    print("plotting... close the window to exit")
    pg.exec()
    return 0


def cmd_demo(dev, cal, args):
    """Choreographed load sequence emulating an IoT device duty cycle.

    Rendered as a calibrated live pyqtgraph plot; uses only the onboard
    calibration loads - nothing external needed.
    """
    import pyqtgraph as pg
    from pyqtgraph.Qt import QtCore

    if not cal.valid:
        print("calibration required for the demo - run: ppk2.py cal",
              file=sys.stderr)  # fmt: skip
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

    roller = _Roller(cal, dev.rate, True, args.scale, args.window)
    seq = {"i": 0, "t": time.monotonic(), "phase": script[0][0]}

    app, pw = _pqg_setup(
        "PPK2 on Dawn - IoT profile demo",
        "current",
        "A",
        args.window,
        args.scale,
    )
    if args.scale == "log":
        pw.setYRange(-5.3, -1.0)  # 5 uA .. 0.1 A (log10 units)
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
            # round-trip costs tens of ms under the 100 kS/s stream and
            # would stall the render loop, freezing and jumping the plot.
            safe(dev.set_load, load, True)
            if ohms[load] > ohms[prev]:
                # Stepping down in current: the analog auto-range latches
                # high, so blink the rails to re-acquire - drop ana_en now,
                # raise it 60 ms later off a timer, never blocking the loop.
                safe(dev.setio, "ana_en", 0, True)
                QtCore.QTimer.singleShot(
                    60, lambda: safe(dev.setio, "ana_en", 1, True)
                )
            seq["t"] = now
            seq["phase"] = name

        roller.add(dev.drain())
        res = roller.trace()
        if res is None:
            return
        xs, ys = res
        curve.setData(xs, ys)
        pw.setTitle(
            f"{seq['phase']}   ·   range {roller.range}   ·   "
            f"now {fmt_amps(ys[-1]).strip()}   ·   "
            f"{dev.rate / 1e3:g} kS/s self-calibrated, auto-range tagged",
            color="#ffd166",
            size="12pt",
        )

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
    """Averaged current computed HOST-side from the full tagged stream.

    Each sample is de-ranged and calibrated individually, so the average
    is true amps even across auto-range switches. The over-level trigger
    stays on the device (thresholdvalue), carrying raw ADC that the live
    range scales.
    """
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
            pts = [split_sample(s & 0xFFFF) for s in sub]
            if cal.valid:
                amps = [cal.amps(a, r) for a, r in pts]
                curstr = fmt_amps(sum(amps) / len(amps)).strip()
            else:
                curstr = f"{sum(a for a, _ in pts) / len(pts):.0f} cnt"
            fired = [x for x in trig if x != 0]
            if fired:
                if cal.valid:
                    tmax = fmt_amps(cal.amps(max(fired), rng)).strip()
                else:
                    tmax = f"{max(fired)} cnt"
                trg = f"{len(fired)} samples, peak {tmax}"
            else:
                trg = "-"
            print(f"{time.monotonic() - t0:5.1f}  {curstr:>12}   {trg}")
    except KeyboardInterrupt:
        pass
    return 0


def main():
    ap = argparse.ArgumentParser(
        prog="ppk2.py",
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="\n".join(__doc__.splitlines()[2:]),
    )
    add_common_args(ap)
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
    for name in ("measure", "csv"):
        p = sub.add_parser(name)
        if name == "csv":
            p.add_argument("file")
        p.add_argument("-t", "--time", type=float, default=1.0)
        p.add_argument(
            "--decimate",
            type=float,
            default=None,
            help="output rate in S/s (default: the wire rate)",
        )
    p = sub.add_parser("plot")
    p.add_argument(
        "--raw",
        action="store_true",
        help="plot raw ADC counts even when calibrated",
    )
    p.add_argument(
        "--window",
        type=float,
        default=2.0,
        help="rolling window in seconds (default 2)",
    )
    p.add_argument("--scale", choices=["log", "linear"], default="linear")
    p = sub.add_parser(
        "monitor",
        help="averaged current (host-side) + on-device over-level trigger",
    )
    p.add_argument("-t", "--time", type=float, default=10.0)
    p.add_argument(
        "--interval",
        type=float,
        default=0.5,
        help="print interval in seconds (default 0.5)",
    )
    p = sub.add_parser("demo")
    p.add_argument("volts", type=float, nargs="?", default=3.0)
    p.add_argument("--window", type=float, default=6.0)
    p.add_argument("--scale", choices=["log", "linear"], default="log")
    p = sub.add_parser(
        "cal",
        add_help=False,
        help="run self-calibration (extra args passed through)",
    )
    p.add_argument("calargs", nargs=argparse.REMAINDER)

    args = ap.parse_args()

    if args.cmd == "cal":
        import ppk2_cal

        argv = ["--port", args.port, "--cal-file", str(args.cal_file)]
        argv += ["--descriptor", str(args.descriptor)]
        return ppk2_cal.main(argv + args.calargs)

    handlers = {
        "info": cmd_info,
        "on": cmd_on,
        "off": cmd_off,
        "volt": cmd_volt,
        "mode": cmd_mode,
        "load": cmd_load,
        "get": cmd_get,
        "set": cmd_set,
        "measure": cmd_measure,
        "csv": cmd_csv,
        "plot": cmd_plot,
        "demo": cmd_demo,
        "monitor": cmd_monitor,
    }
    needs_stream = args.cmd in ("measure", "csv", "plot", "demo")

    dev, cal = open_device(args, stream=needs_stream)
    try:
        return handlers[args.cmd](dev, cal, args) or 0
    finally:
        dev.disconnect()


if __name__ == "__main__":
    sys.exit(main())
