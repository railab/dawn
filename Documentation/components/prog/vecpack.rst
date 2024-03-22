===========
Vector Pack
===========

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgVecPack combines multiple IO values into one multidimensional output IO.
Inputs are copied in descriptor order into a cached output vector. When any
notify-capable input changes, the program rebuilds the full vector and writes
it to the output only if the cached value changed.

This is the inverse of CProgVecSplit.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Deferred ``virt`` inputs are initialized as scalar notify-capable IOs.
- Deferred ``virt`` outputs are initialized with the combined input dimension.
- Inputs and output must use the same dtype.
- The output may be a physical multichannel IO, such as ``pwm``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_VECPACK``: enables the program.

YAML
----

.. code-block:: yaml

   ios:
     - id: pwm_ch0
       type: virt
       dtype: uint32
     - id: pwm_ch1
       type: virt
       dtype: uint32
     - id: pwm0
       type: pwm
       dtype: uint32
       config:
         device: 0

   programs:
     - id: pwm_pack
       type: vecpack
       config:
         inputs: [pwm_ch0, pwm_ch1]
         output: pwm0

Doxygen
=======

- `dawn::CProgVecPack <../../doxygen/classdawn_1_1CProgVecPack.html>`_
