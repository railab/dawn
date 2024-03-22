.. _adc:

======
ADC IO
======

**Component Type:** Input

**Status:** In Progress

Overview
========

The ADC IO family provides dedicated interfaces for Analog-to-Digital
Converters. Each instance maps directly to one ADC device
(for example ``/dev/adc0``).

- **Multi-channel**: Returns all available ADC channels for the given device.
- **Data Type**: Only ``DTYPE_INT32`` is supported.
- **Variants**:

  - ``CIOAdcFetch`` for on-demand reads.
  - ``CIOAdcSync`` for hardware-triggered single-sample reads.
  - ``CIOAdcStream`` for batched high-throughput reads.

Implementation
==============

The ADC classes interact with the system's ADC driver through standard file
operations. During the ``configure()`` phase, the selected ADC class opens
the ADC device and queries the number of available channels using
``adc_get_nchans()``. The data dimension is automatically determined by the
hardware driver's reported channel count.

Each ADC IO instance maps to one OS ADC device, such as ``/dev/adc0`` or
``/dev/adc1``. Reads return all channels exposed by that device as one
vector. Native per-channel selection is not part of the ADC IO interface.
When an ADC exposes many channels but a program or protocol expects one exact
channel, use :doc:`../prog/vecsplit` to split the ADC vector into narrower
virtual outputs.

ADC IO objects require ``DTYPE_INT32``. Any value conversion or scaling should
be done by a program stage, such as adjust, rather than by the ADC IO itself.

``CIOAdcFetch`` performs an on-demand single-sample read. ``getData()`` only
supports one sample at a time. The read path starts the ADC conversion and
then reads one full channel vector.

``CIOAdcSync`` reads samples from an ADC device that is expected to be driven
by synchronous triggering. ``trigger()`` starts conversion with
``adc_start()``, and ``stop()`` calls ``adc_stop()``. The ``trigger_freq``
configuration field is reserved for timer-trigger configuration, but current
NuttX ADC drivers do not support this as a portable ADC driver feature.

``CIOAdcStream`` is the batched ADC variant. It preallocates a read buffer for
the configured ``batch_size`` and accepts ``getData()`` requests up to that
batch size. ``start()`` and ``trigger()`` call ``adc_start()``, while
``stop()`` calls ``adc_stop()``.

.. warning::

   The main limiting factor for richer ADC IO behavior is the current NuttX ADC
   driver API. Planned ADC IO features such as portable hardware trigger setup,
   channel subsetting, and detailed conversion-mode control require substantial
   ADC driver API improvements before Dawn can expose them consistently.
   In the current state, most ADC driver behavior in NuttX is selected by the OS
   Kconfig and board driver implementation, so descriptors have limited runtime
   control over ADC behavior.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_ADC_FETCH``: enables on-demand ADC fetch IO.
- ``CONFIG_DAWN_IO_ADC_SYNC``: enables hardware-triggered ADC sync IO.
- ``CONFIG_DAWN_IO_ADC_STREAM``: enables batched ADC stream IO.

YAML
----

On-demand single-sample reads:

.. code-block:: yaml

   ios:
     - id: adc1
       type: adc_fetch
       dtype: int32
       config:
         device: 0

The ``device`` value matches the ADC instance number in the OS.

Batched stream reads:

.. code-block:: yaml

   ios:
     - id: adc_stream1
       type: adc_stream
       dtype: int32
       config:
         device: 0
         batch_size: 16

Splitting one ADC channel out of a multi-channel ADC vector:

.. code-block:: yaml

   ios:
     - id: adc1
       type: adc_fetch
       dtype: int32
       config:
         device: 0
     - id: adc_ch0
       type: virt
       dtype: int32

   programs:
     - id: adc_split
       type: vecsplit
       config:
         source: adc1
         outputs: [adc_ch0]

``adc_sync`` also has a reserved ``trigger_freq`` field. Do not depend on it
for runtime behavior until the target NuttX ADC driver supports timer trigger
frequency configuration. Most ADC driver behavior is currently fixed by OS
Kconfig and board-driver setup, so descriptors can select the ADC device and
Dawn-side options but cannot fully configure ADC conversion behavior.

External Control
================

- ``ControlIO``: start/stop is available through the ADC classes. ``adc_fetch``
  has no start/stop state, ``adc_sync`` uses stop to call ``adc_stop()``, and
  ``adc_stream`` uses start/stop to call ``adc_start()``/``adc_stop()``.
- ``TriggerIO``: supported as a software trigger. It calls ``adc_start()`` on
  the ADC device.

Brainstorming & Future Ideas
============================

- Native ADC channel selection or channel subsetting in ADC IO, if
  ``vecsplit`` is not sufficient for a target workflow.
- Portable hardware trigger configuration, including timer and external-pin
  triggers.
- Conversion-mode control for one-shot, continuous, scan, and discontinuous
  operation.
- A priority model for high-priority injected conversions versus low-priority
  regular conversions.
- Clear start/stop/trigger semantics for future hardware-triggered and
  continuous conversion modes.

Doxygen
=======

- `dawn::CIOAdcFetch <../../doxygen/classdawn_1_1CIOAdcFetch.html>`_
- `dawn::CIOAdcSync <../../doxygen/classdawn_1_1CIOAdcSync.html>`_
- `dawn::CIOAdcStream <../../doxygen/classdawn_1_1CIOAdcStream.html>`_
