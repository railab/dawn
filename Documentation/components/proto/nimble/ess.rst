===========================
ESS - Environmental Sensing
===========================

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphEss`` - Bluetooth SIG Environmental Sensing Service
(UUID ``0x181A``).

Units conversion: see :ref:`nimble_units`.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_ESS``: enables the Environmental Sensing
  Service.
- ``CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA``: enables optional metadata
  descriptor support for standard services.
- ``CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_USER_DESCRIPTION``: enables
  Characteristic User Description descriptors (``0x2901``).
- ``CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_VALID_RANGE``: enables Valid Range
  descriptors (``0x2906``).
- ``CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_MEASUREMENT``: enables ES Measurement
  descriptors (``0x290C``).
- ``CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_CONFIGURATION``: enables ES
  Configuration descriptors (``0x290B``).
- ``CONFIG_DAWN_PROTO_NIMBLE_ESS_DESC_TRIGGER_SETTING``: enables ES Trigger
  Setting descriptors (``0x290D``).

YAML
----

.. code-block:: yaml

   services:
     ess:
       characteristics:
         - type: temperature
           data: temp1
           metadata:
             user_description: "Ambient temperature"
             valid_range:
               min: 4294963296  # raw sint16 -4000, 0.01 C
               max: 8500        # raw sint16 8500, 0.01 C
             measurement:
               measurement_period: temp_measurement_period_cfg
               update_interval: temp_update_interval_cfg
             configuration: temp_es_configuration_cfg
             trigger_setting: temp_es_trigger_setting_cfg
         - type: humidity
           data: hum1

Supported characteristic ``type`` values:

- ``temperature``
- ``humidity``
- ``pressure``
- ``uv_index``
- ``wind_speed``
- ``wind_direction``
- ``gas_resistance`` (uses non-standard 0x272A UUID; see :ref:`nimble_units`)
- ``light``

Supported metadata fields:

- ``user_description``: UTF-8 text for the Characteristic User Description
  descriptor (0x2901), truncated to 16 bytes by the current generator.
- ``valid_range``: raw BLE-format ``min``/``max`` values for the Valid Range
  descriptor (0x2906). Values are encoded little-endian using the measurement
  characteristic width.
- ``measurement``: fields for the ES Measurement descriptor (0x290C).
  Supported keys are ``flags``, ``sampling_function``,
  ``measurement_period``, ``update_interval``, ``application``, and
  ``uncertainty``. Each configured field is an IO reference, normally a
  ``config`` IO bound to the sensor IO configuration that owns that setting.
  Missing fields are encoded as zero. The descriptor value uses the Bluetooth
  ESS layout: flags as ``uint16``, sampling function as ``uint8``,
  measurement period and update interval as ``uint24`` seconds, application as
  ``uint8``, and uncertainty as ``uint8`` in 0.5 percent units.
- ``configuration``: IO reference for the ES Configuration descriptor
  (0x290B). The referenced IO returns the Bluetooth-format descriptor bytes,
  typically through a ``config`` IO bound to the sensor setting.
- ``trigger_setting``: IO reference for the ES Trigger Setting descriptor
  (0x290D). The referenced IO returns the Bluetooth-format descriptor bytes,
  typically through a ``config`` IO bound to the sensor setting.

Optional Characteristics
========================

Supported optional descriptors:

- Characteristic User Description descriptor (UUID ``0x2901``) through
  ``metadata.user_description``.
- Valid Range descriptor (UUID ``0x2906``) through ``metadata.valid_range``.
- ES Measurement descriptor (UUID ``0x290C``) through
  ``metadata.measurement``.
- ES Configuration descriptor (UUID ``0x290B``) through
  ``metadata.configuration``.
- ES Trigger Setting descriptor (UUID ``0x290D``) through
  ``metadata.trigger_setting``.

Limitations
===========

The ESS Descriptor Value Changed characteristic is not implemented. Dynamic
changes to descriptor metadata are not announced through ESS service-level
metadata characteristics.

Doxygen
=======

- `dawn::CProtoNimblePrphEss <../../../doxygen/classdawn_1_1CProtoNimblePrphEss.html>`_
