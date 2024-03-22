############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for simulator NXScope serial streaming."""

import queue
import time

import pytest
from _descriptor_common import io_objid
from _ntfc_common import (
    assert_set_ok,
    close_dawn_shell,
    open_dawn_shell,
    shell_resolve_io_objid_from_expected,
    shell_set_io_scalar,
)

SerialDevice = pytest.importorskip(
    "nxslib.intf.serial",
    reason="nxslib is required for NXScope host tests",
).SerialDevice
NxscopeHandler = pytest.importorskip(
    "nxslib.nxscope",
    reason="nxslib is required for NXScope host tests",
).NxscopeHandler
Parser = pytest.importorskip(
    "nxslib.proto.parse",
    reason="nxslib is required for NXScope host tests",
).Parser

DESC_PATH = "descriptors/examples/nxscope_serial.yaml"
NXSCOPE_HOST_PATH = "/tmp/ttyNX0"
NX_DUMMY_NOTIFY_ID = io_objid(DESC_PATH, "nx_dummy_notify1")
NXSCOPE_DUMMY_CHAN = 0


def _open_nxscope(timeout_s=5.0):
    deadline = time.monotonic() + timeout_s
    last_exc = None

    while time.monotonic() < deadline:
        try:
            nxscope = NxscopeHandler(SerialDevice(NXSCOPE_HOST_PATH), Parser())
            nxscope.connect()
            return nxscope
        except Exception as exc:
            last_exc = exc
            time.sleep(0.1)

    pytest.fail(
        "Could not connect NXScope host on " f"{NXSCOPE_HOST_PATH}: {last_exc!r}"
    )


def _wait_for_nxscope_value(subq, expected, timeout_s=2.0):
    deadline = time.monotonic() + timeout_s
    seen = []

    while time.monotonic() < deadline:
        left = deadline - time.monotonic()
        try:
            samples = subq.get(block=True, timeout=min(0.2, left))
        except queue.Empty:
            continue

        for sample in samples:
            if len(sample.data) < 1:
                continue
            # nxslib returns per-sample data as vdim-sized vectors; the
            # dummy channel is scalar (vdim=1) so index into the vector.
            first = sample.data[0]
            scalar = first[0] if hasattr(first, "__len__") else first
            value = int(scalar) & 0xFFFFFFFF
            seen.append(value)
            if expected is None or value == expected:
                return value

    pytest.fail(
        f"Timeout waiting for NXScope value {expected}, " f"received: {seen[-10:]}"
    )


def _drain_nxscope_queue(subq):
    """Drop already buffered stream samples from subscription queue."""
    while True:
        try:
            subq.get_nowait()
        except queue.Empty:
            return


def _assert_no_nxscope_samples(subq, timeout_s=0.8):
    """Ensure no stream data arrives within timeout window."""
    try:
        subq.get(block=True, timeout=timeout_s)
    except queue.Empty:
        return

    pytest.fail("Unexpected NXScope samples while disabled")


def _configure_dummy_channel(nxscope, chan):
    """Configure target channel for stream capture."""
    nxscope.channels_default_cfg()
    nxscope.ch_enable(chan)
    nxscope.ch_divider(chan, 1)


class TestNxscope:
    """Validate NXScope stream activity and shell-driven dummy updates."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NXSCOPE"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NXSCOPE_SERIAL"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_SHELL"),
        pytest.mark.dep_config("CONFIG_DAWN_IO_DUMMY_NOTIFY"),
    ]

    def test_nxscope_stream_with_shell_controlled_dummy(self):
        """Ensure NXScope stream is active and dummy state is writable."""

        product = open_dawn_shell(settle_s=0.3)
        try:
            runtime_dummy_id = shell_resolve_io_objid_from_expected(
                product, NX_DUMMY_NOTIFY_ID
            )
        except RuntimeError:
            runtime_dummy_id = NX_DUMMY_NOTIFY_ID
        nxscope = _open_nxscope()
        subq = None

        try:
            _configure_dummy_channel(nxscope, NXSCOPE_DUMMY_CHAN)
            subq = nxscope.stream_sub(NXSCOPE_DUMMY_CHAN)
            nxscope.stream_start()
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            # Disable/enable stream and verify no data/data transitions.
            _drain_nxscope_queue(subq)
            nxscope.stream_stop()
            _assert_no_nxscope_samples(subq, timeout_s=0.8)
            nxscope.stream_start()
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            # Disable/enable channel and verify no data/data transitions.
            _drain_nxscope_queue(subq)
            nxscope.ch_disable(NXSCOPE_DUMMY_CHAN, writenow=True)
            assert nxscope.dev_channel_get(NXSCOPE_DUMMY_CHAN).data.en is False
            _assert_no_nxscope_samples(subq, timeout_s=0.8)

            nxscope.ch_enable(NXSCOPE_DUMMY_CHAN, writenow=True)
            nxscope.ch_divider(NXSCOPE_DUMMY_CHAN, 1, writenow=True)
            assert nxscope.dev_channel_get(NXSCOPE_DUMMY_CHAN).data.en is True
            _wait_for_nxscope_value(subq, expected=None, timeout_s=2.0)

            ret = shell_set_io_scalar(product, runtime_dummy_id, 0x11)
            assert_set_ok(ret)

            sampled = _wait_for_nxscope_value(
                subq,
                expected=0x11,
                timeout_s=2.0,
            )
            assert sampled == 0x11

            ret = shell_set_io_scalar(product, runtime_dummy_id, 0x77)
            assert_set_ok(ret)

            sampled = _wait_for_nxscope_value(
                subq,
                expected=0x77,
                timeout_s=2.0,
            )
            assert sampled == 0x77
        finally:
            if subq is not None:
                nxscope.stream_unsub(subq)
            nxscope.disconnect()
            # Teardown should not fail due to shell prompt timing/transport
            # behavior. Data-path assertions above are the test objective.
            try:
                close_dawn_shell(product)
            except Exception:
                pass
