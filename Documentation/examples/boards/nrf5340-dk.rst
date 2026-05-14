=================
Nordic nRF5340-DK
=================

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

rpmsghci_sdc_cpunet
-------------------

Network-core support configuration for SoftDevice Controller HCI over RPMsg.

All nRF5340-DK NimBLE NTFC app-core configs pair with this network-core
configuration in the NTFC YAML so the board is flashed as a dual-core target
while host-side tests talk only to the app-core shell.
