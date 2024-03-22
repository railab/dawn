===========
One To Many
===========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgOneToMany`` connects one notify-capable input IO to many output IOs.
On start it seeds every output from the current input value. Each later input
change is written to every configured output.

For lifecycle control specifically, :doc:`../io/control` can already bind one
control IO to multiple target objects. ``onetomany`` is for generic IO data
fan-out.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_ONETOMANY``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: fanout_value
       type: onetomany
       config:
         input: source_value
         outputs: [target0, target1]

Doxygen
=======

- `dawn::CProgOneToMany <../../doxygen/classdawn_1_1CProgOneToMany.html>`_
