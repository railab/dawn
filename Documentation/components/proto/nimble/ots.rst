=====================
OTS - Object Transfer
=====================

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphOts`` - Bluetooth SIG Object Transfer Service v1.0
(UUID ``0x1825``).

Implementation
==============

- GATT: 8 mandatory chrs (Feature, Name, Type, Size, ID, Properties, OACP,
  OLCP).
- Bulk data: L2CAP CoC server on PSM ``0x0025``, MPS 512.
- Backing IO must be seekable. ``createOTS`` rejects any bound IO whose
  ``isSeekable()`` returns ``false`` (inverse of the AIOS rule).
  Compatible: ``CIOFile``, ``CIODescriptor``, ``CIOCapabilities``.

Supported opcodes:

- OACP: Read (0x04), Write (0x06), Abort (0x07).
- OLCP: First, Last, Previous, Next, Go To.
- Everything else returns ``OPCODE_NOT_SUPPORTED``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_OTS``: enables the Object Transfer Service.
  Depends on ``CONFIG_DAWN_IO_SEEKABLE``.
- ``CONFIG_NIMBLE_L2CAP_COC_MAX_NUM`` must be at least ``1`` so NimBLE
  compiles in L2CAP CoC support.

YAML
----

.. code-block:: yaml

   services:
     ots:
       objects:
         - name: some_file_ro
           type: file            # file | descriptor | capabilities
           access: read          # read | write | rw
           io: *id_file_ro

Supported fields per object:

- ``name``: object name (max 16 bytes).
- ``type``: ``file``, ``descriptor``, or ``capabilities``.
- ``access``: ``read``, ``write``, or ``rw``.
- ``io``: IO reference. Must resolve to a seekable IO.

Limitations
===========

- One transfer per connection at a time.
- Optional characteristics (Object Changed, List Filter, First-Created,
  Last-Modified) not implemented.
- No bonded-only access enforcement.

Doxygen
=======

- `dawn::CProtoNimblePrphOts <../../../doxygen/classdawn_1_1CProtoNimblePrphOts.html>`_
