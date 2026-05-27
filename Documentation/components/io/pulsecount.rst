===========
Pulse Count
===========

**Component Type:** Output

**Status:** Implemented

Overview
========

``CIOPulseCount`` drives the NuttX ``/dev/pulsecountN`` finite pulse-train
interface. Each data write is a scalar ``uint32_t`` pulse count: write ``N``
and the device emits exactly ``N`` pulses using the configured timing.

Pulse timing comes from descriptor config:

- ``high_ns``: pulse high time in nanoseconds
- ``low_ns``: pulse low time in nanoseconds

If omitted, timings default to the Kconfig defaults.

Implementation
==============

``CIOPulseCount`` opens ``/dev/pulsecount<devno>`` during ``configure()``.
``setData()`` validates a non-zero count, programs the current
``high_ns``/``low_ns``/``count`` triple with
``PULSECOUNTIOC_SETCHARACTERISTICS``, then starts emission with
``PULSECOUNTIOC_START``. The driver is opened with ``O_NONBLOCK`` so Dawn
writes do not block for the full pulse train duration.

The IO is write-only, scalar, non-batch, and timestamp-capable.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_PULSECOUNT``: enables pulsecount-backed IO objects.
- ``CONFIG_DAWN_IO_PULSECOUNT_DEFAULT_HIGH_NS``: default pulse high time in
  nanoseconds. Default: ``500000``.
- ``CONFIG_DAWN_IO_PULSECOUNT_DEFAULT_LOW_NS``: default pulse low time in
  nanoseconds. Default: ``500000``.

YAML
----

.. code-block:: yaml

   ios:
     - id: pulsecount1
       type: pulsecount
       dtype: uint32
       config:
         device: 0
         high_ns: 100000
         low_ns: 100000

     - id: pulsecount_high1
       type: config
       dtype: uint32
       rw: true
       config:
         objid_ref: pulsecount1
         objcfg_ref: high_ns

     - id: pulsecount_low1
       type: config
       dtype: uint32
       rw: true
       config:
         objid_ref: pulsecount1
         objcfg_ref: low_ns

External Control
================

ConfigIO: ``high_ns`` and ``low_ns`` can be exposed through ``config`` IOs
when the descriptor marks those fields writable.

ControlIO: not implemented.

TriggerIO: not supported.

Board / Test Notes
==================

The Dawn simulator uses ``CONFIG_DAWN_FAKE_PULSECOUNT`` to register fake
``/dev/pulsecountN`` devices for unit tests and shell demos, without any
changes in ``external/nuttx``.

A scan of the upstream NuttX tree found pulsecount support on some STM32/Tiva
platforms, but not on Dawn's current simulator, QEMU, nRF52, nRF53, or
``nucleo-c071rb`` board support packages.

Doxygen
=======

- `dawn::CIOPulseCount
  <../../doxygen/classdawn_1_1CIOPulseCount.html>`_
