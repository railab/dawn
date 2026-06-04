===========
LTE Manager
===========

**Component Type:** System

**Status:** Implemented

Overview
========

``CSysLte`` brings up an LTE data connection. It is a System object: it holds
the connection parameters as config items and connects on ``start()`` /
disconnects on ``stop()`` through the common NuttX LTE API (``lapi``). It has
no data path.

Implementation
==============

- Connects in ``doStart()`` via ``lte_port_connect()`` (porting layer over
  ``lapi``); disconnects in ``doStop()`` via ``lte_port_disconnect()``.
- Parameters are read from descriptor config items, falling back to the
  ``CONFIG_DAWN_SYS_LTE_*`` Kconfig defaults when an item is absent.
- An empty APN means "use the network/SIM default".
- No modem chip is referenced: the porting layer talks only to ``lapi``, so
  the object works on any target with an LTE API backend.

Config items (``cls = SYS_CLASS_LTE``):

=========================  ==========  =========================================
Id                         DTYPE       Meaning
=========================  ==========  =========================================
``LTE_CFG_APN``            CHAR        APN name
``LTE_CFG_USERNAME``       CHAR        APN username
``LTE_CFG_PASSWORD``       CHAR        APN password
``LTE_CFG_AUTHTYPE``       UINT8       0=none, 1=PAP, 2=CHAP
``LTE_CFG_IPTYPE``         UINT8       0=IPv4, 1=IPv6, 2=IPv4/IPv6
``LTE_CFG_REG_TIMEOUT``    UINT32      registration timeout (seconds)
=========================  ==========  =========================================

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_SYS_LTE``: enables the LTE System object (depends on
  ``LTE_LAPI``).
- ``CONFIG_DAWN_SYS_LTE_APN`` / ``_USERNAME`` / ``_PASSWORD`` /
  ``_AUTHTYPE`` / ``_IPTYPE`` / ``_REG_TIMEOUT``: default parameter values.

YAML
----

.. code-block:: yaml

   system:
   - id: lte_main
     type: lte
     config:
       apn: ''        # empty: network/SIM default
       auth_type: 0
       ip_type: 0

External Control
================

- ``ConfigIO``: supported (read/write the parameters above).
- ``ControlIO``: supported (``start`` = connect, ``stop`` = disconnect).
- ``TriggerIO``: not supported.

Doxygen
=======

- `dawn::CSysLte <../../doxygen/classdawn_1_1CSysLte.html>`_
