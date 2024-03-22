.. _dummy:

========
Dummy IO
========

**Component Type:** Input/Output

**Status:** Implemented

Overview
========

``CIODummy`` is a test IO object used for development and validation
workflows.

Implementation
==============

``CIODummy`` is a memory-backed in-process IO useful for tests and pipeline
prototyping. It allocates a value buffer sized by ``IO_DUMMY_CFG_DIM`` (data
dimension) and an optional ``IO_DUMMY_CFG_INITVAL`` (initial value array
applied during ``init()``). Reads return the stored buffer; writes overwrite
it under a mutex and update the timestamp when
``CONFIG_DAWN_IO_TIMESTAMP`` is enabled. Reads support batch (the same
buffer is replicated across requested samples).

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_IO_DUMMY``: enables the dummy IO implementation.

YAML
----

.. code-block:: yaml

   ios:
     - id: dummy1
       type: dummy
       dtype: uint32
       rw: true
       config:
         init_value: 123

External Control
================

ControlIO: not supported.

TriggerIO: not supported.

Doxygen
=======

- `dawn::CIODummy <../../doxygen/classdawn_1_1CIODummy.html>`_
