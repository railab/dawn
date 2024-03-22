====
UUID
====

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOUuid`` exposes the device UUID through an IO object.

Implementation
==============

``CIOUuid`` exposes the board-unique identifier returned by NuttX's
``board_uniqueid()`` API. The output buffer is a fixed
``CONFIG_BOARDCTL_UNIQUEID_SIZE`` byte array of ``DTYPE_UINT8``. The IO is
read-only, does not support batch (multi-sample requests return
``-ENOTSUP``), and does not produce notifications.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_UUID``: enables the UUID IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: uuid1
       type: uuid
       dtype: uint8
       rw: false

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOUuid <../../doxygen/classdawn_1_1CIOUuid.html>`_
