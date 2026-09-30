===================
Nordic nRF54LM20-DK
===================

Single-core nRF54LM20 with the SoftDevice Controller and NimBLE on the same
core. Console on UARTE20, J-Link VCOM1 (``/dev/ttyACM1``). Used as an NTFC
hardware target, see :ref:`ntfc`.

Configs
=======

nimble_ntfc
-----------

NTFC all-services NimBLE profile using ``ntfc_nimble_all.yaml``.

nimble_ntfc_buffer
------------------

NTFC NimBLE buffer-transfer profile using ``ntfc_nimble_buffer.yaml``.

nimble_ntfc_custom
------------------

NTFC custom-service NimBLE profile using ``ntfc_nimble_custom.yaml``.

nimble_sensor_producer
----------------------

NimBLE-to-user-sensor reference profile using
``nimble_sensor_producer.yaml``.
