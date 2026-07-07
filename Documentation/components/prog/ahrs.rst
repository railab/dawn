====
AHRS
====

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgAhrs`` - AHRS sensor-fusion program. Fuses an accelerometer IO and
a gyroscope IO (x-io Fusion library, Madgwick's revised algorithm) into
world-frame linear acceleration: gravity removed, Z up, m/s².

- accelerometer + gyroscope in (float, dim >= 3; extra elements such as a
  trailing temperature are ignored), optional magnetometer for absolute
  heading (9-DOF; shares the rejection threshold with the accelerometer)
- world-frame linear acceleration out (float virt, dim 3, notify)
- runtime-writable filter parameters (via a writable ``config`` IO)

Data types
----------

The x-io Fusion library computes in ``float`` only (not type-parameterised),
so all bound IOs must be ``float``. For other sources convert upstream with
``adjust`` — the program expects m/s² and rad/s.

Implementation
==============

The program is notifier-driven (no thread). Each accelerometer notification
caches the latest sample (converted to g). Each gyroscope notification
(converted to deg/s, gyro-offset-corrected) advances the AHRS with a ``dt``
measured from the monotonic clock (clamped to [1 ms, 100 ms]; outside the
window the nominal ``1/rate`` is used) and writes the earth acceleration
(in m/s²) to the output IO. No output is produced until the first
accelerometer sample has been seen.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_AHRS``: enables the AHRS program. Depends on
  ``CONFIG_LIB_MADGWICK``.

Descriptor
----------

.. code-block:: yaml

   - id: ahrs1
     type: ahrs
     config:
       accel: *accel0          # float sensor/virt IO, notify
       gyro: *gyro0            # float sensor/virt IO, notify
       mag: *mag0              # optional magnetometer (9-DOF)
       output: *fused_src      # float virt IO (shape owned by the program)
       params:
         gain: 0.5             # filter gain
         accel_rejection: 10.0 # acceleration rejection threshold [deg]
         recovery_period: 5.0  # recovery trigger period [s]
         rate: 50.0            # nominal sample rate [Hz]

``params`` is a single 4-float config item. A writable ``config`` IO with
``objcfg_ref: params`` makes it runtime-writable; out-of-range writes are
rejected (gain 0-10, accel_rejection 0-90, recovery_period 0-60,
rate 1-1000).
