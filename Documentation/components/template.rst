===============================
Standardized Component Template
===============================

Every IO, Program, and Protocol component page must use the section order
below. Keep the content concise and wrap lines at 80 characters when possible.

Status Labels
=============

Every component page must state its implementation status on line 2 or 3
(after the title), using exactly one of the labels below:

- ``**Status:** Stable`` — mature, tested, no known issues
- ``**Status:** Implemented`` — functional; may have known limitations
- ``**Status:** In Progress`` — under active development, partial coverage
- ``**Status:** Planned`` — designed or discussed but not yet coded

Required Section Order
======================

Overview
--------

Provide a functional description of the component. Explain what it does, when
to use it, and any relevant behavior visible to descriptor authors.

Implementation
--------------

Document implementation details that matter to maintainers. Describe the main
runtime flow, constraints, and notable interactions with handlers, bindings, or
external devices.

Configuration
-------------

Show descriptor examples in the same YAML shape accepted by ``dawnpy``.
Default examples should omit ``instance`` unless a fixed instance is part of
the documented behavior.

Kconfig
^^^^^^^

List any relevant build-time options here. If the component has no dedicated
Kconfig switches worth documenting, say so explicitly.

YAML
^^^^

Show the runtime descriptor example here.


.. code-block:: yaml

   ios:
     - id: my_io
       type: dummy
       dtype: uint32
       rw: true
       config:
         init_value: 123


.. code-block:: yaml

   programs:
     - id: my_prog
       type: stats_min
       config:
         inputs:
           - my_input
         outputs:
           - my_output


.. code-block:: yaml

   protocols:
     - id: my_proto
       type: shell
       config:
         bindings:
           - my_io
         prompt: "dawn> "

External Control
----------------

This section is optional. Use it to list supported object-to-object control or
trigger relationships such as ``ControlIO`` and ``TriggerIO``. When this
section is present, list each relevant interface explicitly and mark whether it
is supported.

Example
^^^^^^^

- ``ControlIO``: supported
- ``TriggerIO``: not supported

Brainstorming & Future Ideas
----------------------------

This section is optional. Use it for design notes, follow-up ideas, or known
limitations that should not be presented as current behavior. Keep it
forward-looking and avoid repeating current implementation facts.

Doxygen
-------

End every component page with links to the generated Doxygen pages.

.. code-block:: rst

   - `API Reference Index <../../api/index.html>`_
   - `dawn::MyComponent <../../doxygen/classdawn_1_1MyComponent.html>`_
