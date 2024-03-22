# examples/out-of-tree-demo/dawnpy_types.py
#
# SPDX-License-Identifier: Apache-2.0
#
# dawnpy descriptor-types extension for the Dawn out-of-tree demo. Loaded
# directly via the `--types-from` global CLI flag - no install required:
#
#     python -m dawnpy --types-from examples/out-of-tree-demo/dawnpy_types.py \
#         build /tmp/oot examples/out-of-tree-demo/boards/.../nsh_user_shell
#
# yaml_type names mirror the C++ class-name stems from
# external/src/{io,prog,proto}/. With these entries merged, a YAML
# descriptor in this OOT project can reference `my_io_dummy`,
# `my_prog_dummy`, and `my_proto_dummy` like any built-in type.

from dawnpy.descriptor.definitions.registry import (
    ConfigField,
    IOTypeInfo,
    ProgTypeInfo,
    ProtoTypeInfo,
    TypeRegistration,
)

registration = TypeRegistration(
    name="dawn-oot-demo",
    io_types={
        "my_io_dummy": IOTypeInfo(
            cpp_class="oot_demo::CIOMyDummy",
            header="my_io_dummy.hxx",
            helper_func="{cpp_class}::objectId",
            params=["instance"],
            config_fields=[
                # Optional `init_value:` under the YAML object's
                # `config:` block. Emits one cfgIdInitval() helper +
                # one uint32 word into the descriptor.
                ConfigField(
                    name="init_value",
                    cpp_helper="oot_demo::CIOMyDummy::cfgIdInitval",
                    value_type="int",
                    value_format="hex",
                ),
            ],
        ),
    },
    prog_types={
        "my_prog_dummy": ProgTypeInfo(
            cpp_class="oot_demo::CProgMyDummy",
            header="my_prog_dummy.hxx",
            config_fields=[
                # Optional `tag:` under the YAML object's `config:`
                # block. Emits one cfgIdTag() helper + one uint32 word
                # into the descriptor.
                ConfigField(
                    name="tag",
                    cpp_helper="oot_demo::CProgMyDummy::cfgIdTag",
                    value_type="uint32",
                ),
            ],
        ),
    },
    proto_types={
        "my_proto_dummy": ProtoTypeInfo(
            cpp_class="oot_demo::CProtoMyDummy",
            header="my_proto_dummy.hxx",
        ),
    },
)
