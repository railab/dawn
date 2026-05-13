.. _host_development:

========================
Host-based Development
========================

Dawn is built on top of `Apache NuttX <https://nuttx.apache.org/>`_,
which gives it access to powerful host-side execution targets:

* the **NuttX simulator** - a NuttX kernel that builds as a regular
  Linux user-space process and runs natively on the host;
* **NuttX QEMU** targets - full NuttX images that boot inside a
  ``qemu-system-*`` virtual machine with KVM acceleration on the host.

Combined with Dawn's :doc:`/components/fake_devices`, these targets
allow a large part of Dawn development, demonstration, and testing to
happen on a developer workstation, with no real embedded board involved.

Why develop on host
===================

Working on the host shortens the iteration loop and removes the usual
hardware friction:

* No flashing, wiring, or board bring-up between code changes.
* Standard Linux tools (``gdb``, ``perf``, ``valgrind``,
  ``strace``, log capture) work directly on the simulator process.
* The same descriptor-driven IO/PROG/PROTO pipeline that runs on real
  silicon also runs on the host, so the bulk of framework and
  application logic can be developed and exercised before any hardware
  is touched.
* Demos and reproductions are easy to share.
* The CI / QA pipeline depends on host targets (see :doc:`/qa/index`).
  Anything that runs on the host can also be automated.

NuttX simulator
===============

The simulator is the primary host target. NuttX builds as a Linux
process (``./<build_dir>/nuttx``) and Dawn runs inside it as if it were
on real hardware: the same drivers, the same descriptor parsing, the
same IO/PROG/PROTO objects.

Dawn keeps a dedicated simulator board under
``boards/sim/sim/sim/`` (custom variant of the NuttX simulator board
trimmed for Dawn use). Its configurations include the showcase
``nsh_shell`` config (the easiest entry point, see
:doc:`/quickstart`), the unit-test configurations ``tests`` and
``nsh_tests``, and a number of protocol-focused demos
(``nsh_can``, ``nsh_serial``, ``nsh_modbus_rtu``, ``nsh_nxscope``,
``nsh_gateway``, ``nsh_programs_can``, ``nsh_blinky_shell``,
``dynamic_desc``).

Most of these configurations enable the fake drivers so that ADC
samples, sensor streams, GPIO state, LEDs, buttons, and similar
peripherals look real to Dawn.

NuttX QEMU
==========

QEMU targets fill the gap where the plain simulator is not enough -
mainly when a real network stack, BSP code closer to a physical
architecture, or a particular CPU feature set is required.

Dawn ships QEMU board configurations under ``boards/<arch>/qemu/...``.
The QA pipeline currently uses them for UDP and Modbus-TCP integration
tests, where the QEMU guest is bridged to the host network through a
``tap`` interface. The same fake drivers are available, so the
peripheral side of the system behaves the same way as in the simulator.

Bringing a QEMU target up requires a working ``qemu-system-*`` binary
and KVM (for x86_64 targets). See :doc:`/guides/environment` for the full
list of host packages and kernel features.

Where to go next
================

* :doc:`/quickstart` - install the toolchain and run the first
  simulator build (``nsh_shell``).
* :doc:`/guides/environment` - host packages, kernel features, and one-time
  setup needed for the simulator and QEMU.
* :doc:`/components/fake_devices` - fake NuttX drivers used to make
  hardware-less targets behave like real boards.
* :doc:`/qa/index` - the QA pipeline that drives the simulator and
  QEMU targets in CI.
* :doc:`/examples/boards/index` - per-board, per-config catalogue of
  available demos.
