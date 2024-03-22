.. _stats_avg:

===============
Statistics: Avg
===============

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgStatsAvg`` computes a running average from source IO samples and
exposes the result through an output IO.

Implementation
==============

This program is a template-based processor that inherits from
:ref:`CProgProcessTemplate <process>`. It uses the ``StatsOpAvg`` policy
to perform element-wise running average computation.

On every new sample from the source IO:

1. If it is the first sample since start or reset, the result is initialized
   directly from the input.
2. For each element, a simple running average is calculated:
   ``average = (current_average + new_sample) / 2``.
3. The updated average is written to the output IO.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_STATS_AVG``: enables the stats-average program.

YAML
----

.. code-block:: yaml

   programs:
     - id: statsavg1
       type: statsavg
       config:
         iobind:
           - sensor1
           - virt1

The ``iobind`` field takes a list where the first element is the source IO
and the second is the output IO to store the result.

External Control
================

- ``ControlIO``: supported. When stopped, incoming samples are ignored and
  the current result is frozen.
- ``TriggerIO``: supported. ``CMD_RESET`` clears the current average and
  initializes it from the next available sample.

Doxygen
=======

- `dawn::CProgStatsAvg <../../doxygen/classdawn_1_1CProgStatsAvg.html>`_
