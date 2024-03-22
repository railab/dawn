=====
Uname
=====

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOUname`` exposes selected system identity strings as an IO object.

The documented variant is hostname.

Implementation
==============

``CIOUname`` calls ``uname(2)`` and copies the ``nodename`` field into a
fixed ``HOST_NAME_MAX``-byte buffer (``DTYPE_CHAR``). The IO is read-only,
does not support batch operations, and currently exposes only the
``IO_CLASS_SYSTEM_HOSTNAME`` sub-class.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_UNAME``: enables ``uname`` IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: hostname1
       type: uname
       dtype: string
       rw: false
       variant: hostname

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOUname <../../doxygen/classdawn_1_1CIOUname.html>`_
