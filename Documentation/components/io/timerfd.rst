.. _timerfd:

=======
Timerfd
=======

Overview
========

``CIOTimerfd`` is a helper class that provides periodic notifications for IO
objects that need to be updated at a fixed interval.

This component uses the system's timerfd mechanism to generate event-driven
notifications. It is typically used as a base class for other IO objects
(like sensors) that don't have their own hardware interrupt but still
need to provide data periodically.

Implementation
==============

It allows setting an interval in microseconds and provides a file
descriptor that can be used with the standard Dawn notification system.

Doxygen
=======

- `dawn::CIOTimerfd <../../doxygen/classdawn_1_1CIOTimerfd.html>`_
