.. _environment:

================
Host Environment
================

This page describes the host environment used to develop, build, and test
Dawn. It is intended as a reference for contributors and users setting up
a new machine.

.. note::

   Dawn has been developed and tested on **Linux only**. Other operating
   systems (macOS, Windows, WSL, BSD) have not been tested and most
   likely require additional work to be functional, especially for the
   QA pipeline (which depends on SocketCAN, ``tap``/``bridge``
   networking, and host-side ``socat`` PTYs).

   Help from users with testing or patches that add support for other
   host operating systems is very welcome.

This page is intentionally a general guide - it should grow over time as
the project evolves and as more host setups are validated.

Supported host
==============

* **Linux** - primary and only fully supported development host.
  Recent mainstream distributions (Arch, Ubuntu LTS, Fedora, Debian
  stable) are expected to work.
* **Other POSIX systems** - not tested. Some effort has been made to
  keep portable code free of OS-specific calls, but the host-side QA
  tooling is Linux-centric.

Base requirements
=================

These are required to build Dawn and run the simulator / Dawn Shell:

* ``git`` - repository checkout, submodule init.
* ``cmake`` and ``ninja`` - NuttX build system.
* A working C/C++ toolchain. Use **gcc 14 or older** for the current NuttX
  libcxx build path; gcc 15 does not currently build it cleanly. Add
  ``-e CXX=g++-14 -e CC=gcc-14`` to ``dawnpy build`` invocations when
  your default host compiler is newer than gcc 14 (see :doc:`/quickstart`).
* Cross toolchains for non-host targets you intend to build (e.g.
  ``arm-none-eabi-gcc`` for ARM boards). Not required for sim-only
  development.
* **Python 3.12 or newer** with ``venv`` and ``pip`` - for ``dawnpy``
  and the rest of the tooling. Check with ``python --version`` before
  creating the virtual environment. On distributions where ``python``
  is unavailable or points to an older interpreter, use ``python3`` or
  an explicit command such as ``python3.12``. ``venv`` is recommended
  (isolates the install from the system Python and avoids permission
  issues), but installing ``dawnpy`` directly with ``pip --user`` also
  works.
* ``kconfiglib`` - required by the NuttX Kconfig step. It is installed
  automatically with ``dawnpy`` when using ``pip install -e tools/dawnpy``.
  If you install it manually, install it into the same Python environment
  used to run ``dawnpy``; a system ``python3-kconfiglib`` package is not
  visible from an active virtual environment.
* Standard NuttX build prerequisites (``kconfig-frontends`` /
  ``kconfig-tools``, ``flex``, ``bison``, ``gperf``, ``make``,
  ``genromfs``). Refer to the NuttX documentation for the exact list
  for your distribution.

Documentation build
-------------------

To build the Sphinx documentation locally:

* ``python`` with the packages listed in
  ``Documentation/requirements.txt``.
* ``make`` (the documentation uses ``Documentation/Makefile``).

QA / test requirements
======================

The full QA pipeline (``dawnpy-tests``) runs unit tests inside the
NuttX **simulator** (a NuttX build that runs as a regular Linux
process) and integration tests (NTFC) against both the simulator and
**QEMU** VMs. These layers depend on host facilities only available on
Linux. See :doc:`/guides/host_development` for the wider host-based workflow
the QA pipeline plugs into.

The one-time environment bootstrap is performed by ``testenv_init.sh``
in the repository root. It needs ``sudo`` to configure kernel
networking interfaces.

Tooling
-------

* ``socat`` - creates the PTY pair used to bridge the simulator's
  serial output to host-side test clients.
* ``iproute2`` (``ip`` command) - creates and configures the virtual
  CAN device, the bridge, and the ``tap`` interface.
* ``sudo`` - required by ``testenv_init.sh`` to add kernel interfaces.
* ``qemu-system-x86_64`` with **KVM** - runs the ``qemu-intel64``
  target used for UDP and Modbus-TCP integration tests.
* ``stty``, ``arp`` - used by ``testenv_init.sh``.

Kernel features
---------------

The Linux kernel must provide:

* **SocketCAN** with the ``vcan`` module - required for all CAN-based
  integration tests. ``testenv_init.sh`` brings up a virtual ``can0``
  interface.
* **TUN/TAP** and **bridging** (``CONFIG_TUN``, ``CONFIG_BRIDGE``) -
  required to attach the QEMU guest to the host network for UDP /
  Modbus-TCP tests. ``testenv_init.sh`` creates ``br0`` and ``tap0``.
* **KVM** (``/dev/kvm`` accessible to the user) - recommended
  for QEMU.

For the per-suite breakdown of which host components each NTFC
integration test depends on, see :doc:`/qa/ntfc`.

Help wanted
===========

Contributions to broaden host support are welcome:

* Reports of working / non-working setups on other Linux distributions.
* Patches adding (or documenting workarounds for) macOS, BSD variants,
  WSL, or native Windows support.
* Improvements to ``testenv_init.sh`` and ``dawnpy-tests`` so the QA
  pipeline can degrade gracefully on hosts that lack a particular
  facility (e.g. running only the suites that the host can support).

If you try Dawn on a non-Linux host, please open an issue describing
what worked and what did not - it is the most useful starting point
for adding proper support.
