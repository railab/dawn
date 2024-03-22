====================
Descriptor Selector
====================

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIODescSelector`` is an IO class used to select the active descriptor slot
at runtime.

``CIODescSelector`` is a read-write control IO:

- Read returns the currently active descriptor slot index.
- Write requests a slot switch.

Before accepting a switch request, Dawn validates the target slot descriptor
binary (CRC + structure validation). Invalid slots are rejected.

Implementation
==============

- Read returns active slot index as ``uint32``.
- Write validates target descriptor slot then requests switch.
- Writing the currently active slot is a no-op and returns success.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_DESC_SELECTOR``: enables descriptor selector IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: descselector0
       type: descselector
       dtype: uint32
       rw: true

Configuration Fields
--------------------

``CIODescSelector`` has no class-specific configuration fields.
It uses only common IO fields.

- ``id``: IO identifier.
- ``type``: must be ``descselector``.
- ``dtype``: use ``uint32``.
- ``rw``: must be ``true`` (selector is read/write).
- ``config.device``: optional common field, defaults to instance index.

Example with explicit device:

.. code-block:: yaml

   ios:
     - id: descselector0
       type: descselector
       dtype: uint32
       rw: true
       config:
         device: 0

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Add optional access-control policy for slot switching.
- Extend telemetry with last-switch reason/error code for diagnostics.

Doxygen
=======

- `dawn::CIODescSelector
  <../../doxygen/classdawn_1_1CIODescSelector.html>`_
