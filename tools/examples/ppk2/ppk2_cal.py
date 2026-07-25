#!/usr/bin/env python3
"""PPK2 on-request self-calibration over nxscope (CDC/ACM serial).

Sweeps the four onboard calibration loads across VLDO setpoints, reads the
active range from the SW1..4 status inputs (GET_IO) and the current channel
from the stream, then fits a per-range linear model:

    I = G[r] * (adc - O[r])        r = 0..4 (0 = lowest current range)

The fit intercept gives the range offset, the slope the range gain, so no
range forcing is required. The sweep runs two phases: fine steps from zero
current (hysteresis keeps points in the low ranges) and the mid-range
sweep. Ranges 1..3 are fitted directly; ranges 0 and 4 are chained from
the nominal shunt ratios on purpose - the onboard loads span 5 uA..47 mA,
while range 0 lives below and range 4 (50 mA..1 A) above that, so a nominal
1122R/0.05R chain is more trustworthy than a corner fit. Range-top
compression points (flat adc at the comparator threshold) are dropped
automatically. Absolute scale comes from the embedded wiper->VLDO model
(anchored on the USB 5 V rail, a few % absolute); pass --anchor-volts with
a DMM reading of VDUT at wiper 224 to refine it.

The result is stored in the board EEPROM (source of truth for the tools)
and written to a JSON file together with per-point verification
residuals. Requires firmware with GET_IO support and sw1..4 bound to
nxscope (nxscope config), nothing connected to the DUT output.

Usage:
    ppk2_cal.py [--port /dev/ttyACM0] [--cal-file ppk2-cal.json]
                [--anchor-volts <DMM VDUT at wiper 224>]
                [--factory-backup <eeprom dump>]
"""

import argparse
import json
import struct
import sys
import time
import zlib
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from ppk2_dev import (  # noqa: E402
    ANCHOR_WIPER,
    CALEE_MAGIC,
    CALEE_OFFSET,
    CALEE_SIZE,
    LOADS,
    Ppk2,
    add_common_args,
    split_sample,
    wiper_volts,
)

# Nominal shunt chain (ohms) per range, highest resistance first. Used only
# to extrapolate gains for ranges the sweep cannot reach.

SHUNT_EFF = [1122.05, 122.05, 12.05, 1.05, 0.05]

# A calibration point must clear the resting ADC by at least this many
# counts to be usable; below it the current is unresolved in its range.

MIN_HEADROOM = 20.0

WIPERS = [30, 60, 90, 128, 170, 200, 230]
WIPERS_FINE = [16, 24, 34, 45, 60, 80, 104, 128, 160, 192, 230, 250]
SETTLE_S = 0.3
CAPTURE_S = 0.4


def capture(dev):
    """Return (adc mean, std, n, majority range) from tagged samples.

    Only samples tagged with the majority range enter the statistics,
    so range transitions inside the window cannot skew a point.
    """
    time.sleep(SETTLE_S)
    dev.drain()
    time.sleep(CAPTURE_S)
    snap = [split_sample(s) for s in dev.drain()]
    if len(snap) < 1000:
        raise RuntimeError(f"stream starved ({len(snap)} samples)")
    counts = {}
    for _, r in snap:
        counts[r] = counts.get(r, 0) + 1
    rng = max(counts, key=counts.get)
    adcs = [a for a, r in snap if r == rng]
    mean = sum(adcs) / len(adcs)
    var = sum((s - mean) ** 2 for s in adcs) / len(adcs)
    return mean, var**0.5, len(adcs), rng


def range_reset(dev):
    """Blink the analog rails so the auto-range re-acquires.

    Removes the hysteresis path dependence between calibration points.
    The rail-on write is retried in case the first ACK is dropped.
    """
    dev.setio("ana_en", 0)
    time.sleep(0.05)
    for _ in range(5):
        try:
            dev.setio("ana_en", 1)
            break
        except RuntimeError:
            time.sleep(0.1)
    else:
        raise RuntimeError("range_reset: ana_en did not come back up")
    time.sleep(0.1)


def cal_smu_on(dev):
    """Output on at the lowest sweep setpoint, default VBB headroom."""
    for n in ("cal100k", "cal10k", "cal1k", "cal100", "vout_en"):
        dev.setio(n, 0)
    dev.setio("ana_en", 1)
    dev.setio("reg_en", 1)
    dev.setio("vldo", WIPERS[0])
    time.sleep(0.3)
    dev.setio("vldo_en", 1)
    dev.setio("vout_en", 1)


def cal_smu_off(dev):
    """Best-effort output off."""
    for n in (
        "cal100k",
        "cal10k",
        "cal1k",
        "cal100",
        "vout_en",
        "vldo_en",
        "reg_en",
        "ana_en",
    ):
        try:
            dev.setio(n, 0)
        except Exception:
            pass


def store_eeprom(dev, gains, offsets):
    """Store the calibration blob in the EEPROM tail.

    Only writes if the target region is virgin (0xFF) or holds one of
    our blobs.
    """
    cur = dev.read_seek("calmem", CALEE_OFFSET, CALEE_SIZE)
    is_ours = struct.unpack_from("<I", cur, 0)[0] == CALEE_MAGIC
    if not is_ours and any(b != 0xFF for b in cur):
        raise RuntimeError(
            "EEPROM cal region is neither virgin nor ours - refusing "
            f"to write (offset {CALEE_OFFSET:#x})"
        )
    body = struct.pack("<II5f5f", CALEE_MAGIC, 1, *gains, *offsets)
    blob = body + struct.pack("<I", zlib.crc32(body))
    dev.write_seek("calmem", CALEE_OFFSET, blob)
    back = dev.read_seek("calmem", CALEE_OFFSET, len(blob))
    if back != blob:
        raise RuntimeError("EEPROM readback mismatch after write")
    return len(blob)


def fit_line(pts):
    """Least squares I = g*adc + b over (adc, i) pairs."""
    n = len(pts)
    sx = sum(a for a, _ in pts)
    sy = sum(i for _, i in pts)
    sxx = sum(a * a for a, _ in pts)
    sxy = sum(a * i for a, i in pts)
    d = n * sxx - sx * sx
    if abs(d) < 1e-12:
        return None
    g = (n * sxy - sx * sy) / d
    b = (sy - g * sx) / n
    return g, b


def sweep(dev, points, vscale, load, wipers, phase, reset=False):
    """Sweep one load over the wiper list, appending calibration points."""
    ohms = LOADS[load]
    dev.setio(load, 1)
    for w in wipers:
        dev.setio("vldo", w)
        # A mid-range load connects with an inrush spike that latches the
        # auto-range high; blink the rails so it re-acquires the natural
        # range. The low phase rises gradually from zero and never
        # latches, so a blink there would only inject its own inrush and
        # strand tiny currents in range 4.
        if reset:
            range_reset(dev)
        mean, std, n, r = capture(dev)
        amps = wiper_volts(w) * vscale / ohms
        points.append(
            {
                "load": load,
                "wiper": w,
                "amps": amps,
                "adc": mean,
                "std": std,
                "range": r,
                "phase": phase,
            }
        )
        print(
            f"{phase:5s} {load:8s} w={w:3d} I={amps * 1e3:9.4f} mA "
            f"adc={mean:7.1f} std={std:5.1f} range={r}"
        )
    dev.setio(load, 0)


def fit_model(points, zero):
    """Fit per-range gains/offsets, chaining unreachable ranges."""
    # Drop range-top compression points: inside one (load, phase) sweep
    # the adc must rise with current; a flat/declining tail sits at the
    # analog comparator threshold and would bias the fit.

    bykey = {}
    for p in points:
        bykey.setdefault((p["load"], p["phase"]), []).append(p)
    for pts in bykey.values():
        pts.sort(key=lambda q: q["amps"])
        for a, b in zip(pts, pts[1:]):
            if b["adc"] <= a["adc"] + 1.0 and b["range"] == a["range"]:
                b["saturated"] = True

    # Drop offset-dominated points: a current that latched into a range
    # too coarse to resolve it sits within a handful of counts of the
    # resting ADC and carries no information.

    resting = zero.get("adc", 0.0)
    for p in points:
        if p["adc"] - resting < MIN_HEADROOM:
            p["saturated"] = True

    gains = [None] * 5
    offsets = [None] * 5
    for r in range(5):
        pts = [
            (p["adc"], p["amps"])
            for p in points
            if p["range"] == r and not p.get("saturated")
        ]
        if len(pts) >= 2:
            res = fit_line(pts)
            if res:
                g, b = res
                gains[r] = g
                offsets[r] = -b / g if g else 0.0

    # Zero-current offset for the resting range beats the extrapolated one

    zr = zero.get("range")
    if zr is not None and gains[zr] is None:
        offsets[zr] = zero["adc"]

    # Chain unreachable ranges from nominal shunt ratios

    known = [r for r in range(5) if gains[r] is not None]
    if not known:
        return None, None
    for r in range(5):
        if gains[r] is None:
            ref = min(known, key=lambda k: abs(k - r))
            gains[r] = gains[ref] * SHUNT_EFF[ref] / SHUNT_EFF[r]
            if offsets[r] is None:
                offsets[r] = offsets[ref]
            print(f"range {r}: unreachable, gain chained from range {ref}")
    return gains, offsets


def main(argv=None):
    """Run the calibration sweep, store the result and print a report."""
    ap = argparse.ArgumentParser(
        description=__doc__.splitlines()[0],
        formatter_class=argparse.RawDescriptionHelpFormatter,
        epilog="\n".join(__doc__.splitlines()[2:]),
    )
    add_common_args(ap)
    ap.add_argument(
        "--anchor-volts",
        type=float,
        default=None,
        help=f"DMM-measured VDUT at wiper {ANCHOR_WIPER} (absolute anchor)",
    )
    ap.add_argument(
        "--factory-backup",
        default=None,
        help="EEPROM dump to verify the factory region (first 256 B)",
    )
    args = ap.parse_args(argv)

    vscale = 1.0
    if args.anchor_volts is not None:
        vscale = args.anchor_volts / wiper_volts(ANCHOR_WIPER)
        print(
            f"anchor: VDUT(w={ANCHOR_WIPER}) = {args.anchor_volts:.4f} V "
            f"-> voltage scale {vscale:.4f}"
        )

    dev = Ppk2(args.port, args.descriptor)
    dev.connect(stream=True)
    time.sleep(0.3)
    print("connected, output must be disconnected from any DUT")

    points = []
    zero = {}
    try:
        cal_smu_on(dev)

        # Zero-current point: auto-range rests in the lowest range.

        mean, std, n, r = capture(dev)
        zero = {"adc": mean, "std": std, "range": r}
        print(f"zero-current: adc={mean:7.1f} std={std:5.1f} range={r}")

        # Phase LOW: come up from zero current with fine steps so the
        # auto-range hysteresis keeps points in the low ranges as long as
        # possible (upward transitions only happen at range full-scale).

        dev.setio("vldo", WIPERS_FINE[0])
        time.sleep(0.3)
        sweep(dev, points, vscale, "cal100k", WIPERS_FINE, "low")
        sweep(dev, points, vscale, "cal10k", WIPERS_FINE, "low")

        # Phase MID: the verified mid-range sweep.

        dev.setio("vldo", WIPERS[0])
        time.sleep(0.3)
        sweep(dev, points, vscale, "cal1k", WIPERS, "mid", reset=True)
        sweep(dev, points, vscale, "cal100", WIPERS, "mid", reset=True)

    finally:
        cal_smu_off(dev)
        dev.disconnect()

    gains, offsets = fit_model(points, zero)
    if gains is None:
        print("ERROR: no range could be fitted", file=sys.stderr)
        return 1

    # Verification: apply the model to every measured point

    errs = []
    for p in points:
        if p.get("saturated"):
            p["err_pct"] = None
            continue
        r = p["range"]
        i_cal = gains[r] * (p["adc"] - offsets[r])
        err = 100.0 * (i_cal - p["amps"]) / p["amps"] if p["amps"] else 0.0
        p["err_pct"] = err
        errs.append(abs(err))

    result = {
        "model": "I = G[r] * (adc - O[r])",
        "gains_amps_per_count": gains,
        "offsets_counts": offsets,
        "anchor": (
            "dmm" if args.anchor_volts is not None else "nominal-usb-5v"
        ),
        "zero_current": zero,
        "points": points,
        "verify_max_err_pct": max(errs) if errs else None,
        "verify_avg_err_pct": (sum(errs) / len(errs)) if errs else None,
    }
    with open(args.cal_file, "w") as f:
        json.dump(result, f, indent=2)

    # Persist on the device and optionally verify the factory area

    dev = Ppk2(args.port, args.descriptor)
    dev.connect()
    try:
        n = store_eeprom(dev, gains, offsets)
        print(f"calibration stored in EEPROM at {CALEE_OFFSET:#x} ({n} bytes)")
        if args.factory_backup:
            ref = open(args.factory_backup, "rb").read(0x100)
            dev0 = dev.read_seek("calmem", 0, 0x100)
            if dev0 == ref:
                print("factory region intact")
            else:
                print("WARNING: factory region differs from backup!")
    finally:
        dev.disconnect()

    print("\nrange   gain [A/count]   offset [counts]")
    for r in range(5):
        print(f"  {r}     {gains[r]:.6e}   {offsets[r]:9.2f}")
    print(
        f"\nverification: avg |err| = {result['verify_avg_err_pct']:.2f}%"
        f", max |err| = {result['verify_max_err_pct']:.2f}%"
        f" (vs nominal model, anchor: {result['anchor']})"
    )
    print(f"calibration written to {args.cal_file}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
