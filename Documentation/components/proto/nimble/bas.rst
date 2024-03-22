=============
BAS - Battery
=============

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphBas`` - Bluetooth SIG Battery Service (UUID ``0x180F``).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_BAS``: enables the Battery Service.

YAML
----

.. code-block:: yaml

   services:
     bas:
       battery_level: batt1

Supported fields:

- ``battery_level``: IO bound to the Battery Level characteristic
  (``uint8`` percent).

Limitations
===========

The BAS binding supports notify-capable IO only. The bound
``battery_level`` IO must report ``isNotify() == true`` (for example
``dummy_notify`` in tests and demos). At startup Dawn reads the current IO
value and passes it to NimBLE's Battery Service; later IO notifications update
the same Battery Level characteristic and trigger BLE notifications through
NimBLE.

BAS v1.1 Characteristics
========================

Dawn exposes Battery Level only. BAS v1.1 status, energy, time, health,
information, and identity characteristics are not implemented.

Doxygen
=======

- `dawn::CProtoNimblePrphBas <../../../doxygen/classdawn_1_1CProtoNimblePrphBas.html>`_
