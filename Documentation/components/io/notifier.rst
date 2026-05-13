.. _notifier:

===========
IO Notifier
===========

Overview
========

Dawn's notification system delivers IO data asynchronously via callbacks
when new data is available. The architecture supports multiple notifier
strategies, each optimized for different use cases:

- **Poll notifier** (``CIONotifier``): Multiplexes multiple IOs per thread
  using ``poll()``. Suitable for slow-to-medium IOs (buttons, slow sensors).
- **Stream notifier** (``CStreamNotifier``): Dedicated thread per IO for
  high-frequency data streaming (ADC, serial). Requires
  ``CONFIG_DAWN_IO_NOTIFY_STREAM``.

All notifiers implement the ``IIONotifier`` interface and use
``getData()`` to read data, preserving the IO abstraction.

Notifier Manager
================

``CIONotifierManager`` routes each IO to the correct notifier instance
based on its configured notifier type and priority. It is owned by
``CIOHandler`` and handles the full lifecycle:

1. **Registration** (``regIO()``): During initialization, each notifiable
   IO is routed to a notifier instance. Poll notifiers are shared by IOs
   with the same priority. Stream notifiers are always created 1:1 per IO.
2. **Start/Stop**: Starts and stops all notifier threads.

Poll Notifier
=============

``CIONotifier`` maintains a ``pollfd`` array that is dynamically updated
whenever a new callback is registered. The worker thread performs a blocking
``poll()`` call; when an event occurs, it:

1. Identifies which IO object is ready.
2. Reads data via ``getData()`` with the configured batch count.
3. Iterates through all registered callbacks for that IO and executes them.

Multiple IOs with the same priority share a single poll notifier thread,
reducing thread count for systems with many slow IOs.

Stream Notifier
===============

``CStreamNotifier`` provides a dedicated worker thread for a single IO
object. It uses ``poll()`` on a single file descriptor with periodic
timeout for clean thread shutdown.

The stream notifier is designed for high-frequency data paths where:

- Each IO needs its own thread to avoid contention.
- Independent thread priority per IO is required.
- The simpler code path (single fd, no array management) reduces overhead.

Priority-Based Threading
========================

The ``IO_CFG_NOTIFY`` descriptor item configures notifier type, thread
priority, and batch count per IO:

- **Type**: ``poll`` (default) or ``stream``.
- **Priority**: OS worker-thread priority. Poll notifiers with the same
  priority share one thread and therefore one effective scheduler priority.
  Stream notifiers always get their own thread.
- **Batch**: Number of samples to read per event (default 1). The notifier
  calls ``ddata_alloc(batch)`` and ``getData(data, batch)`` with this count.

YAML Configuration
==================

Notifier settings are configured in the IO's ``notify`` config block:

.. code-block:: yaml

   ios:
     - id: adc_fast
       type: adc_stream
       config:
         device: 0
         batch_size: 128
         notify:
           type: stream
           priority: 255
           batch: 128

     - id: button_user
       type: gpi
       config:
         devno: 7
         notify:
           type: poll
           priority: 100

When ``notify`` is omitted, the IO uses the poll notifier with default
priority (0 = inherited/default worker priority) and batch count (1).

The descriptor does not currently expose notifier stack size or scheduler
policy. Those remain internal class-level thread settings.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_NOTIFY``: Enables the notification system, poll notifier,
  and ``CIONotifierManager``.
- ``CONFIG_DAWN_IO_NOTIFY_STREAM``: Enables ``CStreamNotifier`` (depends on
  ``CONFIG_DAWN_IO_NOTIFY``, default disabled).

Usage
-----

To support notifications, an IO class must:

1. Return ``true`` from ``isNotify()``.
2. Provide a valid file descriptor via ``getFd()``.

Notifications are registered internally by the framework when a Program or
Protocol is bound to a notifiable IO.

Doxygen
=======

- `dawn::IIONotifier <../../doxygen/classdawn_1_1IIONotifier.html>`_

- `dawn::CIONotifier <../../doxygen/classdawn_1_1CIONotifier.html>`_

- `dawn::CStreamNotifier <../../doxygen/classdawn_1_1CStreamNotifier.html>`_

- `dawn::CIONotifierManager <../../doxygen/classdawn_1_1CIONotifierManager.html>`_
