====================================
IMDS - Industrial Measurement Device
====================================

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphImds`` - Bluetooth SIG Industrial Measurement Device
Service (UUID ``0x185A``).

Units conversion: see :ref:`nimble_units`.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_IMDS``: enables the Industrial Measurement
  Device Service.
- ``CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA``: enables optional metadata
  descriptor support for standard services.
- ``CONFIG_DAWN_PROTO_NIMBLE_IMDS_DESC_USER_DESCRIPTION``: enables
  Characteristic User Description descriptors (``0x2901``).

YAML
----

.. code-block:: yaml

   services:
     imds:
       temperature:
         data: temp1
         metadata:
           user_description: "Process temperature"
       humidity:       hum1
       pressure:       press1
       uv_index:       uv1
       gas_resistance: gas1
       light:          light1

Supported keys:

- ``temperature``
- ``humidity``
- ``pressure``
- ``uv_index``
- ``gas_resistance`` (uses non-standard 0x272A UUID; see :ref:`nimble_units`)
- ``light``

Each measurement may be either a direct IO reference or a mapping with
``data``/``io`` and ``metadata``. Supported metadata:

- ``user_description``: UTF-8 text for the Characteristic User Description
  descriptor (UUID ``0x2901``), truncated to 16 bytes.

Optional Characteristics
========================

Implemented per-measurement descriptors:

- Characteristic User Description (``0x2901``) from
  ``metadata.user_description``.

Other IMDS service-level status, lifecycle, control, and historical-data
characteristics are not implemented yet.

Doxygen
=======

- `dawn::CProtoNimblePrphImds <../../../doxygen/classdawn_1_1CProtoNimblePrphImds.html>`_
