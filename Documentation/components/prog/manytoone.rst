===========
Many To One
===========

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgManyToOne`` connects many notify-capable input IOs to one output IO.
On start it seeds the output from the first configured input. After that, the
last input that changes is forwarded to the output.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_MANYTOONE``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: bridge_inputs
       type: manytoone
       config:
         inputs: [src0, src1, src2]
         output: selected_value

Doxygen
=======

- `dawn::CProgManyToOne <../../doxygen/classdawn_1_1CProgManyToOne.html>`_
