=============
Wakaama LwM2M
=============

**Component Type:** Protocol

**Status:** In Progress

Overview
========

``CProtoWakaama`` is an LwM2M client protocol object based on Eclipse
Wakaama. It exposes descriptor-bound Dawn IO objects as LwM2M resources over
UDP/CoAP.

Implementation
==============

- ``CProtoWakaama`` owns the Wakaama client context, UDP socket, registration
  objects, and worker thread.
- Built-in Security, Server, and Device objects are created by the protocol
  instance. Descriptor bindings are created as internal Wakaama object bindings
  objects.
- The built-in Device object (3) is firmware-owned, but selected resources may
  be fed from descriptor IOs through the ``config.device`` block (the same
  pattern as nimble's built-in services): ``battery_voltage``,
  ``battery_level`` and ``battery_status`` bind IOs to resources 7 (Power
  Source Voltage), 9 (Battery Level) and 20 (Battery Status), which the Device
  object then serves through the generic IO read path, converting to the LwM2M
  units/enum.
- LwM2M server configuration is descriptor-owned. Kconfig values only provide
  defaults for descriptors that do not list ``config.servers``.
- UDP no-security, UDP multi-server registration, bootstrap provisioning, and
  DTLS PSK transport are implemented. DTLS requires TinyDTLS and
  ``CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK``.
- Upstream Wakaama allocates during normal runtime request handling. Dawn keeps
  that rule violation contained by routing Wakaama allocation hooks through a
  protocol-owned static memory pool instead of the system heap.
- Standard object/resource names are firmware-owned. Descriptor tooling
  resolves names from ``CProtoWakaama::WAKAAMA_OBJECT_*`` and
  ``CProtoWakaama::WAKAAMA_RESOURCE_*`` definitions.
- ``DTYPE_BLOCK`` resources are encoded as LwM2M opaque values. When the
  server requests text format, Wakaama returns opaque payloads as base64 text.

Observe/Notify
--------------

Observe is handled through Wakaama. When Dawn notify support is enabled, writes
to bound IO resources queue resource-change notifications and the Wakaama
thread publishes them with ``lwm2m_resource_value_changed``.

Limitations
-----------

- Wakaama is an exception to the Dawn rule that dynamic allocation is allowed
  only during initialization. The upstream core calls its allocation hook from
  read, discover, write, observe, registration, and bootstrap processing. The
  allocation is still runtime allocation, but it is bounded by
  ``CONFIG_DAWN_PROTO_WAKAAMA_MEMORY_POOL_SIZE`` and isolated from the system
  heap.
- DTLS support is PSK-only.
- Bootstrap coverage currently provisions no-security UDP client registration.
- Bootstrap-created ``coaps://`` final servers must be preconfigured in YAML so
  their DTLS buffers and credentials exist before runtime.


Configuration
=============

Kconfig
-------

- ``CONFIG_DAWN_PROTO_WAKAAMA``: enables Wakaama protocol support.
- ``CONFIG_DAWN_PROTO_WAKAAMA_ENDPOINT``: default endpoint name.
- ``CONFIG_DAWN_PROTO_WAKAAMA_SERVER_HOST``: default LwM2M server host.
- ``CONFIG_DAWN_PROTO_WAKAAMA_SERVER_PORT``: default server UDP port.
- ``CONFIG_DAWN_PROTO_WAKAAMA_LOCAL_PORT``: default local UDP port.
- ``CONFIG_DAWN_PROTO_WAKAAMA_LIFETIME``: default registration lifetime.
- ``CONFIG_DAWN_PROTO_WAKAAMA_SHORT_SERVER_ID``: default short server ID.
- ``CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK``: enables ``coaps://`` connections
  with TinyDTLS PSK credentials supplied by YAML.
- ``CONFIG_DAWN_PROTO_WAKAAMA_MEMORY_POOL_SIZE``: static arena size used by
  the Wakaama allocation hooks for runtime request processing.
- ``CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MANUFACTURER``: Device object
  manufacturer string used when YAML does not override it.
- ``CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_MODEL_NUMBER``: Device object model
  number string used when YAML does not override it.
- ``CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_SERIAL_NUMBER``: Device object serial
  number string used when YAML does not override it.
- ``CONFIG_DAWN_PROTO_WAKAAMA_DEVICE_FIRMWARE_VERSION``: Device object
  firmware version string used when YAML does not override it.

YAML
----

.. code-block:: yaml

   protocols:
     - id: wakaama_main
       type: wakaama
       config:
         endpoint: ntfc-wakaama
         local_port: 56830
         servers:
           - host: 127.0.0.1
             port: 5683
             lifetime: 60
             short_server_id: 123
           - host: 127.0.0.1
             port: 5684
             lifetime: 60
             short_server_id: 124
             security_instance: 1
             server_instance: 1
         device:
           manufacturer: Dawn
           model_number: simulator
           serial_number: dawn-wakaama
           firmware_version: '0.1'
         objects:
           - object: temperature
             instance: 0
             resources:
               - resource: sensor_value
                 io: temperature
                 access: read
           - object: digital_output
             instance: 0
             resources:
               - resource: digital_output_state
                 io: led
                 access: rw
           - object_id: 33000
             instance: 0
             resources:
               - resource_id: 1
                 io: counter
                 access: rw
           - object: binary_app_data_container
             instance: 0
             resources:
               - resource: binary_app_data
                 io: blob
                 access: rw

Supported fields:

- ``config.endpoint``: LwM2M endpoint name. If omitted, the Kconfig default
  is used.
- ``config.server_host``: LwM2M server IPv4 address or host name. If omitted,
  the Kconfig default is used.
- ``config.server_port``: LwM2M server UDP port. If omitted, the Kconfig
  default is used.
- ``config.local_port``: local UDP port used by the client. If omitted, the
  Kconfig default is used.
- ``config.lifetime``: registration lifetime in seconds. If omitted, the
  Kconfig default is used.
- ``config.short_server_id``: short server ID used by the default server when
  ``config.servers`` is omitted.
- ``config.servers``: explicit Security/Server Object instances. When present,
  this list replaces the Kconfig server defaults.
- ``config.device``: Device object strings. Each field is optional and falls
  back to its Kconfig default when omitted.
- ``config.objects``: object/resource bindings. Use ``object`` and
  ``resource`` for firmware-defined names, or ``object_id`` and
  ``resource_id`` for numeric IDs.

Supported server fields:

- ``host``: server IPv4 address or host name. Defaults to
  ``config.server_host`` or the Kconfig default.
- ``port``: server UDP port.
- ``scheme``: ``coap`` or ``coaps``. ``coaps`` requires
  ``CONFIG_DAWN_PROTO_WAKAAMA_DTLS_PSK``.
- ``security_mode``: ``none`` or ``psk``.
- ``psk_identity``: PSK identity string for ``security_mode: psk``.
- ``psk_key``: PSK key as a hexadecimal string.
- ``lifetime``: registration lifetime for this server.
- ``short_server_id``: LwM2M short server ID.
- ``security_instance``: Security Object instance ID.
- ``server_instance``: Server Object instance ID.
- ``bootstrap``: marks the server as a bootstrap server. Requires
  ``CONFIG_WAKAAMA_BOOTSTRAP``.
- ``holdoff``: Security Object bootstrap holdoff.
- ``bootstrap_timeout``: Security Object bootstrap timeout.

Supported resource fields:

- ``io``: Dawn IO object bound to the resource.
- ``access``: ``read``, ``write``, or ``rw``.
- ``resource``: standard resource name.
- ``resource_id``: numeric custom resource ID.

Supported Objects
^^^^^^^^^^^^^^^^^

The generic Wakaama object binding supports one or more instances per LwM2M
object. Each configured resource maps to one Dawn IO.

Standard objects currently used by the qemu NTFC target:

============================== ======
Name                           ID
============================== ======
``digital_input``              3200
``digital_output``             3201
``analog_input``               3202
``analog_output``              3203
``generic_sensor``             3300
``temperature``                3303
``humidity``                   3304
``actuation``                  3306
``light_control``              3311
``binary_app_data_container``  19
============================== ======

Additional standard object names are defined for later bindings:
``illuminance``, ``pressure``, ``barometer``, ``voltage``, ``current``,
``accelerometer``, ``magnetometer``, ``gyrometer``,
``connectivity_monitoring``, and ``cellular_connectivity``.

Supported Resources
^^^^^^^^^^^^^^^^^^^

Common scalar resources:

- ``digital_input_state`` (``5500``)
- ``digital_output_state`` (``5550``)
- ``analog_input_current_value`` (``5600``)
- ``analog_output_current_value`` (``5650``)
- ``sensor_value`` (``5700``)
- ``sensor_units`` / ``units`` (``5701``)
- ``min_measured_value`` (``5601``)
- ``max_measured_value`` (``5602``)
- ``min_range_value`` (``5603``)
- ``max_range_value`` (``5604``)
- ``application_type`` (``5750``)
- ``on_off`` (``5850``)
- ``dimmer`` (``5851``)
- ``colour`` / ``color`` (``5706``)

Block resources:

- ``binary_app_data`` (``/19/x/0``): opaque data backed by seekable
  ``DTYPE_BLOCK`` IO.

External Control
================

ControlIO: supported.

``CProtoWakaama`` supports runtime start/stop control through ``CIOControl``.
When stopped, the Wakaama worker thread and UDP client processing are inactive.
When started again, the client resumes registration and resource processing.

TriggerIO: not supported.

Doxygen
========

- `dawn::CProtoWakaama <../../doxygen/classdawn_1_1CProtoWakaama.html>`_
