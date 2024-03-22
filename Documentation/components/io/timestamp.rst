=========
Timestamp
=========

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOTimestamp`` is an input object that returns the current system
timestamp.

Implementation
==============

``CIOTimestamp`` is backed by a NuttX ``timerfd`` configured with an interval
specified by ``IO_TIMESTAMP_CFG_INTERVAL``. ``getData()`` samples the host
timestamp counter into the output buffer (single value per call) and
acknowledges the timerfd event. The IO acts as a periodic source for
``CIONotifier``-driven pipelines; values are host-native.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_TIMESTAMPIO``: enables timestamp IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: ts1
       type: timestamp
       dtype: uint64
       config:
         interval_us: 100000

External Control
================

ControlIO: supported.

TriggerIO: not supported.

``CIOTimestamp`` supports runtime start/stop control through ``CIOControl``.
When stopped, periodic timestamp updates pause. When started again, updates
resume.

Doxygen
=======

- `dawn::CIOTimestamp <../../doxygen/classdawn_1_1CIOTimestamp.html>`_
