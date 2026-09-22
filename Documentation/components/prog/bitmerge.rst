=========
Bit Merge
=========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgBitMerge`` packs masked, shifted input values into one integer output
word::

   out = OR over inputs of ((in & mask) << shift)

Its purpose is tagging a sample stream: one batched input (e.g. a 12-bit ADC
stream) provides the payload while slow inputs (range, state bits) are folded
into the spare bits of every sample.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Inputs and output are 8/16/32-bit integer or ``bool`` IOs. Fields are
  merged in a 32-bit word, so wider types are rejected at ``init()``.
- At most one input may be batched. It is the stream: it must be
  notify-capable, it drives the output rate and shape (dimension and batch),
  and the output element size must match it. The remaining inputs are slow
  operands: scalar (dimension 1), read once per batch.
- Without a batched input the output is a scalar rebuilt whenever a
  notify-capable input changes and once at start.
- Every field must fit the real output width (not just 32 bits), have a
  non-empty mask and not overlap another field; anything else fails
  ``init()`` instead of silently truncating.
- The output is producer-owned by ``bitmerge``: a deferred ``virt`` output is
  initialized with the stream shape and notify support.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_BITMERGE``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: cur_tag
       type: bitmerge
       config:
         inputs:
           - io: cur_sat       # batched int16 stream, 12-bit payload
             shift: 0
             mask: 0x0fff
           - io: range_bits    # slow input, folded into bits 12..14
             shift: 12
             mask: 0x7
         output: cur_tagged

``dawnpy`` validates dtypes, field layout and the output element size at
descriptor build time. It only recognizes a batch declared on the input's
own ``notify.batch``: a ``virt`` batched by its producer (e.g. a
``saturate`` output fed from a batched stream) is not seen as batched, so
for such inputs the one-batched-input and output-size rules are enforced
by ``init()`` only.

Doxygen
=======

- `dawn::CProgBitMerge <../../doxygen/classdawn_1_1CProgBitMerge.html>`_
