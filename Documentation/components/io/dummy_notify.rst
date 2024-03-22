.. _dummy_notify:

============
Dummy Notify
============

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIODummyNotify`` is an in-memory IO that can automatically notify other
components when it is time to read its value.

This component is a version of the :ref:`dummy` IO that includes a built-in
timer. While a regular dummy IO just sits in memory until someone reads it,
the "notify" version uses an internal timer to proactively tell the system
that new data is ready.

This is particularly useful during development to simulate sensors or other
hardware that "pushes" data at a regular interval. It allows you to test
event-driven Programs or Protocols without needing real hardware or
complex interrupts.

Implementation
==============

The component uses a :ref:`timerfd` helper to generate periodic events. It
inherits from both ``CIOCommon`` and ``CIOTimerfd``.

During the ``start()`` phase, the internal timer is activated. When the
timer expires, it triggers the Dawn notification system, which in turn
calls any registered callbacks (such as those from a Program or Protocol).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_DUMMY_NOTIFY``: enables the dummy notify IO object.
- ``CONFIG_DAWN_IO_NOTIFY``: required for notification support.

YAML
----

.. code-block:: yaml

   ios:
     - id: sim_sensor
       type: dummy_notify
       dtype: float
       config:
         init_value: [25.5]
         interval: 1000000  # 1 second in microseconds
         dim: 1

External Control
================

- ``ControlIO``: supported
- ``TriggerIO``: not supported

Doxygen
=======

- `dawn::CIODummyNotify <../../doxygen/classdawn_1_1CIODummyNotify.html>`_
