.. _dawnpy-tests:

============
dawnpy-tests
============

Dawn's QA and test orchestration package. Exposes the standalone
``dawnpy-tests`` CLI - the single entry point for the QA pipeline that
must pass before any code change is committed.

This is also where the QA runner lives that other documentation pages
(notably :doc:`/qa/index`) link back to.

Source: `github.com/railab/dawnpy-tests
<https://github.com/railab/dawnpy-tests>`_

Installation
============

Requires the core ``dawnpy`` package plus all transport extensions -
``dawnpy-tests`` depends on the full communication feature surface
because the integration suites exercise every transport::

    pip install -e tools/dawnpy
    pip install -e tools/dawnpy-serial \
                   tools/dawnpy-can \
                   tools/dawnpy-udp \
                   tools/dawnpy-lwm2m \
                   tools/dawnpy-modbus
    pip install -e tools/dawnpy-tests

Direct dependencies (installed automatically): ``dawnpy``,
``dawnpy-serial``, ``dawnpy-can``, ``dawnpy-udp``, ``dawnpy-lwm2m``,
``dawnpy-modbus``, ``click >= 8.1``, ``ntfc >= 0.0.2``.

Command
=======

::

    dawnpy-tests
    dawnpy-tests -j 8
    dawnpy-tests --test-timeout 120
    dawnpy-tests --ntfc-only
    dawnpy-tests --skip-ntfc
    dawnpy-tests --size-only
    dawnpy-tests --help

Common options:

* ``-c`` / ``--config-file PATH``: batch build configuration file.
  Defaults to ``tools/config-build-all.txt``.
* ``--build-root PATH``: root directory for build directories. Defaults
  to ``build``.
* ``--test-build-dir NAME``: simulator test build directory under
  ``--build-root``. Defaults to ``build-sim-sim-tests``.
* ``--test-timeout SECONDS``: simulator test timeout. Defaults to ``60``.
* ``-j`` / ``--jobs N``: number of parallel build jobs.
* ``-v`` / ``--verbose``: enable verbose runner output.
* ``--batch-only``: run only batch build and build size analysis.
* ``--skip-ntfc``: skip NTFC tox and integration test steps.
* ``--ntfc-only``: run only NTFC tox and integration test steps.
* ``--ntfc-list PATH``: NTFC manifest YAML file. Defaults to
  ``ntfc/manifest-host.yaml``; hardware examples include
  ``ntfc/manifest-nrf52840dk.yaml``, ``ntfc/manifest-nrf5340dk.yaml``,
  and ``ntfc/manifest-nucleo-c071rb.yaml``.
* ``--size-only``: run only build size analysis, building missing
  configurations first.
* ``--debug`` / ``--no-debug``: enable verbose logging.

Pipeline
========

The runner executes the following steps in order:

1. **Code format check** - runs ``tools/scripts/check-format.sh fix``,
   auto-fixing C/C++ formatting in-place.
2. **cppcheck** - runs ``tools/scripts/cppcheck.sh`` static analysis.
3. **NTFC tox checks** - runs ``tox -c ntfc/tox.ini`` unless NTFC is
   skipped.
4. **Build all configurations** - compiles every board/config listed in
   ``tools/config-build-all.txt``. Supports ``-j N`` for parallel builds.
5. **Simulator unit tests** - runs the ``nuttx`` binary from the simulator
   test build directory.
   See :doc:`/qa/unit_tests`.
6. **NTFC integration tests** - runs each entry from the active NTFC
   manifest. The default manifest is ``ntfc/manifest-host.yaml``.
   See :doc:`/qa/ntfc`.
7. **Build size analysis** - reports ARM binary memory usage for tracked
   configurations. Regression visibility only, not a pass/fail gate.

Required Outcome
================

**All steps must pass. No exceptions.**

* Formatting must be clean after auto-fix.
* cppcheck must pass.
* NTFC tox checks must pass when NTFC is enabled.
* Every configuration in ``tools/config-build-all.txt`` must compile.
* All unit tests must pass with no failures.
* All active NTFC manifest entries must pass.

If any step fails, fix the issue and re-run the full suite before
committing. The runner depends on host facilities prepared by
``testenv_init.sh``; see :doc:`/qa/index` for prerequisites.

Tests (the runner's own tox suite)
==================================

The package itself is also tested with tox::

    cd tools/dawnpy-tests && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking
