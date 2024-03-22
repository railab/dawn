==========
Expression
==========

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgExpression applies a configurable arithmetic operation to a single input
with a constant operand, writing the result to an output IO.

Supported operations:

=======================  ==========================
Op code                  Behaviour
=======================  ==========================
``OP_SHIFT_LEFT``        ``output = input << c``
``OP_SHIFT_RIGHT``       ``output = input >> c``
``OP_CONST_LEFT_SHIFT``  ``output = c << input``
``OP_ADD``               ``output = input + c``
``OP_SUB``               ``output = input - c``
=======================  ==========================

Implementation
==============

- Callback-driven (CProgProcess-based), no thread.
- The operation and constant are configured as a 2-word array
  ``[op_type, constant]``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_EXPRESSION``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     # Compute 1 << p1 (constant left-shift, used to select an LED):
     - id: expr_led_select
       type: expression
       config:
         sources: [virt_p1]
         outputs: [expr_out]
         op: [2, 1]    # OP_CONST_LEFT_SHIFT, constant = 1

Brainstorming & Future Ideas
============================

- Two-input variant: accept two runtime values and apply the operation between
  them (e.g. ``a << b`` where both ``a`` and ``b`` come from separate inputs).

Doxygen
=======

- `dawn::CProgExpression <../../doxygen/classdawn_1_1CProgExpression.html>`_
