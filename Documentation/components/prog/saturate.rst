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
- Bounds are decoded once into the source type and every sample is clamped
  in that type.
- The output type may differ from the input type; the clamp makes a
  narrowing store safe. ``b16`` is clamped as raw fixed point and cannot be
  mixed with other types.
- Either bound may be omitted; that side then defaults to the narrower of
  the input and output type limits.
- A NaN input is stored as ``min`` in an integer output and passed through
  to a real output.
- A bound outside the input or output range, or ``min > max``, fails
  ``init()`` (``-ERANGE`` / ``-EINVAL``) rather than silently never firing.

Runtime configuration
=====================

``min`` and ``max`` are runtime-writable when their config items are marked
rw (a ``config`` IO targeting them). A write that would leave the pair
invalid is rejected and the active bounds are kept.

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
bounds fit both the source and the output ranges.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProgSaturate <../../doxygen/classdawn_1_1CProgSaturate.html>`_
