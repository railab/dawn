.. _encoder:

=======
Encoder
=======

**Component Type:** Input

**Status:** Implemented

Overview
========

Dawn provides two quadrature encoder IO classes:

- **encoder** (``CIOEncoder``): Provides basic position tracking.
- **encoder_index** (``CIOEncoderIndex``): Provides position tracking along with 
  index position and index count information.

These components interface with the underlying OS quadrature encoder driver 
to provide rotation or linear position data to the framework.

Implementation
==============

Both encoder classes use the ``DTYPE_INT32`` data type for position values.
They interact with the OS through the porting layer (``dawn/include/dawn/porting/encoder.hxx``).

- ``CIOEncoder`` returns a single ``int32_t`` value representing the current 
  position.
- ``CIOEncoderIndex`` returns a vector of three ``int32_t`` values:
  1. Current position
  2. Last index position
  3. Index event counter

Both classes support the ``CMD_RESET`` trigger to reset the hardware encoder 
count.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_ENCODER``: Enables the basic encoder IO class.
- ``CONFIG_DAWN_IO_ENCODER_INDEX``: Enables the index-aware encoder IO class.

YAML
----

Example configuration for a basic encoder:

.. code-block:: yaml

   ios:
     - id: rot_enc
       type: encoder
       dtype: int32
       config:
         devno: 0
         posmax: 1024

Example configuration for an encoder with index support:

.. code-block:: yaml

   ios:
     - id: spindle_enc
       type: encoder_index
       dtype: int32
       config:
         devno: 1
         posmax: 4096

Supported ``config`` fields:

- ``devno``: Driver device number (e.g., ``0`` for ``/dev/qenc0``).
- ``posmax``: (Optional) Maximum position value before wrap-around.

Doxygen
=======

- `dawn::CIOEncoder <../../doxygen/classdawn_1_1CIOEncoder.html>`_
- `dawn::CIOEncoderIndex <../../doxygen/classdawn_1_1CIOEncoderIndex.html>`_
