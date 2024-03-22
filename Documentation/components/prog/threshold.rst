===============================
Threshold Program (Bool Output)
===============================

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgThreshold`` compares input samples against descriptor-configured
threshold values and writes boolean alert results to an output IO.

Supported modes:

- ``above``: output ``1`` when ``x >= high``
- ``below``: output ``1`` when ``x <= low``
- ``hysteresis``: set when ``x >= high``, clear when ``x <= low``
- ``window``: output ``1`` when ``low <= x <= high``

Implementation
==============

Program type: callback/process based (no worker thread).

Input/output binding:

- source IO must be notify-capable
- destination must be a writable output IO with ``DTYPE_BOOL``

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

- ``CONFIG_DAWN_PROG_THRESHOLD``: enables the threshold program.

YAML
----

.. code-block:: yaml

   programs:
     - id: threshold1
       type: threshold
       config:
         iobind:
           - sensor1
           - virt_threshold1
         mode: 2   # 0=above, 1=below, 2=hysteresis, 3=window
         low: 8
         high: 12

External Control
================

ControlIO: not documented.

TriggerIO: supported for ``reset``.

Doxygen
=======

- `dawn::CProgThreshold <../../doxygen/classdawn_1_1CProgThreshold.html>`_
