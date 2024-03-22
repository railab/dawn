======================
Moving Average Filter
======================

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgMovingAverage`` computes a sliding-window arithmetic mean from
notified input samples and publishes the filtered result through a
output IO.

- notify-driven processing (via ``CProgProcess``)
- supports multiple source/output bindings in one instance (``N -> N``)
- resettable with ``CMD_RESET``

Implementation
==============

``CProgMovingAverage`` runs in the notify/callback processing path and updates
the filtered output for each incoming source sample.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_MOVING_AVG``: enables the moving-average program.

YAML
----

.. code-block:: yaml

   programs:
     - id: movingavg1
       type: movingavg
       config:
         iobind:
           - src1
           - virt1
         window: 8

Uses a packed ``IOBIND`` list of source/output pairs (same pattern as other
process-style programs):

- source IO (notify-capable)
- destination output IO (filtered output)

Additional configuration parameters:

- ``window``: sliding average window length in samples (must be ``> 0``)

External Control
================

ControlIO: supported.

``CProgMovingAverage`` supports runtime start/stop control through
``CIOControl``. When stopped, incoming samples are ignored and the current
output is frozen. When started again, filter updates resume.

TriggerIO: supported for ``reset``.

Brainstorming & Future Ideas
============================

- Supports ``int32``, ``uint32``, and ``float``
- Integer outputs use integer division (truncation)
- Use ``sampling`` before ``movingavg`` when the source IO only supports fetch

Doxygen
=======

- `dawn::CProgMovingAverage <../../doxygen/classdawn_1_1CProgMovingAverage.html>`_
