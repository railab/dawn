========================
DIS - Device Information
========================

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphDis`` - Bluetooth SIG Device Information Service
(UUID ``0x180A``).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_DIS``: enables the Device Information Service.

YAML
----

.. code-block:: yaml

   services:
     dis:
       enabled: true

No IO bindings; the service only advertises static device-information
strings supplied by NimBLE.

Limitations
===========

Dawn exposes the static NimBLE Device Information Service strings. It does not
bind DIS characteristics or descriptors to Dawn IO objects.

Doxygen
=======

- `dawn::CProtoNimblePrphDis <../../../doxygen/classdawn_1_1CProtoNimblePrphDis.html>`_
