======
Switch
======

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgSwitch is a multi-input AND-gate. It monitors N control inputs. When
**all** inputs equal their configured match values, the switch writes an
on-command to a target IO; otherwise it writes an off-command. State tracking
prevents redundant writes.

Typical target: a CIOControl object that starts/stops a Sequencer or
other program.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Registers notification callbacks on all notify-capable control inputs.
- On any input change, re-evaluates the AND condition.
- The target IO is specified by ObjectId and receives the configured
  ``onCmd`` / ``offCmd`` values via ``setData()``.
- Commands are ``uint8_t`` (0 = stop, 1 = start for CIOControl).
- Input objects are consumer-side only and must already be usable.
- If the target is a deferred ``virt`` output, ``switch`` initializes or reuses
  it because ``switch`` defines that output-side contract as a scalar
  ``uint32``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_SWITCH``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: switch_seq0
       type: switch
       config:
         inputs:
           - io: running
             match: 1
           - io: mode
             match: 0
         target: [ctrl_seq0, 1, 0]   # [target_io, on_cmd, off_cmd]

Doxygen
=======

- `dawn::CProgSwitch <../../doxygen/classdawn_1_1CProgSwitch.html>`_
