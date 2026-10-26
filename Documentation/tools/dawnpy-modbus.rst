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
``pymodbus >= 3.13, < 4``.

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

Dawn Modbus register addresses are used exactly as declared in the
descriptor register map; the console does not apply 1-based Modbus offset
conversion. Seekable IOs use NxModbus's standard FC20/FC21 file records,
following :ref:`modbus-seekable-files`.

``r [node] <objid>`` retains seekable window reads and register-list output.
The client advances a local byte cursor after each successful read. ``w``
retains scalar writes at the current cursor, without advancing it. The
console preserves the existing little-endian interpretation of byte pairs
in seekable register lists. On the wire, file records carry the IO bytes
unchanged.

``dawnpy_modbus.files.SeekableFile`` provides explicit byte-offset access.
Its arguments are a client implementing ``read_file_records`` and
``write_file_records``, the unit ID, the first file number (binding address
plus one), and the reserved file count (window width plus one). It splits
requests at PDU and file boundaries::

    from dawnpy_modbus.files import SeekableFile

    file = SeekableFile(client, unit=1, number=3601, count=257)
    file.write(0, b"configuration bytes!")
    assert file.read(0, 20) == b"configuration bytes!"

Reads accept byte offsets and lengths, including odd values, and return
zero padding beyond the current IO end. Writes require even offsets and
lengths because FC21 carries complete two-byte records. They retain the
backing IO's offset-write behavior, including replacement at offset zero
for FileIO and descriptor slots. File size and upload completion are not
part of these standard operations.

Keep existing ``type: seekable`` descriptors, addresses, and window widths.
Update Modbus firmware and clients together to use FC20/FC21 on the wire.

The CLI mirrors the Modbus wire contract documented by Dawn's Modbus
protocols: it does not define the data layout itself, it only decodes and
encodes values using the protocol's register and byte-order rules.

For single-binding holding/input groups whose runtime width is larger than
what the descriptor alone can prove (for example board ADCs whose channel
count is selected by the target), the client probes for the full register
width after an exception-4 short read and then decodes the payload as a
vector of dtype-sized elements.

Tests
=====

::

    cd tools/dawnpy-modbus && tox
    tox -e py        # tests + coverage
    tox -e format    # formatting check
    tox -e flake8    # linting
    tox -e type      # type checking
