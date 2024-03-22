=================
Device Components
=================

CDevInspector
=============

**Singleton class** providing read-only access to Dawn handlers for
debugging and introspection.

Available when ``CONFIG_DAWN_INSPECT`` is enabled.

Usage
-----

Handlers (IO, PROG, PROTO) automatically register with CDevInspector
during initialization. Inspector provides const access to:

* ``getIOHandler()`` - access to all I/O objects
* ``getProgHandler()`` - access to all program objects
* ``getProtoHandler()`` - access to all protocol objects

Primary use: Dawn Shell inspector commands (list, inspect, tree, stats).


CDevDescriptor
==============

Device descriptor loader and manager.

Doxygen
=======

- `dawn::CDevInspector <../../doxygen/classdawn_1_1CDevInspector.html>`_

- `dawn::CDevDescriptor <../../doxygen/classdawn_1_1CDevDescriptor.html>`_
