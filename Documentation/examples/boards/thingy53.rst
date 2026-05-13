================
Nordic Thingy:53
================

Configs
=======

nimble
------

NimBLE example focused on environmental sensor data.
Uses ``nimble_env_sensor_demo.yaml``.

rpmsghci_sdc_cpunet
-------------------

Network-core support configuration for SoftDevice Controller HCI over RPMsg.

shell
-----

Shell and NimBLE environmental example using ``shell_nimble_env_demo.yaml``.

nimble_thingy53
---------------

Placeholder for full Thingy:53 demo for Dawn.

For now only RGB LED over BLE is enabled.

Uses ``nimble_thingy53_demo.yaml`` and advertises as ``dawn-thingy53``.

Host control helper:

.. code-block:: console

   # List nearby BLE devices.
   python3 tools/examples/thingy53_demo_cli.py scan

   # Read or set the RGB LED through the NimBLE custom service. If --device is
   # omitted, the helper auto-detects the ``dawn-thingy53`` advertisement.
   python3 tools/examples/thingy53_demo_cli.py rgb get
   python3 tools/examples/thingy53_demo_cli.py rgb set '#ff4000'

   # Gradually sweep around the RGB palette, then turn the LED off. Use
   # --cycles 0 to run until interrupted.
   python3 tools/examples/thingy53_demo_cli.py rgb fade --steps 96 --period 0.03

The helper currently exposes the RGB feature. Additional Thingy:53 demo
features should be added as new subcommands under the same script.
