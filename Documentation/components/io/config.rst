=========
Config IO
=========

**Component Type:** Input

**Status:** Implemented

Overview
========

``CIOConfig`` is a special-purpose IO used to read one configuration item on
one or more bound objects. When the descriptor marks the ConfigIO as writable,
``dawnpy`` also marks the selected target configuration item as read-write.

It acts as a proxy between normal IO access and
``getObjConfig()`` / ``setObjConfig()`` on the target objects.

Implementation
==============

Implemented behavior:

- ``CIOConfig`` is always readable and writable.
- Notifications are not supported.
- Batch mode is not supported.
- ``getDataSize()`` and ``getDataDim()`` are derived from target
  ``objectCfg`` metadata (DTYPE + size in words).
- Config payload is interpreted as target ``objectCfg`` DTYPE, not hardcoded
  ``uint32_t``.
- Current limitation: ``int64``/``uint64``/``double`` config payloads are not
  supported in ConfigIO conversion and limit validation paths.

Binding behavior:

- ``config.objid_ref`` selects the target configuration ID.
- ``objid_ref_alloc`` is used internally in the generated descriptor to
  allocate the object IDs that this Config IO will bind to.
- At runtime, bound objects are attached through ``bind()``.

Read and write behavior:

- ``getData()`` reads the selected config value from the first bound object.
- ``setData()`` writes the same selected config value to all bound objects.
- Default behavior is immediate apply: a successful ConfigIO write commits
  runtime configuration in the same operation.
- When ``CONFIG_DAWN_IO_LIMITS`` is enabled and ConfigIO has common IO limits
  configured
  (``IO_CFG_LIMIT_MIN/MAX/STEP``), it validates incoming writes before
  forwarding data to bound object configs.
- This allows one ConfigIO per config item to define item-specific runtime
  limits while keeping target objects unchanged.
- For multi-dimensional config payloads, limits use per-element word arrays
  (same word size as the target config payload). Validation is applied
  element-by-element when limit array size matches payload size.
- Access permissions are enforced by the target config item. A read-only target
  can be read, but write will fail.
- ``dawnpy`` sets the target config item's RW bit only when a writable
  ConfigIO exposes that exact field. Other config fields remain read-only by
  default.
- If no object is bound, access fails.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_CONFIG``: enables the Config IO.

YAML
----

.. code-block:: yaml

   ios:
     - id: cfgio1
       type: config
       dtype: uint32
       rw: true
       config:
         objid_ref: dummy1

Supported fields:

- ``config.objid_ref``: target object reference
- ``config.objcfg_ref``: target config field in referenced object
- ``rw``: when ``true`` on a ConfigIO object, grants runtime write access to
  the referenced target config field. On non-ConfigIO objects, ``rw`` does not
  grant configuration write access.
- ``config.offset``: (optional) uint32 word offset within the config field.
  When set, ``getData``/``setData`` operate on a sub-range starting at
  this word offset. Defaults to 0 (full field).
- ``config.size``: (optional) number of uint32 words exposed.
  When set together with ``offset``, only ``size`` words are read/written.
  Defaults to 0 (use the full config field size).

When both ``offset`` and ``size`` are omitted or zero, the Config IO
operates on the entire target config field (backward-compatible
behavior).

When ``offset`` and ``size`` are specified, read operations return only
the requested sub-range. Write operations perform a read-modify-write:
the full config field is read from the target, the sub-range is patched
with the incoming data, and the patched field is written back. This
preserves other fields within the same config item.

Example: expose individual dwell times from a sequencer states table::

   ios:
     - id: cfg_dwell_off
       type: config
       dtype: uint32
       rw: false
       config:
         objid_ref: blinky_seq1
         objcfg_ref: states
         offset: 1   # skip state[0].value
         size: 1     # expose only state[0].dwell_us

     - id: cfg_dwell_on
       type: config
       dtype: uint32
       rw: false
       config:
         objid_ref: blinky_seq1
         objcfg_ref: states
         offset: 3   # skip state[0](value+dwell) + state[1].value
         size: 1     # expose only state[1].dwell_us

``dawnpy`` resolves the target config ID and handles allocation/binding details.

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Brainstorming & Future Ideas
============================

- support for read-only-once and write-only-once ? does it make sense ?
- to read limits from device we have to read descriptor and decode it
- A staged write-back model may be useful when many config values must be
  changed together.
- A future extension can add an optional descriptor-level ``apply_trigger``
  mode (stage on write, commit on trigger). Default behavior should remain
  immediate apply when no trigger mode is configured.
- A future design may need an explicit "write to memory" or "commit" step.
- It is still open how to handle many config objects at once without forcing
  one shared read-back value.

Diagrams
========

.. uml::
   :align: center

   component CDescriptor {
      component obj1Descriptor
   }

   component AnyIO {
      component CObject {
           component CDescObject
      }
   }

   component ConfigIO {
     component Limits
   }

   ConfigIO -> CDescObject : setConfig
   ConfigIO <- CDescObject : getConfig

   CDescObject <-> obj1Descriptor : BoundTo

.. end of uml

Doxygen
=======

- `dawn::CIOConfig <../../doxygen/classdawn_1_1CIOConfig.html>`_
