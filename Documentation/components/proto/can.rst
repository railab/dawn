===================
CAN Simple Protocol
===================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoCan`` is a CAN protocol object that provides multiple methods for
accessing IO objects over a CAN bus. It supports standard 11-bit and
extended 29-bit identifiers, as well as CAN FD frames.

Implementation
==============

Supported endpoints:

#. input read with notify (async read)

   - send notification with CAN ID = (nodeId + offset)

#. input read with request (sync read)

   - the same CAN ID for request and response with RTR frames

#. ouput write

   - write data from message with CAN ID = (nodeID + offset)

#. segmented read/write (long data)

   - CAN_TYPE_READ_SEG / CAN_TYPE_WRITE_SEG
   - ``READ_SEG`` request uses a data frame (``rtr=0``), typically empty payload
   - uses seg header in data[0]
   - supports payloads larger than 8B

#. indexed single-ID read/write (complex interaction)

   - CAN_TYPE_INDEXED_READ / CAN_TYPE_INDEXED_WRITE
   - one CAN ID per group
   - request format: data[0]=seg, data[1]=index
   - index 1..N selects object, 0 selects all (read or write-all)
   - responses include data[0]=seg, data[1]=index

Segmented Transfers (ISO-TP)
----------------------------

For data that exceeds the size of a single CAN frame, Dawn uses the
``CIsoTp`` helper class. This implements a subset of the ISO 15765-2
standard:

- **Single Frame (SF)**: For data up to 7 bytes.
- **First Frame (FF)**: Indicates the start of a multi-frame transfer and
  total length.
- **Consecutive Frame (CF)**: Carries the remaining data segments with a
  sequence counter.

*Note: The current implementation does not use Flow Control (FC) frames
and relies on the hardware's reliability.*

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_CAN``: enables the CAN protocol.
- ``CONFIG_DAWN_PROTO_CAN_NODEID``: default node ID for CAN bindings.
- ``CONFIG_DAWN_PROTO_CAN_EXTID``: enables 29-bit CAN identifiers.
- ``CONFIG_DAWN_PROTO_CAN_CANFD``: enables CAN FD frames.
- ``CONFIG_DAWN_PROTO_CAN_RTR``: enables RTR-based read requests.
- ``CONFIG_DAWN_PROTO_CAN_SIMPLE``: per-IO CAN IDs
- ``CONFIG_DAWN_PROTO_CAN_SEG``: segmented access methods (ISO-TP format)
- ``CONFIG_DAWN_PROTO_CAN_SEG_TIMEOUT_US``: timeout for segmented transfers
- ``CONFIG_DAWN_PROTO_CAN_SINGLE_ID``: indexed single-ID access methods

YAML
----

.. code-block:: yaml

   protocols:
     - id: can1
       type: can
       config:
         node_id: 0x120
         objects:
           - type: push          # default if omitted
             can_id_start: 0x180
             bindings:
               - io_push1
           - type: read
             can_id_start: 0x200
             bindings:
               - io_read1
           - type: write
             can_id_start: 0x300
             bindings:
               - io_write1
           - type: read_indexed
             can_id_start: 0x380
             bindings:
               - io_idx1
               - io_idx2
           - type: write_indexed
             can_id_start: 0x390
             bindings:
               - io_idx1
               - io_idx2
           - type: read_seg
             can_id_start: 0x3a0
             bindings:
               - io_seg_read1
               - io_seg_read2
           - type: write_seg
             can_id_start: 0x3b0
             bindings:
               - io_seg_write1

Supported ``objects[*]`` fields:

- ``type``: ``push``, ``read``, ``write``, ``read_indexed``,
  ``write_indexed``, ``read_seg``, or ``write_seg``.
- ``can_id_start``: first CAN ID used by the object.
- ``bindings``: IOs exposed by the object.

External Control
================

ControlIO: supported.

``CProtoCan`` supports runtime start/stop control through ``CIOControl``.
When stopped, CAN processing thread and register handling are inactive.
When started again, protocol servicing resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoCan <../../doxygen/classdawn_1_1CProtoCan.html>`_

- `dawn::CIsoTp <../../doxygen/classdawn_1_1CIsoTp.html>`_
