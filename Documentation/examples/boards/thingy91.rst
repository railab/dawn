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
Lightwell   Colour (rw) 33002/0/0     ``rgbled0``
Sense LED   Colour (rw) 33003/0/0     ``rgbled1``
GNSS        Latitude    6/0/0         ``sensor_gnss0``
GNSS        Longitude   6/0/1         ``sensor_gnss0``
GNSS        Altitude    6/0/2         ``sensor_gnss0``
GNSS        Radius/eph  6/0/3         ``sensor_gnss0``
GNSS        Time (UTC)  6/0/5         ``sensor_gnss0``
GNSS        Speed       6/0/6         ``sensor_gnss0``
GNSS        Satellites  33005/0/0     ``sensor_gnss0``
GNSS        Course      33005/0/1     ``sensor_gnss0``
GNSS        Accuracy V  33005/0/2     ``sensor_gnss0``
GNSS        HDOP        33005/0/3     ``sensor_gnss0``
GNSS        PDOP        33005/0/4     ``sensor_gnss0``
GNSS        VDOP        33005/0/5     ``sensor_gnss0``
=========== =========== ============= =============================

Notes:

* Runs non-secure behind the dedicated Thingy:91 ``miniboot_s`` bootloader, which
  grants I2C2 to the non-secure app (``CONFIG_NRF91_SERIAL2_NS``) and the RGB-LED
  PWMs (``CONFIG_NRF91_PWM0_NS``, ``CONFIG_NRF91_PWM1_NS``).
* The BH1749 and both RGB LEDs are on the 3.3 V rail; ``nrf91_pmic.c`` enables the
  ADP5360 buck-boost at bring-up (off at power-on reset). BME680 is on 1.8 V.
* The board has two RGB LEDs, driven by ``nrf91_rgbled.c``: the **lightwell** LED
  (the large LED in the enclosure light pipe) on PWM0 (P0.29/30/31) as
  ``/dev/rgbled0``, and the **sense** LED (next to the sensors) on PWM1
  (P0.0/1/2) as ``/dev/rgbled1``. Objects 33002/0/0 (lightwell) and 33003/0/0
  (sense) are writable packed ``0xRRGGBB`` colours: write e.g. ``0xFF0000``
  (16711680) from Leshan to set a LED red.
* Each sensor is cached by a ``latest`` program into a ``virt`` IO so on-demand
  reads always return the last sample (uorb reads return ``-EAGAIN`` between
  samples). The BH1749 RGB vector is split by a ``vecsplit`` program.
* GNSS shares the modem with LTE-M (``AT%XSYSTEMMODE=1,0,1,0``). The nRF91 GNSS
  driver defers starting the engine until the LTE stack has powered the modem
  (CFUN), so opening ``sensor_gnss0`` early does not fail.
* GNSS uses four dedicated sub-IOs (:cpp:class:`dawn::CIOSensorGnss`), all
  reading ``sensor_gnss0``: ``gnss`` (position+velocity), ``gnss_time`` (UTC,
  ``uint64`` seconds), ``gnss_info`` (accuracy + DOP) and ``gnss_sats``
  (``uint32`` satellite count). The ``gnss`` and ``gnss_info`` vectors are
  fanned out by ``vecsplit`` into the LwM2M Location object (6) and the custom
  object 33005; ``gnss_time`` / ``gnss_sats`` are cached by a ``latest``
  program so reads return the last value (0 until the first fix) rather than
  ``-ENODATA``. A position fix requires sky view, so all GNSS resources read 0
  until the modem locks.
* ``CONFIG_NRF91_MODEM_GNSS_BOOST_PRIO`` is **enabled**, but the boost is
  *bounded* and *satellite-gated*: GNSS only preempts LTE idle once it actually
  sees satellites (``CONFIG_NRF91_MODEM_GNSS_BOOST_MINSV``, default 4), and the
  priority is released after a timeout if no fix arrives, with a cool-down
  before retrying (``..._BOOST_TIMEOUT`` / ``..._BOOST_COOLDOWN``). Indoors GNSS
  sees nothing, so LTE is never preempted and the device stays reachable;
  outdoors GNSS grabs window time, gets the fix, then yields back to LTE.
* LTE power-save is configurable via ``CONFIG_DAWN_SYSTEM_LTE_PSAVE``
  (0 = none, 1 = PSM, 2 = eDRX), applied through the LTE API. The default
  (none) disables PSM/eDRX so the device stays reachable for server-initiated
  reads.
* The on-board 24CW160 (16-Kbit / 2 KB) I2C EEPROM (I2C2, 0x50) is registered
  as ``/dev/eeprom0`` via ``nrf91_eeprom.c`` (generic ``24xx16`` geometry). It
  is not yet consumed by Dawn -- exposed for later use.

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
