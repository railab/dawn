.. _stats_max:

===============
Statistics: Max
===============

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgStatsMax`` tracks the maximum value seen on a source IO and
exposes the result through an output IO.

Implementation
==============

This program is a template-based processor that inherits from
:ref:`CProgProcessTemplate <process>`. It uses the ``StatsOpMax`` policy
to perform element-wise comparisons on incoming data.

On every new sample from the source IO:

1. If it is the first sample since start or reset, the result is initialized
   directly from the input.
2. Each element of the incoming sample is compared against the stored maximum;
   if the new value is larger, the maximum is updated.
3. If any change occurred, the updated maximum is written to the output IO.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_STATS_MAX``: enables the stats-maximum program.

YAML
----

.. code-block:: yaml

   programs:
     - id: statsmax1
       type: statsmax
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
- ``TriggerIO``: supported. ``CMD_RESET`` clears the current maximum and
  initializes it from the next available sample.

Doxygen
=======

- `dawn::CProgStatsMax <../../doxygen/classdawn_1_1CProgStatsMax.html>`_
