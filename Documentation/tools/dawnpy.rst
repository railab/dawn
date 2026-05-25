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

Descriptor Tooling Architecture
===============================

`dawnpy` processes descriptor YAML in stages:

1. decode YAML into descriptor object models,
2. validate descriptor structure and enabled component/Kconfig constraints,
3. generate C++ descriptor source or raw binary output,
4. load lightweight client-side descriptor views for host tooling.

This page documents the tooling entry points and workflow. Descriptor schema,
YAML structure, include-block syntax, and multi-descriptor YAML rules are
owned by :doc:`/components/descriptors`.

Within `dawnpy`, the main descriptor tooling surfaces are:

* ``dawnpy.descriptor.definitions.objects`` - object decoding,
* ``dawnpy.descriptor.validation`` - validation/reporting,
* ``dawnpy.descriptor.generation`` - C++ descriptor generation,
* ``dawnpy.descriptor.encoding`` - binary serialization,
* ``dawnpy.descriptor.client`` - host-side descriptor loading.

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
