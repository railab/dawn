============
Applications
============

``dawn/apps`` directory includes example Dawn applications. They can be used as
an out of the box solution, or they can be a guide for the user to implement their
own custom application.

dawn
====

The default dawn application uses dynamic allocation to create objects.

Not the best solution for small systems or applications where we want to avoid
dynamic allocation.

Customizing the default app
---------------------------

The default app is extended without forking ``dawn_main.cxx`` via the
weak-symbol hooks declared in ``<dawn/oot.hxx>``. Out-of-tree projects
provide strong overrides in ``<oot>/external/dawn_oot_hooks.cxx``,
which the build attaches to ``apps_dawn`` through
``dawn_oot_user_setup_app()``. Two groups of hooks are available:

User factories
^^^^^^^^^^^^^^

``user_io_factory`` / ``user_prog_factory`` / ``user_proto_factory`` let
the project supply custom IO / PROG / PROTO classes. ``apps_dawn`` calls
all three at startup and passes the results into ``CDawn``. The handler
consults the user factory first, then falls back to the built-in factory,
so a user factory only needs to handle the class IDs it owns. Reuse a
built-in class ID to override the default implementation, or pick an ID
in the user-reserved range (``IO_CLASS_USER`` 500,
``PROG_CLASS_USER`` / ``PROTO_CLASS_USER`` 511) to add a brand-new type.
See :doc:`../customization` and :doc:`../customization/oot_api` for the full recipe.

App lifecycle hooks
^^^^^^^^^^^^^^^^^^^

``user_init``, ``user_post_load``, ``user_on_idle`` and
``user_pre_shutdown`` run custom code at startup, after descriptor load,
on each main-loop tick, and before teardown. See
:ref:`App lifecycle hooks <app-lifecycle-hooks>` in the customization
guide for signatures and failure semantics.

Descriptor Source Modes
-----------------------

``dawn`` supports two descriptor source approaches selected by Kconfig.

YAML descriptor (recommended)
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

Use YAML as source of truth and generate C++ descriptor during build.

Required Kconfig options:

- ``CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_YAML=y``
- ``CONFIG_DAWN_APPS_EXAMPLE_DESC_YAML_PATH=".../descriptor.yaml"``

Build flow:

1. CMake runs dawnpy descriptor generator.
2. Generated file is written to build directory as
   ``generated_descriptor.cxx``.
3. ``dawn_descriptor.cxx`` includes generated file in compilation.

Legacy C++ descriptor
^^^^^^^^^^^^^^^^^^^^^

Use pre-generated static C++ descriptor file.

Required Kconfig options:

- ``CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_CXX=y``
- ``CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH=".../descriptor.cxx"``

This mode is still supported for compatibility, but YAML mode is the
recommended default for board configurations.

dawn_static
===========

An idea for an application where objects are created statically.
Not implemented yet, just an idea for the future.
