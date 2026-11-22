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

led16a1
-------

RGB LED example for the X-NUCLEO-LED16A1 expansion board (LED1642GW 16-channel
LED driver) with the RGB LED daughterboard. Uses ``led16a1_serial_demo.yaml``
and exposes the five RGB LEDs as ``rgb_led`` IOs over the Dawn serial protocol
on the ST-LINK Virtual COM Port.

The LED1642GW is a PWM lower-half driver with one channel per output. The
board logic registers each RGB LED as a ``/dev/rgbledN`` device on three of
its outputs.

=========== =========== =====================
Signal      Arduino pin MCU pin
=========== =========== =====================
CLK         D13         PA5 (shared with LD1)
SDI         D11         PA7
LE          D2          PA10
PWCLK       D6          PC8 (TIM3_CH3, 1 MHz)
=========== =========== =====================

Host-side control is available via
:file:`tools/examples/led16a1/serial_led16a1_cli.py`:

.. code-block:: bash

   python3 tools/examples/led16a1/serial_led16a1_cli.py set 0 ff0000
   python3 tools/examples/led16a1/serial_led16a1_cli.py all blue
   python3 tools/examples/led16a1/serial_led16a1_cli.py demo
   python3 tools/examples/led16a1/serial_led16a1_cli.py off

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
