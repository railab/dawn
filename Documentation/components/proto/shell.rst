================
Dawn Shell (dsh)
================

**Component Type:** Protocol

**Status:** Implemented

Overview
========

``CProtoShellPretty`` - interactive shell protocol used to inspect and
control Dawn objects for development and debugging.

Implementation
==============

Shell Commands
--------------

Basic Commands
~~~~~~~~~~~~~~

* ``help`` print help message.

* ``exit`` exit from shell.

* ``info`` show the compact inventory of IO objects bound to this shell
  protocol instance. This is the shortest way to find IDs for objects that
  can be used with ``getio`` and ``setio`` from the current shell.

* ``getio <IO objectID>`` get value for a given objectID.

* ``setio <IO objectID> <v1..vN>`` set IO value words.

* ``getioloop`` get all object values in loop.

* ``setcfg <objectID> <cfgId> <v1..vN>`` set configuration object words.
  ``N`` is taken from ``cfgId`` size.

* ``getcfg <objectID> [cfgId]`` get configuration objects. Without
  ``cfgId``, raw descriptor-backed config words are printed.

Inspector Commands
~~~~~~~~~~~~~~~~~~

Available when ``CONFIG_DAWN_PROTO_SHELL_INSPECT`` is enabled.

* ``list`` show a compact inventory with object names, IDs, data types,
  flags, and I/O statistics.

* ``list io``, ``list prog``, and ``list proto`` filter the inventory to a
  single object family.

* ``list verbose`` show the same inventory with the older detailed columns
  including object type, class, data dimension, data size, and padded
  counters.

* ``inspect <objectID>`` detailed view of a specific object (type, class,
  data type, flags, instance). Shows I/O statistics for I/O objects.

* ``tree`` display hierarchical data flow - shows PROTO objects at top
  level with their bound PROG/IO objects in a tree structure. PROGs show
  their bound I/O objects as sub-branches. Unbound objects listed
  separately.

* ``stats`` aggregate I/O statistics across all objects - total
  reads/writes/errors and most active objects.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_SHELL``: enables the shell protocol.
- ``CONFIG_DAWN_PROTO_SHELL_PRETTY``: enables the pretty shell frontend.
- ``CONFIG_DAWN_PROTO_SHELL_INSPECT``: enables inspector commands such as
  ``list``, ``inspect``, ``tree``, and ``stats``.

YAML
----

.. code-block:: yaml

   protocols:
     - id: shell1
       type: shell
       config:
         bindings:
           - io1
           - io2
         prompt: "dawn> "

Supported fields:

- ``config.bindings``: standard IO binding list.
- ``config.prompt``: interactive shell prompt string.

External Control
================

ControlIO: supported.

``CProtoShellPretty`` supports runtime start/stop control through
``CIOControl``. When stopped, interactive shell handling is inactive.
When started again, shell processing resumes.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProtoShellPretty <../../doxygen/classdawn_1_1CProtoShellPretty.html>`_
