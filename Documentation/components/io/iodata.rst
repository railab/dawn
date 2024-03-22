.. _iodata:

=======
IO Data
=======

``IOData`` is the common container used by IO read/write paths
(:cpp:func:`dawn::CIOCommon::getData`, :cpp:func:`dawn::CIOCommon::setData`)
and notifier callbacks.

Overview
========

- Common base interface: :cpp:class:`dawn::IODataCmn`
  provides item count, data size, per-batch pointers, and timestamp access.
- Timestamps are **per-instance**: each buffer is created with or without
  per-batch timestamp storage. When timestamps are disabled, batches contain
  only data elements, enabling contiguous memory layout.
- Static variant: :cpp:class:`dawn::io_sdata_t` (template
  ``io_sdata_t<T, N, M, TS>``) keeps storage in the object itself
  (compile-time size, no heap allocation). The ``TS`` parameter (default
  ``false``) controls whether per-batch timestamps are included.
- Dynamic variant: :cpp:class:`dawn::io_ddata_t` allocates storage on heap
  (runtime-configurable type size, items, batch count, and timestamp flag).
- View variant: :cpp:class:`dawn::io_data_view_t` wraps caller-owned storage
  without allocating or copying. It is useful for temporary protocol buffers
  and other synchronous adapters into the IO API.
- Timestamp type: :cpp:class:`dawn::io_ts_t` (``uint64_t``).

Timestamp Control
=================

Timestamp storage is controlled per-instance, not globally. Each data buffer
is created with or without per-batch timestamps:

- **``io_sdata_t<T, N, M, TS>``**: the ``TS`` template parameter (default
  ``false``) selects between two memory layouts at compile time.
- **``io_ddata_t(t, n, m, dt, ts)``**: the ``ts`` constructor parameter
  (default ``false``) selects the layout at runtime.

When ``TS``/``ts`` is ``false``:

- Batch layout: ``[data[0] .. data[N-1]]`` (no timestamp overhead).
- ``getPtr()`` and ``getDataPtr()`` return the same address.
- ``getTs()`` returns a shared dummy reference (writes are discarded).
- ``hasTimestamp()`` returns ``false``.
- For multi-batch buffers (``M > 1``), all batches are stored contiguously
  in memory. This layout enables efficient burst reads where a single
  ``read(fd, buffer, M * batch_size)`` call fills the entire buffer without
  per-sample overhead.

When ``TS``/``ts`` is ``true``:

- Batch layout: ``[timestamp | data[0] .. data[N-1]]``.
- ``getPtr()`` returns the start of the batch (timestamp + data).
- ``getDataPtr()`` skips the timestamp and returns the data start.
- ``getTs()`` returns the per-batch timestamp reference.
- ``hasTimestamp()`` returns ``true``.

IO drivers that interact with kernel structs (e.g., NuttX sensor drivers)
always receive ``[timestamp | data]`` from the kernel. The driver reads
into a temporary buffer and copies only the data portion into the user
buffer via ``getDataPtr()``. If ``isTimestamp()`` is enabled for the IO
instance, the kernel timestamp is also extracted into ``getTs()``.

Typical use
===========

- Use :cpp:class:`dawn::io_sdata_t` when shape is fixed and known at compile
  time.
- Use :cpp:class:`dawn::io_ddata_t` when shape comes from descriptor-provided
  runtime configuration.
- Use :cpp:class:`dawn::io_data_view_t` when an existing buffer should be
  passed to ``getData()`` or ``setData()`` without transferring ownership.
- Access payload bytes via ``getDataPtr()`` and batch timestamp via ``getTs()``.
- Use ``hasTimestamp()`` to check whether per-batch timestamps are stored.

Non-Owning Views
================

``io_data_view_t`` implements ``IODataCmn`` over a caller-provided buffer.
The view object and the backing buffer may live on the stack, heap, or in
static storage; ``io_data_view_t`` does not own or free that memory.

``getPtr()`` and ``getDataPtr()`` both return the viewed buffer.
``hasTimestamp()`` is always ``false`` and ``getTs()`` returns a dummy
timestamp reference.

The caller must ensure that the backing buffer remains valid for the whole
``getData()`` or ``setData()`` call. IO implementations must not retain the
view object or its returned pointer after the call returns.

Doxygen
=======

- `dawn::IODataCmn <../../doxygen/structdawn_1_1IODataCmn.html>`_

- `dawn::io_data_s <../../doxygen/structdawn_1_1io_data_s.html>`_

- `dawn::io_data_nots_s <../../doxygen/structdawn_1_1io_data_nots_s.html>`_

- `dawn::io_ddata_t <../../doxygen/structdawn_1_1io_ddata_t.html>`_

- `dawn::io_data_view_t <../../doxygen/structdawn_1_1io__data__view__t.html>`_

- `dawn::io_sdata_t <../../doxygen/structdawn_1_1io_sdata_t.html>`_
