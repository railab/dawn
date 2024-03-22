.. _simplebase:

====================
Simple Protocol Base
====================

Overview
========

``CProtoSimpleBase`` is a base class that provides a common framework for
framed communication protocols such as Serial, UDP, and IPC.

This class handles the repetitive tasks required for protocols that exchange
data in a standard frame format. It includes a built-in handler for common
commands like reading or writing IO objects, managing configuration, and
handling object subscriptions.

Protocol implementations that inherit from this base class don't need to
reinvent the frame parsing or command logic; they only need to implement the
low-level code to send and receive raw data over their specific transport
(like a serial port or a network socket).

Doxygen
=======

- `dawn::CProtoSimpleBase <../../doxygen/classdawn_1_1CProtoSimpleBase.html>`_
