.. _ntfc:

======================
NTFC Integration Tests
======================

NTFC (NuttX Test Framework Client) tests exercise Dawn end-to-end: a
firmware image runs on a target and the host communicates with it over
real protocol interfaces (CAN socket, serial PTY, UDP, Modbus RTU,
Bluetooth LE).

Sessions are split across two manifests so the host CI suite stays
independent of physical hardware:

* ``ntfc/manifest-host.yaml`` - sim and QEMU sessions; runs in normal
  CI as part of ``dawnpy-tests``. This is the ``--ntfc-list`` default.
* ``ntfc/manifest-nrf52840dk.yaml`` - hardware-in-the-loop sessions
  for the Nordic nRF52840-DK. **Not** run by default; opt in with
  ``--ntfc-list``.
* ``ntfc/manifest-nucleo-c071rb.yaml`` - hardware-in-the-loop Modbus
  RTU session for the STM32 Nucleo-C071RB. **Not** run by default; opt
  in with ``--ntfc-list``.

Targets
=======

* **sim** - NuttX simulator running as a Linux process. Requires
  ``testenv_init.sh`` to set up the virtual ``can0`` interface.
* **qemu-intel64** - QEMU ``qemu-system-x86_64`` VM with KVM and an
  emulated e1000 NIC bridged via ``tap0``. Used for UDP tests.
* **nrf52840-dk** - real hardware. A connected nRF52840-DK over USB
  (DAPLink/J-Link), ``nrfutil`` for flashing/reset, and a BLE-capable
  Linux host (BlueZ + L2CAP CoC). Used for NimBLE service, OTS, and
  descriptor-defined custom GATT tests.
* **nucleo-c071rb** - real hardware. A connected STM32 Nucleo-C071RB
  over USB for ST-LINK flashing and console, plus a host Modbus RTU
  adapter wired to the board USART1 RS485 pins through a compatible
  RS485 transceiver. The Modbus client uses ``/dev/ttyUSB0`` by default;
  override with ``DAWN_NTFC_MODBUS_PORT``.

Host requirements
-----------------

Each suite below depends on different host facilities (Linux only -
see :doc:`/environment` for the general host setup):

* CAN-based suites (``can``, ``programs_can``, ``gateway``) need
  SocketCAN with the ``vcan`` kernel module - ``testenv_init.sh``
  brings up the virtual ``can0`` interface.
* Serial-based suites (``serial``, ``modbus`` RTU, ``nxscope``,
  ``dynamic_desc``) need the ``socat`` PTY bridge between
  ``/tmp/ttySIM0`` and ``/tmp/ttyNX0``.
* Hardware Modbus RTU on ``nucleo-c071rb`` needs ``st-flash`` and a
  host RS485 adapter.
* QEMU-based suites (``udp``, ``modbus_tcp``) need
  ``qemu-system-x86_64`` with KVM and kernel ``tun``/``bridge``
  support; ``testenv_init.sh`` sets up ``br0`` + ``tap0``.
* Other ``sim`` suites (``shell``, ``blinky_shell``) only need the
  base build environment.

Test Suites
===========

.. list-table::
   :widths: 18 16 66
   :header-rows: 1

   * - Suite
     - Target
     - What is tested
   * - ``shell``
     - sim
     - Shell commands (``help``, ``info``, ``getio``, ``setio``,
       ``getcfg``); inspector (``list``, ``inspect``, ``tree``,
       ``stats``); error handling.
   * - ``can``
     - sim
     - Simple write, RTR read, ISO-TP segmented read/write,
       DescriptorIO segmented read, indexed read/write, push
       notifications.
   * - ``serial``
     - sim
     - Ping, IO list, read/write round-trip (int32, uint32,
       uint64, float).
   * - ``modbus``
     - sim
     - Coil read/write, holding register read/write (uint16 +
       uint64 span), input register read, seekable descriptor and
       capabilities windows, seekable FileIO read/write.
   * - ``modbus_hardware``
     - nucleo-c071rb
     - Same Modbus RTU dummy register map as the sim suite, exercised
       on the board USART1 RS485 interface.
   * - ``udp``
     - qemu-intel64
     - Ping, IO list, read/write round-trip (uint16, float).
   * - ``gateway``
     - sim
     - Serial->CAN and CAN->Serial data routing; segmented uint64
       in both directions.
   * - ``programs_can``
     - sim
     - Full PROG pipeline over CAN: latest, min, max, RMS,
       redirect, moving average, IIR filter, threshold
       (bool + value), push notifications, program start/stop
       freeze, segmented uint64 sampling, ring buffer select
       and stat array read.
   * - ``dynamic_desc``
     - sim
     - Runtime descriptor upload, slot switch validation, and
       rollback to slot 0 over serial.
   * - ``nimble_ntfc``
     - nrf52840-dk
     - NimBLE all-services hardware target using fake GPIO,
       ``dummy_notify`` sensor values, BAS battery notifications, and OTS
       file transfer. Pulls ``dawnpy-ble`` for the GATT/L2CAP CoC client.
   * - ``nimble_ntfc_buffer``
     - nrf52840-dk
     - Hardware NimBLE custom-service buffer test. Captures 1024 timestamp
       samples into ``CProgBuffer``, exposes the selected buffer window over a
       descriptor-defined GATT characteristic, and verifies 32-sample bulk
       reads configured by ``buffer.chunk_size``.

Running
=======

.. code:: shell

   dawnpy-tests               # full host suite (manifest-host.yaml)
   dawnpy-tests --ntfc-only   # NTFC step only

   # hardware-in-the-loop manifest (nRF52840-DK + BLE)
   dawnpy-tests --ntfc-only \
       --ntfc-list ntfc/manifest-nrf52840dk.yaml

   # hardware-in-the-loop manifest (Nucleo-C071RB + Modbus RTU)
   dawnpy-tests --ntfc-only \
       --ntfc-list ntfc/manifest-nucleo-c071rb.yaml

   # single host suite
   python -m ntfc test \
       --confpath ntfc/configs/sim/can/config.yaml \
       --testpath ntfc/tests/can

   # single hardware suite (requires connected nRF52840-DK)
   python -m ntfc test \
       --confpath ntfc/configs/nrf52840-dk/nimble_ntfc/config.yaml \
       --testpath ntfc/tests/nimble_ntfc

   # single hardware custom-service buffer suite
   python -m ntfc test \
       --flash \
       --confpath ntfc/configs/nrf52840-dk/nimble_ntfc_buffer/config.yaml \
       --testpath ntfc/tests/nimble_ntfc_buffer

   # single hardware Modbus suite (requires connected Nucleo-C071RB and RS485)
   python -m ntfc test \
       --flash \
       --confpath ntfc/configs/nucleo-c071rb/modbus_ntfc/config.yaml \
       --testpath ntfc/tests/modbus_hardware
