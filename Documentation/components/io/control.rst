==========
Control IO
==========

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIOControl`` is a special-purpose IO that controls the lifecycle
state of bound objects (IOs, Programs, or Protocols).

- Write ``1`` -> calls ``start()`` on all bound objects
- Write ``0`` -> calls ``stop()`` on all bound objects
- Read -> returns current state of bound objects (``getState()``)

Each instance uses an ``allowed_cmds`` bitmask that restricts which
commands it may issue. A single Control IO can be bound to multiple
objects of any type (IO, PROG, PROTO); all targets receive the same command.

Differs from :doc:`config` (which controls data/configuration values) and
:doc:`trigger` (which fires one-shot actions on already-running objects).

Implementation
==============

Data Format
-----------

Read and write operations use a single ``uint8_t`` byte:

- Write ``0`` - stop command
- Write ``1`` - start command
- Read - current state (``0`` = stopped, ``1`` = running)

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
   * - ``CTRL_ALLOW_STOP``
     - ``(1 << 0)``
     - Allow write ``0`` (stop command)
   * - ``CTRL_ALLOW_START``
     - ``(1 << 1)``
     - Allow write ``1`` (start command)

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_CONTROL``: enables the Control IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: ctrl1
       type: control
       config:
         targets:
           - sampling1
         allowed:
           - start
           - stop

Supported ``allowed`` values: ``start``, ``stop``.

External Control
================

ControlIO: implemented by this IO.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

Object Documentation Policy
---------------------------

Each object documentation page must explicitly state ``ControlIO`` support
status for that object:

- ``ControlIO: supported``
- ``ControlIO: not supported``
- ``ControlIO: not supported yet`` (if planned but not implemented)

Doxygen
=======

- `dawn::CIOControl <../../doxygen/classdawn_1_1CIOControl.html>`_
