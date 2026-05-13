=========================
Out-of-Tree API Reference
=========================

This page documents the user-extension API surface for out-of-tree Dawn
projects. See :doc:`../customization` for the operational guide.

User-factory contract
=====================

``CDawn`` accepts three optional factory pointers (one per kind):

.. code-block:: cpp

   class CDawn
   {
   public:
     explicit CDawn(IIOFactory *iofactory      = nullptr,
                    IProgFactory *progfactory  = nullptr,
                    IProtoFactory *protofactory = nullptr);
   };

Each factory implements ``create(CDescObject &desc)``. The handler
consults the user factory first; if it returns ``nullptr``, the built-in
factory tries to handle the class. This means a user factory only needs
to handle the class IDs it owns and can transparently ignore everything
else.

Header: ``<dawn/oot.hxx>``
==========================

User-factory hooks
------------------

Three free functions form the factory side of the user-extension API:

.. code-block:: cpp

   namespace dawn::oot
   {
     IIOFactory    *user_io_factory   (void);
     IProgFactory  *user_prog_factory (void);
     IProtoFactory *user_proto_factory(void);
   }

``apps_dawn`` calls all three at startup and passes the results into
``CDawn``. dawn library provides weak default implementations that return
``nullptr`` (in ``dawn/src/oot.cxx``); OOT projects typically override
them with strong definitions in ``<oot>/external/dawn_oot_hooks.cxx``:

.. code-block:: cpp

   #include "dawn/oot.hxx"
   #include "my_io_factory.hxx"
   #include "my_prog_factory.hxx"
   #include "my_proto_factory.hxx"

   namespace dawn::oot {
     IIOFactory *user_io_factory(void) {
       static MyIOFactory s; return &s;
     }
     IProgFactory  *user_prog_factory (void) { static MyProgFactory  s; return &s; }
     IProtoFactory *user_proto_factory(void) { static MyProtoFactory s; return &s; }
   }

``<oot>/external/dawn_oot.cmake`` then attaches that file to the
``apps_dawn`` target via the ``dawn_oot_user_setup_app()`` hook. dawnpy
passes that hook file into CMake explicitly as
``-DDAWN_OOT_CMAKE_FILE=<oot>/external/dawn_oot.cmake``. The strong
overrides beat the weak defaults at final link time.

App lifecycle hooks
-------------------

Four free functions cover the lifecycle side of the user-extension API:

.. code-block:: cpp

   namespace dawn
   {
   class CDawn;

   namespace oot
   {
     int  user_init        (void);
     int  user_post_load   (CDawn *dawn);
     void user_on_idle     (CDawn *dawn);
     int  user_pre_shutdown(CDawn *dawn);
   }
   }

.. list-table::
   :header-rows: 1
   :widths: 25 50 25

   * - Hook
     - Call site
     - Default / failure handling
   * - ``user_init``
     - ``apps_dawn`` after ``dawn_board_init()``, before any descriptor
       work.
     - ``return OK``; negative aborts startup.
   * - ``user_post_load``
     - ``apps_dawn`` after ``CDawn::load_descriptor()``, before
       ``CDawn::start()``.
     - ``return OK``; negative aborts startup.
   * - ``user_on_idle``
     - ``CDawn::start()`` main loop, in place of the default
       ``sleep(1)``.
     - ``sleep(1)``; hook owns cadence.
   * - ``user_pre_shutdown``
     - ``CDawn::start()`` after the main loop exits, before handlers are
       stopped. Only present when ``CONFIG_DAWN_LIFECYCLE_TEARDOWN=y``.
     - ``return OK``; negative is logged, teardown still runs.

``user_init`` and ``user_post_load`` are tied to the default ``apps_dawn``
entry point. ``user_on_idle`` and ``user_pre_shutdown`` fire from
``CDawn::start()`` itself, so they also apply to any external app that
constructs and drives ``CDawn`` directly.

OOT projects override any subset by adding strong definitions to
``<oot>/external/dawn_oot_hooks.cxx`` (no extra CMake wiring needed --
the file is already attached via ``dawn_oot_user_setup_app()``). Weak
defaults from ``dawn/src/oot.cxx`` cover everything not overridden, so
omitting a hook costs nothing at runtime.

Class IDs reserved for users
============================

User-defined classes must use IDs in the reserved ranges. Picking IDs
inside the built-in range will collide with the built-in factory's
``switch``.

================ ====================================================
Kind             User-reserved range start (from common.hxx)
================ ====================================================
IO               ``CIOCommon::IO_CLASS_USER`` (500) and above
PROG             ``CProgCommon::PROG_CLASS_USER`` (511) and above
PROTO            ``CProtoCommon::PROTO_CLASS_USER`` (511) and above
================ ====================================================

Capabilities IO currently reports only upstream built-in object classes. OOT
extension class IDs are not yet supported in the capabilities bitmap. Support
for advertising OOT IO, PROG, and PROTO classes must be implemented in the
future before descriptor-aware clients can discover those extensions through
``CIOCapabilities``.

Build-system hooks
==================

``dawn/cmake/dawn_oot.cmake``
-----------------------------

Three functions, all no-ops when ``DAWN_OOT_ROOT`` is unset. OOT build
extensions are declared in an explicit CMake hook file passed as
``DAWN_OOT_CMAKE_FILE``.

The module also exposes ``DAWN_CMAKE_DIR``, the upstream Dawn common
CMake module directory. OOT applications can include reusable helpers
from there, for example::

   include("${DAWN_CMAKE_DIR}/yaml_generator.cmake")

``dawn_oot_link()`` - called from ``dawn/src/CMakeLists.txt``:

1. Includes ``${DAWN_OOT_CMAKE_FILE}`` when present and
   calls ``dawn_oot_user_link_dawn(dawn)`` so the OOT project can attach
   IO / PROG / PROTO sources however it wants.
2. Adds ``$DAWN_OOT_ROOT/external/include`` to the dawn library's
   ``PUBLIC`` include search path if the directory exists.

OOT-side CMake code is expected to call ``target_sources()`` /
``target_include_directories()`` on the supplied targets directly.

``dawn_oot_link_apps()`` - called from ``dawn/apps/CMakeLists.txt``:

1. Includes ``${DAWN_OOT_CMAKE_FILE}`` when present and
   calls ``dawn_oot_user_link_apps()`` so the OOT project can register
   any extra NuttX apps through normal CMake logic.

``dawn_oot_app_setup(<target>)`` - called from
``dawn/apps/dawn/CMakeLists.txt`` with ``apps_dawn`` as the target:

1. Adds ``$DAWN_OOT_ROOT`` to the target's PRIVATE include path so
   user descriptor paths in ``CONFIG_DAWN_APPS_EXAMPLE_DESC_PATH``
   resolve relative to the OOT project.
2. Includes ``${DAWN_OOT_CMAKE_FILE}`` when present and
   calls ``dawn_oot_user_setup_app(apps_dawn)`` so the OOT project can
   attach any required C++ sources to the application target.

The matching Kconfig hook is in ``dawn/apps/Kconfig``::

   orsource "$(DAWN_OOT_ROOT)/external/apps/Kconfig"

Environment variables
---------------------

``DAWN_OOT_ROOT``
    Absolute path to the OOT project root (the directory containing
    ``boards/`` and ``external/``). dawnpy auto-exports this when the
    requested defconfig lives outside the upstream Dawn repo. Consumed
    by ``dawn_oot_link()``, by ``dawn/apps/dawn/CMakeLists.txt`` for
    include-path setup, and by OOT board ``Kconfig`` files for sourcing
    user-extension Kconfig.

``DAWN_OOT_CMAKE_FILE``
    Absolute path to the OOT CMake extension entry point. dawnpy passes
    it as a CMake cache entry when ``<oot>/external/dawn_oot.cmake``
    exists. Consumed only by ``dawn/cmake/dawn_oot.cmake``.

``DAWN_EXTENSION_APPS_KCONFIG``
    Absolute path to an optional Dawn app-extension Kconfig file. dawnpy
    exports it when ``<oot>/external/apps/Kconfig`` exists. Consumed only
    by ``dawn/apps/Kconfig`` so Dawn does not hardcode an OOT apps path.

``DAWN_BOARDS_COMMON``
    Absolute path to the upstream ``boards/common/`` directory. Already
    used by the upstream sim board; OOT boards may source its Kconfig the
    same way.

``DAWN_CMAKE_DIR``
    CMake cache variable pointing at Dawn's shared CMake helper directory.
    OOT CMake code may include common modules such as
    ``${DAWN_CMAKE_DIR}/yaml_generator.cmake``.

dawnpy ``Project`` API
======================

``dawnpy.dawn.project.Project`` is the in-tree/OOT project model:

.. code-block:: python

   @dataclass(frozen=True)
   class Project:
       project_root: Path
       dawn_root: Path
       is_oot: bool

       @classmethod
       def resolve(cls, start_path: Path | None = None) -> "Project": ...

       def cmake_env(self) -> dict[str, str]: ...

``Project.resolve(confpath)`` walks up from a defconfig path to find
the nearest ``boards/`` root; ``cmake_env()`` returns the environment
mapping (``DAWN_BOARDS_COMMON`` always, ``DAWN_OOT_ROOT`` only when
``is_oot``). The build commands feed it into the cmake invocation.

dawnpy ``TypeRegistration`` extension API
=========================================

OOT projects can register custom IO / PROG / PROTO types so YAML
descriptors can reference them. A registration is a
``dawnpy.descriptor.definitions.registry.TypeRegistration`` value loaded from a
project-local Python file or a Python package.

The registration object
-----------------------

.. code-block:: python

   # examples/out-of-tree-demo/dawnpy_types.py
   from dawnpy.descriptor.definitions.registry import (
       IOTypeInfo, ProgTypeInfo, ProtoTypeInfo, TypeRegistration,
   )

   registration = TypeRegistration(
       name="dawn-oot-demo",
       io_types={
           "my_io_dummy": IOTypeInfo(
               cpp_class="CIOMyDummy",
               header="my_io_dummy.hxx",
               helper_func="{cpp_class}::objectId",
               params=["instance"],
           ),
       },
       prog_types={
           "my_prog_dummy": ProgTypeInfo(
               cpp_class="CProgMyDummy",
               header="my_prog_dummy.hxx",
           ),
       },
       proto_types={
           "my_proto_dummy": ProtoTypeInfo(
               cpp_class="CProtoMyDummy",
               header="my_proto_dummy.hxx",
           ),
       },
   )

The module may instead expose a ``registrations`` iterable to bundle
multiple :class:`TypeRegistration` values into one file.

Loading: ``--types-from`` (recommended)
---------------------------------------

The simplest path: drop ``dawnpy_types.py`` somewhere in your OOT
project and pass it to dawnpy via the global ``--types-from`` flag. No
``pip install``, no entry-point machinery.

.. code-block:: console

   python -m dawnpy \
       --types-from examples/out-of-tree-demo/dawnpy_types.py \
       build /tmp/oot examples/out-of-tree-demo/boards/.../nsh_user_shell

The flag is accepted multiple times; later registrations layer on top
of earlier ones. The argument may be either a single ``.py`` file or a
package directory (a directory with ``__init__.py``).

Loading: published Python package (optional)
--------------------------------------------

If an OOT project wants to ship its registration as a reusable plugin
distributed to multiple consumers, the same ``TypeRegistration``
object can be exposed as a ``dawnpy.extensions:descriptor_types``
entry-point in ``pyproject.toml``:

.. code-block:: toml

   [project.entry-points."dawnpy.extensions"]
   descriptor_types = "my_pkg.dawnpy_ext:registration"

Once the package is ``pip install``-ed into the same environment as
dawnpy, the registration is picked up at module import time. Use this
path for vendor SDKs distributed to multiple downstreams; use
``--types-from`` for an ordinary single-project setup.

Conflicts and overrides
-----------------------

Existing built-in entries with the same yaml_type are overwritten with
a ``WARNING`` log line. This lets OOT projects override default
upstream types as well as add new ones.

Custom config items
-------------------

``IOTypeInfo`` / ``ProgTypeInfo`` / ``ProtoTypeInfo`` accept an
optional ``config_fields`` list. Each entry is a :class:`ConfigField`
or an equivalent dict matching the built-in
``dawnpy/config/{io,prog,proto}_config_fields.yaml`` shape. The
generator emits one ``cfgIdXxx(...)`` block per field that the YAML
descriptor's ``config:`` map sets.

``ProtoTypeInfo`` additionally has ``uses_standard_bindings: bool =
True`` controlling whether ``cfgIdIOBind`` is appended after the
typed fields.

Field shapes that don't fit the existing generators (custom packing,
multi-word payloads) require the C++ descriptor path.
