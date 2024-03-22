.. _limits:

=========
IO Limits
=========

Overview
========

``CIOLimits`` is a framework-level utility used by IO objects to enforce
runtime constraints on data values. It allows defining a valid range and
step for input and output data, which is especially useful for setpoint
validation or protecting hardware from out-of-bounds commands.

The validation is applied against three optional descriptor-backed arrays:
``min``, ``max``, and ``step``.

Implementation
==============

The ``CIOLimits`` class stores pointers to data in the descriptor and
performs validation when its ``validate()`` method is called. It supports
all standard Dawn data types (integer, fixed-point, and floating-point).

Validation rules:
1. **Min/Max**: The value must be within the inclusive range ``[min, max]``.
2. **Step**: If a non-zero ``step`` is defined, the value must satisfy
``(value - min) % step == 0``. For floating-point types, this is
checked with an appropriate epsilon.

If the ``CONFIG_DAWN_IO_LIMITS`` Kconfig option is disabled, the class
becomes a no-op to save resources.

Configuration
=============

Limits are typically configured as part of a compatible IO object's
configuration block.

Kconfig
-------

- ``CONFIG_DAWN_IO_LIMITS``: Enables the runtime limits validation logic.

YAML
----

While ``CIOLimits`` is a helper class, its configuration is exposed
through protocol-specific or IO-specific configuration items. In YAML,
this is often represented by ``min``, ``max``, and ``step`` fields:

.. code-block:: yaml

   ios:
     - id: analog_output
       type: dac
       config:
         device: 0
         min: [0]
         max: [4095]
         step: [1]

Doxygen
=======

- `dawn::CIOLimits <../../doxygen/classdawn_1_1CIOLimits.html>`_
