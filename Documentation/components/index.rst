==============
Implementation
==============

Overview
========

Dawn runtime is descriptor-driven.
The descriptor binary defines which objects exist, how they are configured,
and how they are connected.

A general implementation view is shown below.

.. uml::
   :scale: 80%
   :align: center

   skinparam componentStyle rectangle

   package IOPackage {
        component IOFactory
        component IOHandler {
            collections IO
        }
   }

   package ProgPackage {
        component ProgFactory
        component ProgHandler {
                collections Prog
        }
   }

   package ProtoPackage {
        component ProtoFactory
        component ProtoHandler {
                collections Proto
        }
   }

   cloud ExternalWorld

   package DescriptorPackage {
        component Descriptor {
           collections CDescObject
        }
   }

   IOFactory --> IO : create
   ProgFactory --> Prog : create
   ProtoFactory --> Proto : create

   Descriptor --> IOFactory : define
   Descriptor --> ProgFactory : define
   Descriptor --> ProtoFactory : define

   Prog <-> IO : get, set, manage VirtIO

   IO <--> Proto : get, set
   Proto -- ExternalWorld

.. end of uml

Core Model
==========

The core implementation element is :doc:`common/descriptor`, a binary data array
in memory that defines system objects and configuration payloads.
Object model details are documented in :doc:`common/object`.

Objects are created by factories from descriptor entries.
Dawn uses three main object families:

- :doc:`io/index`:
  IO objects read from or write to OS resources and Dawn resources.
- :doc:`prog/index`:
  Program objects process IO data and manage virtual IO data flow.
- :doc:`proto/index`:
  Protocol objects expose IO data to external systems.

Common runtime utilities and abstractions are in :doc:`common/index`.
Porting interfaces are in :doc:`porting/index`.

Descriptor source and binary forms are summarized in :ref:`descriptor-format`.
Runtime loading and binary layout details are documented in
:ref:`descriptor`.

Special IO Components
=====================

Some IO classes are framework-facing control/bridge objects:

- :doc:`io/virtual`:
  virtual IO surface used by program pipelines.
- :doc:`io/config`:
  runtime get/set path for selected object configuration items.
- :doc:`io/control`:
  control command path to target objects.
- :doc:`io/trigger`:
  trigger command path to target objects.
- :doc:`io/descriptor`:
  descriptor data read access through IO API.

Deployment Patterns
===================

Common device compositions:

1. IO + PROTO:
   externally exposed node. Example: remote LED and button.
2. IO + PROG:
   edge-processing node. Example: local LED sequencer.
3. IO + PROG + PROTO:
   combined processing and external exposure node.
   Example: filtered sensor over BLE.

Repository Map
==============

Main repository areas:

- ``boards/``: board and build configuration directories.
- ``Documentation/``: documentation sources.
- ``dawn/include/``: framework public headers.
- ``dawn/src/``: framework implementation.
- ``dawn/apps/``: Dawn app entry points.
- ``dawn/tests/``: unit tests.
- ``descriptors/``: reusable/example descriptor sources.
- ``ntfc/``: integration/system test scenarios.
- ``tools/dawnpy/``: core build/descriptor/tooling CLI.
- ``tools/dawnpy-tests/``: Dawn QA runner extension.
- ``tools/dawnpy-serial/``, ``tools/dawnpy-can/``,
  ``tools/dawnpy-udp/``, ``tools/dawnpy-modbus/``,
  ``tools/dawnpy-lwm2m/``:
  protocol-specific tooling extensions.
- ``external/nuttx/`` and ``external/apps/``: integrated NuttX trees.

Build Composition
=================

.. uml::
   :scale: 80%
   :align: center

   package ScenarioSpecific {
        database DawnDescriptor
        database DawnConfiguration
        database BoardSupport
        database UserObjects
   }

   collections DawnSource
   cloud DawnBuild
   rectangle DawnFirmware

   DawnDescriptor --> DawnBuild
   DawnConfiguration --> DawnBuild
   BoardSupport --> DawnBuild
   UserObjects --> DawnBuild
   DawnSource --> DawnBuild

   DawnBuild --> DawnFirmware

.. end of uml

Build inputs:

- ``DawnSource``: Dawn framework code.
- ``DawnDescriptor``: descriptor binary used by the target.
- ``DawnConfiguration``: scenario and feature configuration.
- ``BoardSupport``: board/RTOS integration.
- ``UserObjects``: optional user-provided objects/factories.

Build output:

- ``DawnFirmware``: final firmware image.

Components
==========

All IO/PROG/PROTO component pages follow the standardized template in
:doc:`template`.

.. toctree::
   :maxdepth: 2

   template.rst
   apps.rst
   dawn/index.rst
   descriptors.rst
   common/index.rst
   io/index.rst
   prog/index.rst
   proto/index.rst
   porting/index.rst
   fake_devices.rst
