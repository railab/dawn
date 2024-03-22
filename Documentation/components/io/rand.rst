=======
Rand IO
=======

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIORand`` is an input object that returns pseudo-random values on read.


Implementation
==============

``CIORand`` opens ``/dev/urandom`` during ``configure()`` and uses a
``timerfd`` (interval set by ``IO_RAND_CFG_INTERVAL``) to drive periodic
sampling. Each ``getData()`` reads ``tlen`` bytes per sample slot from
``/dev/urandom`` and acknowledges the timerfd event so the IO stays
responsive to the next tick. The IO supports batch reads (one read per
sample slot).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_RANDIO``: enables random-number IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: rand1
       type: rand
       dtype: uint32

External Control
================

ControlIO: supported.

TriggerIO: not supported.

``CIORand`` supports runtime start/stop control through ``CIOControl``.
When stopped, periodic random updates pause. When started again, updates
resume.

Doxygen
=======

- `dawn::CIORand <../../doxygen/classdawn_1_1CIORand.html>`_
