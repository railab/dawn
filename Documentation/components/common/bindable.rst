.. _bindable:

=============
Dawn Bindable
=============

``CBindableObject`` is a base class that allows Dawn objects to find and use
other components in the system.

Overview
========

This class is used by Programs and Protocols that need to interact with IO
objects. Since the exact address of these objects isn't known until the
system is built from a descriptor, ``CBindableObject`` maintains an internal
list of the components it needs.

During the setup phase, the system "binds" these objects by looking up the
IDs specified in the configuration and providing the actual memory addresses.
Once bound, the component can safely use its dependencies to perform tasks
like data processing or communication.

Doxygen
=======

- `dawn::CBindableObject <../../doxygen/classdawn_1_1CBindableObject.html>`_
