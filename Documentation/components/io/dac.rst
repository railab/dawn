======
DAC IO
======

**Component Type:** Output

**Status:** In Progress

Overview
========

``CIODac`` is a DAC output object that writes analog output values to a DAC
driver.

Implementation
==============

``CIODac`` opens the DAC character device at ``/dev/dac<devno>`` (controlled
by the common ``cfgIdDevno`` configuration item) during ``configure()``.
Writes go through the porting-layer ``dac_write()`` helper, which issues the
NuttX DAC ioctl. The IO is write-only (single channel, ``DTYPE_INT32``),
does not support batch operations, and rejects multi-item writes with
``-ENOMEM``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_DAC``: enables DAC IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: dac1
       type: dac
       dtype: uint32

External Control
================

ControlIO: not supported yet.

TriggerIO: not supported.

Planned: ``CIODac`` should support start/stop via ``CIOControl``.

Brainstorming & Future Ideas
============================

* Triggers
* Multi-channel support

Doxygen
=======

- `dawn::CIODac <../../doxygen/classdawn_1_1CIODac.html>`_
