======
Buffer
======

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgBuffer`` stores notify-driven source samples in program-owned
history RAM and exposes selected samples or selected sample chunks plus
selector/status registers through program-owned virtual IO channels.

- ``src``: notify source
- ``out``: selected sample/chunk (same dtype as source)
- ``sel``: selector (``0`` newest, ``1`` previous, ...)
- ``stat``: status words (count/depth/head/overflow/flags)

Implementation
==============

``CProgBuffer`` is a notify-driven program that stores samples in internal RAM
and serves selected history entries on demand.

Virtual IO ownership is role-specific:

- ``src`` is a consumer-side input and must already have a valid fixed shape
  before ``buffer`` starts
- ``out``, ``sel``, and ``stat`` are output-side roles whose shape may be
  defined by ``buffer`` itself and may therefore target deferred ``virt``
  objects
- ``chunk_size`` controls how many source samples are exposed through ``out``
  per read; ``buffer`` initializes ``out`` with ``src.dim * chunk_size``
  elements

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_BUFFER``: enables the Buffer program.

YAML
----

.. code-block:: yaml

   programs:
     - id: buffer1
       type: buffer
       config:
         iobind:
           - src: src1
             out: out1
             sel: sel1
             stat: stat1
         depth: 16
         flags: 1
         chunk_size: 4

   ios:
     - id: out1
       type: virt
       dtype: uint32

Descriptor fields:

- ``iobind``: one entry with ``src``, ``out``, ``sel``, ``stat`` IDs
- ``depth``: sample capacity
- ``chunk_size``: number of samples returned through ``out`` per selected read
  (defaults to 1)
- ``flags``:
  - bit0: auto-start capture
  - bit1: one-shot mode (stop capture when full)
  - bit2: keep data on stop

Chunk Output
============

``out`` provides a bulk-read view over the captured ring when ``chunk_size`` is
greater than 1. The selector value is the first history offset in the chunk.
With scalar ``src`` and ``chunk_size: 32``, selector ``0`` returns offsets
``0..31`` (newest to older), selector ``32`` returns offsets ``32..63``, and
so on.

If the selected range reaches past the number of captured samples,
``CProgBuffer`` zero-fills the remainder of the chunk. Readers should use the
``stat`` count/depth words to determine how much returned data is valid.

External Control
================

ControlIO: supported.

``CProgBuffer`` supports runtime start/stop control via ``CIOControl``.
Start/stop controls capture activity. Data retention on stop depends on
``flags`` bit2.

TriggerIO: supported.

- ``reset`` clears stored samples.
- ``trigger1`` starts capture.
- ``trigger2`` stops capture without stopping the program object.

Brainstorming & Future Ideas
============================

Notes
-----

- One ``CProgBuffer`` instance supports a single binding.
- ``sel`` out-of-range returns error and keeps previous selected sample.
- Existing descriptors with scalar ``out`` keep the original single-sample
  behavior.

Doxygen
=======

- `dawn::CProgBuffer <../../doxygen/classdawn_1_1CProgBuffer.html>`_
