=============================
Nordic Power Profiler Kit II
=============================

nRF52840-based power measurement tool (PCA63100). Flashing requires SWD
access (J-Link on the test points / unpopulated P3 header) and erases the
Nordic factory firmware, which can be restored with the nRF Connect
Programmer. Before the first flash, back up the stock firmware
(``JLinkExe savebin`` or ``nrfjprog --readcode``). The onboard 24CW160
EEPROM stores the Nordic factory calibration and is registered read-only.

Configs
=======

shell
-----

Bring-up config with NSH and the Dawn shell on the Segger RTT console.
Uses ``ppk2_shell.yaml``: measurement rails (adc_fetch), range switch
status (gpi), power path control and calibration loads (gpo), SMU voltage
and in-amp offset (pot, MCP4451) and the RGB lightwell (rgb_led).

nxscope
-------

NxScope over USB CDC/ACM using ``ppk2_nxscope.yaml``. Dual CDC/ACM
composite: NSH console on ``/dev/ttyACM0`` (opens once the host asserts
DTR), NxScope data on ``/dev/ttyACM1``. The USB DATA/POWER port must be
connected, otherwise boot blocks waiting for VBUS.

nxscope_fast
------------

The main config: boots straight into dawn and streams the current channel
at 100 kS/s over a single CDC/ACM port (``nxscli serial /dev/ttyACM0``).
Samples are self-describing: ``adc = v & 0xfff``, range bits ``v >> 12``
(GPIOTE edge inputs fused by the bitpack and tag programs).

nxscope_udp
-----------

Network transport reference: the same stream over nxscope-UDP on a CDC-ECM
USB-net link (device ``192.168.10.2:50000``, DHCP served by the board).

Host tooling
============

``tools/ppk2/ppk2.py`` controls everything over the nxscope link: SMU
output 0.8..5.0 V (soft-start; VBB headroom raised automatically above
~4.2 V), ampere-meter mode (``mode ampere`` - external supply through the
VEXT path), calibration loads, live plot, measure/CSV with ``--rate``
(full 100 kS/s on the wire, block-averaged host-side - lower rate = more
resolution), and the self-calibration sweep (``ppk2.py cal``; ranges 1..3
fitted, 0 and 4 nominal, ~1 % avg with the measured wiper map; add
``--anchor-volts <DMM VDUT reading>`` for an absolute anchor).

Flashing
========

After ``nrfutil device recover`` + ``program`` always write UICR.APPROTECT
or the next power-on re-arms access port protection and the next debug
connect mass-erases the chip::

   nrfutil device x-write --address 0x10001208 --value 0xFFFFFF5A
   nrfutil device x-write --address 0x1000120C --value 0xFFFFFFFE

(the second write keeps P0.09/P0.10 as GPIOs - an erased UICR puts them in
NFC mode and kills the logic port).
