# ntfc/tests/nimble_ots/test_ots.py
#
# SPDX-License-Identifier: Apache-2.0
#
"""NTFC tests for the NimBLE Object Transfer Service demo.

Board-agnostic. Any product flashed with a descriptor that exposes the
``nimble_ots_demo.yaml`` shape (GAP name ``dawn-ots`` and the two
seekable file IOs ``some_file_ro`` / ``some_file_rw``) is supported -
add a per-board ``ntfc/configs/<board>/nimble_ots/config.yaml`` and a
session entry referring to ``testpath: ntfc/tests/nimble_ots`` to run
this suite against another target.

The suite launches the dawn application in the background (``dawn &``)
so NSH stays alive for ``cat`` verification, then drives every
interesting OTS surface from the host:

  * read OTS Feature, Object Name/Type/Size/ID/Properties
    characteristics for both ``some_file_ro`` and ``some_file_rw``,
  * read the read-only file and check its initial content,
  * round-trip a write+read on ``some_file_rw`` several times with
    different payloads,
  * verify on-device via ``cat /tmp/some_file_rw.txt`` that the last
    payload landed.

Requires ``dawnpy-ble`` (``pip install -e tools/dawnpy-ble``) and a
host with BlueZ + L2CAP CoC support.
"""

from __future__ import annotations

import asyncio
import time

import pytest
from _ntfc_common import shell_cmd
from dawnpy_ble.services.ots import (
    PROP_READ,
    PROP_WRITE,
    RES_SUCCESS,
    OtsClient,
)

GAP_NAME = "dawn-ots"
RO_OBJECT = "some_file_ro"
RW_OBJECT = "some_file_rw"
RW_PATH = "/tmp/some_file_rw.txt"
INITIAL_CONTENT = b"initial data"
ADV_SETTLE_S = 3.0


def _run(coro):
    """Run an async coroutine synchronously inside a pytest test."""
    return asyncio.run(coro)


class TestNimbleOts:
    """End-to-end OTS service validation against any nimble_ots target."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_NIMBLE_OTS"),
    ]

    @pytest.fixture(scope="class", autouse=True)
    def _start_dawn(self):
        """Launch the dawn app in the background so NSH stays usable.

        ``dawn`` blocks until the BLE peripheral stops (forever in this
        demo), so we background it with ``&``. NSH then accepts further
        commands (``cat /tmp/some_file_rw.txt``) on the same session.
        Must be a fixture, not ``setup_class``, because
        ``pytest.products`` is only bound by the ntfc plugin once
        collection enters the per-product session.
        """
        product = pytest.products[0].core(0)
        product.sendCommand("dawn &", timeout=2)
        time.sleep(ADV_SETTLE_S)
        TestNimbleOts._product = product
        yield

    def test_feature_advertised(self):
        """OTS Feature characteristic advertises the supported opcodes."""

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                return await ots.read_feature()
            finally:
                await ots.disconnect()

        feat_oacp, feat_olcp = _run(go())
        # OACP: Read (bit4), Write (bit5), Truncation (bit7), Abort (bit9).
        assert feat_oacp & (1 << 4)
        assert feat_oacp & (1 << 5)
        assert feat_oacp & (1 << 9)
        # OLCP: Go To (bit0).
        assert feat_olcp & (1 << 0)

    def test_list_two_objects(self):
        """OLCP First/Next walks both demo objects with correct metadata."""

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                return await ots.list_objects()
            finally:
                await ots.disconnect()

        objs = _run(go())
        names = [o.name for o in objs]
        assert RO_OBJECT in names
        assert RW_OBJECT in names

        ro = next(o for o in objs if o.name == RO_OBJECT)
        rw = next(o for o in objs if o.name == RW_OBJECT)

        assert ro.props & PROP_READ
        assert not (ro.props & PROP_WRITE)
        assert rw.props & PROP_READ
        assert rw.props & PROP_WRITE

        assert ro.size_current == len(INITIAL_CONTENT)
        assert rw.size_current == len(INITIAL_CONTENT)

    def test_read_ro_file(self):
        """OACP Read on ``some_file_ro`` returns the seeded content."""

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                return await ots.read_object(RO_OBJECT)
            finally:
                await ots.disconnect()

        data = _run(go())
        assert data == INITIAL_CONTENT

    def test_read_rw_file_initial(self):
        """OACP Read on ``some_file_rw`` returns the seeded content."""

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                return await ots.read_object(RW_OBJECT)
            finally:
                await ots.disconnect()

        data = _run(go())
        assert data == INITIAL_CONTENT

    @pytest.mark.parametrize("iteration", range(4))
    def test_write_then_read_rw_loop(self, iteration: int):
        """Write a fresh payload to ``some_file_rw`` and read it back."""

        payload = f"ntfc-iter-{iteration:02d}-{int(time.time()) & 0xFFFF}".encode()

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                new_size = await ots.write_object(RW_OBJECT, payload)
                assert new_size == len(payload), (
                    f"server reports size={new_size}B after writing " f"{len(payload)}B"
                )
                return await ots.read_object(RW_OBJECT)
            finally:
                await ots.disconnect()

        readback = _run(go())
        assert readback == payload, (
            f"iteration {iteration}: read back {readback!r} " f"expected {payload!r}"
        )

    def test_write_then_uart_verify(self):
        """Write via OTS, then confirm the file content over the UART shell.

        Bridges the BLE write path with the on-device filesystem to make
        sure the bytes really hit the file in /tmp and were not just
        cached by the OTS service.
        """
        marker = b"ntfc-uart-marker"

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                await ots.write_object(RW_OBJECT, marker)
            finally:
                await ots.disconnect()

        _run(go())

        # Give the firmware a moment to drain L2CAP credits before reading.
        time.sleep(0.5)
        ret = shell_cmd(TestNimbleOts._product, f"cat {RW_PATH}", timeout=2)
        assert marker.decode() in ret.output, ret.output

    def test_write_to_ro_object_rejected(self):
        """OACP Write must be rejected on the read-only object."""

        async def go():
            ots = await OtsClient.from_name(GAP_NAME)
            try:
                meta = await ots.select_by_name(RO_OBJECT)
                assert meta is not None
                await ots.open_l2cap()
                try:
                    return await ots.oacp_write(0, 4, mode=0x02)
                finally:
                    await ots.close_l2cap()
            finally:
                await ots.disconnect()

        result = _run(go())
        assert result != RES_SUCCESS
