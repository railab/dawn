=======
Sysinfo
=======

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOSysinfo`` provides read-only system metrics as IO values.

Documented variants are uptime and CPU load.

Implementation
==============

``CIOSysinfo`` calls ``sysinfo(3)`` and returns a class-specific view of the
result:

- ``IO_CLASS_SYSTEM_UPTIME``: 64-bit uptime in seconds (single ``uint64_t``).
- ``IO_CLASS_SYSTEM_CPULOAD``: the three NuttX load averages as ``float``
  (or ``ub16`` fixed-point when ``DTYPE_UB16`` is selected).

The IO is read-only and does not support batch operations.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_SYSINFO``: enables ``sysinfo`` IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: uptime1
       type: sysinfo
       dtype: float
       rw: false
       variant: uptime

     - id: cpuload1
       type: sysinfo
       dtype: float
       rw: false
       variant: cpuload

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOSysinfo <../../doxygen/classdawn_1_1CIOSysinfo.html>`_
