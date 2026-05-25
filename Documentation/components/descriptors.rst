.. _descriptor-format:

=================
Descriptor Format
=================

A Dawn descriptor declares which IO objects, programs, and protocols should
exist, how they are connected, and which static configuration values they use.

Most users should write descriptors in YAML. ``dawnpy`` converts YAML into the
C++ descriptor source compiled into the firmware image.

Advanced users may write the C++ descriptor directly. YAML is a convenience
format, not a runtime requirement.

Descriptors are intentionally independent from board configurations. A board
or build config provides the OS resources and enabled Dawn components; the
descriptor describes the Dawn object graph that should run when those
resources are available.

Build flow
==========

With YAML, the normal build flow is:

1. ``dawnpy`` validates the YAML descriptor.
2. ``dawnpy`` generates C++ descriptor source, usually
   ``generated_descriptor.cxx`` in the build directory.
3. The C++ descriptor is compiled into the firmware.
4. At runtime, Dawn loads the compiled descriptor data from the firmware image.

Some tools can also serialize YAML to a raw binary file for inspection,
transport, or host-side workflows, but firmware builds still use C++
descriptor source.

Descriptor forms
================

``YAML``
  The human-friendly source form. This is the recommended way to write and
  review descriptors.

``C++``
  The firmware source form. ``dawnpy`` usually generates it from YAML, but it
  can also be hand-written for advanced use.

``Binary``
  The compact 32-bit-word layout loaded by the runtime and exchanged with a
  running device. For binary internals, see :ref:`descriptor`.

YAML top-level format
=====================

A single-descriptor YAML file has these top-level sections:

``metadata``
  Human-readable information and resource notes. ``os_required`` is
  informational metadata only; Dawn does not enforce it at build time or
  runtime.

``includes``
  Optional reusable descriptor blocks loaded from other YAML files.
  ``dawnpy`` expands them before validation and code generation.

``ios``
  IO objects such as dummy values, GPIO, sensors, file IO, descriptor slots,
  controls, triggers, and virtual values.

``programs``
  Processing objects that connect IOs together, for example sampling,
  filtering, buffering, selectors, counters, gateways, and expression logic.

``protocols``
  Protocol objects that expose IOs and program outputs through transports such
  as shell, serial, CAN, UDP, Modbus, NXScope, IPC, or NimBLE.

Minimal YAML example
====================

.. code-block:: yaml

   metadata:
     title: Minimal Shell Demo
     version: "1.0"
     description: >
       Exposes one dummy value over the shell protocol.
     os_required: []

   ios:
   - &dummy1
     id: dummy1
     type: dummy
     dtype: uint32
     config:
       init_value: 42

   protocols:
   - id: shell1
     type: shell
     config:
       prompt: "dawn> "
       objects:
       - *dummy1

YAML object entries
===================

Each object entry has a stable ``id`` and a ``type``. Most IO objects also
declare a ``dtype``. Object-specific options live under ``config``.

YAML anchors are the usual way to connect objects. In the example above,
``&dummy1`` defines an anchor and ``*dummy1`` references the same IO from the
shell protocol configuration. The generator resolves these references and emits
the object IDs used by the generated C++ descriptor.

YAML include blocks
===================

Descriptors and reusable blocks may define an ``includes`` list. Each entry
has:

- ``id`` - namespace used when referring to exported block outputs,
- ``path`` - YAML file path, resolved relative to the including YAML file,
- ``inputs`` - optional mapping that connects the block's declared input ports
  to outer descriptor object IDs or earlier include outputs.

An included block may contain any normal Dawn sections (``ios``, ``programs``,
``protocols``) and may also declare a public interface:

``inputs``
  List of input port names that the parent descriptor must connect.

``outputs``
  List or mapping of exported names to internal object references. Only these
  exports are visible to the including descriptor.

Inside the block, refer to declared inputs with quoted strings such as
``"@INPUTS.control"``. Outside the block, refer to exported outputs with
quoted strings such as ``"@blinky.control"`` where ``blinky`` is the include
entry ``id``.

``dawnpy`` automatically namespaces block-internal object IDs during expansion,
so the parent descriptor only sees the declared ``outputs`` contract and not
other internal IDs.

Example:

.. code-block:: yaml

   includes:
     - id: blinky
       path: include_blocks/blinky_common.yaml

   protocols:
     - id: serial1
       type: serial
       bindings:
         - "@blinky.led"
         - "@blinky.control"

Blocks can also encapsulate protocols and consume outer IO through declared
``inputs``:

.. code-block:: yaml

   inputs:
     - led

   protocols:
     - id: serial1
       type: serial
       bindings:
         - "@INPUTS.led"
       config:
         path: /dev/ttyS0
         baudrate: 115200

Include expansion works in normal single-descriptor YAML files and inside each
``descriptorN`` block of multi-descriptor YAML.

YAML ``metadata.os_required``
=============================

``os_required`` is a user-facing checklist of resources expected by the
descriptor. Use concrete paths when the descriptor depends on a specific
OS-visible node, for example ``/dev/can0``, ``/dev/ttyS1``, ``/dev/gpio0``, or
``/tmp``.

Use short subsystem names when there is no concrete descriptor path but the
protocol needs a subsystem to be available:

- ``udp`` for UDP-based descriptors such as Dawn UDP, Wakaama LwM2M, and
  NXScope UDP,
- ``tcp`` for TCP-based descriptors such as Modbus TCP,
- ``ble`` for NimBLE descriptors.

YAML multi-descriptor files
===========================

A YAML file can also define multiple descriptor slots with ``descriptor0``,
``descriptor1``, and so on. ``descriptor0`` is the default boot descriptor.
Additional slots can be used for descriptor switching workflows.

.. code-block:: yaml

   descriptor0:
     metadata:
       title: Default
       version: "1.0"
     ios: []
     protocols: []

   descriptor1:
     metadata:
       title: Alternate
       version: "1.0"
     ios: []
     protocols: []

Related pages
=============

- :doc:`/examples/descriptors` lists the descriptor files available in this
  repository and groups them by feature.
- :doc:`/tools/dawnpy` documents descriptor generation commands such as
  ``desc-gen`` for C++ generation and ``desc-bin`` for raw binary export.
- :doc:`/components/index` describes the IO, Program, and Protocol component
  types that descriptor entries can instantiate.
- :ref:`descriptor` documents the runtime descriptor loader and binary
  container internals.
