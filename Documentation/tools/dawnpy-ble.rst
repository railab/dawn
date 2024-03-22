.. _dawnpy-ble:

==========
dawnpy-ble
==========

BLE transport tool for :doc:`dawnpy` projects. Provides the standalone
``dawnpy-ble`` CLI; it does not add BLE commands to the core ``dawnpy``
CLI.

Source: `github.com/railab/dawnpy-ble
<https://github.com/railab/dawnpy-ble>`_

Installation
============

Requires the core ``dawnpy`` package to be installed first::

    pip install -e tools/dawnpy-ble

Dependencies (installed automatically): ``dawnpy``, ``click >= 8.1``,
``bleak >= 0.22``.

Command
=======

::

    dawnpy-ble ble AA:BB:CC:DD:EE:FF --descriptor descriptor.yaml
    dawnpy-ble ble --scan --descriptor descriptor.yaml
    dawnpy-ble ble AA:BB:CC:DD:EE:FF --dump-services
    dawnpy-ble ots list --name dawn-ots
    dawnpy-ble --help

The package provides two CLI command groups:

* ``dawnpy-ble ble`` - descriptor-aware NimBLE console for read/write IO,
  GAP/DIS/BAS metadata, notifications, and raw GATT service dumps.
* ``dawnpy-ble ots`` - Bluetooth SIG Object Transfer Service (OTS)
  client for the Dawn ``nimble_ots`` peripheral demo.

Use ``dawnpy-ble ble --help`` and ``dawnpy-ble ots --help`` for the
complete command help.

Common ``ble`` options:

* ``IDENTIFIER``: BLE address or platform-supported device identifier.
* ``--descriptor`` / ``-d``: descriptor file or directory containing
  ``descriptor.yaml``. Required for descriptor-aware console access.
* ``--scan``: scan nearby BLE devices and choose one interactively.
* ``--scan-timeout SECONDS``: BLE scan duration. Defaults to ``5.0``.
* ``--dump-services``: connect and dump the complete GATT service tree
  instead of opening the descriptor-aware console. Does not require a
  descriptor.
* ``--debug`` / ``--no-debug``: enable verbose protocol logging.

Console
=======

The ``ble`` command opens a descriptor-aware interactive console unless
``--dump-services`` is used. It shares the normalized command style
documented in :ref:`dawnpy-console-commands` (see :doc:`dawnpy-serial`)
and adds BLE service and notification commands.

Tests
=====

::

    cd tools/dawnpy-ble && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking

Features
========

OTS - Object Transfer
---------------------

The ``dawnpy-ble`` package ships a spec-compliant OTS v1.0 client. It
uses GATT for OACP/OLCP control points and metadata, and L2CAP CoC
(PSM ``0x0025``) for bulk data.

Linux is required for end-to-end transfers because raw
``AF_BLUETOOTH`` L2CAP sockets are used for the data channel. The host
must be configured so the Python interpreter can open those sockets
without elevated command invocation, for example by granting
``cap_net_raw,cap_net_admin``.

Subcommands
^^^^^^^^^^^

::

    dawnpy-ble ots scan  [--timeout 5]
    dawnpy-ble ots list  --name dawn-ots
    dawnpy-ble ots read  --name dawn-ots --object NAME [--offset 0] [--length N] [--out FILE]
    dawnpy-ble ots write --name dawn-ots --object NAME [--offset 0] --in FILE

Quick Start With The nrf52840-dk Demo
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Build and flash the OTS demo::

    dawnpy build build/ots boards/arm/nrf52/nrf52840-dk/configs/nimble_ots/

On the host, list objects on the advertising peripheral::

    dawnpy-ble ots list --name dawn-ots

Read the read-only file into a local file::

    dawnpy-ble ots read --name dawn-ots --object some_file_ro --out /tmp/fetched.txt

Truncate and replace the read-write file::

    echo "hello over OTS" > /tmp/payload.txt
    dawnpy-ble ots write --name dawn-ots --object some_file_rw --in /tmp/payload.txt

The ``write`` command reads back the Object Size after the transfer
completes and prints it as a confirmation::

    Done. Server reports object size = 16B (wrote 16B at offset 0).

Programmatic API
^^^^^^^^^^^^^^^^

::

    import asyncio

    from dawnpy_ble.services.ots import OtsClient

    async def main():
        ots = await OtsClient.from_name("dawn-ots")
        try:
            for obj in await ots.list_objects():
                print(obj.index, obj.name, obj.size_current, obj.access_str())
            data = await ots.read_object("some_file_ro")
            new_size = await ots.write_object("some_file_rw", b"hello\n")
            print("server now reports", new_size, "bytes")
        finally:
            await ots.disconnect()

    asyncio.run(main())

``OtsClient`` exposes the full OTS surface:

* ``read_feature`` / ``read_object_name`` / ``read_object_size`` /
  ``read_object_props`` - characteristic reads.
* ``olcp_first`` / ``olcp_last`` / ``olcp_next`` / ``olcp_previous`` /
  ``olcp_goto`` - OLCP procedures.
* ``oacp_read`` / ``oacp_write`` / ``oacp_abort`` - OACP procedures.
* ``open_l2cap`` / ``close_l2cap`` / ``transfer_read`` /
  ``transfer_write`` - raw L2CAP CoC plumbing for users who want to
  drive transfers manually.
* ``read_object`` / ``write_object`` - end-to-end helpers that handle
  OLCP selection, channel lifecycle, and OACP signalling.

Limitations
^^^^^^^^^^^

* One transfer at a time per connection. OTS allows interleaved
  transfers on a multi-channel server, but ``dawnpy-ble`` does not expose
  that.
* Bonded-only access enforcement is not implemented.
* L2CAP CoC bulk transfer is Linux-only. macOS and Windows users can
  drive the GATT control-point side via the same module, but need an
  external L2CAP CoC implementation for bulk data.
