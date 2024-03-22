.. _object:

===========
Dawn Object
===========

``Dawn Object`` - Common representation of Dawn objects.

Objects in Dawn consists of two elements:

- ``SObjectId`` - which is a 32-bit number that uniquely defines an object,
  thanks to which we can refer to a given object.

- ``SObjectCfg`` - which defines the object configuration stored in the form
  of 32-bit values. The configuration is specific to a given object type.
  Configuration is optional, for some objects it can be empty.

Object Lifecycle
================

Object setup is split into separate phases. This is required because
configuration can fail and some one-time initialization depends on bindings
that are created only after configuration.

Lifecycle phases:

- ``configure()`` - parse descriptor configuration, validate values, store IDs
  and options. Do not do one-time allocations that depend on bound objects.
- ``init()`` - one-time initialization after binding. Allocate buffers, create
  register maps, and initialize dependency-backed state.
- ``start()`` / ``doStart()`` - runtime activation. This phase may run many
  times during one object lifetime.
- ``stop()`` / ``doStop()`` - runtime deactivation. This phase may run many
  times during one object lifetime.
- ``deinit()`` - one-time teardown for resources allocated in ``init()``.

Design Rules
============

Restart safety is required: ``start()`` and ``stop()`` may be called multiple
times for the same object instance.

Use these rules when implementing objects:

- Put descriptor parsing and validation in ``configure()``.
- Put one-time allocations in ``init()``.
- Put matching one-time cleanup in ``deinit()``.
- Keep ``doStart()`` / ``doStop()`` free of one-time alloc/free work.
- Treat ``doStart()`` / ``doStop()`` as runtime-only transitions.

Examples
========

Belongs in ``init()``:

- ``ddata_alloc(...)``
- protocol register-map creation (for example ``createRegs()``)
- IO/protocol buffer creation (for example ``createBuffers()``)
- virtIO initialization that depends on bound source IO dimensions

Belongs in ``doStart()`` / ``doStop()``:

- thread start/stop
- enabling/disabling runtime processing
- runtime state reset that must happen on every start

ObjectID and ConfigID Creation Guide
====================================

For tools generating or parsing descriptors, understanding how to construct and
interpret ObjectID and ConfigID values is essential.

**Creating an ObjectID (SObjectId::UObjectId)**

ObjectID binary format (32-bit word)::

    Bits 30-31 (2b): type      - OBJTYPE_IO(1), OBJTYPE_PROTO(2), OBJTYPE_PROG(3), OBJTYPE_ANY(0)
    Bits 21-29 (9b): cls       - Object class (0-511, type-specific)
    Bit 20 (1b):     ext       - Reserved for future 64-bit support
    Bits 16-19 (4b): dtype     - Data type (DTYPE_UINT32, DTYPE_FLOAT, etc.)
    Bits 14-15 (2b): flags     - Type-specific flags
    Bits 0-13 (14b): priv      - Instance ID or private data (0-16383)

**Example: GPIO output (class=5), instance=10, uint32 dtype**::

    type  = OBJTYPE_IO (1)      -> bits 30-31: 0b01
    cls   = 5                   -> bits 21-29: 0b000000101
    ext   = 0                   -> bit 20:     0b0
    dtype = DTYPE_UINT32 (7)    -> bits 16-19: 0b0111
    flags = 0                   -> bits 14-15: 0b00
    priv  = 10                  -> bits 0-13:  0b00000000001010
    Result: 0x4500000A (binary: 0100 0101 0000 0000 0000 0000 0000 1010)

**Creating a ConfigID (SObjectCfg::UObjectCfgId)**

ConfigID binary format (32-bit word)::

    Bits 30-31 (2b):  type      - Same as ObjectID (OBJTYPE_IO, OBJTYPE_PROTO, etc.)
    Bits 21-29 (9b):  cls       - Same as ObjectID
    Bit 20     (1b):  ext       - Reserved for future use
    Bits 16-19 (4b):  dtype     - Configuration data type (DTYPE_UINT32, DTYPE_CHAR, etc.)
    Bit 15     (1b):  rw        - Read-write flag (0=read-only, 1=read-write)
    Bits 5-14  (10b): size      - Configuration data size in 32-bit words (0-1023)
    Bits 0-4   (5b):  id        - Configuration identifier (0-31)

**Example: Read-write uint32 config, id=1, size=1**::

    type  = OBJTYPE_IO (1)      -> bits 30-31: 0b01
    cls   = 5                   -> bits 21-29: 0b000000101
    ext   = 0                   -> bit 20:     0b0
    dtype = DTYPE_UINT32 (7)    -> bits 16-19: 0b0111
    rw    = 1                   -> bit 15:     0b1
    size  = 1                   -> bits 5-14:  0b0000000001
    id    = 1                   -> bits 0-4:   0b00001

    Result: 0x45008001 (binary: 0100 0101 0000 0000 1000 0000 0000 0001)

Doxygen
=======

- `dawn::CObject <../../doxygen/classdawn_1_1CObject.html>`_

- `dawn::SObjectId <../../doxygen/structdawn_1_1SObjectId.html>`_

- `dawn::SObjectCfg <../../doxygen/classdawn_1_1SObjectCfg.html>`_
