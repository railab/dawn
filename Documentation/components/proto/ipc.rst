============
IPC Protocol
============

**Component Type:** Protocol

**Status:** Implemented

Overview
========

The ``IPC Protocol`` exposes the Dawn simple binary protocol over two
named pipes. It is intended for peer tasks running on the same MCU or
simulator instance.

``CProtoIpc`` uses one FIFO for inbound commands and one FIFO for outbound
responses and notifications. The frame format and command set match the
serial and UDP simple transports.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_IPC``: enables the IPC protocol.
- ``CONFIG_DAWN_PROTO_IPC_RX_PATH``: default command FIFO path.
- ``CONFIG_DAWN_PROTO_IPC_TX_PATH``: default response FIFO path.
- ``CONFIG_DAWN_PROTO_IPC_UNLINK_FIFO``: remove FIFOs during deinit.

YAML
----

.. code-block:: yaml

   protocols:
     - id: ipc0
       type: ipc
       config:
         bindings:
           - io1
           - io2
         rx_path: "/var/pipe/dawn_rx"
         tx_path: "/var/pipe/dawn_tx"

Supported fields:

- ``config.bindings``: standard IO binding list.
- ``config.rx_path``: FIFO path used by Dawn to receive commands.
- ``config.tx_path``: FIFO path used by Dawn to send responses.

External Control
================

ControlIO: supported.

``CProtoIpc`` supports runtime start/stop control through ``CIOControl``.
When stopped, the worker thread no longer polls the receive FIFO.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoIpc <../../doxygen/classdawn_1_1CProtoIpc.html>`_
