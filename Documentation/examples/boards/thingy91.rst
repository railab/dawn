================
Nordic Thingy:91
================

Configs
=======

wakaama
-------

Wakaama LwM2M demo using ``descriptors/examples/thingy91_wakaama_lte.yaml``.
Brings up LTE, registers with the public Eclipse Leshan sandbox, and exposes the
on-board sensors as LwM2M objects:

=========== =========== ============= =============================
Sensor      Reading     LwM2M         uorb topic
=========== =========== ============= =============================
BME680      Temperature 3303/0/5700   ``sensor_ambient_temp0``
BME680      Humidity    3304/0/5700   ``sensor_humi0``
BME680      Pressure    3315/0/5700   ``sensor_baro0``
BME680      Gas         3300/0/5700   ``sensor_gas0``
BH1749      Colour R    33001/0/0     ``sensor_rgb0``
BH1749      Colour G    33001/0/1     ``sensor_rgb0``
BH1749      Colour B    33001/0/2     ``sensor_rgb0``
=========== =========== ============= =============================

Notes:

* Runs non-secure behind the dedicated Thingy:91 ``miniboot_s`` bootloader, which
  grants I2C2 to the non-secure app (``CONFIG_NRF91_SERIAL2_NS``).
* The BH1749 and RGB LEDs are on the 3.3 V rail; ``nrf91_pmic.c`` enables the
  ADP5360 buck-boost at bring-up (off at power-on reset). BME680 is on 1.8 V.
* Each sensor is cached by a ``latest`` program into a ``virt`` IO so on-demand
  reads always return the last sample (uorb reads return ``-EAGAIN`` between
  samples). The BH1749 RGB vector is split by a ``vecsplit`` program.
* ``nrf91_modem.c`` disables PSM/eDRX so the device stays reachable for reads.

Flashing
========

This non-secure application runs behind the ``miniboot_s`` secure bootloader.
Build both images and flash over the external J-Link:

.. code:: bash

   source .venv/bin/activate

   python -m dawnpy build build_thingy91_wakaama \
     boards/arm/nrf91/thingy91/configs/wakaama \
     -e CXX=g++-14 -e CC=gcc-14

   python -m dawnpy build build_thingy91_miniboot_s \
     boards/arm/nrf91/thingy91/configs/miniboot_s \
     -e CXX=g++-14 -e CC=gcc-14

   nrfutil device recover
   nrfutil device program --firmware build_thingy91_miniboot_s/nuttx.hex \
       --options chip_erase_mode=ERASE_ALL
   nrfutil device program --firmware mfw_nrf9160_<ver>.zip
   nrfutil device program --firmware build_thingy91_wakaama/nuttx.hex \
       --options chip_erase_mode=ERASE_RANGES_TOUCHED_BY_FIRMWARE
   nrfutil device reset

* ``nrfutil device recover`` clears access-port protection and also erases the
  modem firmware -- re-flash ``mfw_nrf9160_<ver>.zip`` after a recover.
* The application is flashed with ``ERASE_RANGES_TOUCHED_BY_FIRMWARE`` to keep
  the bootloader intact.

RTT log
=======

Console and syslog are on RTT channel 0 (there is no UART). View it with
``JLinkRTTViewer`` (device ``nRF9160_xxAA``, SWD, channel 0), or:

.. code:: bash

   JLinkExe -device nRF9160_xxAA -if SWD -speed 4000 -autoconnect 1
   # in another terminal:
   JLinkRTTClient
