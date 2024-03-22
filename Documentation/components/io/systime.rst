=======
Systime
=======

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIOSystime`` exposes system time through an IO object.

The documented representation is a ``uint64`` timestamp value.

Implementation
==============

``CIOSystime`` reads the wall-clock time via ``clock_gettime(CLOCK_REALTIME)``
and returns it as nanoseconds since the epoch in a single ``uint64_t``
slot. ``setData()`` writes a new ``CLOCK_REALTIME`` value with
``clock_settime``, allowing the host to be re-synced at runtime. The IO is
read-write, single-sample only (multi-sample reads return ``-ENOTSUP``),
and does not produce notifications.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_SYSTIME``: enables the system-time IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: systime1
       type: systime
       dtype: uint64
       rw: false

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOSystime <../../doxygen/classdawn_1_1CIOSystime.html>`_
