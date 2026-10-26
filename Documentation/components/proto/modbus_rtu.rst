===================
Modbus RTU Protocol
===================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoModbusRtu`` is a Modbus RTU slave protocol object that maps
NxModbus callbacks to Dawn IO.

Implementation
==============

- Supported register group types:
  ``MODBUS_TYPE_COIL``, ``MODBUS_TYPE_COIL_PACKED``,
  ``MODBUS_TYPE_DISCRETE``, ``MODBUS_TYPE_DISCRETE_PACKED``,
  ``MODBUS_TYPE_INPUT``, ``MODBUS_TYPE_HOLDING``, ``MODBUS_TYPE_SEEKABLE``.

- Overlap in the same register class is rejected at configuration
  (assert + error return).

- ``MODBUS_TYPE_COIL_PACKED`` uses Modbus bit encoding for bool IO:
  0 -> bit 0, nonzero -> bit 1.

- ``MODBUS_TYPE_INPUT`` and ``MODBUS_TYPE_HOLDING`` support mixed-width
  IO (16/32/64-bit) inside one group.
- Multi-register scalar values are transferred as one contiguous register
  payload in big-endian register order, so 32/64-bit IO values preserve their
  numeric value when written and read back.
- Multi-dimensional IO is flattened element-by-element in declaration order;
  the protocol is responsible for reconstructing the typed payload on reads
  and splitting it back into registers on writes.
- Endianness and packing are Modbus protocol rules, not Dawn internal layout
  rules. External clients should mirror this wire contract.

- ``coil`` and ``holding`` groups may bind write-only Dawn IOs. In that
  case Modbus reads return the protocol's cached last written value because
  the target IO itself has no read path.

- Register shadow buffer is also used for write-only Modbus outputs.

.. _modbus-seekable-files:

Seekable IO and file records
----------------------------

``type: seekable`` and ``MODBUS_TYPE_SEEKABLE = 7`` remain the public
mapping for seekable IO. NxModbus supplies Read File Record (FC20,
``0x14``) and Write File Record (FC21, ``0x15``), including PDU parsing,
validation, responses, and exceptions. Dawn registers ``file_cb`` and maps
its file and record arguments to the IO's existing offset read/write API.
Reference type is 6. Both RTU and TCP use this implementation.

Existing descriptor fields and binding addresses are retained::

   registers:
     - type: seekable
       start: 4096
       config: 256
       bindings: [descriptor0, descriptor1, capabilities0]

``config`` is the window width in words (8 when zero or omitted). Each IO
reserves ``config + 1`` positions, so these binding addresses remain 4096,
4353, and 4610. Seekable allocations occupy file-number space independently
of coils and registers. Overlapping seekable allocations are rejected.

For a binding at zero-based address ``A``, file ``A + 1`` starts with IO
byte zero. Each reserved file carries 10,000 two-byte records. Subsequent
files extend the same IO's byte range, preserving access to IOs larger than
20,000 bytes. The offset for a request is::

   byte_offset = (file_number - A - 1) * 20000 + record_number * 2

Each request must stay within one file; clients split requests at file
boundaries. All allocated file numbers must fit 1--65535. The backing IO
must fit the allocation. This is Dawn's mapping of IOs into the file-number
space served by the standard functions.

File-record payloads contain IO bytes in their original order. For example,
IO bytes ``12 34`` are carried in the record ``0x1234``. Reads zero-fill
unused bytes and records past the IO's current end, within its allocation,
preserving the previous seekable window behavior. Each request specifies
its offset; reads do not advance a shared server cursor.

Writes carry complete two-byte records and call the IO's existing offset
write method. FileIO and writable descriptor slots start a replacement on
a write at offset zero. FileIO write-once permissions still lock after the
first write. Errors from the backing IO are returned through NxModbus.
FC20/FC21 do not convey an exact byte length, truncate an empty file, or
provide begin/finish/abort operations. Applications transferring an odd
number of bytes must account for the final record's padding in their data
format; both bytes of a write record are passed to the IO.

NxModbus supports at most 121 records per single-group FC20 request and
122 per FC21 request. The host helper uses 120-record writes because the
current NxModbus RTU receiver drops an exactly full 256-byte frame.

``CONFIG_DAWN_PROTO_MODBUS_SEEKABLE`` automatically enables the NxModbus
FC20/FC21 handlers when Modbus and seekable IO are enabled. Update firmware
and clients together: seekable wire access now uses file records instead
of holding-register windows and seek-control writes. The Python console
keeps its window-sized results and tracks its cursor locally; see
:doc:`../../tools/dawnpy-modbus`.

Register Management
-------------------

The mapping between Dawn IO objects and Modbus registers is handled by the
``CProtoModbusRegs`` base class. This logic is shared across all Modbus
transport implementations (RTU, TCP).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_MODBUS``: enables Modbus protocol support.
- ``CONFIG_DAWN_PROTO_MODBUS_RTU``: enables the RTU transport.
- ``CONFIG_DAWN_PROTO_MODBUS_RTU_PATH``: default RTU device path.
- ``CONFIG_DAWN_PROTO_MODBUS_RTU_BAUD``: default RTU baudrate.
- ``CONFIG_DAWN_PROTO_MODBUS_RTU_ADDR``: default Modbus slave address.
- ``CONFIG_DAWN_PROTO_MODBUS_RTU_PARITY``: default RTU parity mode.

YAML
----

.. code-block:: yaml

   protocols:
     - id: modbus1
       type: modbus_rtu
       config:
         path: "/dev/ttyS1"
         registers:
           - type: coil
             config: 0
             start: 0
             bindings:
               - coil1
               - coil2
           - type: discrete_packed
             config: 0
             start: 100
             bindings:
               - di1
               - di2
               - di3
           - type: holding
             config: 0
             start: 1000
             bindings:
               - holding1
           - type: input
             config: 0
             start: 2000
             bindings:
               - input1

Supported fields:

- ``config.path``: serial device path.
- ``config.registers``: register group definitions.

Supported ``registers[*].type`` values:

- ``coil``
- ``discrete``
- ``coil_packed``
- ``discrete_packed``
- ``input``
- ``holding``
- ``seekable``

Supported ``registers[*]`` fields:

- ``type``: register group type.
- ``config``: private group configuration; window width for ``seekable``.
- ``start``: first register/bit address, or zero-based seekable allocation address.
- ``bindings``: IOs mapped into the group.

External Control
================

ControlIO: supported.

``CProtoModbusRtu`` supports runtime start/stop control through ``CIOControl``.
When stopped, Modbus polling/service loop is inactive. When started again,
register servicing resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoModbusRtu <../../doxygen/classdawn_1_1CProtoModbusRtu.html>`_

- `dawn::CProtoModbusRegs <../../doxygen/classdawn_1_1CProtoModbusRegs.html>`_
