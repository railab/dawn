====================
AIOS - Automation IO
====================

**Component Type:** NimBLE GATT Service

**Status:** Implemented

Overview
========

``CProtoNimblePrphAios`` - Bluetooth SIG Automation IO Service
(UUID ``0x1815``).

Each bound IO is exposed as one digital or analog characteristic. Seekable
IOs are rejected at start time; use OTS for those.

Use Digital characteristics for boolean or bitfield-style inputs and outputs,
such as GPIO. Use Analog characteristics for scalar numeric values, including
DAC outputs and PWM duty-cycle controls. A PWM duty cycle is not a boolean
Digital state, so expose PWM duty-cycle controls through ``analog_outputs``.

For multichannel drivers, bind AIOS characteristics to scalar software IO
surfaces and bridge them to the driver with a program. For example, expose one
scalar ``virt`` Analog Output per PWM channel and use ``vecpack`` to write one
multichannel vector to the physical PWM IO.

Runtime configuration surfaces can also be exposed as scalar Analog
characteristics through ``config`` IO bindings. For example, a ``config`` IO
bound to a PWM ``freq`` config item can provide runtime frequency control.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS``: enables the Automation IO Service.
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_AGGREGATE``: enables the Aggregate
  characteristic (``0x2A5A``).
- ``CONFIG_DAWN_PROTO_NIMBLE_EXTENDED_METADATA``: enables optional metadata
  descriptor support for standard services.
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_USER_DESCRIPTION``: enables
  Characteristic User Description descriptors (``0x2901``).
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_NUMBER_OF_DIGITALS``: enables Number
  of Digitals descriptors (``0x2909``).
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_VALUE_TRIGGER_SETTING``: enables Value
  Trigger Setting descriptors (``0x290A``).
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_TIME_TRIGGER_SETTING``: enables Time
  Trigger Setting descriptors (``0x290E``).
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_PRESENTATION_FORMAT``: enables
  Characteristic Presentation Format descriptors (``0x2904``).
- ``CONFIG_DAWN_PROTO_NIMBLE_AIOS_DESC_EXTENDED_PROPERTIES``: enables
  Characteristic Extended Properties descriptors (``0x2900``).

YAML
----

.. code-block:: yaml

   services:
      aios:
        aggregate: true
        groups:
         - digital_inputs:
             - data: din1
               metadata:
                 user_description: "AIOS input 1"
                 number_of_digitals: 1
                 value_trigger_setting: din1_trigger_cfg
                 time_trigger_setting: din1_time_trigger_cfg
                 extended_properties: 0x0000
                 presentation_format:
                   format: 1
                   exponent: 0
                   unit: 0x2700
                   namespace: 1
                   description: 0
             - din2
           digital_outputs: [dout1, dout2]
           analog_outputs:  [aout1]
         - analog_inputs:   [ain1]

Supported fields:

- ``groups``: list of IO groups. Each group may contain ``digital_inputs``,
  ``digital_outputs``, ``analog_inputs``, ``analog_outputs``.
- ``aggregate``: optional boolean. When enabled, Dawn exposes one AIOS
  Aggregate characteristic (UUID ``0x2A5A``) that reads the current values of
  readable Digital characteristics followed by readable Analog characteristics.
- group entries may be plain IO references or objects with ``data`` (or
  ``io``) and optional ``metadata``.
- ``analog_outputs`` may bind writable scalar numeric outputs. For a
  multichannel PWM device, bind these to scalar ``virt`` IOs and use
  ``vecpack`` to update the physical PWM vector.
- ``metadata.user_description``: optional read-only Characteristic User
  Description descriptor (UUID ``0x2901``), encoded as UTF-8 and limited to
  16 bytes in the descriptor configuration.
- ``metadata.number_of_digitals``: optional read-only Number of Digitals
  descriptor (UUID ``0x2909``), encoded as a single unsigned byte. This is
  intended for Digital characteristics.
- ``metadata.value_trigger_setting``: optional read-only Value Trigger Setting
  descriptor (UUID ``0x290A``). The value is read from the referenced IO object,
  whose bytes must already be encoded in the Bluetooth AIOS Value Trigger
  Setting descriptor format.
- ``metadata.time_trigger_setting``: optional read-only Time Trigger Setting
  descriptor (UUID ``0x290E``). The value is read from the referenced IO object,
  whose bytes must already be encoded in the Bluetooth AIOS Time Trigger Setting
  descriptor format. AIOS uses Time Trigger Setting together with Value Trigger
  Setting on the same characteristic.
- ``metadata.presentation_format``: optional read-only Characteristic
  Presentation Format descriptor (UUID ``0x2904``). Fields are encoded in the
  Bluetooth GATT Presentation Format layout: one-byte ``format``, signed
  one-byte ``exponent``, little-endian ``uint16`` ``unit``, one-byte
  ``namespace``, and little-endian ``uint16`` ``description``.
- ``metadata.extended_properties``: optional read-only Characteristic Extended
  Properties descriptor (UUID ``0x2900``). The value is encoded as the standard
  little-endian ``uint16`` bitfield. Set only bits that match the characteristic
  behavior exposed by the binding.

Optional Characteristics
========================

Supported:

- Aggregate characteristic (UUID ``0x2A5A``) through ``aggregate: true``.
- Characteristic User Description descriptor (UUID ``0x2901``) through
  ``metadata.user_description`` on each Digital or Analog binding.
- Number of Digitals descriptor (UUID ``0x2909``) through
  ``metadata.number_of_digitals`` on Digital bindings.
- Value Trigger Setting descriptor (UUID ``0x290A``) through
  ``metadata.value_trigger_setting`` on Digital or Analog bindings.
- Time Trigger Setting descriptor (UUID ``0x290E``) through
  ``metadata.time_trigger_setting`` on Digital or Analog bindings.
- Characteristic Presentation Format descriptor (UUID ``0x2904``) through
  ``metadata.presentation_format`` on Digital or Analog bindings.
- Characteristic Extended Properties descriptor (UUID ``0x2900``) through
  ``metadata.extended_properties`` on Digital or Analog bindings.

Value Trigger Setting Format
============================

The Value Trigger Setting descriptor uses the Bluetooth AIOS descriptor layout:
one condition octet followed by a comparison value. The comparison value length
and meaning depend on the condition:

- no comparison bytes for conditions that use the ``None`` value format;
- ``uint16`` little-endian for an Analog comparison value;
- a Digital bit mask with the same two-bit-field layout and size as the
  corresponding Digital characteristic;
- two ``uint16`` little-endian values for Analog boundary comparisons.

The Dawn descriptor does not reinterpret these bytes. The referenced IO must
return exactly the bytes that should be exposed through descriptor ``0x290A``.

Time Trigger Setting Format
===========================

The Time Trigger Setting descriptor uses the Bluetooth AIOS descriptor layout:
one condition octet followed by a comparison value. The comparison value length
and meaning depend on the condition:

- no comparison bytes for condition ``0x00``, no time-based triggering;
- ``uint24`` little-endian time interval in seconds for conditions ``0x01`` and
  ``0x02``;
- ``uint16`` little-endian count for condition ``0x03``.

The Dawn descriptor does not reinterpret these bytes. The referenced IO must
return exactly the bytes that should be exposed through descriptor ``0x290E``.

Doxygen
=======

- `dawn::CProtoNimblePrphAios <../../../doxygen/classdawn_1_1CProtoNimblePrphAios.html>`_
