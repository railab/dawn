=======
Gateway
=======

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgGateway`` is a protocol-to-protocol IO gateway that bridges pairs
of Virtual IO objects so different protocols can share the same IO
state.

- write propagation: writing to one side pushes the value to the other
- fetch on demand: reading one side pulls the current value from the other
- multiple independent io1/io2 pairs per instance
- per-binding permission flags control which operations are active

Implementation
==============

``CProgGateway`` connects two ``CIOVirt`` endpoints in either direction
(``FLAG_IO1_READ``/``FLAG_IO2_READ`` for pull, ``FLAG_IO1_WRITE``/
``FLAG_IO2_WRITE`` for push). For each direction enabled by the bind flags,
it installs a get or set callback on the corresponding virtIO; when the
callback fires it copies the value from one side to the other through a
shared per-bind buffer. Multiple bind groups are supported via a single
``PROG_GATEWAY_CFG_IOBIND`` containing one ``SProgGatewayIOBind`` per
endpoint pair.

``virtIO`` ownership is per endpoint pair, not globally tied to gateway:

- when a side is already used by protocol or application code as an input
  source, that external side owns the fixed-shape contract and ``gateway``
  only consumes it
- when a bind declares the shape for a deferred side, ``gateway`` may
  initialize and own that output-side ``virtIO`` contract

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_GATEWAY``: enables the Gateway program.

YAML
----

.. code-block:: yaml

   programs:
     - id: gateway1
       type: gateway
       config:
         iobind:
           - virt_a
           - virt_b
           - 3
           - 1

Each binding payload is a 4-word tuple ``{io1, io2, flags, dim}``.
Multiple tuples are packed after a single ``cfgIdIOBind(size)`` header,
where ``size`` is the total number of uint32_t words (4 per binding).

Flags:

- ``FLAG_IO1_READ``  (bit 0) - reading io1 fetches from io2
- ``FLAG_IO1_WRITE`` (bit 1) - writing io1 propagates to io2
- ``FLAG_IO2_READ``  (bit 2) - reading io2 fetches from io1
- ``FLAG_IO2_WRITE`` (bit 3) - writing io2 propagates to io1

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- Thread-safety improvements for concurrent protocol access.

Doxygen
=======

- `dawn::CProgGateway <../../doxygen/classdawn_1_1CProgGateway.html>`_
