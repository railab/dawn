#!/usr/bin/env python3
"""PPK2 on-request self-calibration over nxscope (CDC/ACM serial).

Sweeps the four onboard calibration loads across VLDO setpoints, reads the
active range from the SW1..4 status inputs (GET_IO) and the current channel
from the 100 kS/s stream, then fits a per-range linear model:

    I = G[r] * (adc - O[r])        r = 0..4 (0 = lowest current range)

The fit intercept gives the range offset, the slope the range gain, so no
range forcing is required. The sweep runs three phases: fine steps from zero
current (hysteresis keeps points in the low ranges), the mid-range sweep,
and a high-range attempt. Ranges 1..3 are fitted directly; ranges 0 and 4
are chained from the nominal shunt ratios on purpose - the onboard loads
span 5 uA..47 mA, while range 0 lives below and range 4 (50 mA..1 A) above
that, so a nominal 1122R/0.05R chain is more trustworthy than a corner fit.
Range-top compression points (flat adc at the comparator threshold) are
dropped automatically. Absolute scale comes from the embedded
wiper->VLDO model (anchored on the USB 5 V rail, a few % absolute); pass
--anchor-volts with a DMM reading of VDUT at wiper 230 to refine it.

Result is written to a JSON cal file (default ppk2-cal.json) together with
per-point verification residuals. Requires: firmware with GET_IO support and
sw1..4 bound to nxscope (nxscope_fast config), nothing connected to the DUT
output during calibration.

Usage:
    python tools/ppk2/ppk2_cal.py [--port /dev/ttyACM0] [--out ppk2-cal.json]
                                  [--anchor-volts <DMM VDUT at wiper 230>]
"""

import argparse
import json
import struct
import sys
import time

import nxslib.intf.serial as sermod
from nxslib.comm import AckMode
from nxslib.nxscope import NxscopeHandler
from nxslib.proto.parse import Parser

# Object IDs from the ppk2_nxscope_fast descriptor (dawn boot log / shell
# "info"). Pot order follows the descriptor: vbb, vldo, iaoff.

OBJ = {
    "sw1": 0x47870000, "sw2": 0x47870001, "sw3": 0x47870002,
    "sw4": 0x47870003,
    "vext_en": 0x47A70000, "vldo_en": 0x47A70001, "ana_en": 0x47A70002,
    "reg_en": 0x47A70003, "vout_en": 0x47A70004,
    "cal100k": 0x47A70005, "cal10k": 0x47A70006, "cal1k": 0x47A70007,
    "cal100": 0x47A70008,
    "vbb": 0x4AE60000, "vldo": 0x4AE60001, "iaoff": 0x4AE60002,
    "lp_en": 0x47A70009, "calmem": 0x412F0000,
}

SET_IO = 8
GET_IO = 10
GET_IO_SEEK = 11
SET_IO_SEEK = 9

# Self-calibration blob in the 24CW160 EEPROM: stored in the virgin tail,
# far away from the Nordic factory data (bytes 0x000..0x0FC, backed up in
# ppk2-backup/). Layout: magic, version, 5 gains f32, 5 offsets f32, crc32.

CALEE_OFFSET = 0x7C0
CALEE_MAGIC = 0x434E5744  # "DWNC"
CALEE_SIZE = 52
CALEE_CHUNK = 32  # fits both the device cmd RX and TX buffers per transfer

# Onboard calibration loads (0.1% parts on the PPK2)

LOADS = {"cal100k": 100e3, "cal10k": 10e3, "cal1k": 1e3, "cal100": 100.0}

# Wiper -> VLDO volts model. Counts measured on the rails sense ADC (shell
# config), volts anchored on the USB VIN reading (1169 counts ~ 5.0 V, same
# divider assumed): V = counts * 5.0 / 1169.

# Dense map measured on this board via the shell-config rails ADC
# (VIN = USB 5.00 V assumed for the counts->volts anchor). The pot is
# nonlinear; below wiper 24 the output is unstable and above ~224 VLDO
# saturates at the default VBB rail (raise VBB for outputs above 4.3 V).

WIPER_V_TABLE = [(24, 0.852), (32, 1.049), (40, 1.239), (48, 1.393), (56, 1.583), (64, 1.699), (72, 1.854), (80, 2.026), (88, 2.129), (96, 2.31), (104, 2.46), (112, 2.623), (120, 2.714), (128, 2.869), (136, 2.989), (144, 3.122), (152, 3.269), (160, 3.415), (168, 3.57), (176, 3.69), (184, 3.836), (192, 3.952), (200, 4.094), (208, 4.236), (216, 4.408), (224, 4.434)]

# VBB (LDO input rail) wiper map - needed as headroom for VLDO > 4.2 V.

VBB_V_TABLE = [(0, 2.714), (16, 2.976), (32, 3.195), (48, 3.441), (64, 3.643), (80, 3.828), (96, 4.021), (112, 4.245), (128, 4.413), (144, 4.61), (160, 4.821), (176, 4.993), (192, 5.161), (208, 5.342), (224, 5.557), (240, 5.733), (256, 5.944)]
ANCHOR_WIPER = 224

# Nominal shunt chain (ohms) per range, highest resistance first. Used only
# to extrapolate gains for ranges the sweep cannot reach.

SHUNT_EFF = [1122.05, 122.05, 12.05, 1.05, 0.05]

# A calibration point must clear the resting ADC by at least this many counts
# to be usable; below it the current is unresolved in the range it landed in.

MIN_HEADROOM = 20.0

WIPERS = [30, 60, 90, 128, 170, 200, 230]
WIPERS_FINE = [16, 24, 34, 45, 60, 80, 104, 128, 160, 192, 230, 250]
SETTLE_S = 0.3
CAPTURE_S = 0.4


def wiper_volts(w):
    t = WIPER_V_TABLE
    if w <= t[0][0]:
        lo, hi = t[0], t[1]
    elif w >= t[-1][0]:
        lo, hi = t[-2], t[-1]
    else:
        lo, hi = next((a, b) for a, b in zip(t, t[1:])
                      if a[0] <= w <= b[0])
    return lo[1] + (hi[1] - lo[1]) * (w - lo[0]) / (hi[0] - lo[0])


class Ppk2Cal:
    def __init__(self, port):
        self.samples = []
        self.getio_resp = []
        self._buf = bytearray()
        self._hook_stream()
        self.intf = sermod.SerialDevice(port)
        self.nxs = NxscopeHandler(self.intf, Parser())

    def _hook_stream(self):
        outer = self
        orig = sermod.SerialDevice._read

        def read(dev):
            b = orig(dev)
            if b:
                buf = outer._buf
                buf.extend(b)
                while len(buf) >= 3:
                    if buf[0] != 0x55:
                        del buf[0]
                        continue
                    fl = buf[1] | (buf[2] << 8)
                    if fl < 7:
                        del buf[0]
                        continue
                    if len(buf) < fl:
                        break
                    if buf[3] == 0x01:
                        p = 5
                        while p + 3 <= fl - 2:
                            v = buf[p + 1] | (buf[p + 2] << 8)
                            outer.samples.append(
                                (v & 0xFFF, bin(v >> 12).count("1")))
                            p += 3
                    del buf[:fl]
            return b

        sermod.SerialDevice._read = read

    def connect(self):
        self.nxs.connect()
        self.nxs.add_user_frame_listener(self._on_user, [GET_IO])
        self.nxs.ch_enable([0])
        self.nxs.ch_divider([0], 0)
        self.q = self.nxs.stream_sub(0)
        self.nxs.stream_start()
        time.sleep(0.3)

    def disconnect(self):
        self.nxs.stream_stop()
        self.nxs.disconnect()

    def _on_user(self, frame):
        self.getio_resp.append(bytes(getattr(frame, "data", frame)))
        # Mark handled so the response frame is not routed to the command
        # queue, where it would desynchronize the ACK stream.
        return True

    def setio(self, name, value):
        payload = struct.pack("<IH", OBJ[name], 4) + \
            int(value).to_bytes(4, "little")
        ack = self.nxs.send_user_frame(SET_IO, payload,
                                       ack_mode=AckMode.ENABLED)
        if not getattr(ack, "state", True):
            raise RuntimeError(f"SET_IO {name}={value} rejected")

    def getio_u32(self, name):
        self.getio_resp.clear()
        # ACK enabled so the request/ack stream stays paired; the response
        # frame itself is the authoritative result.
        self.nxs.send_user_frame(GET_IO, struct.pack("<I", OBJ[name]),
                                 ack_mode=AckMode.ENABLED)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 1.0:
            for r in self.getio_resp:
                objid, size = struct.unpack_from("<IH", r, 0)
                if objid == OBJ[name] and size == 4:
                    return struct.unpack_from("<I", r, 6)[0]
            time.sleep(0.01)
        raise RuntimeError(f"GET_IO {name}: no response")

    def read_range(self):
        return sum(self.getio_u32(f"sw{i}") for i in (1, 2, 3, 4))

    def range_reset(self):
        """Blink the analog rails: the auto-range comparators reset and
        re-acquire the natural range for the present current, removing
        the hysteresis path dependence between calibration points.
        The rail-on write is retried in case the first ACK is dropped."""
        self.setio("ana_en", 0)
        time.sleep(0.05)
        for _ in range(5):
            try:
                self.setio("ana_en", 1)
                break
            except RuntimeError:
                time.sleep(0.1)
        else:
            raise RuntimeError("range_reset: ana_en did not come back up")
        time.sleep(0.1)

    def _read_chunk(self, name, offset, size):
        self.getio_resp.clear()
        self.nxs.send_user_frame(
            GET_IO_SEEK,
            struct.pack("<IIH", OBJ[name], offset, size),
            ack_mode=AckMode.ENABLED)
        t0 = time.monotonic()
        while time.monotonic() - t0 < 1.0:
            for r in self.getio_resp:
                objid, rsize = struct.unpack_from("<IH", r, 0)
                if objid == OBJ[name]:
                    return r[6:6 + rsize]
            time.sleep(0.01)
        raise RuntimeError(f"GET_IO_SEEK {name}: no response")

    def read_seek(self, name, offset, size):
        out = b""
        while len(out) < size:
            n = min(CALEE_CHUNK, size - len(out))
            out += self._read_chunk(name, offset + len(out), n)
        return out

    def write_seek(self, name, offset, data):
        pos = 0
        while pos < len(data):
            chunk = data[pos:pos + CALEE_CHUNK]
            payload = struct.pack("<IIH", OBJ[name], offset + pos,
                                  len(chunk)) + chunk
            ack = self.nxs.send_user_frame(SET_IO_SEEK, payload,
                                           ack_mode=AckMode.ENABLED)
            if not getattr(ack, "state", True):
                raise RuntimeError(
                    f"SET_IO_SEEK {name} rejected "
                    f"(ret {getattr(ack, 'retcode', '?')})")
            pos += len(chunk)

    def store_eeprom(self, gains, offsets):
        """Store the calibration blob in the EEPROM tail - only if the
        target region is virgin (0xFF) or holds one of our blobs."""
        import zlib
        cur = self.read_seek("calmem", CALEE_OFFSET, CALEE_SIZE)
        is_ours = struct.unpack_from("<I", cur, 0)[0] == CALEE_MAGIC
        if not is_ours and any(b != 0xFF for b in cur):
            raise RuntimeError(
                "EEPROM cal region is neither virgin nor ours - refusing "
                f"to write (offset {CALEE_OFFSET:#x})")
        body = struct.pack("<II5f5f", CALEE_MAGIC, 1, *gains, *offsets)
        blob = body + struct.pack("<I", zlib.crc32(body))
        self.write_seek("calmem", CALEE_OFFSET, blob)
        back = self.read_seek("calmem", CALEE_OFFSET, len(blob))
        if back != blob:
            raise RuntimeError("EEPROM readback mismatch after write")
        return len(blob)

    def capture(self):
        """Return (adc mean, std, n, majority range) from tagged samples.

        Only samples tagged with the majority range enter the statistics,
        so range transitions inside the window cannot skew a point.
        """
        time.sleep(SETTLE_S)
        self.samples.clear()
        time.sleep(CAPTURE_S)
        snap = list(self.samples)
        if len(snap) < 1000:
            raise RuntimeError(f"stream starved ({len(snap)} samples)")
        counts = {}
        for _, r in snap:
            counts[r] = counts.get(r, 0) + 1
        rng = max(counts, key=counts.get)
        adcs = [a for a, r in snap if r == rng]
        mean = sum(adcs) / len(adcs)
        var = sum((s - mean) ** 2 for s in adcs) / len(adcs)
        return mean, var ** 0.5, len(adcs), rng

    def smu_on(self):
        for n in ("cal100k", "cal10k", "cal1k", "cal100", "vout_en"):
            self.setio(n, 0)
        self.setio("ana_en", 1)
        self.setio("reg_en", 1)
        self.setio("vldo", WIPERS[0])
        time.sleep(0.3)
        self.setio("vldo_en", 1)
        self.setio("vout_en", 1)

    def smu_off(self):
        for n in ("cal100k", "cal10k", "cal1k", "cal100",
                  "vout_en", "vldo_en", "reg_en", "ana_en"):
            try:
                self.setio(n, 0)
            except Exception:
                pass


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


def main():
    ap = argparse.ArgumentParser(description=__doc__.splitlines()[0])
    ap.add_argument("--port", default="/dev/ttyACM0")
    ap.add_argument("--out", default="ppk2-cal.json")
    ap.add_argument("--anchor-volts", type=float, default=None,
                    help="DMM-measured VDUT at wiper 230 (absolute anchor)")
    args = ap.parse_args()

    vscale = 1.0
    if args.anchor_volts is not None:
        vscale = args.anchor_volts / wiper_volts(ANCHOR_WIPER)
        print(f"anchor: VDUT(w={ANCHOR_WIPER}) = {args.anchor_volts:.4f} V "
              f"-> voltage scale {vscale:.4f}")

    cal = Ppk2Cal(args.port)
    cal.connect()
    print("connected, output must be disconnected from any DUT")

    points = []
    zero = {}
    try:
        cal.smu_on()

        # Zero-current point: auto-range rests in the lowest range.

        mean, std, n, r = cal.capture()
        zero = {"adc": mean, "std": std, "range": r}
        print(f"zero-current: adc={mean:7.1f} std={std:5.1f} range={r}")

        def sweep(load, wipers, phase, reset=False):
            ohms = LOADS[load]
            cal.setio(load, 1)
            for w in wipers:
                cal.setio("vldo", w)
                # A mid-range load connects with an inrush spike that latches
                # the auto-range high; blink the rails so it re-acquires the
                # natural range.  The low phase rises gradually from zero and
                # never latches, so a blink there would only inject its own
                # inrush and strand tiny currents in range 4.
                if reset:
                    cal.range_reset()
                mean, std, n, r = cal.capture()
                amps = wiper_volts(w) * vscale / ohms
                points.append({"load": load, "wiper": w, "amps": amps,
                               "adc": mean, "std": std, "range": r,
                               "phase": phase})
                print(f"{phase:5s} {load:8s} w={w:3d} I={amps * 1e3:9.4f} mA "
                      f"adc={mean:7.1f} std={std:5.1f} range={r}")
            cal.setio(load, 0)

        # Phase LOW: come up from zero current with fine steps so the
        # auto-range hysteresis keeps points in the low ranges as long as
        # possible (upward transitions only happen at range full-scale).

        cal.setio("vldo", WIPERS_FINE[0])
        time.sleep(0.3)
        sweep("cal100k", WIPERS_FINE, "low")
        sweep("cal10k", WIPERS_FINE, "low")

        # Phase MID: the verified mid-range sweep.

        cal.setio("vldo", WIPERS[0])
        time.sleep(0.3)
        sweep("cal1k", WIPERS, "mid", reset=True)
        sweep("cal100", WIPERS, "mid", reset=True)

    finally:
        cal.smu_off()
        cal.disconnect()

    # Drop range-top compression points: inside one (load, phase) sweep the
    # adc must rise with current; a flat/declining tail sits at the analog
    # comparator threshold and would bias the fit.

    bykey = {}
    for p in points:
        bykey.setdefault((p["load"], p["phase"]), []).append(p)
    for pts in bykey.values():
        pts.sort(key=lambda q: q["amps"])
        for a, b in zip(pts, pts[1:]):
            if b["adc"] <= a["adc"] + 1.0 and b["range"] == a["range"]:
                b["saturated"] = True

    # Drop offset-dominated points: a current that latched into a range too
    # coarse to resolve it sits within a handful of counts of the resting
    # (zero-current) ADC and carries no information - it only biases the fit
    # and inflates the residuals.

    resting = zero.get("adc", 0.0)
    for p in points:
        if p["adc"] - resting < MIN_HEADROOM:
            p["saturated"] = True

    # Per-range least squares

    gains = [None] * 5
    offsets = [None] * 5
    for r in range(5):
        pts = [(p["adc"], p["amps"]) for p in points
               if p["range"] == r and not p.get("saturated")]
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
        print("ERROR: no range could be fitted", file=sys.stderr)
        return 1
    for r in range(5):
        if gains[r] is None:
            ref = min(known, key=lambda k: abs(k - r))
            gains[r] = gains[ref] * SHUNT_EFF[ref] / SHUNT_EFF[r]
            if offsets[r] is None:
                offsets[r] = offsets[ref]
            print(f"range {r}: unreachable, gain chained from range {ref}")

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
        "anchor": ("dmm" if args.anchor_volts is not None else
                   "nominal-usb-5v"),
        "zero_current": zero,
        "points": points,
        "verify_max_err_pct": max(errs) if errs else None,
        "verify_avg_err_pct": (sum(errs) / len(errs)) if errs else None,
    }
    with open(args.out, "w") as f:
        json.dump(result, f, indent=2)

    # Persist on the device and verify the factory area was untouched

    cal2 = Ppk2Cal(args.port)
    cal2.connect()
    try:
        n = cal2.store_eeprom(gains, offsets)
        print(f"calibration stored in EEPROM at {CALEE_OFFSET:#x} "
              f"({n} bytes)")
        try:
            ref = open("ppk2-backup/ppk2_eeprom.bin", "rb").read(0x100)
            dev0 = cal2.read_seek("calmem", 0, 0x100)
            print("factory region intact"
                  if dev0 == ref else
                  "WARNING: factory region differs from backup!")
        except OSError:
            pass
    finally:
        cal2.disconnect()

    print("\nrange   gain [A/count]   offset [counts]")
    for r in range(5):
        print(f"  {r}     {gains[r]:.6e}   {offsets[r]:9.2f}")
    print(f"\nverification: avg |err| = {result['verify_avg_err_pct']:.2f}%"
          f", max |err| = {result['verify_max_err_pct']:.2f}%"
          f" (vs nominal model, anchor: {result['anchor']})")
    print(f"calibration written to {args.out}")
    return 0


if __name__ == "__main__":
    sys.exit(main())
