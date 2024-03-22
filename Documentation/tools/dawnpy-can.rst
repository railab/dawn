.. _dawnpy-can:

==========
dawnpy-can
==========

CAN transport extension for :doc:`dawnpy`. Provides the ``dawnpy-can``
CLI for talking to a Dawn device over a SocketCAN (or compatible)
interface, plus the same interactive console as ``dawnpy-serial``.

Source: `github.com/railab/dawnpy-can
<https://github.com/railab/dawnpy-can>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-can

Dependencies (installed automatically): ``dawnpy``, ``click >= 8.1``,
``python-can >= 4.0``.

Command
=======

::

    dawnpy-can descriptors/ntfc/ntfc_can_dummy.yaml --ifname can0
    dawnpy-can --help

The command requires one or more descriptor paths. Each descriptor
describes one CAN node for descriptor-aware multi-node access.

Common options:

* ``--ifname IFACE``: SocketCAN interface name. Defaults to ``can0``.
* ``--extended`` / ``--standard``: force extended or standard CAN IDs.
* ``--heartbeat-mult N``: heartbeat timeout multiplier.
* ``--heartbeat-default SECONDS``: fallback heartbeat interval when the
  descriptor does not define one.
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

    cd tools/dawnpy-can && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking
