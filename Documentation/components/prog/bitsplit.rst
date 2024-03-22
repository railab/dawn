============
Bit Splitter
============

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgBitSplit extracts logical bit slices from one input into N output IO
outputs. Each output starts at its configured ``bit`` offset in the source
logical bitstream. ``bool`` outputs receive one logical bit per element;
other fixed-width scalar dtypes receive the raw bit slice for their size and
dimension.

This is a callback-driven standalone program.  It is the complement of
CProgBitPack.

Implementation
==============

- Source IO must be notify-capable.
- Each bind specifies one bit position; the number of binds must match
  the number of entries in the ``bits`` config array.
- On every incoming sample, the program copies the requested logical bit slice
  into the corresponding output via ``setData()``.
- Unsupported dtypes: ``char`` and ``block``.

Logical bit model
=================

- For ``bool`` IOs, each element contributes one bit.
- For other supported fixed-width scalar dtypes, each element contributes
  ``dtype_size * 8`` bits.
- The ``bits`` entry for a bind is the start offset in the source logical
  bitstream.
- Example: ``uint32`` input ``0x44332211`` with ``bit: 8`` and ``uint16``
  output yields ``0x3322``.

Configuration
=============

Kconfig
-------
- ``CONFIG_DAWN_PROG_BITSPLIT``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: my_bitsplit
       type: bitsplit
       config:
         inputs: [button_io, button_io, button_io, button_io]
         outputs: [virt_btn0, virt_btn1, virt_btn2, virt_btn3]
         bits: [0, 1, 2, 3]

The ``inputs`` must all reference the same source IO. ``outputs`` and
``bits`` must have the same length.

Supported descriptor patterns include:

- ``uint32 -> bool`` button-style bit extraction
- ``uint32 -> uint16`` slice extraction
- Any supported fixed-width scalar input/output combination where the output
  bit width fits inside the source logical bit range

Doxygen
=======

- `dawn::CProgBitSplit <../../doxygen/classdawn_1_1CProgBitSplit.html>`_
