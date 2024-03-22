.. _dawnpy-udp:

==========
dawnpy-udp
==========

UDP transport extension for :doc:`dawnpy`. Provides the ``dawnpy-udp``
CLI for talking to a Dawn device over UDP/IP.

Source: `github.com/railab/dawnpy-udp
<https://github.com/railab/dawnpy-udp>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-udp

Dependencies (installed automatically): ``dawnpy``, ``click >= 8.1``.
The Python standard library handles socket I/O, so no extra transport
package is required.

Command
=======

::

    dawnpy-udp 192.0.2.10
    dawnpy-udp 192.0.2.10 --port 50000
    dawnpy-udp 192.0.2.10 --descriptor descriptor.yaml
    dawnpy-udp --help

The command requires the target host address and opens an interactive UDP
console. By default it uses the Dawn simple protocol discovery commands
(``CMD_LIST_IOS`` and ``CMD_GET_INFO``) to query the device at runtime.
Passing ``--descriptor`` / ``-d`` uses the YAML descriptor as the IO list
instead, then reads those ObjectIDs from the device.

Common options:

* ``--descriptor`` / ``-d``: optional ``descriptor.yaml`` path or
  configuration directory. Use this when the host should inspect the full
  descriptor-defined device state instead of relying on runtime
  ``CMD_LIST_IOS`` discovery.
* ``--port`` / ``-p``: UDP port. Defaults to ``50000``.
* ``--debug`` / ``--no-debug``: enable verbose protocol logging.

Console
=======

The interactive console shares the command set documented in
:ref:`dawnpy-console-commands` (see :doc:`dawnpy-serial`).

Tests
=====

::

    cd tools/dawnpy-udp && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking
