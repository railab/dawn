#!/bin/sh
#
# SPDX-License-Identifier: Apache-2.0
#
# PPK2 flash/reset over an external SWD probe: flash.sh <image.hex> | reset
# Set PPK2_SN to the probe serial number when more probes are connected.

set -e

SN=${PPK2_SN:+--serial-number $PPK2_SN}

if [ "$1" = "reset" ]; then
  exec nrfutil device reset $SN
fi

# A protected chip must be recovered (mass erase) before programming

nrfutil device program $SN --firmware "$1" --core Application ||
  { nrfutil device recover $SN && nrfutil device program $SN --firmware "$1" --core Application; }

# Keep the debug port open (APPROTECT disabled) and the reset pin enabled

nrfutil device x-write $SN --address 0x10001208 --value 0xFFFFFF5A
nrfutil device x-write $SN --address 0x1000120C --value 0xFFFFFFFE
