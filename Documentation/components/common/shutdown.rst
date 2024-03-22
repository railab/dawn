.. _shutdown:

=============
Dawn Shutdown
=============

``CShutdown`` provides a global way to signal when the application should
stop running.

Overview
========

This class acts as a central coordinator for stopping the system gracefully.
Any part of the code can request a shutdown, and other components can poll
to see if such a request has been made.

This allows background threads and long-running tasks to detect when the
user or the system wants to exit, giving them a chance to clean up their
resources before the application terminates.

Doxygen
=======

- `dawn::CShutdown <../../doxygen/classdawn_1_1CShutdown.html>`_
