================
NxScope Protocol
================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoNxscope`` is an NxScope protocol object for data streaming and
inspection.

User Extension (Set-IO)
=======================

Dawn extends NxScope via ``nxscope_callbacks_s.userid`` to handle
user-defined frame IDs for runtime IO write operations.

This extension works with all Dawn NxScope transports
(``nxscope_dummy``, ``nxscope_serial``, ``nxscope_udp``), because the
command payload is carried inside the standard NxScope serial protocol
frame format.

Implemented user IDs:

1. ``NXSCOPE_HDRID_USER``: simple set IO
2. ``NXSCOPE_HDRID_USER + 1``: seekable set IO

Callback path:

- ``struct nxscope_callbacks_s.userid``
- ``CProtoNxscope::userIdCb()``
- ``CProtoNxscope::handleUserCommand()``

Payload formats (little-endian):

- Simple set IO:
  ``[objid:4][size:2][data:size]``
- Seekable set IO:
  ``[objid:4][offset:4][size:2][data:size]``

Behavior notes:

- The target IO must be writable (``isWrite() == true``).
- Seek request requires seekable IO (``isSeekable() == true``).
- This extension is only for ``set`` operations.
- ``get`` over user-extension IDs is not supported.
- Streaming/get data stays on standard NxScope notify/stream path.

ACK semantics:

- ACK for set/user requests is optional and controlled by
  ``CONFIG_LOGGING_NXSCOPE_ACKFRAMES`` in NxScope.
- When ACK frames are enabled, Dawn returns ACK with the set handler
  return code.
- When ACK frames are disabled, the operation still executes, but no ACK
  frame is sent.

Implementation
==============

Two operation modes supported:

1. polling data with the sample-thread mode

2. async stream when the notify mode is selected

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NXSCOPE``: enables NxScope protocol support.
- ``CONFIG_DAWN_PROTO_NXSCOPE_DUMMY``: enables the dummy transport.
- ``CONFIG_DAWN_PROTO_NXSCOPE_SERIAL``: enables the serial transport.
- ``CONFIG_DAWN_PROTO_NXSCOPE_UDP``: enables the UDP transport.
- ``CONFIG_DAWN_PROTO_NXSCOPE_STREAMBUF_LEN``: stream buffer length.
- ``CONFIG_DAWN_PROTO_NXSCOPE_RXBUF_LEN``: receive buffer length.
- ``CONFIG_DAWN_PROTO_NXSCOPE_RX_PADDING``: extra receive buffer padding.
- ``CONFIG_DAWN_PROTO_NXSCOPE_CRIBUF_LEN``: critical buffer length.
- ``CONFIG_DAWN_PROTO_NXSCOPE_SAMPLE_THREAD``: polling sample-thread mode.
- ``CONFIG_DAWN_PROTO_NXSCOPE_NOTIFY``: notify-driven streaming mode.
- ``CONFIG_DAWN_PROTO_NXSCOPE_SERIAL_PATH``: default serial device path.
- ``CONFIG_DAWN_PROTO_NXSCOPE_SERIAL_BAUD``: default serial baudrate.
- ``CONFIG_DAWN_PROTO_NXSCOPE_UDP_PORT``: default UDP listen port.
- ``CONFIG_LOGGING_NXSCOPE_ACKFRAMES``: optional ACK frames for set/user
  requests.

YAML
----

.. code-block:: yaml

   protocols:
     - id: nx_dummy1
       type: nxscope_dummy
       config:
         iobind2:
           - id: io1
             name: "a"
           - id: io2
             name: "b"
     - id: nx_serial1
       type: nxscope_serial
       config:
         iobind2:
           - id: io1
             name: "chan1"
           - id: io2
             name: "chan2"
         path: "/dev/ttyS1"
         baudrate: 115200
     - id: nx_udp1
       type: nxscope_udp
       config:
         iobind2:
           - id: io1
             name: "a"
           - id: io2
             name: "f"
         port: 50000

Supported fields:

- ``config.iobind2``: required IO-to-channel mapping list for both variants.
  Each entry contains ``id`` and ``name``.
- ``config.path``: serial device path for ``nxscope_serial`` only.
- ``config.baudrate``: serial baudrate for ``nxscope_serial`` only.
- ``config.port``: UDP local port for ``nxscope_udp`` only.

External Control
================

ControlIO: supported.

``CProtoNxscope`` supports runtime start/stop control through ``CIOControl``.
When stopped, NxScope transport/stream processing is inactive. When started
again, streaming resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoNxscope <../../doxygen/classdawn_1_1CProtoNxscope.html>`_

- `dawn::CProtoNxscopeDummy <../../doxygen/classdawn_1_1CProtoNxscopeDummy.html>`_

- `dawn::CProtoNxscopeSerial <../../doxygen/classdawn_1_1CProtoNxscopeSerial.html>`_
