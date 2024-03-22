========
Boardctl
========

**Component Type:** Input/Output

**Status:** In Progress

Overview
========

``CIOBoardctl`` exposes board-level control and status operations through the
standard IO interface.

Documented variants include reset, reset-cause reporting, and poweroff.

Implementation
==============

``CIOBoardctl`` is a multi-class IO that dispatches on the ObjectID class
field to a NuttX ``boardctl()`` ioctl:

- ``IO_CLASS_SYSTEM_RESET`` (write-only): triggers ``BOARDIOC_RESET`` with
  the integer payload as the reset code.
- ``IO_CLASS_SYSTEM_RESETCAUSE`` (read-only): returns the
  ``boardioc_reset_cause_s`` ``cause`` and ``flag`` fields packed into two
  ``uint32_t`` slots.
- ``IO_CLASS_SYSTEM_POWEROFF`` (write-only): triggers ``BOARDIOC_POWEROFF``.

Each sub-class is gated by its corresponding ``CONFIG_BOARDCTL_*``. Batch
operations are not supported.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_BOARDCTL``: enables ``boardctl`` IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: reset1
       type: boardctl
       dtype: bool
       rw: false
       variant: reset

     - id: reset_cause1
       type: boardctl
       dtype: uint32
       rw: false
       variant: reset_cause

     - id: poweroff1
       type: boardctl
       dtype: bool
       rw: false
       variant: poweroff

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOBoardctl <../../doxygen/classdawn_1_1CIOBoardctl.html>`_
