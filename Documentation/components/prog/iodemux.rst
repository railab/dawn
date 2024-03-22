================
IO Demultiplexer
================

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgIODemux`` routes one notify-capable input to one selected output. A
scalar ``uint32`` control IO selects the active output with zero-based indexes.
On start the input value is written to the selected output. Non-selected
outputs retain their previous values.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_IODEMUX``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: demux_value
       type: iodemux
       config:
         control: selected_output
         input: source_value
         outputs: [target0, target1]

Doxygen
=======

- `dawn::CProgIODemux <../../doxygen/classdawn_1_1CProgIODemux.html>`_
