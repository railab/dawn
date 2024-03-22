========
Bit Pack
========

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgBitPack combines multiple typed inputs into one packed output bitstream.
Each input contributes its logical bits starting at the configured ``bit``
offset. ``bool`` contributes one logical bit per element; other fixed-width
scalar dtypes contribute their raw element bit pattern.

This is the inverse of CProgBitSplit.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Registers notification callbacks on all notify-capable inputs.
- Rebuilds the packed output buffer from all current input values on each
  callback.
- The output is producer-owned by ``bitpack``: deferred ``virt`` outputs are
  initialized with the dimension required to hold the highest configured input
  bit range, and configured writable outputs are validated before use.
- Input objects are consumer-side only and must already be usable before
  ``bitpack`` starts.
- Unsupported dtypes: ``char`` and ``block``.

Logical bit model
=================

- For ``bool`` IOs, each element contributes one bit.
- For other supported fixed-width scalar dtypes, each element contributes
  ``dtype_size * 8`` bits.
- The ``bit`` field is the start offset in the packed output bitstream.
- Example: ``uint8[4]`` at ``bit: 0`` can produce ``0x44332211`` when the
  input bytes are ``[0x11, 0x22, 0x33, 0x44]`` and the output is ``uint32`` or
  ``uint64`` with enough capacity.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_BITPACK``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: my_bitpack
       type: bitpack
       config:
         inputs:
           - io: flag_a
             bit: 0
           - io: flag_b
             bit: 1
           - io: u8_bytes
             bit: 8
         output: packed

Supported descriptor patterns include:

- ``bool + bool -> uint32`` bitmask packing
- ``uint8[4] -> uint32`` or ``uint64``
- ``bool -> uint64``

Doxygen
=======

- `dawn::CProgBitPack <../../doxygen/classdawn_1_1CProgBitPack.html>`_
