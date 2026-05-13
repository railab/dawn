==============
RGB LED Output
==============

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIORgbLed`` controls NuttX RGB LED character devices such as
``/dev/rgbled0``. Dawn exposes the color as one ``uint32`` scalar encoded as
``0x00RRGGBB`` while the NuttX RGB LED driver expects a ``#RRGGBB`` string.
The IO performs that conversion internally on every write.

This is intended for board RGB indicators such as the Nordic Thingy:53 RGB LED
and for simulator tests using ``CONFIG_DAWN_FAKE_RGBLED``.

Implementation
==============

- Opens ``/dev/rgbled<device>`` during ``configure()``.
- Data type: ``uint32``.
- Data shape: one scalar value.
- Value encoding: ``0x00RRGGBB``.
- Write path converts the value to ``#RRGGBB`` and writes the 8-byte NUL
  terminated string expected by NuttX RGB LED devices.
- Read path returns the last value written by Dawn, including the configured
  initial value. The upstream NuttX RGB LED upper half does not currently
  report hardware state, so Dawn keeps the runtime value locally.
- Batch operations are not supported.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_RGB_LED``: enables RGB LED IO support.
- ``CONFIG_DAWN_DTYPE_UINT32``: required data type support.
- ``CONFIG_RGBLED``: required on boards using the upstream NuttX RGB LED
  driver, such as Thingy:53.

YAML
----

.. code-block:: yaml

   ios:
     - id: rgbled0
       type: rgb_led
       dtype: uint32
       config:
         device: 0
         init_val: 0x000000

``config.device`` selects ``/dev/rgbled<device>``.

``config.init_val`` is optional. When present, the value is written during
``init()``.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIORgbLed <../../doxygen/classdawn_1_1CIORgbLed.html>`_
