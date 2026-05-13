.. _thread:

===============
Threaded Object
===============

``CThreadedObject`` is Dawn's common worker-thread owner.
It wraps POSIX pthreads and provides the worker lifecycle used across the
framework.

Overview
========

Components use ``CThreadedObject`` to start, stop, and monitor background
work.
The class keeps the existing Dawn lifecycle model:

- ``setThreadFunc()`` stores the worker entry callback.
- ``threadStart()`` creates the worker thread.
- ``threadStop()`` requests shutdown and joins the worker.
- ``shouldQuit()`` lets the worker observe stop requests.
- ``markThreadFinished()`` marks natural thread exit.

Default worker configuration:

- stack size from the OS,
- scheduler policy inherited from the creating thread,
- priority inherited from the creating thread.

Per-thread Configuration
========================

Each thread owner can optionally configure its worker before calling
``threadStart()``.
Configuration is stored in ``CThreadedObject::SThreadConfig``.

Supported fields:

- ``stackSize``: stack size in bytes, ``0`` keeps the OS default.
- ``priority``: worker priority, ``0`` keeps the creating thread default.
- ``scheduler``: POSIX scheduler policy, ``-1`` keeps the creating thread
  policy.

Helpers are available for individual fields:

- ``setThreadStackSize()``
- ``setThreadPriority()``
- ``setThreadScheduler()``
- ``setThreadConfig()``

Example
=======

.. code-block:: c++

   class CMyWorker
   {
   public:
     CMyWorker()
     {
       threadCtl.setThreadStackSize(8192);
       threadCtl.setThreadPriority(120);
     }

     int start()
     {
       threadCtl.setThreadFunc([this]() { thread(); });
       return threadCtl.threadStart();
     }

   private:
     void thread();
     dawn::CThreadedObject threadCtl;
   };

Notes
=====

- Invalid scheduler or priority values are rejected at ``threadStart()``.
- When scheduler or priority is configured, Dawn creates the pthread with
  explicit scheduling attributes.
- Poll-based helpers such as ``CPollLoopRunner`` use bounded timeouts so
  ``threadStop()`` can join cleanly.

Doxygen
=======

- `dawn::CThreadedObject <../../doxygen/classdawn_1_1CThreadedObject.html>`_
