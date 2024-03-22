.. _protoudp:

============
UDP Protocol
============

**Component Type:** Protocol

**Status:** Implemented

Overview
========

The ``UDP Protocol`` implements a lightweight binary protocol over UDP.
It uses the same framing and command set as the serial protocol, but wrapped
in UDP datagrams.

The protocol listens on a configurable local UDP port and responds to the
sender address and port of the received request.

Implementation
==============

Framing
-------

The protocol uses a simple framing format:

+-------+--------+-------+---------+-----+
| SYNC  | LEN    | CMD   | PAYLOAD | CRC |
+-------+--------+-------+---------+-----+
| 1B    | 2B     | 1B    | NB      | 2B  |
+-------+--------+-------+---------+-----+

- ``SYNC``: Synchronization byte (0xAA).
- ``LEN``: Length of the payload in bytes (2 bytes, little-endian).
- ``CMD``: Command identifier.
- ``PAYLOAD``: Command-specific data.
- ``CRC``: CRC16-CCITT checksum of CMD and PAYLOAD (2 bytes, little-endian).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_UDP``: enables the UDP protocol.
- ``CONFIG_DAWN_PROTO_UDP_PORT``: default local UDP port.

YAML
----

.. code-block:: yaml

   protocols:
     - id: udp0
       type: udp
       config:
         bindings:
           - io1
           - io2
         port: 50000

Supported fields:

- ``config.bindings``: standard IO binding list.
- ``config.port``: local UDP port used by the protocol.

External Control
================

ControlIO: supported.

``CProtoUdp`` supports runtime start/stop control through ``CIOControl``.
When stopped, the UDP protocol thread/socket handling is inactive.
When started again, request handling resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoUdp <../../doxygen/classdawn_1_1CProtoUdp.html>`_
