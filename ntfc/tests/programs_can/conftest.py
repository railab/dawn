############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################

import pytest
from _can_common import assert_can_bus_idle, drain_frames
from _programs_can_common import (
    CONTROL_WRITE,
    ensure_control_state,
    get_runtime_node_id,
    start_dawn_and_socket,
)


@pytest.fixture(scope="session", autouse=True)
def _require_idle_can_bus():
    assert_can_bus_idle()


@pytest.fixture
def can_sock():
    sock = start_dawn_and_socket()
    try:
        yield sock
    finally:
        sock.close()


@pytest.fixture
def node_id(can_sock):
    return get_runtime_node_id(can_sock)


@pytest.fixture(autouse=True)
def _clear_can_frames(can_sock):
    # Keep each test isolated from stale queued frames.
    drain_frames(can_sock, max_frames=120)
    yield
    drain_frames(can_sock, max_frames=120)


@pytest.fixture(autouse=True)
def _restore_program_state(can_sock, node_id):
    yield

    for control_name, write_offset in CONTROL_WRITE.items():
        ensure_control_state(can_sock, node_id, control_name, write_offset, 1)
