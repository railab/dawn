============
Capabilities
============

**Component Type:** Input/Output

**Status:** Implemented

``CIOCapabilities`` is a read-only, seekable IO class that exposes compiled
Dawn capability bitmaps and metadata in one fixed-size binary blob.

Overview
========

Capabilities are exported as a single fixed-size blob:

+---------+-----------+------------------------------------------------------+
| Section | Size      | Contents                                             |
+=========+===========+======================================================+
| Header  | 8 bytes   | Blob version, layout ID, payload length, and         |
|         |           | reserved field.                                      |
+---------+-----------+------------------------------------------------------+
| Payload | 504 bytes | IO, PROG, and PROTO class bitmaps plus metadata.     |
+---------+-----------+------------------------------------------------------+
| Total   | 512 bytes | Complete value returned by ``CIOCapabilities``.      |
+---------+-----------+------------------------------------------------------+

Payload layout:

+--------+----------------+-----------+------------------------------------------------------+
| Offset | Section        | Size      | Contents                                             |
+========+================+===========+======================================================+
| 0      | IO bitmap      | 64 bytes  | Enabled ``CIOCommon::IO_CLASS_*`` IDs.               |
+--------+----------------+-----------+------------------------------------------------------+
| 64     | PROG bitmap    | 64 bytes  | Enabled ``CProgCommon::PROG_CLASS_*`` IDs.           |
+--------+----------------+-----------+------------------------------------------------------+
| 128    | PROTO bitmap   | 64 bytes  | Enabled ``CProtoCommon::PROTO_CLASS_*`` IDs.         |
+--------+----------------+-----------+------------------------------------------------------+
| 192    | Metadata       | 312 bytes | Extended metadata words; undefined words are         |
|        |                |           | reserved.                                            |
+--------+----------------+-----------+------------------------------------------------------+

The blob advertises which object classes the running firmware supports.
Protocol clients and host tools can use it to decide whether a descriptor is
compatible with the target before trying to instantiate or bind objects.

Implementation
==============

- Read-only IO, seekable access supported.
- Returns fixed 512-byte blob:
  8-byte header + 504-byte payload.
- Payload packs IO/PROG/PROTO class bitmaps and extended metadata.

Header
------

The 8-byte header is:

+--------+----------------+---------+-----------------------+
| Offset | Field          | Size    | Value                 |
+========+================+=========+=======================+
| 0      | Version        | 1 byte  | ``2``                 |
+--------+----------------+---------+-----------------------+
| 1      | Layout ID      | 1 byte  | ``0``                 |
+--------+----------------+---------+-----------------------+
| 2      | Payload length | 2 bytes | Little-endian ``504`` |
+--------+----------------+---------+-----------------------+
| 4      | Reserved       | 4 bytes | Must be ``0``         |
+--------+----------------+---------+-----------------------+

Metadata
--------

Version 2 metadata words are little-endian 32-bit values:

+-------+----------------------+----------------+----------------------------------------------------------+
| Word  | Field                | Size           | Meaning                                                  |
+=======+======================+================+==========================================================+
| 0-1   | dtype bitmap         | 64 bits        | Enabled ``SObjectId::DTYPE_*`` values.                   |
+-------+----------------------+----------------+----------------------------------------------------------+
| 2-3   | IO flag bitmap       | 64 bits        | Common IO feature flags, currently timestamp support.    |
+-------+----------------------+----------------+----------------------------------------------------------+
| 4-5   | build flag bitmap    | 64 bits        | Build features: NuttX, filesystem, IO notify, IO stats,  |
|       |                      |                | object names, and dynamic descriptor slots.              |
+-------+----------------------+----------------+----------------------------------------------------------+
| 6     | descriptor slots     | 32 bits        | ``CONFIG_DAWN_DESC_SLOTS``.                              |
+-------+----------------------+----------------+----------------------------------------------------------+
| 7     | descriptor slot size | 32 bits        | ``CONFIG_DAWN_DESC_SLOT_SIZE``.                          |
+-------+----------------------+----------------+----------------------------------------------------------+
| 8     | max IO class ID      | 32 bits        | Highest class ID representable by the IO bitmap.         |
+-------+----------------------+----------------+----------------------------------------------------------+
| 9     | max PROG class ID    | 32 bits        | Highest class ID representable by the PROG bitmap.       |
+-------+----------------------+----------------+----------------------------------------------------------+
| 10    | max PROTO class ID   | 32 bits        | Highest class ID representable by the PROTO bitmap.      |
+-------+----------------------+----------------+----------------------------------------------------------+
| 11..  | reserved             | 32 bits each   | Currently ``0``.                                         |
+-------+----------------------+----------------+----------------------------------------------------------+

Maintenance contract
====================

Every new upstream object class must be represented in Capabilities IO. When
adding a new built-in IO, PROG, or PROTO class:

1. Add the class enum and factory case as usual.
2. Add the matching ``CONFIG_DAWN_*`` guarded ``setBitmapBit()`` call in
   ``dawn/src/io/capabilities.cxx``:

   - IO classes go in ``CIOCapabilities::buildIoBitmap()``.
   - PROG classes go in ``CIOCapabilities::buildProgBitmap()``.
   - PROTO classes go in ``CIOCapabilities::buildProtoBitmap()``.

3. Keep ``tools/dawnpy/src/dawnpy/descriptor/capabilities_blob.py`` in sync if
   the blob layout, metadata words, or flag meanings change.
4. Update the component documentation for the new object and mention any
   relevant capability dependencies.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_CAPABILITIES``: enables capabilities IO.
  Depends on ``CONFIG_DAWN_IO_SEEKABLE``.

YAML
----

.. code-block:: yaml

   ios:
     - id: capabilities0
       type: capabilities
       dtype: block

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Add ``dawnpy`` decode command for capabilities blob pretty-printing.
- Define and document semantics for currently reserved metadata words.

Doxygen
=======

- `dawn::CIOCapabilities
  <../../doxygen/classdawn_1_1CIOCapabilities.html>`_
