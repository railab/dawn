############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for Wakaama LwM2M client support."""

import base64
import time

import pytest
from _ntfc_common import start_dawn
from dawnpy_lwm2m import Lwm2mTestServer

WAKAAMA_IO_CHUNK_CAP = 2048


def _device_type():
    product = pytest.products[0]
    device = product.conf.cfg_core(0).device
    if device is not None:
        return device

    live_device = product.core(0).device
    name = live_device.name
    return name() if callable(name) else name


def _server_host():
    return "127.0.0.1" if _device_type() == "sim" else "0.0.0.0"


def _payload(size):
    """Build deterministic opaque payload data for transfer tests."""
    return bytes(ord("A") + (idx % 26) for idx in range(size))


def _read_opaque(server, path, timeout=10.0):
    """Read a text-format opaque resource and decode Wakaama base64."""
    return base64.b64decode(server.read_path(path, timeout=timeout), validate=True)


def _assert_opaque_roundtrip(server, path, payload, timeout=10.0):
    """Write and read back one opaque LwM2M resource."""
    server.write_path(path, payload, timeout=timeout)
    assert _read_opaque(server, path, timeout=timeout) == payload


def _wait_for_observed_payload(observation, expected, timeout=10.0):
    """Wait until an observation emits the expected payload."""
    deadline = time.monotonic() + timeout
    last = None
    while time.monotonic() < deadline:
        remaining = deadline - time.monotonic()
        try:
            last = observation.next_payload(timeout=max(remaining, 0.1))
        except TimeoutError:
            break
        if last == expected:
            return
    pytest.fail(f"Expected observed payload {expected!r}, last={last!r}")


class TestWakaama:
    """Validate LwM2M registration plus basic resource read/write flows."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_WAKAAMA"),
    ]

    def test_lwm2m_registration_and_resources(self):
        """Test Wakaama registers and exposes configured resources."""
        with (
            Lwm2mTestServer(host=_server_host(), port=5683, timeout=2.0) as server,
            Lwm2mTestServer(host=_server_host(), port=5684, timeout=2.0) as server2,
        ):
            start_dawn()

            registration = server.wait_for_registration(
                endpoint="ntfc-wakaama",
                timeout=15.0,
            )
            registration2 = server2.wait_for_registration(
                endpoint="ntfc-wakaama",
                timeout=15.0,
            )

            assert "</3/0>" in registration.links
            assert "</5/0>" in registration.links
            assert "</19/0>" in registration.links
            assert "</3200/0>" in registration.links
            assert "</3201/0>" in registration.links
            assert "</3202/0>" in registration.links
            assert "</3203/0>" in registration.links
            assert "</3300/0>" in registration.links
            assert "</3303/0>" in registration.links
            assert "</3304/0>" in registration.links
            assert "</3306/0>" in registration.links
            assert "</3311/0>" in registration.links
            assert "</33000/0>" in registration.links
            assert "</33000/0>" in registration2.links

            assert server.read_path("/3200/0/5500", timeout=5.0) == b"1"
            assert server2.read_path("/3200/0/5500", timeout=5.0) == b"1"

            assert server.read_path("/1/0/13", timeout=5.0) == b"0"
            assert server.read_path("/1/0/16", timeout=5.0) == b"1"
            assert server.read_path("/1/0/17", timeout=5.0) == b"5"
            assert server.read_path("/1/0/18", timeout=5.0) == b"60"
            assert server.read_path("/1/0/19", timeout=5.0) == b"86400"
            assert server.read_path("/1/0/20", timeout=5.0) == b"1"
            assert server.read_path("/1/0/22", timeout=5.0) == b"U"
            assert server.read_path("/1/0/23", timeout=5.0) == b"0"
            server_discovery = server.discover_path("/1/0", timeout=5.0)
            assert b"</1/0/8>" in server_discovery
            assert b"</1/0/17>" in server_discovery
            assert b"</1/0/22>" in server_discovery
            server.write_path("/1/0/2", b"3", timeout=5.0)
            server.write_path("/1/0/17", b"7", timeout=5.0)
            server.write_path("/1/0/23", b"1", timeout=5.0)
            assert server.read_path("/1/0/2", timeout=5.0) == b"3"
            assert server.read_path("/1/0/17", timeout=5.0) == b"7"
            assert server.read_path("/1/0/23", timeout=5.0) == b"1"
            assert server2.read_path("/1/1/17", timeout=5.0) == b"5"

            server.write_path("/3201/0/5550", b"1", timeout=5.0)
            assert server.read_path("/3201/0/5550", timeout=5.0) == b"1"

            analog_input = server.read_path("/3202/0/5600", timeout=5.0)
            assert float(analog_input.decode()) == pytest.approx(0.25)

            server.write_path("/3203/0/5650", b"0.5", timeout=5.0)
            analog_output = server.read_path("/3203/0/5650", timeout=5.0)
            assert float(analog_output.decode()) == pytest.approx(0.5)

            generic = server.read_path("/3300/0/5700", timeout=5.0)
            assert float(generic.decode()) == pytest.approx(12.25)

            temperature = server.read_path("/3303/0/5700", timeout=5.0)
            assert float(temperature.decode()) == pytest.approx(23.5)

            humidity = server.read_path("/3304/0/5700", timeout=5.0)
            assert float(humidity.decode()) == pytest.approx(48.0)

            server.write_path("/3306/0/5850", b"1", timeout=5.0)
            assert server.read_path("/3306/0/5850", timeout=5.0) == b"1"

            server.write_path("/3311/0/5851", b"80", timeout=5.0)
            assert server.read_path("/3311/0/5851", timeout=5.0) == b"80"

            with pytest.raises(RuntimeError, match="write /3311/0/5851 failed"):
                server.write_path("/3311/0/5851", b"300", timeout=5.0)
            assert server.read_path("/3311/0/5851", timeout=5.0) == b"80"

            server.write_path("/19/0/0", b"dawn-blob", timeout=5.0)
            blob = server.read_path("/19/0/0", timeout=5.0)
            assert base64.b64decode(blob) == b"dawn-blob"

            _assert_opaque_roundtrip(server, "/19/0/0", b"")
            _assert_opaque_roundtrip(server, "/19/0/0", _payload(1500))

            blob_boundary = _payload(WAKAAMA_IO_CHUNK_CAP)
            _assert_opaque_roundtrip(server, "/19/0/0", blob_boundary)

            with pytest.raises(RuntimeError, match="write /19/0/0 failed"):
                server.write_path(
                    "/19/0/0",
                    _payload(WAKAAMA_IO_CHUNK_CAP + 1),
                    timeout=10.0,
                )
            assert _read_opaque(server, "/19/0/0") == blob_boundary

            server.write_path("/5/0/0", b"dawn-fw", timeout=5.0)
            firmware = server.read_path("/5/0/0", timeout=5.0)
            assert base64.b64decode(firmware) == b"dawn-fw"
            _assert_opaque_roundtrip(server, "/5/0/0", _payload(1536))
            _assert_opaque_roundtrip(server, "/5/0/0", _payload(64))
            firmware_discovery = server.discover_path("/5/0", timeout=5.0)
            assert b"</5/0/2>" in firmware_discovery
            server.execute_path("/5/0/2", timeout=5.0)
            assert server.read_path("/5/0/3", timeout=5.0) == b"0"
            assert server.read_path("/5/0/5", timeout=5.0) == b"0"

            server.write_path("/33000/0/1", b"4321", timeout=5.0)
            assert server.read_path("/33000/0/1", timeout=5.0) == b"4321"

            counter = server.observe_path("/33000/0/1", timeout=5.0)
            counter2 = server2.observe_path("/33000/0/1", timeout=5.0)
            try:
                assert counter.next_payload(timeout=5.0) == b"4321"
                assert counter2.next_payload(timeout=5.0) == b"4321"
                server.write_path("/33000/0/1", b"8765", timeout=5.0)
                _wait_for_observed_payload(counter, b"8765", timeout=10.0)
                _wait_for_observed_payload(counter2, b"8765", timeout=10.0)
            finally:
                counter.cancel()
                counter2.cancel()
            assert server.read_path("/33000/0/1", timeout=5.0) == b"8765"
