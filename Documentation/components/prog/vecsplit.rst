============
Vector Split
============

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgVecSplit splits one multidimensional source IO into multiple output IOs.
Outputs receive contiguous slices in descriptor order. This is useful when a
driver reads several channels at once, such as an ADC vector, while consumers
need scalar per-channel values.

This is the inverse of CProgVecPack.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Registers one notification callback on the source when it supports notify.
- Deferred ``virt`` outputs are initialized as scalar notify-capable IOs.
- Source and outputs must use the same dtype.
- The sum of output dimensions must not exceed the source dimension.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_VECSPLIT``: enables the program.

YAML
----

.. code-block:: yaml

   ios:
     - id: adc0
       type: adc
       dtype: uint32
       config:
         device: 0
     - id: adc_ch0
       type: virt
       dtype: uint32
     - id: adc_ch1
       type: virt
       dtype: uint32

   programs:
     - id: adc_split
       type: vecsplit
       config:
         source: adc0
         outputs: [adc_ch0, adc_ch1]

Doxygen
=======

- `dawn::CProgVecSplit <../../doxygen/classdawn_1_1CProgVecSplit.html>`_
