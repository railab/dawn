=========
Sequencer
=========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgSequencer`` periodically applies configured state values to writable
output IO targets.

Each state contains:

- ``value``: output payload (raw scalar value)
- ``dwell_us``: dwell interval in microseconds before switching to next state
  (must be greater than ``0``)

The transition model is a ring sequence::

  next state index = (current + 1) % state_count

Implementation
==============

- Targets must be writable scalar IOs (dimension ``1``).
- All targets in one sequencer instance must use the same data type and data
  size.
- The internal ``io_ddata_t`` type is derived from the connected target IO
  dtype (not hardcoded).
- Supported payload sizes are ``1..4`` bytes in v1.
- When a target is a deferred ``virt`` output, the sequencer initializes or reuses
  it as a scalar output before runtime starts.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_SEQUENCER``: enables the Sequencer program.

Runtime-Configurable Fields
---------------------------

- ``states``: writable state table (raw ``value,dwell_us`` word pairs)
- ``start_index``: writable start/reset index

Runtime updates are reloaded from object config when the sequencer is
started or when ``CMD_RESET`` is triggered.

YAML
----

.. code-block:: yaml

   programs:
     - id: blinky_seq1
       type: sequencer
       config:
         targets:
           - led1
         states:
           - value: 0
             dwell_us: 500000
           - value: 1
             dwell_us: 500000
         start_index: 0

External Control
================

ControlIO: supported.

``CProgSequencer`` supports runtime start/stop control through ``CIOControl``.

TriggerIO: partially supported.

Supported trigger commands:

- ``CMD_RESET``: restore ``start_index`` and apply that state immediately
  using current runtime configuration values

Doxygen
=======

- `dawn::CProgSequencer <../../doxygen/classdawn_1_1CProgSequencer.html>`_
