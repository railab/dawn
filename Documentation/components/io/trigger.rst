==========
Trigger IO
==========

**Component Type:** Output

**Status:** Implemented

Overview
========

``CIOTrigger`` is a special-purpose write-only IO that fires one-shot
commands on bound objects (IOs, Programs, or Protocols) while they are
running.

Supported commands:

- ``RESET``     -> reset internal state of bound objects (e.g. clear accumulators)
- ``TRIGGER1``  -> object-specific action slot 1 (e.g. ADC single conversion)
- ``TRIGGER2``  -> object-specific action slot 2
- ``TRIGGER3``  -> object-specific action slot 3

Commands are dispatched through the ``trigger(cmd)`` virtual method on
``CObject``. Objects that do not implement a given command return
``-ENOTSUP`` and are silently skipped. Each instance uses an
``allowed_cmds`` bitmask that restricts which commands it may issue.

A single Trigger IO can be bound to multiple objects of any type (IO, PROG,
PROTO). Read is not supported (``isRead()`` returns ``false``).

Differs from :doc:`control` (which manages lifecycle start/stop state) and
:doc:`config` (which controls configuration data values).

Implementation
==============

``CIOTrigger`` is a control-only IO that fans out a single command byte
(reset, trigger1, trigger2, trigger3) to one or more bound ``CObject``
targets. ``IO_TRIGGER_CFG_ALLOWED`` is a bitmask of permitted commands;
attempts to issue a disallowed command return ``-EACCES``. Targets are
resolved during ``CDawn::bindObjects()`` from the IDs supplied via
``IO_TRIGGER_CFG_ALLOCOBJ``. Read operations always return ``-ENOTSUP``.

Data Format
-----------

Write operations use a single ``uint8_t`` byte containing the command code:

.. list-table::
   :header-rows: 1
   :widths: 20 30 50

   * - Value
     - ``CObject`` constant
     - Description
   * - ``0``
     - ``CObject::CMD_RESET``
     - Reset internal state
   * - ``1``
     - ``CObject::CMD_TRIGGER1``
     - Object-specific action slot 1
   * - ``2``
     - ``CObject::CMD_TRIGGER2``
     - Object-specific action slot 2
   * - ``3``
     - ``CObject::CMD_TRIGGER3``
     - Object-specific action slot 3

Allowed Commands
----------------

The ``allowed`` bitmask restricts which commands a given instance may issue.
Attempting a disallowed command returns ``-EACCES``.

.. list-table::
   :header-rows: 1
   :widths: 25 25 50

   * - Flag
     - Value
     - Description
   * - ``TRIG_ALLOW_RESET``
     - ``(1 << 0)``
     - Allow ``CMD_RESET``
   * - ``TRIG_ALLOW_TRIGGER1``
     - ``(1 << 1)``
     - Allow ``CMD_TRIGGER1``
   * - ``TRIG_ALLOW_TRIGGER2``
     - ``(1 << 2)``
     - Allow ``CMD_TRIGGER2``
   * - ``TRIG_ALLOW_TRIGGER3``
     - ``(1 << 3)``
     - Allow ``CMD_TRIGGER3``

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_TRIGGER``: enables the Trigger IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: trig1
       type: trigger
       config:
         targets:
           - sampling1
         allowed:
           - trigger1
           - trigger2

Supported ``allowed`` values: ``reset``, ``trigger1``, ``trigger2``, ``trigger3``.

External Control
================

ControlIO: not supported.

TriggerIO: implemented by this IO.

Doxygen
=======

- `dawn::CIOTrigger <../../doxygen/classdawn_1_1CIOTrigger.html>`_
