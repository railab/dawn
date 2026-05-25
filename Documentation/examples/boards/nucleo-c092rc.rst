================
ST Nucleo C092RC
================

Configs
=======

can
---

CAN example configuration.
Uses ``ntfc_can_dummy.yaml``.

can_minimal
-----------

Minimal CAN button/LED configuration optimized for resources.
Uses ``can_button_led.yaml``.

blinky_can
----------

CAN-controlled blinky demo with serial console enabled.
Uses ``blinky_can_demo.yaml``.

The demo targets the on-board user LED through the sequencer program and
exposes runtime control over CAN. The host-side helper is
:file:`tools/examples/blinky/can_blinky_cli.py`; it can start and stop the sequencer,
reset it, read mapped values, and update the blink period.

The configuration uses ``/dev/can0`` at 250 kbit/s and keeps USART2 as the
serial console.
