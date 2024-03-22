======
Toggle
======

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgToggle is a T flip-flop / latch. On each rising edge of the
input (non-zero value after zero), the output toggles between two
configured values. The internal state persists across samples.

Use cases: start/stop from a button press, enable/disable signals,
alarm acknowledgement.

Implementation
==============

- Callback-driven (CProgProcess-based), no thread.
- Detects rising edges by comparing the current input against the
  previous input.
- ``CMD_RESET`` returns the toggle to the off-value.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_TOGGLE``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: toggle_signal
       type: toggle
       config:
         sources: [virt_button]
         outputs: [virt_running]
         values: [0, 1]    # off_value, on_value

External Control
================

TriggerIO: ``CMD_RESET`` resets the toggle to the off-value.

Doxygen
=======

- `dawn::CProgToggle <../../doxygen/classdawn_1_1CProgToggle.html>`_
