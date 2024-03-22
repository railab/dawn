=====
Dummy
=====

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgDummy`` is a no-op program used for tests and descriptor
placeholders.

- factory-stable target for program factory unit tests
- can parse and reserve IO bindings (no processing logic)
- useful when validating descriptor plumbing without runtime logic

Implementation
==============

``CProgDummy`` reserves configured bindings but does not process data.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_DUMMY``: enables the dummy program.

YAML
----

.. code-block:: yaml

   programs:
     - id: dummy_prog1
       type: dummy
       config:
         iobind:
           - src1
           - out1

Supports standard ``IOBIND`` list of IO object IDs.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProgDummy <../../doxygen/classdawn_1_1CProgDummy.html>`_
