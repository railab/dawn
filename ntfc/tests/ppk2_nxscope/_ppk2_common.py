# ntfc/tests/ppk2_nxscope/_ppk2_common.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""PPK2 host helpers shared by the NTFC tests and the stall receiver."""

import logging
import queue
import sys
import time
from pathlib import Path

ROOT = Path(__file__).resolve().parents[3]
sys.path.insert(0, str(ROOT / "tools/examples/ppk2"))

from ppk2_dev import CalModel, Ppk2  # noqa: E402

RATE = 100_000


class FrameErrors(logging.Handler):
    """Count nxslib frame errors (bad SOF or CRC)."""

    def __init__(self):
        """Count from zero."""
        super().__init__(logging.ERROR)
        self.count = 0

    def emit(self, record):
        """Count one frame error record."""
        msg = record.getMessage()
        if msg.startswith("invalid sof") or msg.startswith("invalid crc16"):
            self.count += 1


def open_ppk2(port, timeout_s=10.0):
    """Connect to the nxscope port, retrying while the device starts."""
    deadline = time.monotonic() + timeout_s
    while True:
        dev = Ppk2(port)
        try:
            dev.connect(stream=True)
            return dev
        except Exception:
            try:
                dev.nxs.disconnect()
            except Exception:
                pass
            if time.monotonic() > deadline:
                raise
            time.sleep(0.5)


def count(q):
    """Drain a stream queue, returning the number of samples."""
    n = 0
    while True:
        try:
            batch = q.get_nowait()
        except queue.Empty:
            return n
        for s in batch:
            n += 1 if isinstance(s.data, tuple) else len(s.data)


def load_cal(dev):
    """Calibration from the board EEPROM only (no host fallback)."""
    cal = CalModel(path="/nonexistent")
    cal.load_device(dev)
    return cal
