.. _virtual_io:

==========
Virtual IO
==========

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIOVirt`` is a virtual IO object managed by a Dawn Program. It acts as a
software-defined data point that can be read from or written to like any
physical IO. It is the standard program-to-program chaining surface in Dawn:
one program can publish data to a ``virt`` object and another program can
consume that same object as an input.

Dawn is built around fixed-shape IO contracts. A valid Dawn program graph must
have an init-time-known output shape at every edge, including edges backed by
``virt`` objects.

Implementation
==============

Virtual IOs are unique because they do not rely on hardware drivers or
the standard OS-level ``poll()`` mechanism for notifications. Instead,
they use a direct pass-through system:

- **Notifications**: When a Program calls ``setVal()``, the virtual IO
  immediately executes the callbacks of all registered consumers. This
  minimizes latency by bypassing the framework's main notification loop.
- **Provider Callbacks**: Programs can register ``get`` and ``set``
  callbacks. These are triggered whenever another component (like a
  Protocol) accesses the IO, allowing the Program to provide data on-demand
  or react to changes instantly.

Initialization Contract
=======================

``CIOVirt::init()`` does not allocate data storage by itself. A virtual IO
becomes usable only after some component defines its runtime shape by calling
``initialize(dim, batch, notify)``.

Descriptors must not configure ``virt`` shape directly. In particular, there is
no YAML ``virt.config.dim`` field. Shape is owned by the producing program,
protocol, or application code that can validate the whole edge contract. Keep
shape options on that owner instead of adding them to generic ``virt`` IOs.

Current Dawn uses a shape-owner model:

- The component that **defines the fixed-shape contract** for a given
  ``virt`` object is responsible for initializing it.
- That owner is often the program that publishes to the ``virt`` object, but
  it may also be protocol-side or application-side code when the ``virt`` is
  used as an externally provided source.
- Components that only **consume** a ``virt`` object must never initialize it.
- In a long program chain, ownership is local to each edge, not global to the
  whole chain.
- Because Dawn uses fixed-shape IO, the shape owner must know the runtime
  contract of that ``virt`` object during ``init()``.

For example::

   IO -> ProgA -> virt1 -> ProgB -> virt2 -> ProgC -> virt3

- ``ProgA`` owns the shape of ``virt1``.
- ``ProgB`` owns the shape of ``virt2``.
- ``ProgC`` owns the shape of ``virt3``.

This means a chain does not need a single top-level owner. Each edge has its
own shape owner.

Deferred Init and Reuse
=======================

The YAML ``virt`` description provides only object identity and dtype. It does
not provide per-instance shape fields such as ``dim`` or ``notify``.

Because of that, every ``virt`` object is one of these at runtime:

- **deferred-init virt**: created from YAML and left uninitialized until a
  shape-owning program, protocol, or application-specific code calls
  ``initialize(...)``.
- **producer-initialized virt**: a producer has already established the
  runtime shape and later users must reuse that shape.

Programs that support ``virt`` outputs should follow this rule:

- if the target ``virt`` is uninitialized and the program can determine the
  required shape exactly, it may initialize it
- if the target ``virt`` is already initialized, the program should validate
  compatibility and reuse it
- if the program cannot determine the shape and the target ``virt`` is still
  uninitialized, ``init()`` should fail

This is not a generic late-bound pipeline mechanism. If a program cannot define
its output shape during ``init()``, it is not a good fit for Dawn's ``virt``
model.

Current Producer Support
========================

Programs that may own deferred output-side ``virt`` shape in the current
runtime include:

- ``CProgProcess`` and its derived one-input producer programs
- ``CProgAdjust``
- ``CProgSampling``
- ``CProgBuffer`` for its ``out``, ``sel``, and ``stat`` roles
- ``CProgGateway`` for endpoint pairs whose shape is defined by gateway config
- ``CProgRedirect``
- ``CProgBitPack``
- ``CProgSwitch`` when the target is a ``virt``
- ``CProgSelector`` when the target is a ``virt``
- ``CProgSequencer`` when the target is a ``virt``

Programs that consume ``virt`` inputs but do not initialize those input-side
objects include:

- ``CProgBitPack`` input ``virt`` objects
- ``CProgSwitch`` input ``virt`` objects
- ``CProgSelector`` control and data ``virt`` objects

Those consumer-side ``virt`` objects must already be usable, either because an
upstream program owns them or because protocol-side or application-side code
initialized them before the consumer starts.

Descriptor Validation
=====================

``dawnpy`` enforces the same contract at descriptor decode time:

- only fixed-shape programs may own output-side ``virt`` shape
- consumer-side ``virt`` references are never treated as owners
- custom target fields such as ``selector.target``, ``switch.target``,
  ``sequencer.targets``, ``buffer.iobind``, and ``gateway.iobind`` are checked
  the same way as standard ``outputs``

This keeps the fixed-shape Dawn invariant explicit in both runtime behavior and
descriptor tooling.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_VIRT``: enables virtual IO objects.

YAML
----

.. code-block:: yaml

   ios:
     - id: virt1
       type: virt
       dtype: uint32

This form declares only the virtual IO identity and data type. Runtime shape is
established later by the owning program, protocol, or application code.

Do not add ``config`` shape fields to ``virt`` descriptors. If a program needs
to expose a scalar, vector, or chunked output through ``virt``, that shape must
be expressed in the producer program configuration and applied by the producer
when it initializes the output.

External Control
================

- ``ControlIO``: not supported.
- ``TriggerIO``: not supported.

Doxygen
=======

- `dawn::CIOVirt <../../doxygen/classdawn_1_1CIOVirt.html>`_
