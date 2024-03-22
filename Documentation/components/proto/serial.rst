======================
Simple Serial Protocol
======================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoSerial`` - binary serial protocol for machine-to-machine
communication with efficient bandwidth and deterministic latency.

Unlike the human-friendly shell protocol, the serial protocol uses compact
binary frames for efficient communication between automated systems.


Implementation
==============

.. code:: shell

  // Byte 0   - FRAME SYNC
  // Byte 1-2 - PAYLOAD LEN (2 bytes, little-endian)
  // Byte 3   - COMMAND
  // Byte 4-n - PAYLOAD
  // Byte n+1-n+2 - crc (2 bytes, little-endian)

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_SERIAL``: enables the serial protocol.
- ``CONFIG_DAWN_PROTO_SERIAL_PATH``: default serial device path.
- ``CONFIG_DAWN_PROTO_SERIAL_BAUD``: default serial baudrate.

YAML
----

.. code-block:: yaml

   protocols:
     - id: serial1
       type: serial
       config:
         bindings:
           - io1
           - io2
         path: "/dev/ttyS1"
         baudrate: 115200

Supported fields:

- ``config.bindings``: standard IO binding list.
- ``config.path``: serial device path.
- ``config.baudrate``: serial baudrate.

External Control
================

ControlIO: supported.

``CProtoSerial`` supports runtime start/stop control through ``CIOControl``.
When stopped, the serial protocol thread is inactive. When started again,
frame processing resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoSerial <../../doxygen/classdawn_1_1CProtoSerial.html>`_
