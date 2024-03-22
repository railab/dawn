.. _poll_loop:

================
Common Poll Loop
================

``CPollLoopRunner`` provides a shared loop for poll-based worker threads.

Overview
========

Several Dawn components use a worker thread that waits on ``poll()`` and then
handles ready file descriptors.

The shared poll loop centralizes this pattern so components do not duplicate
their own ``do/while`` + ``poll()`` control flow.

Timeout and Quit Handling
=========================

Poll timeout uses ``DAWN_POLL_TIMEOUT_MS`` from
``dawn/include/dawn/common/poll_loop.hxx``.

The timeout is intentionally bounded. Without periodic wakeups, a blocking
``poll()`` can prevent a worker from observing ``shouldQuit()`` in time, which
can block ``threadStop()`` while waiting for thread join.

Callback Model
==============

``CPollLoopRunner`` accepts ``SPollLoopCallbacks`` with three optional hooks:

- ``beforePoll``:
  called before each ``poll()`` call. Use for fd-array refresh or revents
  cleanup.
- ``afterPoll``:
  called after each ``poll()`` return. Use for poll-level error handling.
- ``onPollReady``:
  called only when ``poll()`` reports ready descriptors (``ret > 0``).
  Use for actual read/dispatch handling.

Each callback receives a user private pointer so component-specific runtime
state can be passed without global/static storage.

Doxygen
=======

- `dawn::SPollLoopCallbacks <../../doxygen/structdawn_1_1SPollLoopCallbacks.html>`_

- `dawn::CPollLoopRunner <../../doxygen/classdawn_1_1CPollLoopRunner.html>`_
