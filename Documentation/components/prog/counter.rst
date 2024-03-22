=======
Counter
=======

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgCounter increments an internal counter on each rising edge of the
input. The counter wraps back to the configured minimum when it would
exceed the maximum. The current value is written to the output IO.

Use cases: mode cycling (0 -> 1 -> 2 -> 3 -> 0), parameter adjustment,
event counting, round-robin selection.

Implementation
==============

- Callback-driven standalone program, no thread.
- Detects rising edges like CProgToggle.
- Parameters are configured as a 4-word array: ``[min, max, step, _]``.
- ``CMD_RESET`` returns the counter to ``min``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_COUNTER``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: counter_mode
       type: counter
       config:
         sources: [virt_button]
         outputs: [virt_mode]
         params: [0, 3, 1, 0]    # min, max, step, _

External Control
================

TriggerIO: ``CMD_RESET`` resets the counter to ``min``.

Doxygen
=======

- `dawn::CProgCounter <../../doxygen/classdawn_1_1CProgCounter.html>`_
