.. _descriptors:

===================
Descriptor Examples
===================

This page lists YAML descriptors available in the repository.

These descriptors are example and reference configurations. They show how to
define IO, programs, and protocols for Dawn devices, but they do not all have
the same maturity level or resource requirements.

``os_required`` metadata
========================

The ``metadata.os_required`` field is informational metadata. Dawn does not use
it at build time or runtime; it tells users which OS-visible resources a
descriptor expects to exist.

Use the following naming rule when filling the field:

- use the concrete device-node path for OS-exposed instances
  (for example ``/dev/can0``, ``/dev/adc0``, ``/dev/pwm0``, ``/dev/gpio0``,
  ``/dev/leds0``, ``/dev/buttons0``, ``/dev/urandom``)
- use the explicit configured path when the descriptor hardcodes a transport
  endpoint
  (for example ``/dev/ttyS0``, ``/tmp/ttySIM0``)
- use the concrete sensor device path for sensor-backed IO
  (for example ``/dev/uorb/sensor_accel0``,
  ``/dev/uorb/sensor_humi0``)
- use named subsystem resources for protocol prerequisites without a concrete
  device path in the descriptor
  (for example ``udp``, ``tcp``, or ``ble``)

This keeps ``os_required`` aligned with the actual OS-visible paths used by
Dawn on NuttX.

Feature index
=============

.. list-table::
   :header-rows: 1
   :widths: 20 80

   * - Feature
     - Descriptors
   * - Blinky
     - :ref:`Blinky Buttons <descriptor-blinky-buttons-demo>`,
       :ref:`Blinky CAN <descriptor-blinky-can-demo>`,
       :ref:`Blinky NimBLE <descriptor-blinky-nimble-demo>`,
       :ref:`Blinky Modbus RTU <descriptor-blinky-modbus-rtu-demo>`,
       :ref:`Blinky Serial <descriptor-blinky-serial-demo>`,
       :ref:`Blinky Shell <descriptor-blinky-shell-demo>`
       :ref:`Blinky UDP <descriptor-blinky-udp-demo>`
   * - CAN
     - :ref:`Blinky CAN <descriptor-blinky-can-demo>`,
       :ref:`CAN Button/LED <descriptor-can-button-led>`,
       :ref:`CAN Sensor Producer <descriptor-can-sensor-producer>`,
       :ref:`CAN Dummy IO <descriptor-ntfc-can-dummy>`,
       :ref:`Programs Over CAN <descriptor-ntfc-programs-can>`,
       :ref:`Serial to CAN Gateway <descriptor-gateway-serial-can>`
   * - Dynamic descriptors
     - :ref:`Dynamic Descriptor Slot0 <descriptor-dynamic-desc-slot0>`,
       :ref:`Dynamic Descriptor Slot1 <descriptor-dynamic-desc-slot1>`
   * - Gateway
     - :ref:`Serial to CAN Gateway <descriptor-gateway-serial-can>`
   * - IPC
     - :ref:`Local IPC <descriptor-ipc-demo>`
   * - LwM2M
     - :ref:`NTFC Wakaama LwM2M <descriptor-ntfc-wakaama>`
   * - Modbus
     - :ref:`Blinky Modbus RTU Demo <descriptor-blinky-modbus-rtu-demo>`,
       :ref:`Feature Modbus Slot0 <descriptor-feature-modbus-slot0>`,
       :ref:`Feature Modbus Slot1 <descriptor-feature-modbus-slot1>`,
       :ref:`NTFC Modbus RTU Dummy Map <descriptor-ntfc-modbus-rtu-dummy-map>`,
       :ref:`Modbus TCP Dummy Map <descriptor-modbus-tcp-dummy-map>`
   * - NimBLE / BLE
     - :ref:`Blinky NimBLE <descriptor-blinky-nimble-demo>`,
       :ref:`NimBLE AIOS <descriptor-nimble-aios-demo>`,
       :ref:`NimBLE Environmental Sensor <descriptor-nimble-env-sensor-demo>`,
       :ref:`NimBLE GPIO and Analog <descriptor-nimble-gpio-analog-demo>`,
       :ref:`NimBLE OTS <descriptor-nimble-ots-demo>`,
       :ref:`NimBLE Sensor Producer <descriptor-nimble-sensor-producer>`,
       :ref:`NTFC NimBLE All-Services <descriptor-ntfc-nimble-all>`,
       :ref:`NTFC NimBLE Buffer <descriptor-ntfc-nimble-buffer>`,
       :ref:`NTFC NimBLE Custom Service <descriptor-ntfc-nimble-custom>`,
       :ref:`Shell and NimBLE Environmental <descriptor-shell-nimble-env-demo>`
   * - NXScope
     - :ref:`NXScope Serial <descriptor-nxscope-serial>`,
       :ref:`NXScope UDP <descriptor-nxscope-udp>`
   * - Serial
     - :ref:`Blinky Serial <descriptor-blinky-serial-demo>`,
       :ref:`Dynamic Descriptor Slot0 <descriptor-dynamic-desc-slot0>`,
       :ref:`Dynamic Descriptor Slot1 <descriptor-dynamic-desc-slot1>`,
       :ref:`Serial Core <descriptor-serial-core-demo>`,
       :ref:`Serial LEDs and Buttons <descriptor-serial-leds-buttons-demo>`,
       :ref:`Serial to CAN Gateway <descriptor-gateway-serial-can>`
   * - Shell
     - :ref:`Blinky Shell <descriptor-blinky-shell-demo>`,
       :ref:`Shell Core <descriptor-shell-core-demo>`,
       :ref:`Shell GPIO and Config <descriptor-shell-gpio-config-demo>`,
       :ref:`Shell and NimBLE Environmental <descriptor-shell-nimble-env-demo>`
   * - Tests
     - :ref:`Empty Placeholder <descriptor-empty-placeholder>`
   * - UDP
     - :ref:`Basic UDP <descriptor-udp-basic>`,
       :ref:`Blinky UDP <descriptor-blinky-udp-demo>`
       :ref:`NXScope UDP <descriptor-nxscope-udp>`

Catalog
=======

Examples
--------

.. _descriptor-blinky-buttons-demo:

``descriptors/examples/blinky_buttons_demo.yaml``
  **Blinky Buttons Demo (Extended)**

  Extended 4-mode, 4-button runtime-control example. No protocols are used;
  all logic is managed by interconnected programs (BitSplit, Toggle, Counter,
  Switch, Expression, Redirect, Selector, Sequencer). Button 1 toggles
  run/stop. Button 2 cycles the four modes: all LEDs blink, LEDs chase
  one-by-one, selected LED blinks, and selected LED constant-on. Button 3
  selects the active LED for the two selected-LED modes; those modes keep
  independent selections. Button 4 adjusts the dwell time for blinking modes
  in visible steps.

  Data flow::

    buttons0 -> bitsplit -> button state IOs
      Button 1 -> toggle_run -> selector_leds run gate
      Button 2 -> counter_mode -> selector_modes
      Button 3 -> gated counters -> selected LED for modes 2 and 3
      Button 4 -> counter_p2 -> dwell ConfigIO for modes 0, 1, and 2

  Required resources: ``/dev/buttons0``, ``/dev/leds0``

.. _descriptor-blinky-can-demo:

``descriptors/examples/blinky_can_demo.yaml``
  **Blinky CAN Demo**

  Sequencer-driven LED output with CAN control. Start/stop, reset,
  start-index configuration, individual dwell values, current LED state, and
  segmented access to the full sequencer state table are exposed through CAN
  frames.

  The simple write mappings use standard CAN IDs ``0x150`` through ``0x154``.
  The host helper updates the blink period by writing ``cfg_dwell_off`` on
  ``0x153`` and ``cfg_dwell_on`` on ``0x154``.

  Required resources: ``/dev/can0``, ``/dev/leds0``

  Host helper: :file:`tools/examples/can_blinky_cli.py`

.. _descriptor-blinky-nimble-demo:

``descriptors/examples/blinky_nimble_demo.yaml``
  **Blinky NimBLE Demo**

  Sequencer-driven LED output with BLE control via NimBLE. Start/stop, full
  state table, and individual dwell times are exposed over a custom BLE
  service.

  Required resources: ``/dev/gpio4``, ``ble``

  Host helper: :file:`tools/examples/nimble_blinky_cli.py`

.. _descriptor-blinky-shell-demo:

``descriptors/examples/blinky_shell_demo.yaml``
  **Blinky Shell Demo**

  Provides a shell-controlled runtime example where a sequencer drives an LED
  output. The shell exposes start/stop control and runtime configuration via
  ConfigIO objects (start index and state table).

  Required resources: ``/dev/leds0``

.. _descriptor-blinky-serial-demo:

``descriptors/examples/blinky_serial_demo.yaml``
  **Blinky Serial Demo**

  Provides a serial-controlled runtime example where a sequencer drives the
  board LED lower-half. Start/stop and runtime dwell parameters are exposed
  over the Dawn serial protocol together with readback of the sequencer start
  index and current output state.

  Required resources: ``/dev/leds0``, ``/dev/ttyS0``

  Host helper: :file:`tools/examples/serial_blinky_cli.py`

.. _descriptor-blinky-udp-demo:

``descriptors/examples/blinky_udp_demo.yaml``
  **Blinky UDP Demo**

  Provides a UDP-controlled runtime example where a sequencer drives the
  board LED lower-half. Start/stop and runtime dwell parameters are exposed
  over the Dawn UDP protocol together with readback of the sequencer start
  index and current output state.

  Required resources: ``/dev/leds0``, ``udp``

  Host helper: :file:`tools/examples/udp_blinky_cli.py`

.. _descriptor-blinky-modbus-rtu-demo:

``descriptors/examples/blinky_modbus_rtu_demo.yaml``
  **Blinky Modbus RTU Demo**

  Provides a Modbus-RTU controlled blinky demo where a sequencer drives an LED
  output. Start/stop and runtime parameters are exposed as Modbus registers,
  including writable dwell values and start-index control.

  Required resources: ``/dev/leds0``, ``/dev/ttyS1``

  Host helper: :file:`tools/examples/modbus_blinky_cli.py`

.. _descriptor-can-button-led:

``descriptors/examples/can_button_led.yaml``
  **Minimal CAN Buttons and LEDs Demo**

  Provides a minimal CAN mapping with one writable LED output and one pushed
  button-state input.

  Required resources: ``/dev/can0``, ``/dev/leds0``, ``/dev/buttons0``

.. _descriptor-can-sensor-producer:

``descriptors/examples/can_sensor_producer.yaml``
  **CAN Sensor Producer Demo**

  Receives environmental values over CAN and republishes them as NuttX user
  sensor topics. The descriptor creates temperature, humidity, barometer, and
  light topics using ``sensor_producer`` IOs. Another NuttX application can
  read those topics through ``/dev/uorb`` while Dawn only provides the CAN
  write path.

  CAN write mappings start at ``0x150``:
  ``can_temp_sensor_pub`` is one float, ``can_humi_sensor_pub`` is one float,
  ``can_baro_sensor_pub`` is two floats (pressure, temperature), and
  ``can_light_sensor_pub`` is two floats (light, infrared).

  Required resources:
  ``/dev/can0``, ``/dev/usensor``, ``/dev/uorb/sensor_temp10``,
  ``/dev/uorb/sensor_humi11``, ``/dev/uorb/sensor_baro12``,
  ``/dev/uorb/sensor_light13``

.. _descriptor-dynamic-desc-slot0:

``descriptors/examples/dynamic_desc_slot0.yaml``
  **Dynamic Descriptor Slot0 Bootstrap**

  Bootstrap descriptor exposing descriptor slots and selector over serial for
  runtime descriptor upload and switching.

  Required resources: ``/dev/ttyS1``

.. _descriptor-dynamic-desc-slot1:

``descriptors/examples/dynamic_desc_slot1.yaml``
  **Dynamic Descriptor Slot1 Runtime**

  Runtime descriptor for switch verification. Uses a different dummy initial
  value than slot0 to prove descriptor activation.

  Required resources: ``/dev/ttyS1``

.. _descriptor-feature-modbus-slot0:

``descriptors/examples/feature_modbus_slot0.yaml``
  **Feature Demo - Modbus RTU Bootstrap (slot 0)**

  Board-agnostic Modbus RTU bootstrap descriptor. Exposes ADC and PWM plus the
  descriptor slots and selector so a host can upload and activate the full
  feature descriptor at runtime.

  Required resources: ``/dev/ttyS1``, ``/dev/adc0``, ``/dev/pwm0``

.. _descriptor-feature-modbus-slot1:

``descriptors/examples/feature_modbus_slot1.yaml``
  **Feature Demo - Modbus RTU Full Map (slot 1)**

  Feature demo over Modbus RTU. Exercises ADC, PWM, one LED, one button, a
  sequencer-driven blinky program, runtime config, control
  and trigger bridges, descriptor swap, capabilities, and uname/sysinfo
  introspection.

  Required resources: ``/dev/ttyS1``, ``/dev/userleds``,
  ``/dev/buttons0``

.. _descriptor-gateway-serial-can:

``descriptors/examples/gateway_serial_can.yaml``
  **Serial to CAN Gateway Demo**

  Provides a gateway that forwards virtual IO traffic between serial and CAN,
  including both scalar and segmented 64-bit data paths.

  Required resources: ``/dev/ttyS1``, ``/dev/can0``

.. _descriptor-ipc-demo:

``descriptors/examples/ipc_demo.yaml``
  **Local IPC Demo**

  Exposes two dummy IO values through the local FIFO-based IPC transport for
  same-device control and monitoring.

  Required resources: ``/tmp``

.. _descriptor-nimble-aios-demo:

``descriptors/examples/nimble_aios_demo.yaml``
  **NimBLE Automation IO Service (AIOS) Demo**

  Focused AIOS demo. Exposes four GPIO-backed Digital Inputs,
  four GPIO-backed Digital Outputs, one dummy Analog Input, and five Analog
  Outputs backed by a four-channel ``/dev/pwm0`` driver. Four AIOS Analog
  Outputs write scalar virtual IOs; ``vecpack`` combines those values
  into one four-element PWM duty vector. The fifth Analog Output writes a
  ``config`` IO bound to the PWM frequency. PWM is exposed as Analog Output
  because duty cycle and frequency are numeric values rather than boolean
  Digital states.

  Required resources:
  ``/dev/gpio0``, ``/dev/gpio1``, ``/dev/gpio2``, ``/dev/gpio3``,
  ``/dev/gpio4``, ``/dev/gpio5``, ``/dev/gpio6``, ``/dev/gpio7``,
  ``/dev/pwm0``, ``ble``

.. _descriptor-nimble-env-sensor-demo:

``descriptors/examples/nimble_env_sensor_demo.yaml``
  **NimBLE Environmental Sensor Demo**

  Exposes environmental sensor values and one ADC input over NimBLE standard
  services, with dummy values used for additional demo data.

  Required resources:
  ``/dev/adc0``, ``/dev/uorb/sensor_humi0``, ``/dev/uorb/sensor_temp0``,
  ``/dev/uorb/sensor_baro0``, ``/dev/uorb/sensor_gas0``, ``ble``

.. _descriptor-nimble-gpio-analog-demo:

``descriptors/examples/nimble_gpio_analog_demo.yaml``
  **NimBLE GPIO and Analog Demo**

  Exposes digital inputs, digital outputs, and simple analog values over
  NimBLE services using a mixed GPIO and dummy IO layout.

  Required resources:
  ``/dev/gpio0``, ``/dev/gpio1``, ``/dev/gpio2``, ``/dev/gpio3``,
  ``/dev/gpio4``, ``/dev/gpio5``, ``/dev/gpio6``, ``/dev/gpio7``,
  ``ble``

.. _descriptor-nimble-ots-demo:

``descriptors/examples/nimble_ots_demo.yaml``
  **NimBLE Object Transfer Service Demo**

  Exposes two pre-populated tmpfs files over BLE using the standard Bluetooth
  SIG Object Transfer Service. Bulk data is carried over an L2CAP CoC channel;
  metadata and OACP/OLCP control points are over GATT.

  Required resources: ``ble``, ``/tmp``

.. _descriptor-nimble-sensor-producer:

``descriptors/examples/nimble_sensor_producer.yaml``
  **NimBLE Sensor Producer Demo**

  Receives environmental values over a descriptor-defined NimBLE custom
  service and republishes them as NuttX user sensor topics. It uses the same
  topic layout as ``can_sensor_producer.yaml``: temperature on instance 10,
  humidity on instance 11, barometer on instance 12, and light on instance 13.

  The device advertises as ``dawn-sensor-prod``. The descriptor exposes four
  write-only custom BLE characteristics for those sensor values. Host-side
  writes are handled by :file:`tools/examples/nimble_sensor_producer_cli.py`.

  The NTFC hardware suites use this descriptor together with the
  :file:`examples/apps/usensor_reader` helper so a parallel NuttX app can
  confirm that BLE writes were republished into the expected
  ``/dev/uorb/sensor_*`` topics.

  Required resources:
  ``ble``, ``/dev/usensor``, ``/dev/uorb/sensor_temp10``,
  ``/dev/uorb/sensor_humi11``, ``/dev/uorb/sensor_baro12``,
  ``/dev/uorb/sensor_light13``

.. _descriptor-nxscope-serial:

``descriptors/examples/nxscope_serial.yaml``
  **NXScope Serial Demo**

  Streams accelerometer, magnetometer, gyroscope, and a writable dummy-notify
  signal over an NXScope serial endpoint.

  Required resources:
  ``/dev/ttyS1``, ``/dev/uorb/sensor_accel0``,
  ``/dev/uorb/sensor_mag0``, ``/dev/uorb/sensor_gyro0``

.. _descriptor-modbus-tcp-dummy-map:

``descriptors/examples/qemu_modbus_tcp_dummy_map.yaml``
  **Modbus TCP Dummy Map**

  Provides a Modbus TCP register map backed by a large dummy IO set covering
  coil, packed coil, holding, and input register access.

  Required resources: ``tcp``

.. _descriptor-nxscope-udp:

``descriptors/examples/qemu_nxscope_udp.yaml``
  **NXScope UDP Demo**

  Streams a dummy-notify signal over NXScope UDP and exposes writable channels
  for NXScope user extension set requests.

  Required resources: ``udp``, ``/tmp``

.. _descriptor-serial-core-demo:

``descriptors/examples/serial_core_demo.yaml``
  **Serial Core Demo**

  Provides the core demo IO set over both shell and serial protocols,
  including sensors, ADC, DAC, GPIO, PWM, config objects, and descriptor
  inspection.

  Required resources:
  ``/dev/ttyS1``, ``/dev/uorb/sensor_accel0``,
  ``/dev/uorb/sensor_mag0``, ``/dev/uorb/sensor_gyro0``, ``/dev/gpio0``,
  ``/dev/gpio1``, ``/dev/gpio2``, ``/dev/gpio3``, ``/dev/gpio8``,
  ``/dev/gpio9``, ``/dev/gpio10``, ``/dev/gpio4``, ``/dev/gpio5``,
  ``/dev/gpio6``, ``/dev/gpio7``, ``/dev/adc0``, ``/dev/adc1``,
  ``/dev/adc2``, ``/dev/dac0``, ``/dev/dac1``, ``/dev/dac2``,
  ``/dev/pwm0``, ``/dev/pwm1``, ``/dev/pwm2``, ``/dev/qe0``,
  ``/dev/qe1``, ``/dev/urandom``

.. _descriptor-serial-leds-buttons-demo:

``descriptors/examples/serial_leds_buttons_demo.yaml``
  **Serial LEDs and Buttons Demo**

  Provides serial access to demo data together with one LED output and one
  button input.

  Required resources: ``/dev/ttyS0``, ``/dev/leds0``, ``/dev/buttons0``

.. _descriptor-shell-core-demo:

``descriptors/examples/shell_core_demo.yaml``
  **Shell Core Demo**

  Provides a broad shell-accessible demo of mixed Dawn IO, including sensors,
  ADC, DAC, GPIO, PWM, config objects, descriptor inspection, and processing
  outputs.

  Required resources:
  ``/dev/uorb/sensor_accel0``, ``/dev/uorb/sensor_mag0``,
  ``/dev/uorb/sensor_gyro0``, ``/dev/gpio0``, ``/dev/gpio1``,
  ``/dev/gpio2``, ``/dev/gpio3``, ``/dev/gpio8``, ``/dev/gpio9``,
  ``/dev/gpio10``, ``/dev/gpio4``, ``/dev/gpio5``, ``/dev/gpio6``,
  ``/dev/gpio7``, ``/dev/adc0``, ``/dev/adc1``, ``/dev/adc2``,
  ``/dev/dac0``, ``/dev/dac1``, ``/dev/dac2``, ``/dev/pwm0``,
  ``/dev/pwm1``, ``/dev/pwm2``, ``/dev/qe0``, ``/dev/qe1``,
  ``/dev/urandom``

.. _descriptor-shell-gpio-config-demo:

``descriptors/examples/shell_gpio_config_demo.yaml``
  **Shell GPIO and Config Demo**

  Provides shell access to demo IO, GPIO inputs and outputs, ADC, PWM,
  config-backed values, system information, and reset control.

  Required resources:
  ``/dev/adc0``, ``/dev/pwm0``,
  ``/dev/gpio0``, ``/dev/gpio1``, ``/dev/gpio2``, ``/dev/gpio3``,
  ``/dev/gpio4``, ``/dev/gpio5``, ``/dev/gpio6``, ``/dev/gpio7``

.. _descriptor-shell-nimble-env-demo:

``descriptors/examples/shell_nimble_env_demo.yaml``
  **Shell and NimBLE Environmental Demo**

  Provides shell access and NimBLE exposure for an environmental sensor
  oriented demo with reset and power control objects.

  Required resources:
  ``/dev/uorb/sensor_humi0``, ``/dev/uorb/sensor_baro0``, ``ble``

.. _descriptor-udp-basic:

``descriptors/examples/udp_basic.yaml``
  **Basic UDP Demo**

  Provides basic UDP access to two dummy IO values plus one notify-capable IO
  for simple network protocol demonstrations.

  Required resources: ``udp``, ``/tmp``

NTFC
----

.. _descriptor-ntfc-can-dummy:

``descriptors/ntfc/ntfc_can_dummy.yaml``
  **CAN Dummy IO Demo**

  Provides CAN read, write, indexed, and segmented access to demo IO objects,
  with shell inspection available on the same device.

  Required resources: ``/dev/can0``, ``/dev/urandom``

.. _descriptor-ntfc-modbus-rtu-dummy-map:

``descriptors/ntfc/ntfc_modbus_rtu_dummy_map.yaml``
  **NTFC Modbus RTU Dummy Map**

  Provides an NTFC Modbus RTU register map backed by a large dummy IO set
  covering coil, packed coil, holding, input register, seekable, and
  file access.

  Required resources: ``/dev/ttyS1``, ``/tmp``

.. _descriptor-ntfc-wakaama:

``descriptors/ntfc/ntfc_wakaama.yaml``
  **NTFC Wakaama LwM2M Demo**

  Descriptor used by ``ntfc/tests/wakaama``. It registers as
  ``ntfc-wakaama`` and exposes standard digital, analog, sensor, actuation,
  binary app data, and firmware package resources plus one custom object
  backed by dummy/file IO values.

  Required resources: ``udp``, ``/tmp``

.. _descriptor-ntfc-nimble-all:

``descriptors/ntfc/ntfc_nimble_all.yaml``
  **NTFC NimBLE All-Services Target**

  Descriptor used by ``ntfc/tests/nimble_ntfc`` and the
  ``nimble_ntfc`` board configs for nRF52840-DK and nRF5340-DK. It
  advertises as ``ntfc-nimble`` and enables the supported NimBLE
  service set used by the NTFC BLE suite: DIS, BAS, TPS,
  AIOS, ESS, IMDS, and OTS.

  Most measurement values are backed by ``dummy_notify`` IOs with short update
  intervals so the host can test reads and notifications without requiring
  real sensors. GPIO inputs and outputs are mapped through GPIO devices.
  Configuration characteristics use ``config`` IO objects that point back to
  source IO configuration fields, so writes through GATT update the
  descriptor-backed runtime settings. The OTS section exposes object-transfer
  state for the BLE object transfer tests.

  Required resources:
  ``/dev/gpio0``, ``/dev/gpio1``, ``/dev/gpio2``, ``/dev/gpio3``,
  ``/dev/gpio4``, ``/dev/gpio5``, ``/dev/gpio6``, ``/dev/gpio7``,
  ``ble``

.. _descriptor-ntfc-nimble-custom:

``descriptors/ntfc/ntfc_nimble_custom.yaml``
  **NTFC NimBLE Custom Service Target**

  Minimal descriptor used by ``ntfc/tests/nimble_ntfc_custom`` to validate
  descriptor-declared custom GATT services. It advertises as ``ntfc-nimx`` and
  creates one vendor service
  ``12345678-1234-5678-1234-56789abcdef0`` with one read/write characteristic
  bound to a ``uint32`` dummy IO initialized to ``1234``.

  The test reads the characteristic, writes a new value through BLE, then reads
  it back. This verifies that the descriptor custom-service schema, NimBLE
  service registration, GATT read callback, and GATT write callback all use the
  same bound IO object.

  Required resources: ``ble``

.. _descriptor-ntfc-nimble-buffer:

``descriptors/ntfc/ntfc_nimble_buffer.yaml``
  **NTFC NimBLE Buffer Target**

  Descriptor used by ``ntfc/tests/nimble_ntfc_buffer`` to exercise bulk buffer
  reads over a descriptor-defined NimBLE custom service. It advertises as
  ``ntfc-nimb`` and exposes one vendor service
  ``12345678-1234-5678-1234-56789abcdf00`` with four characteristics:

  - ``...df01`` reads ``buf_out_u32``, the selected output window.
  - ``...df02`` reads and writes ``buf_sel_u32``, the history selector offset.
  - ``...df03`` reads ``buf_stat_u32``, the buffer status words.
  - ``...df04`` writes ``buf_trigger``, the start/stop capture trigger.

  Data flow::

    buf_src_ts_u32 -> buf_buffer1 -> buf_out_u32
                          |              ^
                          |              |
                          +-> buf_stat_u32
                          |
                          +<- buf_sel_u32
                          +<- buf_trigger

  ``buf_src_ts_u32`` is a periodic ``timestamp`` IO. ``buf_buffer1`` is a
  ``CProgBuffer`` with depth ``1024`` and one-shot capture enabled. The buffer
  stores incoming timestamp samples in RAM. ``buf_sel_u32`` selects the history
  offset where ``0`` means newest sample; writes to the selector update the
  output window. ``buf_stat_u32`` reports count, depth, head, overflow,
  snapshot sequence, runtime flags, selected offset, and a reserved word.
  ``buf_trigger`` maps BLE writes to buffer trigger commands: trigger1 restarts
  capture and trigger2 stops capture.

  ``buf_buffer1`` is configured with ``chunk_size: 32``. Because the source
  timestamp is one ``uint32`` value, the buffer initializes ``buf_out_u32`` as a
  32-element virtual output and each GATT read returns 32 samples, or 128
  bytes. The test waits until the 1024-sample buffer is full, stops capture to
  freeze capture, walks selector offsets ``0, 32, 64, ...``, and reads all
  pages through the custom characteristic.

  Required resources: ``ble``

.. _descriptor-ntfc-programs-can:

``descriptors/ntfc/ntfc_programs_can.yaml``
  **Programs Over CAN Demo**

  Provides CAN access to edge-processing programs, including sampled data,
  control endpoints, push notifications, and segmented transfers.

  Required resources: ``/dev/can0``, ``/dev/urandom``

Tests
-----

.. _descriptor-empty-placeholder:

``descriptors/tests/empty_placeholder.yaml``
  **Empty Placeholder Descriptor**

  Provides an empty descriptor used as a placeholder for builds that do not
  require any configured IO, programs, or protocols.

  Required resources: none
