======
Latest
======

**Component Type:** Program

**Status:** Implemented

Overview
========

``CProgLatest`` caches the latest sample from a notify-capable source IO
into an output IO. This lets fetch/poll-oriented protocols read the most
recent value without implementing notify handling.

- notify source -> cached output
- latest sample only (intermediate samples are not preserved)
- useful as the reverse adaptation of ``CProgSampling``

Implementation
==============

``CProgLatest`` runs in the notify/callback processing path and overwrites the
cached output with each new source sample.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_LATEST``: enables the Latest program.

YAML
----

.. code-block:: yaml

   programs:
     - id: latest1
       type: latest
       config:
         iobind:
           - src1
           - virt1

Uses a packed ``IOBIND`` list of source/output pairs (same pattern as other
process-style programs):

- source IO (notify-capable)
- destination output IO (cached latest sample)

One instance can cache multiple channels. ``1 -> 1`` is just one pair in the
configuration.

External Control
================

ControlIO: supported.

``CProgLatest`` supports runtime start/stop control through ``CIOControl``.
When stopped, incoming notifications are ignored and cached values are frozen.
When started again, cache updates resume.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CProgLatest <../../doxygen/classdawn_1_1CProgLatest.html>`_
