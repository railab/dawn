<p align="center">
  <img src="Documentation/assets/logo.svg" alt="Dawn Project logo" width="280">
</p>

Dawn is a framework based on Apache NuttX for building descriptor-defined
embedded device nodes.

Instead of hardcoding each application around one board and one protocol, Dawn
describes a node as a set of IO, Program, and Protocol objects:

- IO objects represent device data and control points.
- Program objects implement runtime logic between IO objects.
- Protocol objects expose the node through shell, CAN, serial, BLE/NimBLE,
  UDP, Modbus, or other transports.

The descriptor is the application definition. It decides which objects exist,
how they are configured, and which protocols expose them.

## Descriptor shape

A Dawn descriptor is YAML that connects IO, Program, and Protocol objects:

```yaml
ios:
  - id: input1
    type: dummy
    dtype: uint32

  - id: output1
    type: dummy
    dtype: uint32

programs:
  - id: program1
    type: dummy
    inputs:
      - input1
    outputs:
      - output1

protocols:
  - id: protocol1
    type: dummy
    bindings:
      - input1
      - output1
```

This is a simplified shape example. Complete descriptors are available in
`descriptors/examples/`.

## What Dawn is for

Dawn is intended for embedded data-acquisition and control nodes, especially:

- sensor and actuator nodes
- NuttX-based prototypes
- custom lab and diagnostic tools
- hardware-in-the-loop and integration-test tools

## Start Here

Run a Dawn node without hardware in the NuttX simulator:

- [Quick Start](https://railab.github.io/dawn/quickstart.html)

## Examples

For a first run, use the simulator and simple blinky-style descriptors. They
are small starting points for learning how IO, Program, and Protocol objects
fit together.

- [Descriptor examples](https://railab.github.io/dawn/examples/descriptors.html)
- [Board examples](https://railab.github.io/dawn/examples/boards.html)

The full examples catalog contains more complete protocol, board, gateway,
feature, and hardware-oriented demos.

## Current Limits

- Linux is the tested host environment.
- At most GCC 14 is supported for the current NuttX libcxx build path.
- GCC 15 currently breaks this build path.
- Descriptor schemas, APIs, and protocol behavior may still change.

## Documentation

Dawn documentation is published at <https://railab.github.io/dawn/>.

- [Quick Start](https://railab.github.io/dawn/quickstart.html)
- [Environment](https://railab.github.io/dawn/environment.html)
- [Components](https://railab.github.io/dawn/components/index.html)
- [Examples](https://railab.github.io/dawn/examples/index.html)
- [Python tooling](https://railab.github.io/dawn/tools/dawnpy.html)
- [Out-of-tree customization](https://railab.github.io/dawn/customization.html)
- [Contributing](CONTRIBUTING.md)

## Python Tools

The Python tools live under `tools/` as separate packages:

- [dawnpy](https://github.com/railab/dawnpy) - core CLI, build helpers, descriptor
  validation, and code generation
- [dawnpy-serial](https://github.com/railab/dawnpy-serial) - serial
  transport CLI
- [dawnpy-can](https://github.com/railab/dawnpy-can) - CAN transport CLI
- [dawnpy-udp](https://github.com/railab/dawnpy-udp) - UDP transport CLI
- [dawnpy-modbus](https://github.com/railab/dawnpy-modbus) - Modbus
  transport CLI
- [dawnpy-ble](https://github.com/railab/dawnpy-ble) - BLE transport CLI
- [dawnpy-tests](https://github.com/railab/dawnpy-tests) - QA and test runner
- [dawn-nxscope](https://github.com/railab/dawn-nxscope) - Dawn NxScope
  extension

## License

The code in this repository is under the Apache 2.0 license or a compatible
license. See [LICENSE](LICENSE) for more information.
