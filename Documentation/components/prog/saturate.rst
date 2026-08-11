========
Saturate
========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgSaturate`` is a saturating limiter: it clamps every input sample into
a configured ``[min, max]`` range and writes the result to an output IO.
Unlike IO limits, which reject out-of-range config writes, it conditions the
data itself.

Implementation
==============

``CProgSaturate`` is a :doc:`process` program: each notify-capable source
is clamped into its output on every notification, batches included. A
deferred ``virt`` output takes the source's dimension, batch and notify
support; other outputs get the latest sample of a batch.

- Supported types: ``bool``, 8/16/32-bit integers, ``float``, ``double`` and
  ``b16``. Bounds are encoded in the source type: two's complement or plain
  words for integers, a float word for real types, a 16.16 word for ``b16``.
- The output type must match the input type; a mismatched output is not
  written.
- Either bound may be omitted; that side then defaults to the type limit.
- A NaN input passes through to the output.

Runtime configuration
=====================

``min`` and ``max`` are runtime-writable when their config items are marked
rw (a ``config`` IO targeting them); a new bound applies from the next
sample.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_SATURATE``: enables the Saturate program (requires
  ``CONFIG_DAWN_DTYPE_NUMERIC``).

YAML
----

.. code-block:: yaml

   programs:
     - id: cur_sat
       type: saturate
       dtype: int32        # must match the source dtype
       config:
         sources: [cur]
         outputs: [cur_sat]
         min: 0
         max: 0x0fff

``dawnpy`` checks that the program dtype matches the source dtype and that the
bounds fit its range.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProgSaturate <../../doxygen/classdawn_1_1CProgSaturate.html>`_
