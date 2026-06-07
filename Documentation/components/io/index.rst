.. _ioindex:

=======
Dawn IO
=======

``Dawn IO`` - an object type that can be read or written to system resources or
DAWN resources. IO objects can be accessed by Programs and Protocols.

Overview
========

- ``IO`` objects represent data sources/sinks in Dawn.
- Most IO classes are backed by OS resources (GPIO, ADC, file, timerfd, CAN,
  etc.).
- Some IO classes are framework-facing control points (for example ConfigIO,
  ControlIO, TriggerIO).
- Programs and protocols access IO through :cpp:class:`dawn::CIOHandler`.

Binding Model
=============

IO configuration is descriptor-driven:

1. IO objects are created from descriptor entries.
2. ``configure()`` parses IO-specific configuration items.
3. ``init()`` allocates runtime resources.
4. ``CIOHandler::bindObjects()`` resolves target bindings for special IOs
   (Config/Control/Trigger) after all handlers are initialized.

Lifecycle
=========

IO lifecycle is managed by :cpp:class:`dawn::CIOHandler`:

1. ``init()``: create IO objects from descriptor and attach factories.
2. ``initAll()``: run ``configure()`` and ``init()`` for each IO.
3. ``startAll()`` / ``stopAll()``: activate or stop IO runtime behavior.
   When ``CONFIG_DAWN_IO_NOTIFY`` is enabled, notifier thread lifecycle is
   handled here.
4. ``deinitAll()``: release IO-owned resources.

Data Access Model
=================

Dawn provides two interfaces to access IO data: fetch (pull) and notification
(push).

Fetch reads on request via :cpp:func:`dawn::CIOCommon::getData` in the caller
thread. It is the right fit for synchronous protocols and periodic polling.

Notification delivers data when the IO becomes readable, using
:cpp:class:`dawn::IIONotifier` callbacks registered through
:cpp:func:`dawn::CIOCommon::bindNotifier` and
:cpp:func:`dawn::CIOCommon::setNotifier`. Notification support is controlled by
``CONFIG_DAWN_IO_NOTIFY``; when enabled it is preferred for event-driven or
latency-sensitive flows.

Dawn supports multiple notification strategies:

- **Poll notifier** (``CIONotifier``): Multiplexes multiple IOs per thread
  using ``poll()``. IOs with the same priority share a thread. Default
  strategy.
- **Stream notifier** (``CStreamNotifier``): Dedicated thread per IO.
  Enabled by ``CONFIG_DAWN_IO_NOTIFY_STREAM``. Suited for high-frequency
  data paths requiring independent priority scheduling.

The ``CIONotifierManager`` routes each IO to the correct notifier based on
its ``IO_CFG_NOTIFY`` descriptor configuration (type, priority, batch count).
Both strategies use :cpp:func:`dawn::CIOCommon::getData` internally, so the
IO abstraction is preserved throughout.

See :doc:`notifier` for notifier details and YAML configuration.

See :doc:`iodata` for IO data container details.

Data Model
==========

Dawn IO data is a typed value model, not a protocol wire buffer.

- ``dtype`` defines the scalar element type.
- ``getDataSize()`` returns the byte size of one element.
- ``getDataDim()`` returns how many elements the IO exposes.
- Multi-dimensional IO is a vector/array of typed elements in declaration
  order.

Protocol code owns wire serialization and deserialization at the protocol
boundary. Endianness, register packing, bit packing, and any flattening of a
multi-dimensional IO are protocol responsibilities. External tools should
mirror the protocol wire rules, not define a separate Dawn data layout.

Seekable IO
===========

Seekable (offset-based) IO access is controlled by
``CONFIG_DAWN_IO_SEEKABLE``.

When enabled, seekable IO classes can use partial reads/writes via
``getData()/setData()`` with non-zero ``offset``.

When disabled, offset-based access is not available and returns ``-ENOTSUP``.

Timestamp
=========

Timestamp support requires ``CONFIG_DAWN_IO_TIMESTAMP``.

Timestamps are controlled **per IO instance** via the ``IO_FLAGS_TS`` flag
in the object descriptor. When an IO has timestamps enabled:

- The IO produces timestamp values (typically microseconds since boot).
- Data buffers created with ``TS=true`` (``io_sdata_t``) or ``ts=true``
  (``io_ddata_t``) store a per-batch timestamp alongside the data.
- ``ddata_alloc()`` automatically matches the IO's timestamp setting.

When timestamps are disabled for an IO (the default):

- Data buffers omit per-batch timestamp storage, saving 8 bytes per batch.
- Memory layout is contiguous data, suitable for zero-copy and burst reads.

For inputs, timestamp is the time associated with a given input sample.

For outputs, timestamp is the time when the output was last set.

See :doc:`iodata` for data buffer layout details.

IO Limits
=========

IO limits are common IO configuration items defined in
:cpp:class:`dawn::CIOCommon`:

- ``IO_CFG_LIMIT_MIN``
- ``IO_CFG_LIMIT_MAX``
- ``IO_CFG_LIMIT_STEP``

This feature is controlled by ``CONFIG_DAWN_IO_LIMITS``.

Each limit item stores a word array (``uint32_t`` words) with DTYPE metadata.
The three items must use the same DTYPE and size.

If limits are disabled or not configured, no range/step limit enforcement is
applied.

Limit checks are applied on write paths (``setData``/``setDataImpl``),
not on reads.

For scalar values, one element is checked.
For multi-dimensional payloads, limits are checked element-by-element.

``step == 0`` means "any value between min and max".

Limits are descriptor-defined runtime constraints. They complement, but do not
replace, hardware/static limits enforced by specific IO implementations.

Doxygen
=======

- `dawn::CIOCommon <../../doxygen/classdawn_1_1CIOCommon.html>`_

- `dawn::CIOLimits <../../doxygen/classdawn_1_1CIOLimits.html>`_

- `dawn::CIOHandler <../../doxygen/classdawn_1_1CIOHandler.html>`_

IO Helpers
==========

.. toctree::
   :maxdepth: 1

   iodata.rst
   notifier.rst
   timerfd.rst
   limits.rst

Supported IO
============

.. toctree::
   :maxdepth: 1

   dummy.rst
   dummy_notify.rst
   encoder.rst
   gpi.rst
   gpo.rst
   sensor.rst
   sensor_producer.rst
   battery.rst
   sysinfo.rst
   boardctl.rst
   uname.rst
   uuid.rst
   systime.rst
   virtual.rst
   leds.rst
   rgbled.rst
   buttons.rst
   timestamp.rst
   rand.rst
   adc.rst
   config.rst
   control.rst
   trigger.rst
   pwm.rst
   pulsecount.rst
   dac.rst
   descriptor.rst
   descselector.rst
   capabilities.rst
   fileio.rst
