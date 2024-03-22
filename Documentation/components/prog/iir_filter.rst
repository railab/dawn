==========
IIR Filter
==========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgIIRFilter`` implements a first-order low-pass IIR filter:

``y[n] = y[n-1] + alpha * (x[n] - y[n-1])``

where ``alpha = alpha_num / alpha_den`` from the descriptor.

- notify-driven processing (via ``CProgProcess``)
- supports multiple source/output bindings in one instance (``N -> N``)
- resettable with ``CMD_RESET``

Implementation
==============

``CProgIIRFilter`` runs in the notify/callback processing path and updates the
filtered output for each incoming source sample.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_IIR_FILTER``: enables the IIR filter program.

YAML
----

.. code-block:: yaml

   programs:
     - id: iir1
       type: iirfilter
       config:
         iobind:
           - src1
           - virt1
         alpha_num: 1
         alpha_den: 4

Uses a packed ``IOBIND`` list of source/output pairs (same pattern as other
process-style programs):

- source IO (notify-capable)
- destination output IO (filtered output)

Additional configuration parameters:

- ``alpha_num``: IIR coefficient numerator
- ``alpha_den``: IIR coefficient denominator

External Control
================

ControlIO: supported.

``CProgIIRFilter`` supports runtime start/stop control through ``CIOControl``.
When stopped, incoming samples are ignored and the current output is frozen.
When started again, filter updates resume.

TriggerIO: supported for ``reset``.

Brainstorming & Future Ideas
============================

- Supports numeric scalar dtypes: ``int8``, ``uint8``, ``int16``, ``uint16``,
  ``int32``, ``uint32``, ``b16``, ``ub16``, ``float``, and ``double``
- ``alpha_den`` must be non-zero
- ``alpha_num <= alpha_den`` (``alpha`` in ``[0, 1]``)
- Integer outputs truncate toward zero after each update
- ``int64``, ``uint64``, ``bool``, character, and block dtypes are not filtered
- Use ``sampling`` before ``iirfilter`` when the source IO only supports fetch

Doxygen
=======

- `dawn::CProgIIRFilter <../../doxygen/classdawn_1_1CProgIIRFilter.html>`_
