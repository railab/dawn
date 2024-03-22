=============
Descriptor IO
=============

**Component Type:** Input/Output

**Status:** In Progress

Overview
========

``CIODescriptor`` is a special-purpose IO dedicated to accessing
:doc:`/components/common/descriptor`.

The implementation supports descriptor read access for all slots and
seeked/chunked write access for RAM slots.

Implementation
==============

- ``device: 0`` maps to slot 0 (boot descriptor, read-only).
- ``device: 1..N-1`` maps to writable RAM descriptor slots.
- Write path uses seeked writes so hosts can upload large descriptors
  chunk-by-chunk.

Validation of uploaded descriptors is not performed on write. Validation is
performed when a switch is requested via ``CIODescSelector``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_DESCRIPTOR``: enables descriptor-backed IO access.
  Depends on ``CONFIG_DAWN_IO_SEEKABLE``.

YAML
----

.. code-block:: yaml

   ios:
     - id: descriptor1
       type: descriptor
       dtype: uint32
       rw: true
       config:
         device: 1

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

See Also
========

- :doc:`descselector`

Brainstorming & Future Ideas
============================

- Permanent storage support
- Authorization support

Doxygen
=======

- `dawn::CIODescriptor <../../doxygen/classdawn_1_1CIODescriptor.html>`_
