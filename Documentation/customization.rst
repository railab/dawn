=============
Customization
=============

Dawn can be extended without forking the upstream tree. An out-of-tree
project lives under a separate root and the build system pulls in its
boards, classes, factories and apps. User factories take priority over
the built-in ones, so the same mechanism that adds new types can also
**override** default upstream objects.

A worked example is at ``examples/out-of-tree-demo/``.

For a clean starting point outside the upstream repository, run
``python -m dawnpy project new <path>``. The scaffold includes a local
``.dawnrc`` pointing at the upstream Dawn checkout, a minimal simulator
board overlay, an example descriptor, and the conventional
``external/dawn_oot.cmake`` hook.

What can be extended
====================

Boards
------

Any NuttX board layout under ``<oot>/boards/...``. OOT boards source
``$(DAWN_BOARDS_COMMON)/Kconfig`` for the fake-driver options
(see :doc:`components/fake_devices`).

Configurations
--------------

``defconfig`` + ``defconfig.dawn`` pairs in any
``boards/<...>/configs/<name>/``.

Descriptors
-----------

Hand-written C++ referenced by ``CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH``.
Resolves relative to ``$DAWN_OOT_ROOT``.

IO / PROG / PROTO classes
-------------------------

Subclass ``CIOCommon`` / ``CProgCommon`` / ``CProtoCommon`` and route
through a user factory (``IIOFactory`` / ``IProgFactory`` /
``IProtoFactory``, one per kind). The handler consults the user factory
first, then falls back to the built-in factory. Pick a class ID at or
above the user-reserved range (``IO_CLASS_USER`` 500,
``PROG_CLASS_USER`` / ``PROTO_CLASS_USER`` 511) for new types, or reuse
a built-in class ID to override the default implementation. Sources go
under ``<oot>/external/src/{io,prog,proto}/``; register factories by
defining strong overrides of ``dawn::oot::user_*_factory()`` in
``<oot>/external/dawn_oot_hooks.cxx`` (declared in
``<dawn/oot.hxx>``).

External apps
-------------

Extra NuttX apps under ``<oot>/external/apps/<name>/`` using
``nuttx_add_application(...)``. They build alongside Dawn's apps and
surface as regular NSH builtins. Disabling ``CONFIG_DAWN_APPS_DAWN`` and
constructing ``CDawn`` from one of them also lets the project fully
replace the default ``apps_dawn`` lifecycle.

Public include path
-------------------

``<oot>/external/include/`` is auto-added to dawn's PUBLIC include path.

Reference layout
================

Only ``boards/`` is required for dawnpy to detect an OOT project root.
``external/`` remains optional and is needed only when the project adds
OOT classes, apps, or factory overrides. The rest is convention. ::

    my-dawn-project/
    ├── boards/
    │   └── <arch>/<chip>/<board>/
    │       ├── CMakeLists.txt
    │       ├── Kconfig                         # sources DAWN_BOARDS_COMMON
    │       ├── src/                            # board bringup C
    │       └── configs/<defconfig>/
    │           ├── defconfig                   # NuttX defconfig
    │           └── defconfig.dawn              # Dawn-specific overrides
    ├── external/
    │   ├── dawn_oot_hooks.cxx              # user factory registration
    │   ├── apps/                               # optional: user NuttX apps
    │   │   ├── Kconfig                         #   sourced by dawn/apps/Kconfig
    │   │   ├── CMakeLists.txt                  #   sourced by dawn_oot_link_apps()
    │   │   └── <appname>/
    │   │       ├── Kconfig
    │   │       ├── CMakeLists.txt              #   nuttx_add_application(NAME ...)
    │   │       └── <appname>_main.c
    │   └── src/
    │       ├── io/                             # custom IO classes + factory
    │       ├── prog/                           # custom PROG classes + factory
    │       └── proto/                          # custom PROTO classes + factory
    └── descriptors/
        └── demo.cxx                            # hand-written C++ descriptor

If the project is built outside the upstream Dawn checkout, add a
``.dawnrc`` at the project root so ``dawnpy`` can persist the upstream
path and optional relocated NuttX trees::

    [paths]
    dawn_root = "../dawn-src"
    nuttx_dir = "../vendor/nuttx"
    nuttx_apps_dir = "../vendor/apps"

    [project]
    oot = true
    types_from = "dawnpy_types.py"

The build seam (DAWN_OOT_ROOT)
==============================

When dawnpy detects that the requested defconfig lives in an OOT project,
it exports ``DAWN_OOT_ROOT=<oot project root>`` into the cmake/Kconfig
environment. Three consumers act on it:

1. ``dawn_oot_link()`` (from ``dawn/cmake/dawn_oot.cmake``, called by
   ``dawn/src/CMakeLists.txt``) includes the explicit hook file passed
   in as ``DAWN_OOT_CMAKE_FILE`` and calls the OOT hook
   ``dawn_oot_user_link_dawn(dawn)`` so user class implementations can
   link into the dawn library through normal CMake logic.

2. ``dawn/apps/dawn/CMakeLists.txt`` looks for
   ``DAWN_OOT_CMAKE_FILE``. If it is set, the
   ``dawn_oot_user_setup_app(apps_dawn)`` hook can attach any required
   C++ files, including a factory-registration translation unit that
   provides strong overrides of the ``dawn::oot::user_*_factory()``
   hooks declared in ``<dawn/oot.hxx>``.

3. ``dawn_oot_link_apps()`` (also from ``dawn_oot.cmake``, called by
   ``dawn/apps/CMakeLists.txt``) includes the same hook file from
   ``DAWN_OOT_CMAKE_FILE`` and calls the OOT hook
   ``dawn_oot_user_link_apps()`` so user apps can be registered through
   normal CMake logic.
   Separately, dawnpy exports ``DAWN_EXTENSION_APPS_KCONFIG`` when
   ``<oot>/external/apps/Kconfig`` exists, and ``dawn/apps/Kconfig``
   ``orsource``-s that explicit file path.

Adding IO / PROG / PROTO classes
================================

The same recipe applies to all three kinds. Use a user-reserved class ID
to add a brand-new type, or a built-in class ID to override the default
implementation (the user factory wins).

1. Add a header + source pair under ``external/src/<kind>/`` subclassing
   ``CIOCommon`` / ``CProgCommon`` / ``CProtoCommon``.

2. Add a factory class. Switch on ``desc.getObjectCls()`` and
   ``return new MyClass(desc)`` for IDs you own; ``return nullptr`` for
   the rest so the built-in factory takes over.

3. In ``external/src/<kind>/CMakeLists.txt`` call
   ``target_sources(dawn PRIVATE ...)`` for both class and factory, plus
   ``target_include_directories(dawn PUBLIC ...)`` for the header path.

4. In ``external/src/<kind>/Kconfig`` declare a guard option
   (``DAWN_OOT_..._IO`` etc.). Source it from your board ``Kconfig`` via
   ``$(DAWN_OOT_ROOT)``:

   .. code-block:: kconfig

      source "$(DAWN_BOARDS_COMMON)/Kconfig"
      source "$(DAWN_OOT_ROOT)/external/src/io/Kconfig"
      source "$(DAWN_OOT_ROOT)/external/src/prog/Kconfig"
      source "$(DAWN_OOT_ROOT)/external/src/proto/Kconfig"

5. Register the factories in a normal C++ translation unit such as
   ``external/dawn_oot_hooks.cxx`` by
   defining strong overrides of the three weak hooks declared in
   ``<dawn/oot.hxx>``. Then attach that file from your OOT CMake hook
   file inside ``dawn_oot_user_setup_app()`` so
   the strong definitions beat the weak defaults from the dawn library:

   .. code-block:: cpp

      #include "dawn/oot.hxx"

      #include "my_io_factory.hxx"
      #include "my_prog_factory.hxx"
      #include "my_proto_factory.hxx"

      namespace dawn::oot {
        IIOFactory *user_io_factory(void) {
          static my::CIOMyFactory s; return &s;
        }
        IProgFactory  *user_prog_factory (void) { static my::CProgMyFactory  s; return &s; }
        IProtoFactory *user_proto_factory(void) { static my::CProtoMyFactory s; return &s; }
      }

.. _app-lifecycle-hooks:

App lifecycle hooks
===================

Alongside the factory overrides, ``<dawn/oot.hxx>`` declares four optional
weak-default lifecycle hooks. OOT projects override any subset by adding
strong definitions to the same ``external/dawn_oot_hooks.cxx`` (no
extra CMake wiring needed -- the file is already attached via
``dawn_oot_user_setup_app()``). Default implementations in
``dawn/src/oot.cxx`` are no-ops, so leaving them alone preserves current
behaviour.

.. list-table::
   :header-rows: 1
   :widths: 20 55 25

   * - Hook
     - Where it fires
     - Failure handling
   * - ``user_init``
     - ``apps_dawn`` after ``dawn_board_init()`` succeeds, before any
       descriptor work.
     - Negative ``int`` aborts startup.
   * - ``user_post_load``
     - ``apps_dawn`` after ``CDawn::load_descriptor()`` succeeds, before
       ``CDawn::start()``.
     - Negative ``int`` aborts startup.
   * - ``user_on_idle``
     - ``CDawn::start()`` main loop in place of the default ``sleep(1)``.
       Hook owns cadence -- must return promptly so shutdown /
       descriptor-switch flags get re-checked.
     - ``void``; hook owns the cadence.
   * - ``user_pre_shutdown``
     - ``CDawn::start()`` after the main loop exits, before handlers are
       torn down (only when ``CONFIG_DAWN_LIFECYCLE_TEARDOWN=y``).
     - Negative ``int`` is logged as a warning; teardown still runs.

``user_init`` and ``user_post_load`` are tied to the default ``apps_dawn``
entry point. ``user_on_idle`` and ``user_pre_shutdown`` fire from
``CDawn::start()`` and therefore also apply to any external app that
constructs and drives ``CDawn`` directly.

.. code-block:: cpp

   #include "dawn/oot.hxx"

   namespace dawn::oot {
     int  user_init(void)                      { /* board fixup, log sink */ return OK; }
     int  user_post_load(CDawn *dawn)          { /* inspect objects */       return OK; }
     void user_on_idle(CDawn *dawn)            { /* periodic work */         sleep(1); }
     int  user_pre_shutdown(CDawn *dawn)       { /* persist state */         return OK; }
   }

External apps
=============

OOT projects can ship arbitrary NuttX applications alongside Dawn. They
live under ``<oot>/external/apps/<name>/``, are built by Dawn's apps
phase, and appear as regular NSH builtins.

- ``<oot>/external/apps/CMakeLists.txt`` is the entry point. It typically
  ``add_subdirectory()``-s one or more ``<appname>/`` directories.
- Each ``<appname>/CMakeLists.txt`` calls
  ``nuttx_add_application(NAME <appname> SRCS ...)``.
- ``<oot>/external/apps/Kconfig`` is auto-sourced by ``dawn/apps/Kconfig``
  via ``orsource``. It typically ``source``-s each ``<appname>/Kconfig``.

To **fully replace** the default ``apps_dawn``, disable
``CONFIG_DAWN_APPS_DAWN`` and have your own external app construct
``CDawn(&io, &prog, &proto)`` directly. Use this when you need a
non-trivial main loop, multiple ``CDawn`` instances, or app-level state
that does not fit the standard lifecycle. The minimal template mirrors
``dawn/apps/dawn/dawn_main.cxx``: parse args (if any), call
``dawn_board_init()``, register descriptor slot 0 with
``CDevDescriptor::regDescriptor()``, fill the CRC if required, call
``CDawn::load_descriptor()`` then ``CDawn::start()``.

Descriptor formats
==================

C++ (recommended for OOT):
    Set ``CONFIG_DAWN_APPS_EXAMPLE_DESC_FORMAT_CXX=y`` and point
    ``CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH`` at a hand-written ``.cxx``
    relative to your OOT root (``apps_dawn`` adds ``$DAWN_OOT_ROOT`` to
    its include path). User classes appear via their ``objectId(...)``
    static helpers like any built-in class.

YAML:
    Works for built-in Dawn types and for user types registered via the
    :class:`TypeRegistration` extension API. The simplest route is to
    drop a ``dawnpy_types.py`` file in your OOT project and point the
    dawnpy CLI at it with the global ``--types-from`` flag:

    .. code-block:: console

       python -m dawnpy --types-from examples/out-of-tree-demo/dawnpy_types.py \
           build /tmp/oot examples/out-of-tree-demo/boards/.../nsh_user_shell

    No ``pip install`` required. For vendor SDKs distributed to
    multiple downstreams, the same :class:`TypeRegistration` can be
    exposed as a ``dawnpy.extensions:descriptor_types`` entry-point in
    ``pyproject.toml``. See :doc:`api/oot` for both paths and the API.

    Custom per-type config items go on the TypeInfo's
    ``config_fields`` (:class:`ConfigField` or dict). See
    :doc:`api/oot`. Field shapes that don't fit the existing
    generators require the C++ descriptor path.
