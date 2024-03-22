======================
General Purpose Inputs
======================

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOGpi`` is a general-purpose digital input object.

For button handling, use Buttons IO.

Only interrupt-based GPIO supports notifications.

.. note::

   The minimum reliable pulse duration is bounded by the underlying NuttX
   GPIO interrupt latency.

.. note::

   Pulse counting and pulse edge counting should be done based on COUNTER, not GPI!
   COUNTER not yet implemented.

Implementation
==============

``CIOGpi`` opens the GPIO input device at ``/dev/gpioN`` (selected by
``cfgIdDevno``) during ``configure()`` and reads a single ``bool`` on
``getData()``. When ``CONFIG_DAWN_IO_NOTIFY`` is enabled and the underlying
GPIO supports interrupt-driven notifications, the IO registers itself with
``CIONotifier`` and emits push events on edge transitions. The IO is read
only and does not support batch operations.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_GPI``: enables general-purpose input IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: gpi1
       type: gpi

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

* Bulk access to inputs, so many GPI can be read at once.

Doxygen
=======

- `dawn::CIOGpi <../../doxygen/classdawn_1_1CIOGpi.html>`_
