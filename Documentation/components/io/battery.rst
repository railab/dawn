==========
Battery IO
==========

**Component Type:** Input

**Status:** Implemented

Overview
========

The battery IOs read a NuttX ``battery_gauge`` (``/dev/battN``) and expose the
charge state as ``DTYPE_UINT32`` scalar IOs, so battery telemetry flows through
the generic IO path like any sensor:

- ``CIOBattVolt`` - battery voltage in mV.
- ``CIOBattSoc`` - battery state of charge in percent.
- ``CIOBattState`` - battery charge state as a numeric ``BATTERY_*`` code.

Implementation
==============

Implemented behavior:

- All three IOs are read-only, scalar, ``DTYPE_UINT32``.
- ``configure()`` opens ``/dev/battN`` (``N`` = device number)
- ``getData()`` issues the matching ``BATIOC_VOLTAGE`` / ``BATIOC_CAPACITY`` /
  
This relies on the platform exposing a ``CONFIG_BATTERY_GAUGE`` lower-half at
``/dev/battN`` (for example the Thingy:91 ADP5360 PMIC).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_BATT_VOLT``: enables the battery voltage IO.
- ``CONFIG_DAWN_IO_BATT_SOC``: enables the battery state-of-charge IO.
- ``CONFIG_DAWN_IO_BATT_STATE``: enables the battery charge-state IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: battery_voltage
       type: batt_volt
       dtype: uint32
       config:
         device: 0          # /dev/batt0
     - id: battery_soc
       type: batt_soc
       dtype: uint32
       config:
         device: 0
     - id: battery_state
       type: batt_state
       dtype: uint32
       config:
         device: 0

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOBattVolt <../../doxygen/classdawn_1_1CIOBattVolt.html>`_
- `dawn::CIOBattSoc <../../doxygen/classdawn_1_1CIOBattSoc.html>`_
- `dawn::CIOBattState <../../doxygen/classdawn_1_1CIOBattState.html>`_
