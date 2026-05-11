=========
Simulator
=========

``/boards/sim/sim/sim``

Custom NuttX simulator board used by Dawn:

- unnecessary functionality removed from board files
- most configurations enable Dawn's fake drivers so the descriptor-driven
  pipeline runs on the host without real peripherals; see
  :doc:`/components/fake_devices` and :doc:`/host_development` for the
  broader host workflow

Configs
=======

dynamic_desc
------------

Serial descriptor-switching profile using ``dynamic_desc_slot0.yaml`` as the
boot descriptor. It enables descriptor slots, descriptor upload, descriptor
selection, and capabilities reporting.

nsh_blinky_shell
----------------

Simulator runtime-control example with shell access. A sequencer drives a fake
user LED, while start/stop and dwell reconfiguration are available from the
``blinky>`` shell prompt. Uses ``blinky_shell_demo.yaml``.

nsh_can
-------

CAN protocol reference configuration using ``ntfc_can_dummy.yaml`` with the
Dawn shell enabled for inspection.

nsh_feature_modbus
------------------

Feature Modbus RTU profile using ``feature_modbus_slot0.yaml`` as the boot
descriptor. It enables descriptor slots so the full feature descriptor can be
uploaded and activated at runtime.

nsh_gateway
-----------

Serial-to-CAN gateway reference configuration using
``gateway_serial_can.yaml``.

nsh_modbus_rtu
--------------

Modbus RTU reference configuration using ``ntfc_modbus_rtu_dummy_map.yaml``.

Prepare host link:

.. code:: bash

   socat PTY,link=/tmp/ttySIM0 PTY,link=/tmp/ttyNX0

nsh_nxscope
-----------

NXScope serial reference configuration using ``nxscope_serial.yaml``.

Prepare host link:

.. code:: bash

   socat PTY,link=/tmp/ttySIM0 PTY,link=/tmp/ttyNX0

nsh_programs_can
----------------

Programs-over-CAN reference profile using ``ntfc_programs_can.yaml``.

nsh_usensor_can
---------------

CAN-to-user-sensor reference profile using ``can_sensor_producer.yaml``. Dawn
receives temperature, humidity, barometer, and light values over CAN and
publishes them into ``/dev/uorb/sensor_*`` topics. The ``usensor_reader`` app
from :file:`examples/apps/usensor_reader` is enabled so another NuttX task can
read the produced sensor events while Dawn runs in parallel.

nsh_serial
----------

Serial reference configuration using ``serial_core_demo.yaml``.

.. code:: bash

   socat PTY,link=/tmp/ttySIM0 PTY,link=/tmp/ttyNX0

nsh_shell
---------

Dawn shell configuration using ``shell_core_demo.yaml``.

nsh_tests and tests
-------------------

Unit-test configurations using ``empty_placeholder.yaml``.
