.. _dawnpy-serial:

=============
dawnpy-serial
=============

Serial transport extension for :doc:`dawnpy`. Provides the
``dawnpy-serial`` CLI for talking to a Dawn device over a serial port,
including the interactive console used for IO discovery, read/write,
and live monitoring.

Source: `github.com/railab/dawnpy-serial
<https://github.com/railab/dawnpy-serial>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-serial

Dependencies (installed automatically): ``dawnpy``, ``click >= 8.1``,
``pyserial >= 3.5``.

Command
=======

::

    dawnpy-serial /dev/ttyUSB0
    dawnpy-serial /dev/ttyUSB0 --descriptor descriptor.yaml
    dawnpy-serial upload /dev/ttyUSB0 descriptor.bin
    dawnpy-serial --help

The top-level command opens the interactive serial console when invoked
with a device path. Utility subcommands live under the same executable.
By default the console uses ``CMD_LIST_IOS`` and ``CMD_GET_INFO`` to
discover IOs from the running device. Passing ``--descriptor`` /
``-d`` uses the YAML descriptor as the IO list instead, then reads those
ObjectIDs from the device.

Common options and subcommands:

* ``--descriptor`` / ``-d``: optional ``descriptor.yaml`` path or
  configuration directory. Use this when the host should inspect the full
  descriptor-defined device state instead of relying on runtime
  ``CMD_LIST_IOS`` discovery.
* ``--debug`` / ``--no-debug``: enable verbose protocol logging.
* ``upload PATH DESCRIPTOR_BIN``: upload a raw descriptor binary to a
  runtime descriptor slot over serial. This does not take a YAML
  descriptor for discovery; it addresses Dawn's framework-defined
  descriptor and descriptor-selector IO objects directly.
* ``upload --slot N``: target descriptor slot. Slot 0 is read-only; the
  default upload target is slot 1.
* ``upload --chunk-size N``: set the ``CMD_SET_IO_SEEK`` chunk size.
* ``upload --verify`` / ``--no-verify``: read back the uploaded slot and
  compare SHA256.
* ``upload --switch`` / ``--no-switch``: request a descriptor switch after
  upload.
* ``upload --wait-reconnect SECONDS``: wait for the device to reconnect
  after a switch request.

Console
========

Once connected, the interactive console accepts the shared commands
listed under :ref:`dawnpy-console-commands`. Transport-specific commands
are listed in the CLI help and console help.

Tests
=====

::

    cd tools/dawnpy-serial && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking

.. _dawnpy-console-commands:

Interactive Console Commands
============================

The serial, UDP, BLE, CAN, and Modbus consoles share a normalized command
style. Transport-specific commands are listed in each console help menu.

* ``d``: **Discovery** - List all IO objects found on the device.
* ``devices``: **Devices** - List loaded or discovered device details when
  the transport supports it.
* ``l``: **List** - List cached discovered IOs or descriptor-backed IOs.
* ``i <objid>``: **Info** - Show details for a specified IO object.
* ``r <objid>``: **Read** - Read value(s) from specified IO object(s).
  * ``r 0x40A10001`` - Read a single IO.
  * ``r 0x40A10001,0x40A10002`` - Read multiple IOs.
* ``s <objid>``: **Seek** - Read a seekable/block IO object when supported.
* ``w <objid> <val>``: **Write** - Write a value to a specific IO object.
  * ``w 0x40A10001 100`` - Write a single value.
* ``m [objid]``: **Monitor** - Continuously monitor IO values.
  * ``m`` - Monitor all available IOs.
  * ``m 0x40A10001`` - Monitor a specific IO.
  * ``m 0x40A10001,0x40A10002`` - Monitor multiple IOs.
* ``h``: **Help** - Show the help menu.
* ``q``: **Quit** - Exit the console.

Serial additionally provides ``p`` for dtype parsing and ``t`` for timing
analysis. BLE adds service and notification commands. CAN and Modbus keep
their descriptor-aware multi-node read/write forms.
