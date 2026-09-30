===================
Nordic nRF54L15 TAG
===================

Uses the upstream NuttX ``nrf54l15-tag`` board. Console and syslog on RTT;
SoftDevice Controller and NimBLE on the same core.

Configs
=======

nimble
------

NimBLE example with the BME688 environmental sensors over ESS and IMDS, and
the supply voltage (SAADC VDD input) as battery level over BAS. Uses
``nimble_nrf54l15_tag_demo.yaml`` and advertises as ``dawn-tag``.
