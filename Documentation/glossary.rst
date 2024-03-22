.. _glossary:

========
Glossary
========

This glossary defines the core terminology used throughout the **Dawn**
framework.

.. glossary::

   Binding
      The process of connecting one Dawn object to another at runtime (usually
      during the initialization phase). For example, a "Stats" program binds to
      an "ADC" IO to process its samples.

   ConfigID
      A 32-bit identifier (``SObjectCfg::UObjectCfgId``) used within the
      descriptor to mark a specific configuration parameter for an object. For
      field definitions like **RW** and **Size**, see the :ref:`object` section.

   Descriptor
      A binary array (usually stored in flash or memory) that defines the
      entire configuration of a Dawn device. It contains the list of objects
      to create, their types, and their parameters. See :ref:`descriptor`.

   Dtype (Data Type)
      A 4-bit field in an ObjectID or ConfigID that specifies the data format
      (e.g., INT32, FLOAT, BOOL, Fixed-point). See :ref:`object`.

   Factory
      A component responsible for instantiating objects of a specific type
      (IO, Program, or Protocol) based on their descriptor configuration.

   Handler
      A manager responsible for the lifecycle of a specific group of objects.
      There are three main handlers: :cpp:class:`dawn::CIOHandler`,
      :cpp:class:`dawn::CProgHandler`, and :cpp:class:`dawn::CProtoHandler`.
      See :ref:`handler`.

   Inspector
      A global diagnostic component (:cpp:class:`dawn::CDevInspector`) that
      provides real-time visibility into handlers and objects for debugging.

   Instance (Instance ID)
      A 14-bit field (**Priv**) in an ObjectID used to distinguish between
      multiple objects of the same Type and Class (e.g., ADC channel 0 vs 1).

   IO (Input/Output)
      Objects that interface with hardware or virtual data sources/sinks.
      Examples: ADC, GPIO, PWM, Virtual Registers. See :ref:`ioindex`.

   Notifier
      A service (:cpp:class:`dawn::CIONotifier`) that polls IO objects and
      triggers asynchronous callbacks when data is available.

   NTFC
      NuttX Test Framework for Community. A Python-based testing framework
      used for high-level system validation.

      https://github.com/szafonimateusz-mi/nuttx-ntfc

   nxscope
      A high-speed, real-time data visualization protocol supported by Dawn
      for streaming telemetry to host-side tools.

      https://github.com/railab/nxslib
      https://github.com/railab/nxscli

   Object
      The fundamental unit in Dawn. Every IO, Program, and Protocol is an
      object derived from the :cpp:class:`dawn::CObject` base class. See
      :ref:`object`.

   ObjectID
      A 32-bit identifier that uniquely identifies every object in a Dawn
      system. For field definitions like **Type**, **Cls**, **Dtype**, and
      **Priv**, see the :ref:`object` section.

   Program (PROG)
      Objects that perform edge-processing or data transformation. They "bind"
      to IO objects to read inputs or write outputs. Examples: Statistics
      (Min/Max), Adjust (scaling).

   Protocol (PROTO)
      Objects that handle communication between the Dawn device and the
      external world. Examples: Serial, Modbus, CAN, BLE.

   dawnpy
      A companion Python package used for generating binary descriptors from
      YAML, validating configurations, and interacting with Dawn devices.
