.. _testing:

=======
Testing
=======

Overview
========

Dawn relies heavily on host-based execution for QA - the simulator and
QEMU targets described in :doc:`/host_development`, combined with the
:doc:`/components/fake_devices`. The pipeline below assumes those
targets are available.

Dawn uses a layered testing strategy:

* **Coding standards enforcement** - automated format checks and static
  analysis ensure consistent style before any code reaches the test stage.

* **Unit tests** - each framework component is tested in isolation inside
  the NuttX simulator using the ``dawntest`` application.
  See :doc:`unit_tests`.

* **NTFC integration tests** - full-system scenarios are executed against
  a built firmware image running on a simulator or emulator target,
  verifying end-to-end protocol and program behaviour.
  See :doc:`ntfc`.

All three layers are integrated into one command (``dawnpy-tests``) and
must pass before committing any change.

QA Runner
=========

The single entry point for the full QA pipeline is the ``dawnpy-tests``
command provided by the ``dawnpy-tests`` extension package. It runs
formatting, builds, unit tests, and NTFC integration tests in sequence.

For installation, command flags, pipeline steps, and required outcome,
see :ref:`dawnpy-tests` in the dawnpy tool reference.

Prerequisites
-------------

Before running tests for the first time, initialize the test environment:

.. code:: shell

   sh ./testenv_init.sh

This script configures test infrastructure required across all layers
(virtual ``can0`` interface, QEMU ``br0`` + ``tap0``, ``socat`` PTY
bridge). Per-suite host requirements are listed in :doc:`ntfc`.

Static Analysis & Formatting
============================

These checks run alongside the QA pipeline.

clang-format
------------

All C and C++ sources are formatted with ``clang-format`` using the
Mozilla style, configured in ``.clang-format`` at the repository root.

.. code:: shell

   ./tools/scripts/check-format.sh fix    # auto-fix all files
   ./tools/scripts/check-format.sh check  # check only, no changes

The ``dawnpy-tests`` pipeline runs the fix variant automatically as its
first step.

cppcheck
--------

`cppcheck <https://cppcheck.sourceforge.io/>`_ is used for static
analysis beyond what the compiler reports.

.. code:: shell

   ./tools/scripts/cppcheck.sh

By default the wrapper reports actionable severities only. For exploratory
style cleanup, override the enabled checks:

.. code:: shell

   CPPCHECK_ENABLE=all ./tools/scripts/cppcheck.sh

If a previous interrupted run leaves stale cppcheck cache state, point the
wrapper at a fresh build directory:

.. code:: shell

   CPPCHECK_BUILD_DIR=/tmp/dawn-cppcheck-build ./tools/scripts/cppcheck.sh

Currently a standalone step, not integrated into the ``dawnpy-tests``
pipeline. Integration is planned for a future release.

clang-tidy
----------

`clang-tidy <https://clang.llvm.org/extra/clang-tidy/>`_ provides
additional static analysis driven by the project ``.clang-tidy``
configuration. The wrapper script consumes ``compile_commands.json``
from an existing build directory (``build_tests`` or ``build`` are
auto-detected; override with ``BUILD_DIR=<path>``):

.. code:: shell

   ./tools/scripts/clang-tidy.sh check                      # all files in compile DB
   ./tools/scripts/clang-tidy.sh check-file <path/to.cxx>   # single file

Initial implementation, run standalone like ``cppcheck`` and not yet
integrated into the ``dawnpy-tests`` pipeline. Integration is planned
for a future release.

Contents
========

.. toctree::
   :maxdepth: 1

   standard.rst
   unit_tests.rst
   ntfc.rst
