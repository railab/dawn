=====
Dummy
=====

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoDummy`` is a no-op protocol used for tests and descriptor
placeholders.

- factory-stable target for protocol factory unit tests
- can parse and reserve IO bindings
- useful for testing descriptor binding paths without communication logic

Implementation
==============

``CProtoDummy`` parses configured bindings but exposes no transport behavior.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_DUMMY``: enables the dummy protocol.

YAML
----

.. code-block:: yaml

   protocols:
     - id: proto_dummy1
       type: dummy
       config:
         bindings:
           - io1
           - io2

Supported fields:

- ``config.bindings``: standard IO binding list.

``dummy`` does not define any protocol-specific ``config`` fields.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoDummy <../../doxygen/classdawn_1_1CProtoDummy.html>`_
