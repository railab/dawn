========================================
Threshold Value Program (Gated Output)
========================================

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgThresholdValue`` compares input samples against descriptor-configured
threshold values and writes source values to an output IO only when the
alert condition is active.

Supported modes:

- ``above``: alert when ``x >= high``
- ``below``: alert when ``x <= low``
- ``hysteresis``: set when ``x >= high``, clear when ``x <= low``
- ``window``: alert when ``low <= x <= high``

Output behavior:

- alert active: output equals input sample
- alert inactive: output is ``0`` (typed zero)

Implementation
==============

Program type: callback/process based (no worker thread).

Input/output binding:

- source IO must be notify-capable
- destination must be a writable output IO with the same dtype as source

Supported source dtypes:

- ``DTYPE_INT32``
- ``DTYPE_UINT32``
- ``DTYPE_FLOAT``

Reset behavior:

- ``trigger(CMD_RESET)`` clears internal hysteresis state.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_THRESHOLD_VALUE``: enables the threshold-value program.

YAML
----

.. code-block:: yaml

   programs:
     - id: threshold_value1
       type: thresholdvalue
       config:
         iobind:
           - sensor1
           - virt_threshold_value1
         mode: 2   # 0=above, 1=below, 2=hysteresis, 3=window
         low: 8
         high: 12

External Control
================

ControlIO: not documented.

TriggerIO: supported for ``reset``.

Doxygen
=======

- `dawn::CProgThresholdValue <../../doxygen/classdawn_1_1CProgThresholdValue.html>`_
