========
Sampling
========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgSampling`` is a sampling program that periodically reads N source
IOs and writes results to N corresponding output IOs.

- synchronized multi-IO fetch at fixed interval
- fetch-to-notify bridge: fetch-only IOs can publish through notify-capable
  outputs

Implementation
==============

``CProgSampling`` runs a background thread that wakes on a fixed interval
(``PROG_SAMPLING_CFG_INTERVAL``, microseconds) and, for each configured
binding, calls ``src->getData()`` and forwards the result to the bound
target via ``setData()``. Bindings are declared as pairs in
``PROG_SAMPLING_CFG_IOBIND`` (first half: source IDs, second half: output
IDs). Each binding gets a per-source data buffer allocated during
``init()``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_SAMPLING``: enables the Sampling program.
- ``CONFIG_DAWN_PROG_SAMPLING_INTERVAL``: default polling interval when the
  descriptor does not set one.

YAML
----

.. code-block:: yaml

   programs:
     - id: sampling1
       type: sampling
       config:
         iobind:
           - src1
           - output1
         interval: 100000

External Control
================

ControlIO: supported.

``CProgSampling`` supports runtime start/stop control through ``CIOControl``.
When stopped, the sampling thread is stopped and outputs are not updated.
When started again, periodic sampling resumes.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Configurable thread priority.
- Hardware timer trigger instead of ``usleep`` for tighter timing.

Doxygen
=======

- `dawn::CProgSampling <../../doxygen/classdawn_1_1CProgSampling.html>`_
