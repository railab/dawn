===================
NimBLE BLE Protocol
===================

**Component Type:** Protocol

**Status:** In Progress

Overview
========

``CProtoNimbleHost`` / ``CProtoNimblePrph`` - NimBLE BLE protocol objects for
Dawn. Only BLE peripheral mode is supported.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE``: enables NimBLE protocol support.
- ``CONFIG_DAWN_PROTO_NIMBLE_PERIPHERAL``: selects the peripheral role.
- ``CONFIG_DAWN_PROTO_NIMBLE_TPS``: enables the Tx Power Service.
- ``CONFIG_DAWN_PROTO_NIMBLE_DUMMY``: enables the dummy NimBLE backend.
- Per-service flags: ``DIS``, ``BAS``, ``AIOS``, ``ESS``, ``IMDS``, ``OTS``.

YAML
----

.. code-block:: yaml

   protocols:
     - id: ble1
       type: nimble
       config:
         gap_name: "dawn"
         services:
           dis: { enabled: true }
           bas: { battery_level: batt1 }
           aios: { ... }
           ess:  { ... }
           imds: { ... }
           ots:  { ... }

Supported fields:

- ``config.gap_name``: advertised GAP device name.
- ``config.services``: per-service binding tree (see per-service pages).
- ``config.services.bas.battery_level``: Battery Service source IO; must be
  notify-capable. See :doc:`bas`.

External Control
================

ControlIO: supported.

``CProtoNimblePrph`` supports runtime start/stop control through ``CIOControl``.
When stopped, NimBLE host/controller threads are inactive. When started again,
BLE services resume.

TriggerIO: not supported.

Limitations
===========

Clean shutdown of the NimBLE host and HCI threads is only supported when
``CONFIG_DAWN_PROTO_NIMBLE_DUMMY`` is enabled. Upstream NimBLE does not expose
a teardown API for the real backend, so calling ``stop()`` on the host or HCI
controller in that configuration logs an error and waits for the thread to
exit on its own.

.. _nimble_units:

Units conversion
================

NimBLE services convert from Dawn's default sensor units to the BLE
characteristic units. Dawn's default sensor units are the NuttX sensor
framework units documented in :ref:`dawn_sensor_units`.

- ESS and IMDS sensor characteristics support ``DTYPE_FLOAT`` inputs.
- For IOs not backed by the NuttX sensor framework, the bound IO must provide
  a ``DTYPE_FLOAT`` value in the Dawn convention documented below.
- Fixed-point NuttX sensor values are not converted by the current NimBLE
  bindings.

=================== ====================================== ==================================================
Type                Dawn Unit                              BLE Unit
=================== ====================================== ==================================================
Accelerometer       float, m/s^2, scale = 1                not yet supported in Dawn
Magnetic Field      float, microtesla (uT), scale = 1      not yet supported in Dawn
Orientation         float, degree, scale = 1               not yet supported in Dawn
Gyroscope           float, radians/second, scale = 1       not yet supported in Dawn
Light               float, lux, scale = 1                  uint24, lux, scale = 0.01
Barometer           float, hectopascal (hPa), scale = 1    uint32, pascal (Pa), scale = 1
Temperature         float, degrees Celsius, scale = 1      sint16, degrees Celsius, scale = 0.01
Proximity           float, centimeters, scale = 1          not yet supported in Dawn
RGB                 float, percentage, scale = 1           not yet supported in Dawn
Linear Acceleration float, percentage, scale = 1           not yet supported in Dawn
Relative Humidity   float, percent (%), scale = 1          sint16, percent (%), scale = 0.01
PM1P0               float, SI units (ug/m^3), scale = 1    not yet supported in Dawn
PH                  float, pH unit, scale = 1              not yet supported in Dawn
Gas resistance      float, kilohm, scale = 1               sint16, kilohm, scale = 1 (non-standard UUID 0x272A)
Force               float, N unit, scale = 1               not yet supported in Dawn
Hall                int32_t, hall state                    not yet supported in Dawn
IR                  float, lux unit, scale = 1             uint24, lux, scale = 0.01
HCO                 float, ppb                             not yet supported in Dawn
Noise               float, dB unit, scale = 1              not yet supported in Dawn
UV index            float, UV index unit, scale = 1        uint8, UV index unit, scale = 1
=================== ====================================== ==================================================

Non-uorb sources
----------------

For IOs not backed by the ``uorb`` sensor framework, Dawn fixes the unit by
convention. The bound IO must publish a ``float`` already in the unit below.

==================== ====================================== ==================================================
Type                 Dawn Unit (convention)                 BLE Unit
==================== ====================================== ==================================================
Voltage              float, volt (V), scale = 1             not yet supported in Dawn (medfloat16, V)
Electric Current     float, ampere (A), scale = 1           not yet supported in Dawn (medfloat16, A)
Voltage Frequency    float, hertz (Hz), scale = 1           not yet supported in Dawn (uint16, 0.1 Hz)
==================== ====================================== ==================================================

Doxygen
=======

- `dawn::CProtoNimblePrph <../../../doxygen/classdawn_1_1CProtoNimblePrph.html>`_

Services
========

.. toctree::
   :maxdepth: 1

   dis.rst
   bas.rst
   aios.rst
   ess.rst
   imds.rst
   ots.rst
