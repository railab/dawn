=======
File IO
=======

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIOFile`` provides read/write access to files on the device file system
through the standard Dawn IO abstraction. This lets protocols perform large
data transfers (firmware images, authorization data, binary databases)
through a uniform interface without needing protocol-specific file handling
logic.

Implementation
==============

Security
--------

File access is restricted to three allowed path prefixes:

- ``/data/`` - persistent storage
- ``/tmp/`` - temporary storage
- ``CONFIG_DAWN_IO_FILE_DEV_PREFIX`` (default ``/dev/eeprom``) - device
  nodes only; the opened path must be a character or block device, anything
  else under the prefix is rejected with ``-EACCES``. An empty prefix
  disables device-node access.

Any path that does not start with one of these prefixes is rejected with
``-EACCES`` at ``init()`` time, before the file is opened. Paths containing
the ``..`` sequence are also rejected unconditionally to prevent directory
traversal attacks.

Device nodes have a fixed size (probed with ``SEEK_END``) that writes never
change, and they are never truncated.

The file path is **read-only configuration** embedded in the device descriptor.
It cannot be changed at runtime, eliminating the attack surface of a descriptor
that redirects IO to sensitive system files.

Permissions
-----------

The permission mode is configured as a descriptor config item:

.. list-table::
   :header-rows: 1

   * - Enum value
     - Meaning
   * - ``IO_FILE_PERM_READ`` (0)
     - File can only be read
   * - ``IO_FILE_PERM_WRITE`` (1)
     - File can only be written; created if it does not exist
   * - ``IO_FILE_PERM_RW`` (2)
     - File can be read and written; created if it does not exist
   * - ``IO_FILE_PERM_WRITE_ONCE`` (3)
     - File can be written exactly once; any later write returns
       ``-EPERM``. A non-empty regular file is locked at ``init()``; a
       device node cannot tell whether it was written before, so
       write-once is enforced per session only

``isRead()`` and ``isWrite()`` return based on the configured permission so
that protocols can query capability before performing I/O.

Data Model
----------

``CIOFile`` uses ``DTYPE_BLOCK`` with 1-byte data units and is **seekable**
(``isSeekable()`` returns ``true``).

- ``getDataSize()`` - returns the tracked file size in bytes. It is
  initialized from ``fstat()`` when the file is opened (zero for a
  write-only regular file) and updated after successful writes to regular
  files.
- ``getDataDim()`` - same as ``getDataSize()`` (1-byte units).
- ``getDataImpl()`` / ``setDataImpl()`` - whole-file access (seek to offset 0
  then read/write the entire file).
- ``getDataAtImpl()`` / ``setDataAtImpl()`` - partial access at a byte offset
  (``lseek`` + read/write), used by protocols for chunked file transfer.

Typical Use Cases
-----------------

Descriptor access
  Mount the device descriptor binary as a read-only file IO so that a protocol
  can download the full descriptor in chunks.

Firmware update
  A write-only file IO pointing to an update staging file allows a firmware
  update protocol to stream the image in chunks without special-casing file
  handling.

Authorization data
  Write-only IO for a credentials or token file, written once during
  provisioning.

Database / buffer
  Read-write IO pointing to a persistent data file that multiple protocols
  can read from or write to.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_FILE``: enables file-system IO support.
  Depends on ``CONFIG_DAWN_IO_SEEKABLE``.
- ``CONFIG_DAWN_IO_FILE_DEV_PREFIX``: device-node path prefix additionally
  allowed (default ``/dev/eeprom``); empty disables it.

YAML
----

.. code-block:: yaml

   ios:
     - id: file1
       type: fileio
       config:
         path: "/tmp/mydata.bin"
         perm: 0

Supported ``perm`` values:

- ``0``: read
- ``1``: write
- ``2``: read-write
- ``3``: write-once

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIOFile <../../doxygen/classdawn_1_1CIOFile.html>`_
