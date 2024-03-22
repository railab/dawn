.. _descriptor:

===============
Dawn Descriptor
===============

Overview
========

Descriptor is the single runtime definition for Dawn object graph creation.
It decides which IO/PROG/PROTO objects exist, with what configuration, and
how they are exposed for binding.

If descriptor content changes, runtime behavior changes. No other artifact has
the same authority at object-graph level.

For the supported descriptor source forms, including YAML-generated and
hand-written C++ descriptors, see :ref:`descriptor-format`.

For ObjectID/ConfigID bit layouts and per-field encoding details, see
:ref:`object`.

Boot-Time Role
==============

During ``CDawn::load_descriptor()`` the descriptor drives the whole setup flow:

1. ``CDescriptor::loadBin()`` attaches and validates descriptor binary.
2. IO handler allocates/configures/initializes IO objects from descriptor.
3. PROG handler allocates/configures/initializes PROG objects.
4. PROTO handler allocates/configures/initializes PROTO objects.
5. special IO bindings (Config/Control/Trigger targets) are resolved.

In other words, descriptor is consumed before runtime starts and defines the
object topology used by subsequent ``startAll()`` phases.

Object Order
============

Object entries keep the order from the descriptor source. Each handler walks
the descriptor in that order, accepts entries for its object type, and stores
them in the same relative order. Lifecycle phases then run over that stored
order:

- ``configure()`` and ``init()`` run from first matching descriptor entry to
  last matching descriptor entry,
- ``start()`` also runs from first to last,
- ``stop()``, ``deinit()``, and object deletion run in reverse order.

This ordering is observable for descriptors that compose objects through
runtime-created resources. For example, many programs initialize output
``virt`` IOs during their own ``init()``. A later program that consumes that
``virt`` as a notify-capable source must appear after the producer program in
the ``programs:`` list; otherwise the consumer may see an uninitialized
``virt`` with no notification support.

As a rule, list producers before consumers when the consumer depends on
producer-owned initialization side effects such as ``virt`` shape,
notification support, runtime buffers, or writable config/control targets.
Pure object references are resolved by ObjectID, but initialization side
effects still follow descriptor order within each handler.

Current Support and Target Features
===================================

Currently supported:

- descriptor binary loaded via ``CDescriptor::loadBin()``,
- descriptor-driven object allocation for IO/PROG/PROTO handlers,
- multi-slot descriptor storage via ``CDevDescriptor`` slots,
- FLASH-baked multi-descriptor slots: multiple ``uint32_t`` arrays compiled
  into the firmware and registered at boot via ``dawn_register_flash_slots()``,
  defined with ``descriptor0``/``descriptor1``/... blocks in a single YAML file,
- runtime descriptor upload into writable slots via ``CIODescriptor`` seeked
  writes,
- runtime slot switching via ``CIODescSelector`` + ``CDescSwitch``,
- rollback by selecting slot ``0`` (boot descriptor),
- runtime writable *volatile* updates for RW-enabled config items
  (via object config APIs; reset on reboot),
- descriptor checksum field in format.

Target / desired features (not supported yet):

- source-agnostic descriptor loading (file/other memory regions/transport),
- persistent runtime configuration storage and restore,
- full descriptor image replacement with policy-managed rollback/fallback.

Binary Container Contract
=========================

Descriptor is a stream of 32-bit words with this outer shape:

1. header magic: ``DAWN_DESCRIPTOR_HDR``
2. object count field (stored in descriptor header)
3. object-entry stream
4. footer magic: ``DAWN_DESCRIPTOR_FOOT``
5. checksum word

Object-entry format:

- ``objid`` (1 word)
- config-item count (1 word)
- repeated config items:
  
  - ``cfgid`` header (1 word)
  - ``cfgid.size`` data words

All lengths are 32-bit-word based.

Allocation Semantics
====================

Handlers do not parse arbitrary descriptor bytes on their own.
They rely on ``CDescriptor::alloc_objects()`` iteration and object validity
checks to receive relevant entries.

Allocation behavior:

- each handler accepts only matching object types/classes,
- matching entries are wrapped as :cpp:class:`dawn::CDescObject`,
- factories create concrete runtime instances from wrapped entries,
- duplicate ObjectID detection is logged during allocation.

Metadata Semantics
==================

Entries with ``objid.type == OBJTYPE_ANY`` are descriptor-level metadata/global
options. They are not instantiated as normal IO/PROG/PROTO runtime objects.

Common descriptor metadata IDs:

- ``DESC_CFG_VERSION``
- ``DESC_CFG_STRING``

Validation Model
================

``CDescriptor::binValid()`` is the primary gate before allocation.
Validation is layered:

1. container markers and top-level sanity checks,
2. dtype support checks for object IDs and config IDs,
3. structural walk consistency across all object/config entries,
4. object-level descriptor validation.

CRC check behavior:

- required when ``CONFIG_DAWN_DESC_SLOTS > 1``,
- for single-slot builds, controlled by ``CONFIG_DAWN_DESC_VALID_CRC32``.

Binary Word Examples
====================

Example 1: minimal descriptor, two IO objects, no config items.

.. code-block:: text

   [0]  0x0d0a0302  // DAWN_DESCRIPTOR_HDR
   [1]  0x00000002  // object count

   [2]  0x40270001  // ObjectID: IO, cls=1, dtype=UINT32, priv=1
   [3]  0x00000000  // config item count

   [4]  0x40270002  // ObjectID: IO, cls=1, dtype=UINT32, priv=2
   [5]  0x00000000  // config item count

   [6]  0x02030a0d  // DAWN_DESCRIPTOR_FOOT
   [7]  0xdeadbeef  // checksum word

Example 2: metadata object + one IO object.

.. code-block:: text

   [0]  0x0d0a0302  // DAWN_DESCRIPTOR_HDR
   [1]  0x00000002  // object count (metadata + one IO)

   [2]  0x00000000  // ObjectID: OBJTYPE_ANY metadata object
   [3]  0x00000001  // metadata config item count
   [4]  0x00070021  // ConfigID: DESC_CFG_VERSION, UINT32, size=1
   [5]  0x00020001  // version value

   [6]  0x40270001  // ObjectID: IO, cls=1, dtype=UINT32, priv=1
   [7]  0x00000000  // config item count

   [8]  0x02030a0d  // DAWN_DESCRIPTOR_FOOT
   [9]  0xdeadbeef  // checksum word

Checksum behavior:

- with ``CONFIG_DAWN_DESC_VALID_CRC32`` enabled, checksum must validate,
- otherwise checksum word is still present, but CRC check is not enforced.

ObjectID/ConfigID bit packing reference: :ref:`object`.

Resource and Performance Impact
===============================

Memory
------

- descriptor storage cost is linear to descriptor size
  (``4 * number_of_words`` bytes),
- ``CDescriptor`` keeps a pointer/length view of that binary and a vector of
  ``CDescObject`` wrappers for allocated entries,
- descriptor config payload is not duplicated by loader; wrappers reference
  descriptor memory.

Performance
-----------

- descriptor validation and parsing are startup-time linear walks over
  descriptor words,
- handlers perform descriptor allocation passes during initialization,

Compiler Optimization and Footprint Tuning
------------------------------------------

Descriptor-driven allocation is runtime-selectable, so compile-time optimizer
cannot always prove which object classes are never used by a given deployment.
For small systems, footprint tuning should rely on Kconfig to enable only the
required Dawn components and features.

Doxygen
=======

- `dawn::CDescriptor <../../doxygen/classdawn_1_1CDescriptor.html>`_

- `dawn::CDescObject <../../doxygen/classdawn_1_1CDescObject.html>`_
