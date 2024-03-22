.. _thread:

===============
Threaded Object
===============

``CThreadedObject`` provides a simple wrapper to manage background tasks
within a component.

Overview
========

This class is used by components that need to run a background worker thread.
It handles the creation and destruction of an internal thread and provides
atomic flags to help the worker know when to stop.

A typical worker function will loop until the ``thQuit`` flag becomes true.
Calling ``threadStop()`` will set this flag and wait for the thread to
finish its work and exit safely.

Doxygen
=======

- `dawn::CThreadedObject <../../doxygen/classdawn_1_1CThreadedObject.html>`_
