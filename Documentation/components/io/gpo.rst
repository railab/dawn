=======================
General Purpose Outputs
=======================

**Component Type:** Output

**Status:** Implemented

Overview
========

``CIOGpo`` - general-purpose digital output object.

For LED handling look at LED IO or RGB IO.

Implementation
==============

``CIOGpo`` opens the GPIO output device at ``/dev/gpioN`` (selected by
``cfgIdDevno``) during ``configure()`` and exchanges a single ``bool`` with
it on ``getData()``/``setData()``. The IO supports both read-back and write,
exposes timestamps when ``CONFIG_DAWN_IO_TIMESTAMP`` is enabled, and does
not support batch operations.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_GPO``: enables general-purpose output IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: gpo1
       type: gpo

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

* Bulk access to outputs, so many GPO can be written at once.

Doxygen
=======

- `dawn::CIOGpo <../../doxygen/classdawn_1_1CIOGpo.html>`_
