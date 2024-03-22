=============
Buttons Input
=============

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOButtons`` is a button input object that packs button states as bits in a
``uint32_t`` value.

Implementation
==============

``CIOButtons`` opens the buttons character device at ``/dev/buttons<devno>``
during ``configure()`` and reads a packed ``uint32_t`` bitmap on
``getData()``. Each bit reports one button channel. The buffer's timestamp
is populated when ``CONFIG_DAWN_IO_TIMESTAMP`` is enabled. Batch reads are
not supported (multi-sample requests return ``-ENOTSUP``); push notifications
are wired through ``CIONotifier`` when ``CONFIG_DAWN_IO_NOTIFY`` is enabled.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_BUTTONS``: enables button-backed IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: buttons1
       type: buttons

This IO returns the current button bitmap as a ``uint32_t`` value.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOButtons <../../doxygen/classdawn_1_1CIOButtons.html>`_
