================
ST Nucleo C071RB
================

Configs
=======

blinky_shell
------------

Hardware runtime-control example using the same descriptor as simulator
``nsh_blinky_shell``. Uses ``blinky_shell_demo.yaml``.

blinky_modbus
-------------

Modbus-RTU controlled version of the same LED sequencer demo. Uses
``blinky_modbus_rtu_demo.yaml`` and exposes start/stop plus writable runtime
blink configuration over Modbus holding registers.

Host-side control is available via :file:`tools/examples/blinky/modbus_blinky_cli.py`.
The helper supports start/stop, toggle, status, dwell-period updates, and
interactive usage.

modbus
------

Feature Modbus RTU profile using ``feature_modbus_slot0.yaml`` as the boot
descriptor. It enables descriptor slots for runtime descriptor switching and
exposes both a real ``/dev/pwm0`` output and a real TIM1-backed
``/dev/pulsecount0`` output over Modbus.

On this board the Modbus RS485 direction pin already uses PA8, so the profile
uses TIM1 pulsecount channel 2 on PB3 and moves PWM to TIM14 channel 1 on PA7.

modbus_ntfc
-----------

NTFC Modbus RTU profile using ``ntfc_modbus_rtu_dummy_map.yaml``.

serial
------

Serial example with LED and button access. Uses
``serial_leds_buttons_demo.yaml``.
