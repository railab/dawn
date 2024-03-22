############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

from functools import lru_cache
from pathlib import Path

import yaml
from dawnpy.descriptor.client import load_client_descriptor
from dawnpy.descriptor.definitions.summary import ObjectIdResolver

REPO_ROOT = Path(__file__).resolve().parents[2]


def _desc_path(rel_path):
    return REPO_ROOT / rel_path


@lru_cache(maxsize=None)
def load_descriptor_spec(rel_path):
    with _desc_path(rel_path).open("r", encoding="utf-8") as f:
        return yaml.safe_load(f) or {}


@lru_cache(maxsize=None)
def load_descriptor(rel_path):
    return load_client_descriptor(str(_desc_path(rel_path)))


@lru_cache(maxsize=None)
def io_objids(rel_path):
    desc = load_descriptor(rel_path)
    resolver = ObjectIdResolver()
    result = {}
    for io_id, io in desc.ios.items():
        objid = resolver.io_objid(io)
        if objid is not None:
            result[io_id] = objid
    return result


def io_objid(rel_path, io_id):
    objid = io_objids(rel_path).get(io_id)
    if objid is None:
        raise RuntimeError(f"IO '{io_id}' not found in descriptor {rel_path}")
    return objid


def protocol_spec(rel_path, proto_type):
    spec = load_descriptor_spec(rel_path)
    for proto in spec.get("protocols", []):
        if proto.get("type") == proto_type:
            return proto
    raise RuntimeError(f"No protocol '{proto_type}' found in descriptor {rel_path}")


def protocol_config_value(rel_path, proto_type, key):
    proto = protocol_spec(rel_path, proto_type)
    config = proto.get("config", {})
    if key not in config:
        raise RuntimeError(
            f"Protocol '{proto_type}' in {rel_path} has no config key '{key}'"
        )
    return config[key]


@lru_cache(maxsize=None)
def can_layout(rel_path, check_collisions=False):
    """Return {obj_type: {io_id: can_id}} layout for the CAN protocol.

    :param check_collisions: if True, raise on any CAN ID reused across
        object types (catches descriptor authoring bugs that would
        otherwise surface as silent frame aliasing at runtime).
    """
    proto = protocol_spec(rel_path, "can")
    layout = {}
    global_ids = {} if check_collisions else None
    for obj in proto.get("config", {}).get("objects", []):
        obj_type = obj.get("type")
        start = int(obj.get("can_id_start", 0))
        bindings = obj.get("bindings", [])
        group = layout.setdefault(obj_type, {})
        for idx, binding in enumerate(bindings):
            io_id = binding.get("id")
            if io_id is None:
                continue
            if io_id in group:
                raise RuntimeError(
                    f"Duplicate CAN binding '{io_id}' in {rel_path} ({obj_type})"
                )
            can_id = start + idx
            if global_ids is not None:
                owner = f"{obj_type}:{io_id}"
                prev = global_ids.get(can_id)
                if prev is not None:
                    raise RuntimeError(
                        f"CAN ID collision in {rel_path}: "
                        f"0x{can_id:03x} used by '{prev}' and '{owner}'"
                    )
                global_ids[can_id] = owner
            group[io_id] = can_id
    return layout


@lru_cache(maxsize=None)
def modbus_wire_starts(rel_path, proto_type="modbus_rtu"):
    proto = protocol_spec(rel_path, proto_type)
    result = {}
    for reg in proto.get("config", {}).get("registers", []):
        reg_type = reg.get("type")
        if reg_type is None:
            continue
        result[reg_type] = int(reg.get("start", 0))
    return result


@lru_cache(maxsize=None)
def modbus_seekable_groups(rel_path, proto_type="modbus_rtu"):
    proto = protocol_spec(rel_path, proto_type)
    result = {}
    for reg in proto.get("config", {}).get("registers", []):
        if reg.get("type") != "seekable":
            continue

        window_regs = int(reg.get("config", 8))
        if window_regs <= 0:
            window_regs = 8

        start = int(reg.get("start", 0))
        bindings = reg.get("bindings", [])
        for idx, binding in enumerate(bindings):
            io_id = binding.get("id")
            if io_id is None:
                continue
            if io_id in result:
                raise RuntimeError(
                    f"Duplicate seekable binding '{io_id}' in {rel_path}"
                )
            data_start = start + (idx * (window_regs + 1))
            result[io_id] = {
                "start": data_start,
                "window_regs": window_regs,
                "ctrl": data_start + window_regs,
            }

    return result
