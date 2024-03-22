.. _stats_rms:

===============
Statistics: RMS
===============

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgStatsRms`` computes a running Root Mean Square (RMS) from source
IO samples and exposes the result through an output IO.

Implementation
==============

Unlike simpler statistics programs, ``CProgStatsRms`` does not use a
template policy. It inherits directly from :ref:`CProgProcess <process>`
and maintains internal state for each bound output IO:

- **Sum of Squares**: Stores the accumulated squared values (``double``
  for integers, ``float`` for floating-point).
- **Sample Count**: Tracks the total number of samples processed.

On every new sample:

1. The sample count is incremented.
2. Each element of the incoming sample is squared and added to the
   accumulated sum of squares.
3. The RMS is calculated as: ``sqrt(sum_of_squares / count)``.
4. For integer IO types, the result is safely cast back, with clipping
   to the type's maximum value if necessary.
5. The updated RMS is written to the output IO.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_STATS_RMS``: enables the stats-RMS program.

YAML
----

.. code-block:: yaml

   programs:
     - id: statsrms1
       type: statsrms
       config:
         iobind:
           - sensor1
           - virt1

The ``iobind`` field takes a list where the first element is the source IO
and the second is the output IO to store the result.

External Control
================

- ``ControlIO``: supported. When stopped, incoming samples are ignored and
  the current RMS value is frozen.
- ``TriggerIO``: supported. ``CMD_RESET`` clears the accumulated sum of
  squares and resets the sample count to zero.

Brainstorming & Future Ideas
----------------------------

- Currently uses ``std::sqrt``, which may be expensive on systems without
  an FPU.
- Integer IO types are converted internally to double-precision during
  accumulation to avoid overflow, then cast back to the destination type.

Doxygen
=======

- `dawn::CProgStatsRms <../../doxygen/classdawn_1_1CProgStatsRms.html>`_
