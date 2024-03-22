.. _dawnpy-modbus:

=============
dawnpy-modbus
=============

Modbus transport extension for :doc:`dawnpy`. Provides the
``dawnpy-modbus`` CLI for talking to Dawn devices over Modbus RTU.

Source: `github.com/railab/dawnpy-modbus
<https://github.com/railab/dawnpy-modbus>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-modbus

Dependencies (installed automatically): ``dawnpy``, ``click >= 8.1``,
``pymodbus >= 3.1, < 4``.

Command
=======

::

    dawnpy-modbus /dev/ttyUSB0 device1/descriptor.yaml device2/descriptor.yaml
    dawnpy-modbus --help

The command requires a serial path followed by one or more descriptor
paths. Each descriptor describes one Modbus node for descriptor-aware
multi-node access.

Common options:

* ``--baudrate N``: serial baudrate. Defaults to ``115200``.
* ``--parity N|O|E``: serial parity. Defaults to even parity.
* ``--stopbits 1|2``: serial stop bits. Defaults to ``1``.
* ``--timeout SECONDS``: serial timeout. Defaults to ``1.0``.
* ``--unit N``: Modbus slave address. Defaults to ``1``.
* ``--kconfig-var SYMBOL`` and ``--kconfig-values VALUES``: apply
  Kconfig-derived object ID overrides to descriptor instances.
* ``--debug`` / ``--no-debug``: enable verbose protocol logging.

Console
=======

The interactive console shares the command set documented in
:ref:`dawnpy-console-commands` (see :doc:`dawnpy-serial`).

Tests
=====

::

    cd tools/dawnpy-modbus && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking
