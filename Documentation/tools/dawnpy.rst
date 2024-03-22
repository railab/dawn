.. _dawnpy:

======
dawnpy
======

**dawnpy** is the core Python package of Dawn's tooling family. It owns
repository management, build helpers, plugin loading, and YAML descriptor
handling. Device-facing protocols and the QA test runner are
shipped as separate packages - see the other pages in this section.

Source: `github.com/railab/dawnpy <https://github.com/railab/dawnpy>`_

Installation
============

The recommended workflow uses a virtual environment created at the dawn
repo root::

    python -m venv .venv
    source .venv/bin/activate
    pip install -e tools/dawnpy

Install extension packages on top, only as needed (each has its own page).

After installation, the CLI can be invoked as a module::

    python -m dawnpy --help

Global options must be passed before a sub-command::

    python -m dawnpy --debug <command>
    python -m dawnpy --types-from dawnpy_types.py <command>

Common global options:

* ``--debug`` / ``--no-debug``: enable verbose output. The
  ``DAWNPY_DEBUG`` environment variable provides the same switch.
* ``--types-from PATH``: load out-of-tree descriptor type registrations
  from a Python file or package directory. May be passed multiple times.

Command Reference
=================

Project Management
------------------

* ``init``: Bootstraps a Dawn workspace or inline install, optionally with
  NuttX sources and a global ``dawnrc`` entry.
* ``project list``: Lists bundled out-of-tree project templates.
* ``project new``: Scaffolds a new out-of-tree Dawn project.
* ``build``: Configures and builds the project for a specific board and
  configuration using CMake.
* ``batch``: Configures and builds multiple targets defined in a batch
  configuration file (e.g., ``tools/config-build-all.txt``).
* ``kconfig``: Iteratively builds multiple configurations while varying a
  specific Kconfig symbol.

Descriptor Management
---------------------

* ``desc-new``: Create a new descriptor YAML file stub and print a docs
  update reminder.
* ``desc-valid``: Validates a YAML configuration directory (must
  contain ``descriptor.yaml`` and ``defconfig``) against the framework
  schema.
* ``desc-gen``: Compiles a YAML descriptor into a binary C++
  source file (``descriptor.cxx``).
* ``desc-bin``: Serializes a YAML descriptor into raw
  little-endian binary (``.bin``) with Dawn-compatible CRC.
* ``desc-decode-caps``: Decodes a capabilities IO binary
  blob into human-readable sections (enabled classes, dtypes, flags, slot
  info).
* ``desc-headers-check``: Validates runtime C++ header discovery and
  parsing (use ``--strict`` to also check ``cpp_helper`` / ``enum_prefix``
  references in handlers).

CLI usage examples::

    # Create a new descriptor placeholder in descriptors/examples/
    python -m dawnpy desc-new my_example

    # Validate descriptor and configuration
    python -m dawnpy desc-valid /path/to/config/directory

    # Quiet mode (errors only)
    python -m dawnpy desc-valid -q /path/to/config/directory

    # Verbose mode
    python -m dawnpy desc-valid -v /path/to/config/directory

    # Generate C++ descriptor from YAML
    python -m dawnpy desc-gen descriptor.yaml

    # Generate raw descriptor binary from YAML
    python -m dawnpy desc-bin descriptor.yaml -o descriptor.bin

    # Decode capabilities blob from file
    python -m dawnpy desc-decode-caps capabilities.bin

    # Decode capabilities blob from hex text file
    python -m dawnpy desc-decode-caps capabilities.bin \
      --hex-file capabilities.hex

    # Also accepts shell hexdump/xxd-like text files
    python -m dawnpy desc-decode-caps capabilities.bin \
      --hex-file capabilities_hexdump.txt

    # Specify custom output path
    python -m dawnpy desc-gen descriptor.yaml -o output.cxx

Build-time YAML generation
==========================

When a Dawn app uses YAML descriptor mode
(``CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_YAML=y``), the build system
invokes the dawnpy generator automatically and writes the generated
descriptor C++ file to the build directory (``generated_descriptor.cxx``).

The CMake integration calls the CLI contract directly:

.. code-block:: text

   python -m dawnpy desc-gen <descriptor.yaml> -o <output.cxx>

The integration can be overridden for external package layouts with CMake
cache variables:

* ``DAWN_DAWNPY_COMMAND``: command list used to invoke the CLI
  (default: ``${Python3_EXECUTABLE};-m;dawnpy``)
* ``DAWN_DAWNPY_PYTHONPATH``: optional in-tree fallback path for
  uninstalled sources; leave empty when using installed packages

Legacy static C++ mode
----------------------

When a Dawn app uses C++ descriptor mode
(``CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_CXX=y``), the build includes the
descriptor directly from ``CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH``.

Descriptor Pipeline Architecture
================================

The descriptor pipeline is intentionally staged:

1. **Decode** - YAML entries are decoded into object-specific classes
   (``IoObject``, ``ProgramObject``, ``ProtocolObject`` in
   ``dawnpy.descriptor.definitions.objects``) with strict field checks.
2. **Validate** - Object-level decoding catches schema and type errors
   early (for example malformed tags), while generation-time logic
   resolves references and emits protocol-specific config.
3. **Generate/consume**:

   * ``DescriptorGenerator`` maps decoded objects to C++ descriptor words.
   * Client descriptor loading maps decoded objects to lightweight runtime
     client models.

This split keeps parsing rules local to each object type and keeps
generator/client orchestration focused on translation rather than schema
interpretation.

Example YAML descriptor::

    metadata:
      name: "My Device"
      version: "1.0"

    ios:
      - &adc0
        id: adc0
        type: adc_fetch
        instance: 0
        dtype: int32
        config:
          device: 0

    programs:
      - id: sampler0
        type: sampling
        instance: 0
        config:
          inputs: [*adc0]
          outputs: []

    protocols:
      - id: serial0
        type: serial
        instance: 0
        config:
          bindings: [*adc0]

The generator supports:

* **IOs**: Analog/digital inputs/outputs, sensors, timers, and other IO
  types.
* **Programs**: Data processing (sampling, statistics, adjustments).
* **Protocols**: Communication protocols (Serial, Modbus, CAN, BLE/Nimble,
  etc.).
* **Metadata**: Device information (name, version, manufacturer, etc.).
* **YAML anchors**: Reference objects by ID for bindings and connections.
* **Multi-descriptor YAML**: Multiple descriptor tables in a single YAML
  file for FLASH-based runtime slot switching (see below).
* **Bulk regeneration**: Regenerate all ``descriptor.cxx`` files from
  ``descriptor.yaml`` in the ``boards/`` directory tree.

Multi-descriptor YAML
---------------------

A single YAML file can define multiple descriptor tables using numbered
``descriptorN`` top-level keys. The generator produces one ``uint32_t``
array per block and a ``dawn_register_flash_slots()`` function that
``dawn_main`` calls at boot to register all extra slots from FLASH -
no over-the-air upload is required.

``descriptor0`` is mandatory and becomes the default boot descriptor
(``g_dawn_desc[]``). Additional blocks are optional:

.. code-block:: yaml

    descriptor0:
      metadata:
        version: "1.0"
      ios:
        - &adc_main
          id: adc_main
          type: adc_fetch
          instance: 0
          dtype: int32
          config:
            device: 0
      protocols:
        - id: serial0
          type: serial
          instance: 0
          config:
            bindings: [*adc_main]

    descriptor1:
      ios:
        - &adc_alt
          id: adc_alt
          type: adc_fetch
          instance: 0
          dtype: int32
          config:
            device: 1
      protocols:
        - id: serial0
          type: serial
          instance: 0
          config:
            bindings: [*adc_alt]

Generated output (abbreviated):

.. code-block:: cpp

    uint32_t g_dawn_desc[] = { /* descriptor0 content */ };
    size_t   g_dawn_desc_size = sizeof(g_dawn_desc);

    uint32_t g_dawn_desc1[] = { /* descriptor1 content */ };
    size_t   g_dawn_desc1_size = sizeof(g_dawn_desc1);

    int dawn_register_flash_slots(void)
    {
      /* registers g_dawn_desc1 as slot 1 in CDevDescriptor */
    }

Rules:

* Keys must be contiguous starting from ``descriptor0``. The generator
  stops scanning at the first missing index (``descriptor2`` without
  ``descriptor1`` is silently ignored).
* Macro names that appear in more than one descriptor section are
  ``#undef``'d between sections to prevent redefinition warnings.
* ``CONFIG_DAWN_DESC_SLOTS`` must be at least equal to the total number
  of defined descriptors for all slots to be addressable at runtime.
* Runtime switching uses ``CIODescSelector`` / ``CDescSwitch`` - the
  same mechanism as RAM-upload slots. The active slot index can be
  written to a ``descselector`` IO object to trigger a switch.

The old flat YAML format (``ios``/``programs``/``protocols`` at the top
level) is unchanged and remains the default for single-descriptor
deployments.

Python API
==========

Object ID Decoder
-----------------

Decode and format Dawn Object IDs to understand their structure, including
type, class, data type, and instance information::

    from dawnpy import ObjectIdDecoder

    decoder = ObjectIdDecoder()
    decoded = decoder.decode(0x40A10001)
    print(decoder.format_detailed(decoded))

Descriptor Validator
--------------------

Validate descriptor configurations to ensure all required components are
properly included and configured::

    from dawnpy import DescriptorValidator

    validator = DescriptorValidator()
    result = validator.validate("/path/to/config/directory")
    print(validator.format_report(result))

Descriptor Generator
--------------------

Generate C++ descriptor files from YAML specifications::

    from dawnpy.descriptor.generation.generator import generate_descriptor

    generate_descriptor("descriptor.yaml", "descriptor.cxx")

Standalone Protocol Tools
=========================

Each protocol handler package is its own standalone program:
``dawnpy-serial``, ``dawnpy-can``, ``dawnpy-udp``, ``dawnpy-ble``, and
``dawnpy-modbus``. Installing these packages must not add protocol
commands to the core ``dawnpy`` CLI. The core CLI remains focused on
build, descriptor, project, and configuration workflows.

Tests
=====

Run the package tox suite::

    cd tools/dawnpy && tox

Individual environments::

    tox -e py        # Run tests with coverage
    tox -e format    # Run code formatting checks
    tox -e flake8    # Run linting
    tox -e type      # Run type checking

All Python tool packages under ``tools/`` follow this same tox baseline.
Packages with standalone integration requirements keep those requirements
documented on their tool page, but the package-local tox suite remains the
required quick check after Python changes. Core ``dawnpy`` keeps the 100%
coverage gate; transport packages scope coverage to package-owned code and
exclude standalone/interactive entry points that require live devices.
