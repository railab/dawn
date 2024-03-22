.. _generic_handler:

===============
Generic Handler
===============

``CGenericHandler`` is a template class that manages the lifecycle of a
collection of Dawn objects.

Overview
========

Handlers are responsible for the entire lifecycle of objects of a specific
type (like all IOs or all Programs). This template provides a standard way
to store objects, configure them from the descriptor, and manage their
transition through the init, start, and stop phases.

By using this common base, Dawn ensures that all component types follow the
same structural rules for setup and teardown, making the system predictable
and easier to extend.

Doxygen
=======

- `dawn::CGenericHandler <../../doxygen/classdawn_1_1CGenericHandler.html>`_
