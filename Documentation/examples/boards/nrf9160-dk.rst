=================
Nordic nRF9160-DK
=================

``/boards/arm/nrf91/nrf9160-dk``

Configs
=======

wakaama
-------

Wakaama LwM2M demo using
``descriptors/examples/nrf9160dk_wakaama_lte.yaml``.

Notes:

* Use the nRF9160 ``miniboot_s`` secure bootloader for now, then flash this
  ``wakaama`` build as the non-secure modem application image.
* This app still uses ``CONFIG_NRF91_APP_FORMAT_MCUBOOT`` because the nRF91
  Miniboot flow reuses the MCUboot-compatible slot image format for the
  non-secure application.
* The public Leshan IP in the config is ``23.97.187.154``. Refresh the
  ``leshan.eclipseprojects.io`` A record if that sandbox address changes.

blinky_wakaama
--------------

Sequencer-driven LED blinky controlled over LwM2M (Wakaama). Uses
``blinky_wakaama_demo.yaml`` and the shared ``blinky_common`` core. A custom
LwM2M object (id ``33000``) exposes the LED state plus start/stop, blink-phase
reset, and dwell times; an LTE system object brings up connectivity first, then
the endpoint ``dawn-nrf9160-blinky`` registers with the Leshan sandbox.

Flashing
========

This non-secure application runs behind the ``miniboot_s`` secure bootloader.
Build the bootloader and flash both images:

.. code:: bash

   source .venv/bin/activate

   python -m dawnpy build build_nrf9160_wakaama \
     boards/arm/nrf91/nrf9160-dk/configs/wakaama \
     -e CXX=g++-14 \
     -e CC=gcc-14

   python -m dawnpy build build_miniboot_s \
     external/nuttx/boards/arm/nrf91/nrf9160-dk/configs/miniboot_s \
     -e CXX=g++-14 -e CC=gcc-14

   nrfutil device recover
   nrfutil device program --firmware build_miniboot_s/nuttx.hex \
       --options chip_erase_mode=ERASE_ALL
   nrfutil device program --firmware build_nrf9160_wakaama/nuttx.hex \
       --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE
   nrfutil device reset

* ``nrfutil device recover`` clears access-port protection; without it
  programming has no effect on a locked device.
* The application is flashed with ``ERASE_RANGES_TOUCHED_BY_FIRMWARE`` to keep
  the bootloader intact (the ``nrfutil`` default is ``ERASE_ALL``).
