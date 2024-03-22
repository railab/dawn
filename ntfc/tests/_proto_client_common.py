############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

"""Shared helpers for Dawn simple-protocol clients (serial/UDP).

Consolidates helpers that were previously duplicated across
serial, UDP, and dynamic_desc tests:
- ``connect_client`` - retry-loop connect with ping validation
- ``exchange_status`` - send status-only command, validate STATUS_OK
- ``exchange_write`` - send SET_IO payload, return raw response
- ``wait_for_notification`` - poll for CMD_NOTIFY for a given objid
- ``parse_caps_blob`` - decode capabilities blob header + payload
"""

import struct
import time

import pytest


def connect_client(client_factory, timeout_s=5.0, retry_step_s=0.2):
    """Connect a simple-protocol client, retry until ping responds.

    :param client_factory: callable returning a fresh client instance
    :param timeout_s: total connect deadline
    :param retry_step_s: sleep between attempts
    :return: connected client
    """
    deadline = time.monotonic() + timeout_s
    client = client_factory()

    while time.monotonic() < deadline:
        if client.connect():
            if client.ping():
                return client
            client.disconnect()
        time.sleep(retry_step_s)

    pytest.fail("Could not connect simple-protocol client")


def exchange_status(client, cmd, objid):
    """Send status-only command and validate STATUS_OK response."""
    payload = struct.pack("<I", objid)
    if not client.send_frame(cmd, payload):
        return False

    frame = client.receive_frame()
    if not frame:
        return False

    resp_cmd, response = frame
    return resp_cmd == cmd and len(response) >= 1 and response[0] == client.STATUS_OK


def exchange_write(client, objid, data):
    """Send SET_IO payload and return raw response frame."""
    payload = struct.pack("<I", objid) + data
    if not client.send_frame(client.CMD_SET_IO, payload):
        return None
    return client.receive_frame()


def wait_for_notification(client, expected_objid, retries=5):
    """Wait for notification frame for a specific object ID."""
    for _ in range(retries):
        frame = client.receive_frame()
        if not frame:
            continue

        cmd, payload = frame
        if cmd != client.CMD_NOTIFY or len(payload) < 4:
            continue

        objid = struct.unpack("<I", payload[:4])[0]
        if objid == expected_objid:
            return payload[4:]

    return None


def parse_caps_blob(blob):
    """Parse capabilities blob header and return all fields.

    :param blob: raw capabilities bytes
    :return: tuple (version, category, reserved, payload_len, payload)
    :raises RuntimeError: when blob is too short or payload size mismatches
    """
    if len(blob) < 8:
        raise RuntimeError("Capabilities blob too short")

    version = blob[0]
    category = blob[1]
    payload_len = struct.unpack_from("<H", blob, 2)[0]
    reserved = struct.unpack_from("<I", blob, 4)[0]
    payload = blob[8:]

    if len(payload) != payload_len:
        raise RuntimeError(
            f"Capabilities payload size mismatch: hdr={payload_len} "
            f"actual={len(payload)}"
        )

    return version, category, reserved, payload_len, payload
