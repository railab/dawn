Sensor Producer
===============

``CIOSensorProducer`` is a write-only IO object that publishes Dawn protocol
data into a NuttX user sensor topic. It registers the topic through
``/dev/usensor`` and writes complete NuttX sensor event structures to the
created ``/dev/uorb/sensor_*`` device.

This is useful when Dawn receives values from a protocol such as CAN, serial,
or UDP, but another application running on the same NuttX system should
consume those values through the standard sensor/uORB interface instead of
through Dawn.

The producer uses the same sensor subtypes, units, and scalar representation
as :doc:`sensor`. For example, a temperature producer writes
``struct sensor_temp`` and an accelerometer producer writes
``struct sensor_accel``.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_SENSOR_PRODUCER``: enables sensor-backed IO objects.
- ``CONFIG_SENSORS_USE_FLOAT``: NuttX publishes sensor values as ``float``;
  Dawn sensor descriptors must use ``dtype: float``.
- ``CONFIG_SENSORS_USE_B16``: NuttX publishes sensor values as signed 16.16
  fixed-point ``b16_t``; Dawn sensor descriptors must use ``dtype: b16``.

YAML
----

.. code-block:: yaml

   ios:
     - id: virtual_temp_out
       type: sensor_producer
       subtype: temp
       dtype: float
       config:
         device: 10
         queue_size: 4
         persist: true

``device`` selects the uORB instance number. The example above creates
``/dev/uorb/sensor_temp10``. Use an instance number that does not collide with
real hardware sensor drivers.

``queue_size`` controls the NuttX sensor circular buffer depth. ``persist``
keeps the latest event readable by late subscribers.

External Control
================

- ``ControlIO``: not supported
- ``TriggerIO``: not supported

Doxygen
=======

- `dawn::CIOSensorProducer <../../doxygen/classdawn_1_1CIOSensorProducer.html>`_
