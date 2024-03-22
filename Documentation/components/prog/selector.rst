========
Selector
========

**Component Type:** Program

**Status:** Implemented

Overview
========

CProgSelector routes one of N data inputs to a target IO based
on the value of a control input. When the control changes **or** when
the currently selected data input changes, the program copies that
data input's value to the target.

Indexing is zero-based; out-of-range control values are ignored.

Implementation
==============

- Standalone program (CProgCommon-based), no thread.
- Registers notification callbacks on the control input and on all
  data inputs that support notifications.
- ``applyRoute()`` reads the selected data input via ``getData()`` and
  writes the value to the target via ``setData()``.
- The iodata buffer is sized to match the target IO's ``getDataSize()``.
- If the target is a deferred ``virt`` output, ``selector`` initializes or
  reuses it because ``selector`` defines that output-side fixed shape from its
  data contract.
- Control and data inputs are input-only; ``selector`` does not initialize
  them and expects them to be provided by upstream producers.

Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROG_SELECTOR``: enables the program.

YAML
----

.. code-block:: yaml

   programs:
     - id: selector_leds
       type: selector
       config:
         control: [mode]
         data: [seq0, seq1, seq2, seq3]
         target: [leds0]

Doxygen
=======

- `dawn::CProgSelector <../../doxygen/classdawn_1_1CProgSelector.html>`_
