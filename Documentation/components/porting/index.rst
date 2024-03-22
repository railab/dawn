=============
Porting Layer
=============

The **Dawn** porting layer provides an abstraction between the core framework
and the underlying Operating System (OS) and hardware drivers. Porting
implementations are located in ``dawn/src/porting/`` and
``dawn/include/dawn/porting/``.

Currently, Dawn officially supports the **Apache NuttX** RTOS. There are
**no plans to support other Operating Systems** at this time, although the
architecture remains modular.

System Requirements
===================

To port Dawn to a new system (though not currently planned), the following
features must be provided:

* **C++11 or newer**: Support for ``std::thread``, ``std::mutex``, and atomics.
* **POSIX-like API**: Support for ``open()``, ``close()``, ``read()``,
  ``write()``, and ``poll()``.
* **CMake**: The build system is exclusively based on CMake.

Board Support
=============

Actual board support and hardware-specific configurations are located in the
``boards/`` directory. Dawn utilizes the standard Apache NuttX board support
model: board directories contain board-specific files, ``defconfig``, and
pin mappings.

The OS porting layer (``dawn/src/porting/nuttx/board.cxx``) provides
the logic to bootstrap the framework on the target OS, but the specific
hardware resources are defined in the board support package.

Driver Porting
==============

IO objects interact with hardware through porting headers. To support a new
peripheral type on a platform, implement the corresponding interface in
``dawn/include/dawn/porting/``.

Example Driver Implementation
-----------------------------

When implementing a new driver port (e.g., for ADC):

1. Define the platform-specific structure in the porting header.
2. Implement the hardware access logic in ``dawn/src/porting/<os>/adc.cxx``.
3. Ensure the ADC IO class (for example ``CIOAdcFetch``) can utilize these
   functions.

API Reference
=============

The following files define the required interfaces for hardware abstraction:

.. list-table::
   :widths: 30 70
   :header-rows: 1

   * - Header
     - Description
   * - ``board.hxx``
     - Board-specific bootstrap and resource mapping.
   * - ``gpio.hxx``
     - General Purpose I/O read/write and configuration.
   * - ``adc.hxx``
     - Analog-to-Digital conversion.
   * - ``pwm.hxx``
     - Pulse Width Modulation output.
   * - ``dac.hxx``
     - Digital-to-Analog conversion.
   * - ``can.hxx``
     - Controller Area Network frame exchange.
   * - ``sensors.hxx``
     - Unified sensor framework interface.
   * - ``encoder.hxx``
     - Quadrature encoder interface.
   * - ``leds.hxx``
     - Board-level LED indicators.
   * - ``buttons.hxx``
     - Board-level push button inputs.
   * - ``crc.hxx``
     - CRC calculation.
   * - ``fixedmath.hxx``
     - Fixed-point mathematics support.
   * - ``group.hxx``
     - Documentation group definitions for Doxugen.

The API declarations are located in ``dawn/include/dawn/porting/*.hxx``.

Adding a New OS
===============

While Dawn is currently focused on NuttX, adding a new OS involves:

1. Creating a new directory in ``dawn/src/porting/<new_os>/``.
2. Implementing the core driver abstractions.
3. Providing a ``config.hxx`` that maps OS-specific headers.
4. Updating the CMake build system to recognize the new target.

**Important Note**: The ``boards/`` directory is dedicated to Apache
NuttX-compatible configurations. Any implementation for another OS would need
to handle board support through its own native mechanism and structure.
