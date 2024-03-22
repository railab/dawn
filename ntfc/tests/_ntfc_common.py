############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

import re
import time

import pytest


def start_dawn(timeout=1, settle_s=0.0):
    product = pytest.products[0].core(0)
    product.sendCommand("dawn", timeout=timeout)
    if settle_s > 0:
        time.sleep(settle_s)
    return product


def shell_cmd(product, cmd, timeout=1):
    return product.sendCommandReadUntilPattern(cmd, timeout=timeout)


def open_dawn_shell(timeout=1, settle_s=0.0):
    return start_dawn(timeout=timeout, settle_s=settle_s)


def close_dawn_shell(product):
    return shell_cmd(product, "exit", timeout=3)


def assert_set_ok(ret):
    """Assert shell setio did not report command/permission/runtime errors."""
    out = ret.output.lower()
    assert "invalid" not in out
    assert "not allowed" not in out
    assert "failed" not in out
    assert "no io with objid" not in out


def shell_list_io_entries(product, timeout=2):
    ret = shell_cmd(product, "list io", timeout=timeout)
    out = ret.output.replace("\r", "")

    if "Object Inventory" not in out:
        raise RuntimeError("shell list missing 'Object Inventory'")

    in_io_section = False
    entries = []

    for line in out.splitlines():
        text = line.strip()

        if text.startswith("IOs:"):
            in_io_section = True
            continue
        if text.startswith("PROGs:"):
            break
        if not in_io_section:
            continue

        compact_match = re.search(
            r"^(\S+)\s+0x([0-9a-fA-F]{8})\s+(\S+)\[(\d+)\]\s+"
            r"(\d+)\s+([RWNTC-]{5})\s+(\d+)/(\d+)/(\d+)$",
            text,
        )
        if compact_match is not None:
            entries.append(
                {
                    "name": compact_match.group(1),
                    "objid": int(compact_match.group(2), 16),
                    "type": 1,
                    "cls": int(compact_match.group(5)),
                    "dtype": compact_match.group(3),
                    "reads": int(compact_match.group(7)),
                    "writes": int(compact_match.group(8)),
                    "errors": int(compact_match.group(9)),
                }
            )
            continue

        verbose_match = re.search(
            r"^(\S+)\s+0x([0-9a-fA-F]{8})\s+(\d+)\s+(\d+)\s+(\S+)"
            r".*?(\d+)/(?:\s*)(\d+)/(?:\s*)(\d+)$",
            text,
        )
        if verbose_match is None:
            continue

        entries.append(
            {
                "name": verbose_match.group(1),
                "objid": int(verbose_match.group(2), 16),
                "type": int(verbose_match.group(3)),
                "cls": int(verbose_match.group(4)),
                "dtype": verbose_match.group(5),
                "reads": int(verbose_match.group(6)),
                "writes": int(verbose_match.group(7)),
                "errors": int(verbose_match.group(8)),
            }
        )

    return entries


def shell_resolve_io_objid_by_class(product, io_class, timeout=2):
    entries = shell_list_io_entries(product, timeout=timeout)
    matches = [entry["objid"] for entry in entries if entry["cls"] == io_class]

    if len(matches) != 1:
        raise RuntimeError(
            f"expected 1 IO for class {io_class}, got {len(matches)}: {entries}"
        )

    return matches[0]


def shell_resolve_io_objid_by_name(product, io_name, timeout=2):
    entries = shell_list_io_entries(product, timeout=timeout)
    matches = [entry["objid"] for entry in entries if entry["name"] == io_name]

    if len(matches) != 1:
        raise RuntimeError(
            f"expected 1 IO named '{io_name}', got {len(matches)}: {entries}"
        )

    return matches[0]


def shell_resolve_io_objid_from_expected(product, expected_objid, timeout=2):
    # Ignore dtype bits [19:16] because some descriptor-side helper paths
    # provide a generic/default dtype while runtime IO classes use fixed dtypes.
    dtype_mask = 0x000F0000
    expected_key = expected_objid & ~dtype_mask
    entries = shell_list_io_entries(product, timeout=timeout)
    matches = [
        entry["objid"]
        for entry in entries
        if (entry["objid"] & ~dtype_mask) == expected_key
    ]

    if len(matches) != 1:
        raise RuntimeError(
            "expected 1 IO matching descriptor objid "
            f"0x{expected_objid:08x}, got {len(matches)}: {entries}"
        )

    return matches[0]


def shell_get_io_scalar(product, objid, timeout=1):
    """Single-shot read of the first scalar token from ``getio``.

    Raises ``RuntimeError`` on missing marker or unparsable token.
    Callers that need to ride out transient misses should use
    ``shell_wait_for_io_scalar`` instead.
    """
    ret = shell_cmd(product, f"getio 0x{objid:08x}", timeout=timeout)
    out = ret.output.replace("\r", "")
    marker = f"IO 0x{objid:08x} data:"

    if marker not in out:
        raise RuntimeError(f"shell getio missing object 0x{objid:08x}: {ret.output}")

    match = re.search(rf"IO 0x{objid:08x} data:\n\t([^\n]+)", out)
    if match is None:
        raise RuntimeError(f"shell getio parse failed: {ret.output}")

    token = match.group(1).strip().split()[0]
    try:
        return int(token, 0)
    except ValueError:
        raise RuntimeError(
            f"shell getio scalar parse failed: getio 0x{objid:08x}\n{ret.output}"
        )


def shell_wait_for_io_scalar(product, objid, timeout=1, timeout_s=2.5, step_s=0.05):
    """Retry ``shell_get_io_scalar`` until it succeeds or deadline passes.

    Use this when ``getio`` may race with IO initialization on startup.
    """
    deadline = time.monotonic() + timeout_s
    last_exc = None
    while time.monotonic() < deadline:
        try:
            return shell_get_io_scalar(product, objid, timeout=timeout)
        except RuntimeError as exc:
            last_exc = exc
            time.sleep(step_s)
    raise (
        last_exc
        if last_exc
        else RuntimeError(f"shell getio scalar timeout for 0x{objid:08x}")
    )


def shell_get_io_words(product, objid, timeout=1):
    ret = shell_cmd(product, f"getio 0x{objid:08x}", timeout=timeout)
    out = ret.output.replace("\r", "")

    if f"IO 0x{objid:08x} data:" not in out:
        raise RuntimeError(f"shell getio missing object 0x{objid:08x}: {ret.output}")

    match = re.search(rf"IO 0x{objid:08x} data:\n\t([^\n]+)", out)
    if match is None:
        raise RuntimeError(f"shell getio parse failed: {ret.output}")

    words = [int(token) for token in match.group(1).strip().split()]
    if not words:
        raise RuntimeError(f"shell getio parse failed: {ret.output}")

    return words


def shell_set_io_scalar(product, objid, value, timeout=1):
    return shell_cmd(product, f"setio 0x{objid:08x} 0x{value:08x}", timeout=timeout)


def shell_set_io_words(product, objid, values, timeout=1):
    if not values:
        raise RuntimeError("shell_set_io_words requires at least one value")
    payload = " ".join(f"0x{int(value) & 0xffffffff:x}" for value in values)
    return shell_cmd(product, f"setio 0x{objid:08x} {payload}", timeout=timeout)


def shell_set_cfg_words(product, objid, cfgid, words, timeout=1):
    if not words:
        raise RuntimeError("shell_set_cfg_words requires at least one word")

    # Shell command line is limited, so keep hex tokens compact.
    payload = " ".join(f"0x{int(word) & 0xffffffff:x}" for word in words)
    cmd = f"setcfg 0x{objid:08x} 0x{cfgid:08x} {payload}"
    return shell_cmd(product, cmd, timeout=timeout)


def shell_get_cfg_words(product, objid, cfgid, timeout=1):
    ret = shell_cmd(product, f"getcfg 0x{objid:08x} 0x{cfgid:08x}", timeout=timeout)
    out = ret.output.replace("\r", "")

    if f"CFG 0x{cfgid:08x}:" not in out:
        raise RuntimeError(f"shell getcfg missing cfg 0x{cfgid:08x}: {ret.output}")

    words = [
        int(match.group(1), 16) for match in re.finditer(r"\n\t0x([0-9a-fA-F]{8})", out)
    ]
    if not words:
        raise RuntimeError(f"shell getcfg parse failed: {ret.output}")

    return words


def shell_wait_for_io_state(product, objid, expected, timeout_s=2.0, step_s=0.1):
    deadline = time.monotonic() + timeout_s
    last = None

    while time.monotonic() < deadline:
        try:
            last = shell_get_io_scalar(product, objid)
        except RuntimeError:
            time.sleep(step_s)
            continue

        if last == expected:
            return last
        time.sleep(step_s)

    return last


def shell_get_io_write_count(product, objid, timeout=2):
    entries = shell_list_io_entries(product, timeout=timeout)
    matches = [entry for entry in entries if entry["objid"] == objid]

    if len(matches) != 1:
        raise RuntimeError(f"expected 1 IO for objid 0x{objid:08x}, got {len(matches)}")

    return matches[0]["writes"]
