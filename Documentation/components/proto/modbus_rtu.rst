===================
Modbus RTU Protocol
===================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoModbusRtu`` is a Modbus RTU slave protocol object that maps
FreeModbus callbacks to Dawn IO.

Implementation
==============

- Supported register group types:
  ``MODBUS_TYPE_COIL``, ``MODBUS_TYPE_COIL_PACKED``,
  ``MODBUS_TYPE_DISCRETE``, ``MODBUS_TYPE_DISCRETE_PACKED``,
  ``MODBUS_TYPE_INPUT``, ``MODBUS_TYPE_HOLDING``.

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
             notify: 0
             start: 0
             bindings:
               - coil1
               - coil2
           - type: discrete_packed
             notify: 1
             start: 100
             bindings:
               - di1
               - di2
               - di3
           - type: holding
             notify: 0
             start: 1000
             bindings:
               - holding1
           - type: input
             notify: 0
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

Supported ``registers[*]`` fields:

- ``type``: register group type.
- ``notify``: enable change notification shadowing for the group.
- ``start``: first register or bit address.
- ``bindings``: IOs mapped into the group.

External Control
================

ControlIO: supported.

``CProtoModbusRtu`` supports runtime start/stop control through ``CIOControl``.
When stopped, Modbus polling/service loop is inactive. When started again,
register servicing resumes.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Only one Modbus instance can be active at a time
  (global FreeModbus callback limitation).

- Write NuttX Modbus support from scratch with multiple instances support and
  ready for Modbus TCP

Doxygen
=======

- `dawn::CProtoModbusRtu <../../doxygen/classdawn_1_1CProtoModbusRtu.html>`_

- `dawn::CProtoModbusRegs <../../doxygen/classdawn_1_1CProtoModbusRegs.html>`_
