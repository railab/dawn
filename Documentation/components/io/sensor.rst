.. _sensor:

======
Sensor
======

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOSensor`` is a generic IO object for reading data from hardware sensors
integrated via the NuttX sensor framework. It supports a variety of sensor
types, including accelerometers, gyroscopes, and environmental sensors.

Sensors use the same scalar representation selected for the NuttX sensor
framework: ``DTYPE_FLOAT`` with ``CONFIG_SENSORS_USE_FLOAT`` or ``DTYPE_B16``
with ``CONFIG_SENSORS_USE_B16``. The dimension of the data (e.g., 3 for a
3-axis accelerometer) is automatically determined based on the sensor type.

.. _dawn_sensor_units:

Default Units
=============

Dawn uses the default NuttX sensor framework units for all sensor-backed IO.
These are the units published by the ``/dev/uorb/sensor_*`` devices and
defined by the NuttX sensor structures.

Internal Dawn components should treat sensor values as already being in these
units. Unit conversion belongs at the protocol boundary: a protocol that
publishes a different external representation must convert from the Dawn
default unit to its protocol-specific unit. This keeps the output unit of
internal processing predictable, including the simple case where a sensor IO
is connected directly to a protocol endpoint.

The default units used by Dawn are:

=================== ======================================
Dawn Type           Default Dawn Unit
=================== ======================================
``accel``           ``float`` or ``b16``, meters per second squared
``mag``             ``float`` or ``b16``, microtesla
``gyro``            ``float`` or ``b16``, radians per second
``light``           ``float`` or ``b16``, lux
``baro``            ``float`` or ``b16``, hectopascal
``prox``            ``float`` or ``b16``, centimeters
``humid``           ``float`` or ``b16``, percent
``temp``            ``float`` or ``b16``, degrees Celsius
``atemp``           ``float`` or ``b16``, degrees Celsius
``rgb``             ``float`` or ``b16``, percent
``ir``              ``float`` or ``b16``, lux
``uv``              ``float`` or ``b16``, UV index
``gas``             ``float`` or ``b16``, kilohms
=================== ======================================

Implementation
==============

The component interacts with the NuttX sensor framework through the
``/dev/uorb/sensor_*`` character devices. It uses the ``poll()`` mechanism
to provide asynchronous notifications when new samples are available.

The following table shows the mapping between Dawn sensor types and the
underlying NuttX sensor structures:

.. csv-table::
   :header: "Dawn Type", "NuttX Path", "NuttX Structure"
   :widths: 30, 30, 40

   "accel", "sensor_accel", "struct sensor_accel"
   "mag", "sensor_mag", "struct sensor_mag"
   "gyro", "sensor_gyro", "struct sensor_gyro"
   "light", "sensor_light", "struct sensor_light"
   "baro", "sensor_baro", "struct sensor_baro"
   "prox", "sensor_prox", "struct sensor_prox"
   "humid", "sensor_humi", "struct sensor_humi"
   "temp", "sensor_temp", "struct sensor_temp"
   "atemp", "sensor_ambient_temp", "struct sensor_temp"
   "rgb", "sensor_rgb", "struct sensor_rgb"
   "ir", "sensor_ir", "struct sensor_ir"
   "uv", "sensor_uv", "struct sensor_uv"
   "gas", "sensor_gas", "struct sensor_gas"

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_SENSOR``: enables sensor-backed IO objects.
- ``CONFIG_SENSORS_USE_FLOAT``: NuttX publishes sensor values as ``float``;
  Dawn sensor descriptors must use ``dtype: float``.
- ``CONFIG_SENSORS_USE_B16``: NuttX publishes sensor values as signed 16.16
  fixed-point ``b16_t``; Dawn sensor descriptors must use ``dtype: b16``.

YAML
----

.. code-block:: yaml

   ios:
     - id: temp1
       type: sensor
       subtype: temp
       dtype: float
       config:
         devno: 0

The ``subtype`` determines the sensor class, and ``devno`` matches the
instance number in the OS (e.g., ``/dev/uorb/sensor_temp0``).

External Control
================

- ``ControlIO``: not supported
- ``TriggerIO``: not supported

Brainstorming & Future Ideas
============================

Configuration items not yet implemented:

* Interval
* Orientation
* Scale
* Filters
* Calibration data
* Threshold
* Gain (polarisation)
* Enable/disable
* Unit
* Alerts

Doxygen
=======

- `dawn::CIOSensor <../../doxygen/classdawn_1_1CIOSensor.html>`_
