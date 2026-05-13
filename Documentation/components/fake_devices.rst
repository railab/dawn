.. _fake_devices:

============
Fake Devices
============

Dawn ships a small collection of **fake NuttX drivers** under
``boards/common/`` (sources in ``boards/common/src/``, header in
``boards/common/include/fake_drivers.h``). These plug into the standard
NuttX driver interfaces (ADC, DAC, PWM, sensors, GPIO, LEDs, buttons,
etc.) but contain no real hardware access - they are minimal stubs that
let Dawn run on targets without those peripherals.

Purpose
=======

The fake drivers exist to make Dawn usable on targets that have no real
peripherals: the NuttX **simulator** and **QEMU** targets. With the
fake drivers enabled, the same descriptor-driven IO/PROG/PROTO pipeline
that normally talks to hardware can be exercised entirely on the host.

This is useful for:

* Local development of Dawn itself without a board flashed and wired up.
* Running examples and demos on a Linux machine.
* Driving the QA pipeline (``dawnpy-tests``) - both unit tests and the
  NTFC integration suites depend on the simulator and QEMU configs that
  enable these drivers.

The current implementations are **deliberately simple**. They satisfy
the driver interface and return plausible (often constant or trivially
generated) data so that higher layers behave normally. They are not yet
attempting to model the timing, noise, or behavioural quirks of real
silicon. Improving fidelity is on the roadmap; for now treat them as
behavioural stubs, not simulation models.

Enabling
========

The fake drivers are gated by Kconfig options under
``boards/common/Kconfig``. Any board (in-tree or out-of-tree) that
wants to use them sources that file:

.. code-block:: kconfig

   source "$(DAWN_BOARDS_COMMON)/Kconfig"

The ``DAWN_BOARDS_COMMON`` environment variable is exported by Dawn's
build system so the same line works from in-tree and OOT boards alike.

Once sourced, enable the master switch and any drivers you need in your
board ``defconfig.dawn``:

.. code-block:: kconfig

   CONFIG_DAWN_FAKE_DRIVERS=y
   CONFIG_DAWN_FAKE_ADC=y
   CONFIG_DAWN_FAKE_SENSORS=y
   # ... etc

Each fake driver depends on the corresponding NuttX subsystem
(``CONFIG_ADC``, ``CONFIG_DAC``, ``CONFIG_PWM``, ``CONFIG_SENSORS``,
...) being enabled.

Boards then call the matching ``fake_*_initialize()`` from their bringup
code (see ``boards/sim/sim/sim/src/sim_bringup.c`` for a worked
example).

Supported fake drivers
======================

.. list-table::
   :widths: 22 18 60
   :header-rows: 1

   * - Kconfig option
     - Device node
     - Behaviour
   * - ``DAWN_FAKE_ADC``
     - ``/dev/adcN``
     - 32-channel ADC. A background kthread feeds a fixed sample buffer
       through the standard upper-half callback at a configurable
       interval; supports ``ANIOC_TRIGGER`` / ``ANIOC_STOP`` /
       ``ANIOC_SET_TIMER_FREQ``.
   * - ``DAWN_FAKE_DAC``
     - ``/dev/dacN``
     - DAC sink that accepts writes and logs them via ``ainfo()``. No
       output, no waveform generation.
   * - ``DAWN_FAKE_PWM``
     - ``/dev/pwmN``
     - PWM lower-half stub. Accepts ``start`` / ``stop`` / ``ioctl`` and
       satisfies the upper-half driver; produces no signal.
   * - ``DAWN_FAKE_ENCODER``
     - ``/dev/qeN``
     - Quadrature encoder with software-maintained position. Supports
       position read, position-max set, reset, and index set via the
       standard ``qe_ops_s`` interface.
   * - ``DAWN_FAKE_IOEXPANDER``
     - ``/dev/gpioN``
     - Wraps NuttX's ``ioe_dummy`` I/O expander and registers a small
       set of input, output, and interrupt-capable GPIO pins via
       ``gpio_lower_half()``.
   * - ``DAWN_FAKE_SENSORS``
     - ``/dev/uorb/...``
     - Registers UORB sensor nodes (accelerometer, magnetometer,
       gyroscope; GNSS variants supported by ``fakesensor2``) backed by
       in-memory sample arrays. Streams the array on an interval.
   * - ``DAWN_FAKE_USERLEDS``
     - ``/dev/userleds``
     - User-LED lower-half. Tracks per-LED state in memory; reports the
       supported LED set based on the requested LED count.
   * - ``DAWN_FAKE_BUTTONS``
     - ``/dev/buttonsN``
     - Button input lower-half. Tracks per-button state in memory;
       press/release callbacks can be installed but are not driven by
       any real input source yet.
   * - ``DAWN_FAKE_UID``
     - boardctl
     - Provides a constant 16-byte unique-ID returned by
       ``board_uniqueid()`` (currently ``de ad be ef be ef de af`` in
       the first eight bytes, zero-padded).
   * - ``DAWN_FAKE_RESET``
     - boardctl
     - Implements ``board_reset()`` and ``board_reset_cause()`` as
       no-ops that just log; lets ``BOARDCTL_RESET`` call sites work
       on targets that cannot actually reset.
   * - ``DAWN_FAKE_POWEROFF``
     - boardctl
     - Implements ``board_power_off()`` as a no-op; lets
       ``BOARDCTL_POWEROFF`` call sites work on targets that cannot
       actually power off.

Roadmap
=======

The current implementations are minimal stubs aimed at "make the upper
layers happy". Planned improvements include:

* Configurable sample/value generators (waveforms, noise, scripted
  sequences) instead of constant or fixed-array data.
* Loopback wiring between DAC and ADC, PWM and encoder, GPIO outputs and
  inputs, so PROG pipelines can be exercised end-to-end on the host.
* Optional host-side control hooks (driving values from a test runner)
  so NTFC suites can validate behaviour beyond the current driver stub.

See :doc:`/qa/index` for how the QA pipeline already exercises these
drivers via the simulator and QEMU configs, and :doc:`/guides/host_development`
for the broader picture of host-based Dawn development.
