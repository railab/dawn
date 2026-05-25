================
ST Nucleo H743ZI
================

Configs
=======

blinky_serial
-------------

Serial-controlled blinky demo for the Nucleo-H743ZI board.
Uses ``blinky_serial_demo.yaml``.

The ST-LINK VCP is reserved for the Dawn serial protocol in this config; it is
not used as a text console.

Host helper :file:`tools/examples/serial_blinky_cli.py`.

blinky_modbus_tcp
-----------------

Modbus-TCP controlled blinky demo for the Nucleo-H743ZI board.
Uses ``blinky_modbus_tcp_demo.yaml``.

The board keeps the same default static target address as the UDP demo:
``192.168.8.104``. The Modbus TCP server listens on port ``5020``.

Host helper: :file:`tools/examples/modbus_tcp_blinky_cli.py`

blinky_udp
----------

UDP-controlled blinky demo for the Nucleo-H743ZI board.
Uses ``blinky_udp_demo.yaml``.

The default static target address used by the example helper is
``192.168.8.104``.

Host helper: :file:`tools/examples/udp_blinky_cli.py`
