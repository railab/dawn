# ntfc/tests/ppk2_nxscope/_ppk2_recv.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""Stream receiver run as a child process, so the test can SIGSTOP it.

Usage: _ppk2_recv.py PORT SECONDS. Prints "READY" once streaming, then
"samples=N overflow=N errors=N" at the end.
"""

import logging
import sys
import time

from _ppk2_common import FrameErrors, count, open_ppk2


def main():
    """Receive the stream and print the totals."""
    port, seconds = sys.argv[1], float(sys.argv[2])
    errors = FrameErrors()
    logging.getLogger("nxslib").addHandler(errors)

    dev = open_ppk2(port)
    print("READY", flush=True)

    n = 0
    t0 = time.monotonic()
    while time.monotonic() - t0 < seconds:
        time.sleep(0.05)
        n += count(dev.q)

    ovf = dev.nxs.overflow_count
    dev.disconnect()
    print(f"samples={n} overflow={ovf} errors={errors.count}", flush=True)


if __name__ == "__main__":
    main()
