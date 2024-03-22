.. _stats_count:

=================
Statistics: Count
=================

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgStatsCount`` counts received source IO samples and exposes the
current count through an output IO.

Implementation
==============

This program is a template-based processor that inherits from
:ref:`CProgProcessTemplate <process>`. It uses the ``StatsOpCount`` policy
to increment a counter for every sample received.

On every new sample from the source IO:

1. If it is the first sample since start or reset, the result is initialized
   (typically starts from 1 if configured as count).
2. For each element in the input sample, the corresponding counter in the
   output buffer is incremented by one.
3. The updated count is written to the output IO.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_STATS_COUNT``: enables the stats-count program.

YAML
----

.. code-block:: yaml

   programs:
     - id: statscount1
       type: statscount
       config:
         iobind:
           - sensor1
           - virt1

The ``iobind`` field takes a list where the first element is the source IO
and the second is the output IO to store the result.

External Control
================

- ``ControlIO``: supported. When stopped, incoming samples are ignored and
  the counter is frozen.
- ``TriggerIO``: supported. ``CMD_RESET`` resets the counter to zero.

Doxygen
=======

- `dawn::CProgStatsCount <../../doxygen/classdawn_1_1CProgStatsCount.html>`_
