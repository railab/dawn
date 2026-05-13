===========
LEDs Output
===========

**Component Type:** Output

**Status:** Implemented

Overview
========

``CIOLeds`` - LED output object that packs LED states as bits in a
``uint32_t`` value.

Only SET/RESET LED state is supported by this IO. For more complex LED
effects, we need another IO.

Implementation
==============

``CIOLeds`` opens the LED character device at ``/dev/userleds<devno>``
during ``configure()`` and exchanges a packed ``uint32_t`` bitmap with it on
``getData()``/``setData()``. Each bit selects one LED channel; ``getData``
returns the current LED states, ``setData`` writes a new bitmap. Batch
operations are not supported.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_LEDS``: enables LED-backed IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: leds1
       type: leds

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- How should LED effects be handled?
- Effects can be implemented in software or in hardware
  (for example in dedicated LED driver chips).
- Software effects can run on kernel side or on Dawn side.
- Dawn LED effects can be implemented as Programs or directly in IO.
- LED effect handling can include simple on/off, but simple on/off IO does not
  provide effect features.

Possible LED implementations:

1. Single LED on/off -> handled with GPO.
2. Many LEDs on/off -> handled with LED IO.
3. Single LED PWM -> handled with PWM.
4. Many LEDs PWM -> handled with multi-channel PWM.
5. Single LED effect -> handled with LED IO.
6. Many LEDs effect -> handled with LED IO (not supported yet in NuttX).

Doxygen
=======

- `dawn::CIOLeds <../../doxygen/classdawn_1_1CIOLeds.html>`_
