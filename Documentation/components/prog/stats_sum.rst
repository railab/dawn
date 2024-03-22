.. _stats_sum:

===============
Statistics: Sum
===============

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgStatsSum`` accumulates source IO samples and exposes the current
sum through an output IO.

Implementation
==============

This program is a template-based processor that inherits from
:ref:`CProgProcessTemplate <process>`. It uses the ``StatsOpSum`` policy
to accumulate incoming values into a running total.

On every new sample from the source IO:

1. If it is the first sample since start or reset, the result is initialized
   directly from the input.
2. For each element, the new sample is added to the running total.
3. The updated sum is written to the output IO.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_STATS_SUM``: enables the stats-sum program.

YAML
----

.. code-block:: yaml

   programs:
     - id: statssum1
       type: statssum
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
- ``TriggerIO``: supported. ``CMD_RESET`` clears the current sum and
  initializes it from the next available sample.

Doxygen
=======

- `dawn::CProgStatsSum <../../doxygen/classdawn_1_1CProgStatsSum.html>`_
