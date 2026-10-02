# ntfc/tests/ppk2_nxscope/test_ppk2_nxscope.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""PPK2 hardware tests.

Stream rate, commands under load, host stalls, analog rails and
current/voltage accuracy.

NSH runs on the first CDC/ACM port, the nxscope stream on the second.
"""

import logging
import os
import re
import signal
import subprocess
import sys
import time
from pathlib import Path

import pytest
from _ntfc_common import shell_cmd, start_dawn
from _ppk2_common import RATE, FrameErrors, count, load_cal, open_ppk2
from ppk2_dev import load_objids, split_sample

HERE = Path(__file__).resolve().parent
PROMPT = "ppk2>"

# Minimum sustained stream rate (S/s), 0.5% below nominal

MIN_RATE = RATE * 0.995

LOADS = {"100k": 100e3, "10k": 10e3, "1k": 1e3, "100": 100.0}

pytestmark = [
    pytest.mark.dep_config("CONFIG_DAWN_PROTO_NXSCOPE_SERIAL"),
    pytest.mark.dep_config("CONFIG_NSH_USBCONSOLE"),
]


def _port():
    cfg = pytest.products[0].conf.config.get("ppk2", {})
    return cfg.get("nxscope_port", "/dev/ttyACM1")


@pytest.fixture(scope="module")
def shell():
    """Start dawn once; its shell takes over the NSH console."""
    product = start_dawn(settle_s=1.0)
    yield product
    try:
        shell_cmd(product, "exit", timeout=3)
    except Exception:
        pass


@pytest.fixture
def ppk2(shell):
    """Streaming nxscope connection; output off and loads off afterwards."""
    errors = FrameErrors()
    logging.getLogger("nxslib").addHandler(errors)
    dev = open_ppk2(_port())
    dev.errors = errors
    yield dev
    try:
        dev.set_load("off")
        dev.setio("lp_interval", 0)
        dev.smu_off()
    finally:
        dev.disconnect()
        logging.getLogger("nxslib").removeHandler(errors)


def _rate(dev, seconds):
    """Received samples per second over a window."""
    count(dev.q)
    t0 = time.monotonic()
    n = 0
    while time.monotonic() - t0 < seconds:
        time.sleep(0.05)
        n += count(dev.q)
    return n / (time.monotonic() - t0)


def _load_on(dev, load, volts):
    """Connect a load and ramp the output up.

    The ramp settles the auto-range low; a uA load can still sit in a
    higher range at a few counts, so release it.
    """
    dev.set_load(load)
    dev.smu_on(volts)
    if volts / LOADS[load] < 100e-6:
        dev.range_reset()
    time.sleep(0.5)


def _amps(dev, cal, seconds=1.0):
    """Mean calibrated current over a window and the ranges seen."""
    dev.drain()
    time.sleep(seconds)
    raws = [split_sample(r) for r in dev.drain()]
    assert raws, "no samples"
    ranges = sorted({r for _, r in raws})
    return sum(cal.amps(a, r) for a, r in raws) / len(raws), ranges


def test_dawn_shell(shell):
    """Dawn runs and serves its shell on the NSH console."""
    ret = shell.sendCommandReadUntilPattern("info", pattern=PROMPT, timeout=3)
    assert "shell IO bindings" in ret.output

    objid = load_objids()["vldo"]
    ret = shell.sendCommandReadUntilPattern(
        f"getio 0x{objid:08x}", pattern=PROMPT, timeout=3
    )
    assert f"IO 0x{objid:08x} data:" in ret.output


def test_stream_rate(ppk2):
    """Full rate stream with no frame errors or overflows."""
    rate = _rate(ppk2, 10.0)
    assert rate >= MIN_RATE, f"{rate:.0f} S/s"
    assert ppk2.nxs.overflow_count == 0
    assert ppk2.errors.count == 0


def test_command_stress(ppk2):
    """Acknowledged commands under the full stream: none lost, no drops.

    Every setio waits for the device ACK and raises on NACK or timeout.
    """
    ppk2.smu_on(3.3)
    loads = ["100k", "10k", "1k", "100", "off"]

    count(ppk2.q)
    t0 = time.monotonic()
    n = 0
    for i in range(200):
        ppk2.set_load(loads[i % len(loads)])
        if i % 40 == 0:
            ppk2.setio("lp_interval", 1000 if i % 80 == 0 else 0)
        n += count(ppk2.q)
    rate = n / (time.monotonic() - t0)

    assert rate >= MIN_RATE, f"{rate:.0f} S/s"
    assert ppk2.nxs.overflow_count == 0
    assert ppk2.errors.count == 0


def test_host_stall(shell):
    """Host stalls drop data in whole frames, flagged as overflow."""
    env = dict(os.environ, PYTHONPATH=str(HERE))
    proc = subprocess.Popen(
        [sys.executable, str(HERE / "_ppk2_recv.py"), _port(), "8"],
        stdout=subprocess.PIPE,
        text=True,
        env=env,
    )
    try:
        assert proc.stdout.readline().strip() == "READY"
        time.sleep(1.0)
        for _ in range(5):
            proc.send_signal(signal.SIGSTOP)
            time.sleep(0.5)
            proc.send_signal(signal.SIGCONT)
            time.sleep(0.5)
        out, _ = proc.communicate(timeout=30)
    finally:
        if proc.poll() is None:
            proc.kill()

    res = dict(re.findall(r"(\w+)=(\d+)", out))
    assert int(res["errors"]) == 0, out
    assert int(res["overflow"]) > 0, out


def test_rails_off_on(ppk2):
    """Rails off with the stream running must not wedge the device."""
    for _ in range(10):
        ppk2.setio("ana_en", 0)
        time.sleep(0.2)
        ppk2.setio("ana_en", 1)
        time.sleep(0.05)

    ppk2.setio("ana_en", 0)
    time.sleep(2.0)
    assert ppk2.getio("vldo") >= 0
    ppk2.setio("ana_en", 1)

    rate = _rate(ppk2, 3.0)
    assert rate >= MIN_RATE, f"{rate:.0f} S/s"
    assert ppk2.errors.count == 0


@pytest.mark.parametrize("load", list(LOADS))
def test_load_current(ppk2, load):
    """Onboard load current at 3.3 V matches V/R."""
    cal = load_cal(ppk2)
    if not cal.valid:
        pytest.skip("board not calibrated (ppk2.py cal)")

    volts = 3.3
    _load_on(ppk2, load, volts)

    amps, ranges = _amps(ppk2, cal)
    expect = volts / LOADS[load]
    assert amps == pytest.approx(expect, rel=0.05), f"{amps:.4e} A {ranges=}"


@pytest.mark.parametrize("volts", [1.8, 3.0, 4.5])
def test_output_voltage(ppk2, volts):
    """Output voltage, measured as current through the 1k load."""
    cal = load_cal(ppk2)
    if not cal.valid:
        pytest.skip("board not calibrated (ppk2.py cal)")

    _load_on(ppk2, "1k", volts)

    amps, ranges = _amps(ppk2, cal)
    meas = amps * LOADS["1k"]
    assert meas == pytest.approx(volts, rel=0.05), f"{meas:.3f} V {ranges=}"
