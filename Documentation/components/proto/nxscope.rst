================
NxScope Protocol
================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoNxscope`` is an NxScope protocol object for data streaming and
inspection.

User Extension (Set/Get IO)
===========================

Dawn extends NxScope via ``nxscope_callbacks_s.userid`` with user frame IDs
for on-demand IO access. The commands ride in standard NxScope frames, so
they work with every Dawn transport (``nxscope_dummy``, ``nxscope_serial``,
``nxscope_udp``).

Requests (little-endian, ``USER`` = ``NXSCOPE_HDRID_USER`` = 8):

.. list-table::
   :widths: 20 20 60
   :header-rows: 1

   * - ID
     - Name
     - Payload
   * - ``USER``
     - SET_IO
     - ``[objid:4][size:2][data:size]``
   * - ``USER+1``
     - SET_IO_SEEK
     - ``[objid:4][offset:4][size:2][data:size]``
   * - ``USER+2``
     - GET_IO
     - ``[objid:4]``
   * - ``USER+3``
     - GET_IO_SEEK
     - ``[objid:4][offset:4][size:2]``

Response: both GET requests answer with one frame ``id=USER+2`` carrying
``[objid:4][size:2][data:size]``. It is sent **before** the ACK (when
``CONFIG_LOGGING_NXSCOPE_ACKFRAMES`` is enabled), so a client must consume
the data frame first and then the ACK carrying the handler return code.
Set requests get only the ACK.

Behavior notes:

- SET requires ``isWrite()``, GET requires ``isRead()``; the ``*_SEEK`` forms
  require ``isSeekable()`` and the plain forms reject seekable IOs.
- GET_IO returns the whole IO (``getDataSize()`` bytes). GET_IO_SEEK
  returns ``size`` bytes, at most ``CONFIG_DAWN_PROTO_NXSCOPE_RXBUF_LEN``.
- The response buffer is sized at ``init()`` for the largest bound
  readable IO, so large reads never fail with ``-ENOBUFS``.
- GET_IO_SEEK rejects a window past the end of the IO
  (``offset + size > getDataSize()``) with ``-EINVAL``.
- Get-only channels: a readable IO without notify support stays bound and
  answers GET_IO instead of failing init; it is not an NxScope stream
  channel. Writable IOs fall back to set-only the same way. Both log a
  warning. A notify-capable channel whose notifier cannot be bound fails
  ``start()`` - it was already advertised as a stream channel.
- Stream channel types: UINT8, INT8, UINT16, INT16, INT32, UINT32, UINT64
  and FLOAT map to the matching ``NXSCOPE_TYPE_*``.
- A SET_IO to a stream channel may notify the stream path synchronously;
  the stream lock is recursive for that reason.

Callback path: ``nxscope_callbacks_s.userid`` ->
``CProtoNxscope::userIdCb()`` -> ``CProtoNxscope::handleUserCommand()``.

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
- ``CONFIG_DAWN_PROTO_NXSCOPE_RXBUF_LEN``: receive buffer length (also the
  GET_IO_SEEK chunk limit).
- ``CONFIG_DAWN_PROTO_NXSCOPE_RECV_INTERVAL``: recv thread poll interval in
  microseconds (idle timeout in notify mode).
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
