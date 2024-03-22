==============
IO Multiplexer
==============

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgIOMux`` routes one of many notify-capable inputs to one output. A
scalar ``uint32`` control IO selects the active input with zero-based indexes.
On start the selected input is written to the output. Out-of-range selector
values are ignored.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_IOMUX``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: mux_value
       type: iomux
       config:
         control: mode
         inputs: [value0, value1]
         output: active_value

Doxygen
=======

- `dawn::CProgIOMux <../../doxygen/classdawn_1_1CProgIOMux.html>`_
