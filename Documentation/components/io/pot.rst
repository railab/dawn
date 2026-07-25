======
POT IO
======

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIOPot`` exposes one wiper of a digital potentiometer. Writing sets the
wiper position, reading returns the current position.

Implementation
==============

``CIOPot`` opens the potentiometer character device at ``/dev/pot<devno>``
(common ``cfgIdDevno`` item) during ``configure()`` and checks the configured
wiper index against the wiper count reported by ``POTIOC_GET_INFO``. Reads
and writes go through the porting-layer ``pot_get_wiper()`` /
``pot_set_wiper()`` helpers, which issue the NuttX ``POTIOC_GET_WIPER`` /
``POTIOC_SET_WIPER`` ioctls. The IO is ``DTYPE_INT32``, single element, does
not support notify or batch, rejects multi-item writes with ``-ENOMEM`` and
batched reads with ``-ENOTSUP``. Read timestamps are taken at read time.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_POT``: enables POT IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: vref
       type: pot
       dtype: int32
       config:
         device: 0
         wiper: 1

``wiper`` selects the wiper index (default 0); one IO object per wiper.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

* Relative move, enable and non-volatile store/recall ioctls
* Expose ``max`` / ``rab`` from ``POTIOC_GET_INFO`` as IO limits

Doxygen
=======

- `dawn::CIOPot <../../doxygen/classdawn_1_1CIOPot.html>`_
