.. _handler:

============
Dawn Handler
============

``Dawn Handler`` - Common representation for Dawn handlers.
The role of Dawn handlers is to manage the created objects of a given type.

Lifecycle Sequencing
====================

Handlers execute object lifecycle phases in order. For ``PROG`` and ``PROTO``
objects, binding happens between ``configure()`` and ``init()`` so that
one-time initialization can use resolved dependencies.

Typical sequence:

1. Call object ``configure()`` for all objects.
2. Resolve and bind object dependencies (handler-managed).
3. Call object ``init()`` for all objects.
4. Later, call ``start()`` / ``stop()`` as runtime transitions.
5. Call ``deinit()`` once during final teardown.

Within one handler, objects are processed in descriptor order for allocation,
``configure()``, ``init()``, and ``start()``. Teardown runs in reverse order.
This means descriptor order is part of the lifecycle contract when one object
depends on another object's initialization side effects.

Doxygen
=======

- `dawn::IHandler <../../doxygen/classdawn_1_1IHandler.html>`_

- `dawn::CHandler <../../doxygen/classdawn_1_1CHandler.html>`_
