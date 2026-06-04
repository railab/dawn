.. _systemindex:

===========
Dawn System
===========

``Dawn System`` - an object family for configurable system facilities (LTE,
network interfaces, power modes, ...) that do not fit the IO/PROG/PROTO model.

Difference from IO
==================

A System object has **no data path**: it does not implement
``getData()``/``setData()``. It exposes only:

- the **config API** (``getObjConfig()``/``setObjConfig()``) — its settings,
- the **lifecycle API** (``start()``/``stop()``) — activate/deactivate the
  facility.

An IO is a value you read or write. A System object is a facility you
configure and switch on or off.

Encoding
========

System objects reuse ``OBJTYPE_ANY``. The ``cls`` field selects the facility
type; the ``priv`` field selects the instance:

- ``cls = 0``: reserved for the descriptor metadata object (parsed by
  :cpp:class:`dawn::CDescriptor`, not a System object).
- ``cls = SYS_CLASS_LTE`` (1): LTE connectivity.
- ``priv``: instance number, so several facilities of one type
  (e.g. multiple LTE interfaces) differ only by ``priv``.

:cpp:func:`dawn::SObjectId::objectIsSys` is true for ``OBJTYPE_ANY`` with
``cls != 0``.

Access Model
============

System objects are reached over protocols through special IOs bound to them:

- :doc:`../io/config`: get/set selected config items (the facility's
  parameters).
- :doc:`../io/control`: drive ``start()``/``stop()`` (for LTE: connect /
  disconnect).

``CIOHandler::bindObjects()`` resolves ``OBJTYPE_ANY`` targets against
:cpp:class:`dawn::CSysHandler`, so a Config/Control IO can target a System
object exactly as it targets an IO/PROG/PROTO.

Lifecycle
=========

System objects are owned by :cpp:class:`dawn::CSysHandler` and follow the
common object lifecycle (see :doc:`../common/object`). ``CDawn`` starts them
**after IO and before programs/protocols**, so a facility such as LTE is up
before a protocol needs the network, and stops them in reverse order.

Adding a System Type
====================

1. Add a ``SYS_CLASS_*`` value in :cpp:class:`dawn::CSysCommon`.
2. Implement ``CSys<Name> : CSysCommon`` — config items + ``doStart()`` /
   ``doStop()`` (no ``getData``/``setData``).
3. Register it in :cpp:class:`dawn::CSysFactory`, gated by its Kconfig option.
4. Emit it from the descriptor tooling (``tools/dawnpy``).

Doxygen
=======

- `dawn::CSysCommon <../../doxygen/classdawn_1_1CSysCommon.html>`_
- `dawn::CSysHandler <../../doxygen/classdawn_1_1CSysHandler.html>`_

Supported System Objects
========================

.. toctree::
   :maxdepth: 1

   lte.rst
