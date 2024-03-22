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

Host-side control is available via :file:`tools/scripts/modbus_blinky_cli.py`.
The helper supports start/stop, toggle, status, dwell-period updates, and
interactive usage.

modbus
------

Feature Modbus RTU profile using ``feature_modbus_slot0.yaml`` as the boot
descriptor. It enables descriptor slots for runtime descriptor switching.

modbus_ntfc
-----------

NTFC Modbus RTU profile using ``ntfc_modbus_rtu_dummy_map.yaml``.

serial
------

Serial example with LED and button access. Uses
``serial_leds_buttons_demo.yaml``.
