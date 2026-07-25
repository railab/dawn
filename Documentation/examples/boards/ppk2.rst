============================
Nordic Power Profiler Kit II
============================

nRF52840-based power measurement tool (PCA63100). Flashing requires SWD
access (J-Link on the test points / unpopulated P3 header) and erases the
Nordic factory firmware, which can be restored with the nRF Connect
Programmer. Before the first flash, back up the stock firmware
(``JLinkExe savebin`` or ``nrfjprog --readcode``). The onboard 24CW160
EEPROM (``/dev/eeprom0``, writable) stores the Nordic factory calibration
in bytes 0x000..0x0FC; the host tooling only ever writes the Dawn
self-calibration blob at 0x7C0.

Configs
=======

nxscope
-------

The single config: boots straight into dawn and streams the current channel
at 100 kS/s over USB CDC/ACM (``nxscli serial /dev/ttyACM0``) using
``ppk2_nxscope.yaml``. Samples are self-describing: ``adc = v & 0xfff``,
range bits ``v >> 12`` (on the sample read as uint16; GPIOTE edge inputs
fused by the bitpack and tag programs). Power path, calibration loads, SMU
voltage and in-amp offset (pot, MCP4451), the RGB lightwell and the
calibration EEPROM are writable through NxScope set requests. Syslog and the
debug shell run on the Segger RTT console. The USB DATA/POWER port must be
connected, otherwise boot blocks waiting for VBUS.

Host tooling
============

``tools/examples/ppk2/ppk2.py`` drives the board over nxscope: SMU output
(``on VOLTS``, 0.87..5.15 V), ampere-meter mode
(``mode ampere``), calibration loads, live plot, measure/CSV
(``--decimate`` block-averages the 100 kS/s stream) and self-calibration
(``cal``, optional ``--anchor-volts <DMM reading>``).

Flashing
========

After ``nrfutil device recover`` + ``program`` always write UICR.APPROTECT
or the next power-on re-arms access port protection and the next debug
connect mass-erases the chip::

   nrfutil device x-write --address 0x10001208 --value 0xFFFFFF5A
   nrfutil device x-write --address 0x1000120C --value 0xFFFFFFFE

(the second write keeps P0.09/P0.10 as GPIOs - an erased UICR puts them in
NFC mode and kills the logic port).
