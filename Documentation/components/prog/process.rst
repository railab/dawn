.. _process:

============
CProgProcess
============

Overview
========

``CProgProcess`` is a base class for Program objects that process data
samples from a source IO and publish the results to an output IO.

This class provides the infrastructure for callback-driven sample
processing. It automatically handles the binding to source and output IOs
and registers a notification callback. When the source IO provides new data,
the derived class's ``handle()`` method is called to perform the actual
processing.

The output object is producer-owned by the program path. During ``init()``,
``CProgProcess`` initializes deferred ``virt`` outputs when needed and
validates configured writable outputs against the resolved source shape.

A common pattern is to use the ``CProgProcessTemplate``, which allows
implementing the processing logic once and applying it to different
data types (like integers or floats) through a simple policy class.

Doxygen
=======

- `dawn::CProgProcess <../../doxygen/classdawn_1_1CProgProcess.html>`_

- `dawn::CProgProcessTemplate <../../doxygen/classdawn_1_1CProgProcessTemplate.html>`_
