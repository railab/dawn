=======
Roadmap
=======

This roadmap outlines planned features, improvements, and known issues for the
Dawn project. Items are organized by priority and category.

Tier 1
======

#. dawnpy packages in PIP

#. find a better way to organise descriptors, now it is a mess

#. more real HW demos so we cover all supported IO on real HW

   - CAN network example
   - GUI integration example

#. Demo: Thingy:53

   - [X] set RGB
   - [ ] battery state
   - [ ] read sensors over BLE
   - [ ] gas sensor demo
   - [ ] reconfigure sensors parameters over BLE
   - [ ] RF switch support
   - [ ] low power mode
   - [ ] something with button
   - [ ] something with buzzer
   - [ ] something with microphone
   - [ ] something with QSPI mem
   - [ ] something with USB
   - [ ] something with NFC

#. release nxscope-gui

#. validate support for fixed point math

#. Use ISO-TP from NuttX libs for CAN simple (not yet upstream)

#. generate defconfig.dawn based on YAML descriptor

#. optimise for small systems

   - small system configurations for mem footprint tracking
   - disable more teardown logic for systems than never exit from Dawn app

#. optimise for code execution speed

#. CAN FD support for simple CAN

#. load descriptor from file

   - configure descriptor file from CLI args

Tier 2
======

#. Program: On-off controller

   - setpoint, input, output as IO
   - controller parameters in config

#. Program: PID controller

   - setpoint, input, output as IO
   - controller parameters in config

#. Protocol: MQTT (Sparkplug) support

   - ``spBv1.0/group1/device_1/temperature/sensor_1``
   - https://sparkplug.eclipse.org/specification/
   - https://www.hivemq.com/blog/mqtt-payload-structures-iiot/

#. Protocol: LelyCAN support (CANopen)

   - 16b index and 8b subindex, eg.index=0x6000, subindex=0x01

#. Firmware updates

   - download from web (external link)
   - download over protocol
   - download from external mem (usb, sdcard)
   - reboot to bootlaoder mode or DFU
   - A/B partition support
   - Safe updates with rollback
   - kernel only vs app only vs all

#. Demo: Thingy:52

   - simple BLE demo
   - optimise for size

#. Demo: Thingy:91

   - wakaama demo

#. Demo: Arduino Control

   - need PMIC support in NuttX so we can remove Arduino bootlaoder
   - AMP demo (CM4 + CM7)

#. batch support for sensors

#. batch support for ADC

Tier 3
======

#. IO: read system log (syslog)

#. IO: improve ADC support

   - analyse all ADC use cases
   - optimise for high sample rate

#. IO: DAC support

   - multi channel support
   - how to use DMA + TIM ? new DAC type ?

#. IO: LED effects and RGB LED effects

   - easy to use LED effects

#. IO: GPO bulk output support

   - set many GPIO with single write
   - missing feature on NuttX side

#. IO: GPI bulk input support

   - read many GPIO with single read
   - missing feature on NuttX side

#. IO: Counter Input / Capture Input

#. IO: Comparator Input

#. IO: Opamp IO

   - write for gain control

#. IO: GNSS

   - how to handle complex data struct returned by NuttX ?

   - return all data at once, or many IO for different data ?

#. IO: Protocol Based IO

   - Dawn device as master for other protocols
   - can be hard to configure from descriptor, but should be possible

#. IO: Microphone Input

   - sampling frequency
   - stream IO like for ADC case

#. IO: Buzzer Output

   - sound notifications

#. IO: Joystick

   - easy to use joystick events

#. Program: more filters

   - high pass filters

#. Program: voter

   - Safety redundancy logic.
   - Majority and median-based robust value selection.
   - N inputs -> selected/combined output.

#. how to handle NFC/Contactless

   - shoult it be IO or PROGR, or both ?
   - read from NFC, write to NFC
   - use NFC as device configuratior

#. Watchdog integration

#. Functional safety features

#. Security features

#. Long-running stability tests

#. descriptor linter - check for common mistakes (unused objects, unreachable IOs)

#. Power management support, low power devices

#. Descriptor templates for common patterns

#. Descriptor templates for common patterns

   - BLE sensor
   - GPI/GPO over modbus
   - ADC over modbus
   - CAN node
