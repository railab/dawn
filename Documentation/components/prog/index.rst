.. _programindex:

=========
Dawn Prog
=========

``Dawn Prog`` - edge processing objects that transform IO data.

Programs read source IOs, process data, and publish results through bound IO
objects. Most data-transform programs use the generic ``CIOCommon`` contract:
readable sources, writable outputs, and optional notifications.

Overview
========

In Dawn architecture, ``PROG`` objects sit between data producers and
consumers:

- Source side: read one or more bound IOs.
- Compute side: run a focused algorithm per program class
  (stats/filter/threshold/routing/buffer/sequencer).
- Sink side: publish results to writable output IOs. Some programs also own
  software register surfaces implemented with Virtual IO.

Programs are designed as small reusable processing units that can be composed
into larger pipelines.

Binding Model
=============

Program-to-object association is descriptor-driven:

1. IO objects are declared in the descriptor.
2. Program configuration defines source and destination object IDs.
3. ``CProgHandler::initAll()`` performs ``configure()`` and bind resolution.
4. Program ``init()`` allocates runtime buffers/state after binds are known.

Visibility is explicit: programs can access only objects referenced by their
bind configuration.

Lifecycle
=========

Program lifecycle is managed by :cpp:class:`dawn::CProgHandler`:

1. ``init()``:
   create program objects from descriptor and store handler dependencies.
2. ``initAll()``:
   run ``configure()`` for every program, resolve binds, then call ``init()``.
3. ``startAll()`` / ``stopAll()``:
   activate/deactivate runtime behavior for each program.
4. ``deinitAll()``:
   release program-owned resources.

Execution Model
===============

Program behavior depends on how bound IOs are accessed:

1. Notify-driven:
   source IO notifies when new data arrive; program processes immediately.
2. Fetch-driven:
   data are read periodically, on start, or when a program-owned virtual
   surface is read, depending on the program contract.
3. Hybrid:
   notify path refreshes internal cache; fetch path serves cached data.

The selected model is defined per program class and descriptor bind setup.

IO Binding
==========

Program chaining is implemented by binding downstream program inputs to
upstream program outputs. The output may be any compatible writable IO object.
``type: virt`` remains useful for software-owned pipeline values and protocol
surfaces, while physical or application-specific IOs can be used directly when
their capabilities match the program contract.

Some programs intentionally remain virtual-specific:

- ``buffer`` owns selected-sample, selector, and status register surfaces.
- ``gateway`` mirrors get/set callbacks between pairs of Virtual IO endpoints.

Doxygen
=======

- `dawn::CProgCommon <../../doxygen/classdawn_1_1CProgCommon.html>`_

- `dawn::CProgHandler <../../doxygen/classdawn_1_1CProgHandler.html>`_

Program Helpers
===============

.. toctree::
   :maxdepth: 1
              
   process.rst

Supported Programs
==================

.. toctree::
   :maxdepth: 1

   stats_min.rst
   stats_max.rst
   stats_avg.rst
   stats_sum.rst
   stats_count.rst
   stats_rms.rst
   dummy.rst
   adjust.rst
   sampling.rst
   gateway.rst
   latest.rst
   redirect.rst
   sequencer.rst
   buffer.rst
   moving_avg.rst
   iir_filter.rst
   threshold.rst
   threshold_value.rst
   bitsplit.rst
   toggle.rst
   counter.rst
   switch.rst
   expression.rst
   selector.rst
   bitpack.rst
   vecpack.rst
   vecsplit.rst
   manytoone.rst
   onetomany.rst
   iomux.rst
   iodemux.rst
