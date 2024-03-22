==========
PWM Output
==========

**Component Type:** Output

**Status:** Implemented

Overview
========

``CIOPwm`` is a PWM output object with constant frequency.

Frequency defaults to ``CONFIG_DAWN_IO_PWM_DEFAULT_FREQ`` and may be
overridden per descriptor object with the optional ``freq`` config field. Duty
cycle is changed by data.

Data format: ``uint32_t``, dimension must match PWM driver instance number of
channels (for NuttX this is ``CONFIG_PWM_NCHANNELS``). Every write updates the
full duty vector for the PWM device.

Duty cycle uses 0.1% units (0 - 0%, 10 - 1%, 1000 - 100%).

Protocol and UI surfaces often expose one scalar control per output channel,
while ``CIOPwm`` expects one multichannel vector write for the whole device.
For those integrations, expose writable scalar ``virt`` IOs and connect them
to the multichannel ``pwm`` IO with ``vecpack``. Numeric duty-cycle controls
should use analog-style scalar values; boolean or bitfield-style controls are
not a good representation for variable PWM duty.

Implementation
==============

``CIOPwm`` opens the PWM character device at ``/dev/pwm<devno>`` during
``configure()`` and allocates a per-channel duty buffer. ``setData()`` takes
duty cycles in the 0..1000 range (parts-per-mille); values out of range
return ``-EINVAL``. The number of items in the data buffer must match the
configured channel count. Runtime writes to the ``freq`` config item update
the cached frequency and immediately write the full cached PWM state to the
driver. ``doStart()``/``doStop()`` map to NuttX ``PWMIOC_START``/
``PWMIOC_STOP`` ioctls. The IO is write-only and does not support batch
operations.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_PWM``: enables PWM-backed IO objects.
- ``CONFIG_DAWN_IO_PWM_DEFAULT_FREQ``: default PWM frequency in Hz when the
  descriptor does not provide ``freq``. Default: ``1000``.

YAML
----

.. code-block:: yaml

   ios:
     - id: pwm1
       type: pwm
       config:
         device: 0
         freq: 1000

     - id: pwm_ch0
       type: virt
       dtype: uint32

     - id: pwm_freq
       type: config
       dtype: uint32
       config:
         objid_ref: pwm1
         objcfg_ref: freq

   programs:
     - id: pwm_pack
       type: vecpack
       config:
         inputs: [pwm_ch0, pwm_ch1, pwm_ch2, pwm_ch3]
         output: pwm1

External Control
================

ConfigIO: the ``freq`` config item is read-write. Bind a ``config`` IO to
``objcfg_ref: freq`` to expose runtime frequency control. The PWM object must
include a descriptor ``freq`` item for ConfigIO to read and write; if ``freq``
is omitted, the object still uses ``CONFIG_DAWN_IO_PWM_DEFAULT_FREQ`` but
there is no descriptor-backed field for ConfigIO.

Constant duty with data-controlled frequency is not supported yet. It needs a
new IO object because ``CIOPwm`` data writes are duty updates; frequency is
configuration.

ControlIO: not supported yet.

TriggerIO: not supported.

Planned: ``CIOPwm`` should support start/stop via ``CIOControl``.

Brainstorming & Future Ideas
============================

Use cases:

- PWM output for capture inputs
- Control DC offset with PWM (for example for heaters)
- PWM as DAC output
- Drive induction loads
- Generate pulses
- Control motor speed

Doxygen
=======

- `dawn::CIOPwm <../../doxygen/classdawn_1_1CIOPwm.html>`_
