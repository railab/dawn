==========
Adjustment
==========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgAdjust`` - adjustment program object that applies scaling,
offset, and type conversion: ``x = (type)(a * x + b)``.

- scale and offset

- convert to type

Implementation
==============

``CProgAdjust`` reads from a source IO and writes the converted value into an
output IO. For each sample it applies ``out = in * scale + offset`` using the
conversion table that matches the source/output data types. Scale and offset
come from ``PROG_ADJUST_CFG_PARAMS``; the source/output pair comes from
``PROG_ADJUST_CFG_IOBIND``.

If the source IO does not support notifications, ``adjust`` computes the output
once at program start. If the source IO supports notifications, ``adjust``
updates the output on each source notification. This allows both read-side
conversion and write-through conversion where another protocol writes to a
notify-capable source IO and ``adjust`` forwards the converted value to a
writable target IO.

When scale and offset are both zero, the program defaults to identity
(``scale=1``, ``offset=0``) so it can act as a pure type converter.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_ADJUST``: enables the Adjust program.

YAML
----

.. code-block:: yaml

   programs:
     - id: adjust1
       type: adjust
       config:
         inputs:
           - src1
         outputs:
           - virt1

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Support many IO in one instance when all use the same coefficients.
- The output IO type defines type conversion.
- Add an explicit refresh trigger for non-notifying sources that need updates
  after program start.

Doxygen
=======

- `dawn::CProgAdjust <../../doxygen/classdawn_1_1CProgAdjust.html>`_
