.. _factories:

=========
Factories
=========

Overview
========

Dawn uses a factory pattern to instantiate IO, Program, and Protocol objects
at runtime based on the contents of a descriptor. This architecture enables:

- **Decoupled instantiation**: Handlers don't need to know the specific 
  subclasses they manage.
- **Extensibility**: Out-of-tree projects can provide custom factories to 
  add new object types without modifying the core framework.
- **Conditional compilation**: Only the code for enabled object types is 
  included in the final firmware.

Architecture
============

There are three main factory interfaces, one for each object family:

- ``IIOFactory``: Creates objects derived from ``CIOCommon``.
- ``IProgFactory``: Creates objects derived from ``CProgCommon``.
- ``IProtoFactory``: Creates objects derived from ``CProtoCommon``.

Built-in Factories
------------------

Dawn provides standard implementations of these interfaces:
``CIOFactory``, ``CProgFactory``, and ``CProtoFactory``. These factories 
contain a large ``switch`` statement (guarded by Kconfig options) that maps 
class IDs in the descriptor to concrete C++ classes.

Extending with User Factories
-----------------------------

The ``CDawn`` class and the object handlers support registering a "user 
factory" alongside the built-in one. When creating an object, the handler 
first consults the user factory. If the user factory returns ``nullptr`` 
(indicating it doesn't support that class ID), the handler then falls 
back to the built-in factory.

This mechanism is the foundation for Dawn's :doc:`/customization` and 
out-of-tree support.

Implementation
==============

Each factory implements a ``create()`` method:

.. code-block:: cpp

   virtual CIOCommon* create(CDescObject &desc) = 0;

The ``CDescObject`` passed to the factory contains the class ID and the 
entire configuration payload for that object instance.

Doxygen
=======

- `dawn::IIOFactory <../../doxygen/classdawn_1_1IIOFactory.html>`_
- `dawn::IProgFactory <../../doxygen/classdawn_1_1IProgFactory.html>`_
- `dawn::IProtoFactory <../../doxygen/classdawn_1_1IProtoFactory.html>`_
