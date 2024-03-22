========
Redirect
========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgRedirect`` routes data from source IOs to destination IOs.

- supports multiple bindings in one instance (``N -> N``)
- notify-only: source notification triggers an immediate write to output
- useful for testing/HIL flows like ``ADC -> DAC`` or ``GPI -> GPO``
- use ``sampling`` before ``redirect`` when fetch-based polling is needed

Implementation
==============

``CProgRedirect`` runs in the notify/callback processing path and forwards
incoming source samples directly to their destination IOs.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_REDIRECT``: enables the Redirect program.

YAML
----

.. code-block:: yaml

   programs:
     - id: redirect1
       type: redirect
       config:
         iobind:
           - src1
           - dst1

``IOBIND`` uses packed 2-word tuples:

- ``src``: source IO object ID
- ``dst``: destination IO object ID

External Control
================

ControlIO: supported.

``CProgRedirect`` supports runtime start/stop control through ``CIOControl``.
When stopped, forwarding is paused. When started again, source notifications
are forwarded to destination IOs.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Source and destination should use the same data type and dimension.
- Redirect to ``virt`` destinations is supported; ``redirect`` initializes or
  reuses the destination ``virt`` from the source shape during ``init()``.

Doxygen
=======

- `dawn::CProgRedirect <../../doxygen/classdawn_1_1CProgRedirect.html>`_
