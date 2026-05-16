.. _protoindex:

==========
Dawn Proto
==========

``Dawn Proto`` - communication layer between external clients and Dawn objects.

Protocol objects expose external endpoints (serial, UDP, shell, CAN, BLE,
etc.) and translate endpoint requests into operations on Dawn objects.

Overview
========

In Dawn architecture, ``PROTO`` is the gateway between remote/local clients
and internal object handlers.

- Clients interact only with protocol endpoints.
- Protocol implementations access objects through
  :cpp:class:`dawn::CIOHandler` and object IDs.
- Access is limited to objects explicitly bound in descriptor configuration.

Common operations provided by protocols:

1. Read IO values (``get`` path)
2. Write IO values (``set`` path)
3. Subscribe/notify for event-driven updates (protocol dependent)
4. Read/write protocol-specific configuration items
5. Enumerate or query object information (protocol dependent)

Binding Model
=============

Protocol to object association is descriptor-driven and happens during
initialization:

1. Protocol objects are created from descriptor entries.
2. Protocol configuration is parsed in ``configure()``.
3. Protocol-specific bind configuration (``...IOBIND...`` items) is resolved
   to concrete IO object IDs.
4. ``init()`` allocates runtime resources (buffers, endpoint state, optional
   notifier bindings).

Each protocol sees only objects listed in its own bind configuration items.
Simple protocols declare this list as ``config.bindings``. Mapped protocols
derive the visible IO set from their mapping entries, such as CAN objects,
Modbus register groups, or BLE service characteristics. Objects not bound to
that protocol are not visible to it.

The exact bind item layout is protocol-specific (for example, one protocol may
use multiple bind item types for different endpoint groups/services).

Lifecycle
=========

Protocol lifecycle is managed by :cpp:class:`dawn::CProtoHandler`:

1. ``init()``: create protocol objects from descriptor and attach IO handler.
2. ``initAll()``: configure, resolve binds, and initialize each protocol.
3. ``startAll()`` / ``stopAll()``: activate or stop protocol endpoints.
4. ``deinitAll()``: release protocol resources.

Endpoint Model
==============

Protocol classes define transport and wire format details, while sharing the
same object access model:

- Request/response operations map to IO ``get``/``set`` and configuration
  reads/writes.
- Notify-capable protocols push updates to subscribed clients.
- Endpoint resource ownership (thread/fd/socket/device state) is
  protocol-specific but follows the common lifecycle above.

Doxygen
=======

- `dawn::CProtoCommon <../../doxygen/classdawn_1_1CProtoCommon.html>`_

- `dawn::CProtoHandler <../../doxygen/classdawn_1_1CProtoHandler.html>`_

Protocol Helpers
================

.. toctree::
   :maxdepth: 1

   simplebase.rst

Supported Protocols
===================

.. toctree::
   :maxdepth: 1

   dummy.rst
   shell.rst
   serial.rst
   udp.rst
   ipc.rst
   nimble/index.rst
   wakaama.rst
   nxscope.rst
   can.rst
   modbus_rtu.rst
   modbus_tcp.rst
