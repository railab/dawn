====================
ST STM32F4 Discovery
====================

The board has no wireless or on-chip Ethernet, so the USB OTG-FS port is the
transport for every Dawn protocol:

- serial and Modbus-RTU ride USB **CDC/ACM** (device node ``/dev/ttyACM0``);
- Modbus-TCP and UDP ride USB **CDC-ECM** (a network interface, static IP).

The text console and logs stay on **USART2**, wired to the ST-LINK virtual COM
port, so the USB port is left entirely to the protocol under test.

The configs reuse the shared example descriptors; the serial/Modbus-RTU device
path is taken from ``CONFIG_DAWN_PROTO_SERIAL_PATH`` /
``CONFIG_DAWN_PROTO_MODBUS_RTU_PATH`` (``/dev/ttyACM0`` here) rather than being
hard-coded in the descriptor.

Configs
=======

blinky_shell
------------

Runtime-control example using the same descriptor as the simulator
``nsh_blinky_shell``. Uses ``blinky_shell_demo.yaml`` and is driven from the
NSH prompt on the USART2 console.

serial
------

Serial access to demo data plus the LEDs and button over USB CDC/ACM. Uses the
shared ``serial_leds_buttons_demo.yaml``.

Host tool: :doc:`dawnpy-serial </tools/dawnpy-serial>`.

nxscope
-------

Dedicated NXScope demo over USB CDC/ACM. Streams the on-board LIS3DSH
accelerometer, pushes the user button asynchronously (notify), and drives the
four user LEDs from the host. Uses ``stm32f4disco_nxscope.yaml``.

Host tool: NxScli (``nxscli serial /dev/ttyACMx``) to stream the accel/button
channels, plus the dawn-nxscope plugin's ``set_io`` to control the LEDs.

modbus_rtu
----------

Modbus-RTU controlled LED sequencer over USB CDC/ACM. Uses the shared
``blinky_modbus_rtu_demo.yaml`` and exposes start/stop plus writable runtime
blink configuration over Modbus holding registers.

Host helper: :file:`tools/examples/blinky/modbus_blinky_cli.py`.

modbus_ntfc
-----------

NTFC Modbus-RTU profile using the shared ``ntfc_modbus_rtu_dummy_map.yaml``
over USB CDC/ACM.

modbus_tcp
----------

Modbus-TCP controlled blinky demo over USB CDC-ECM. Uses
``blinky_modbus_tcp_demo.yaml``. The board takes the static address
``192.168.9.2`` and the Modbus TCP server listens on port ``502``.

Host helper: :file:`tools/examples/blinky/modbus_tcp_blinky_cli.py`.

modbus_tcp_ntfc
---------------

NTFC Modbus-TCP profile over USB CDC-ECM. Uses
``qemu_modbus_tcp_dummy_map.yaml`` with the link-local address ``169.254.7.2``
so the host needs no network configuration.

udp
---

UDP controlled blinky demo over USB CDC-ECM. Uses ``blinky_udp_demo.yaml``.
The board takes the static address ``192.168.9.2`` and listens on port
``50000``.

Host helper: :file:`tools/examples/blinky/udp_blinky_cli.py`.

udp_ntfc
--------

NTFC UDP profile over USB CDC-ECM. Uses ``udp_basic.yaml`` with the link-local
address ``169.254.7.2`` on port ``50000``.
