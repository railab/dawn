############################################################################
#
# SPDX-License-Identifier: Apache-2.0
#
############################################################################
"""NTFC integration tests for Wakaama LwM2M bootstrap support."""

import pytest
from _ntfc_common import start_dawn
from dawnpy_lwm2m import Lwm2mBootstrapServer, Lwm2mTestServer


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


class TestWakaamaBootstrap:
    """Validate no-security bootstrap followed by normal registration."""

    pytestmark = [
        pytest.mark.cmd_check("dawn_main"),
        pytest.mark.dep_config("CONFIG_DAWN_PROTO_WAKAAMA"),
        pytest.mark.dep_config("CONFIG_WAKAAMA_BOOTSTRAP"),
    ]

    def test_lwm2m_bootstrap_registration(self):
        """Test bootstrap provisions the server used for registration."""
        host = _server_host()
        with (
            Lwm2mTestServer(host=host, port=5683, timeout=2.0) as server,
            Lwm2mBootstrapServer(
                host=host,
                port=5685,
                final_host="127.0.0.1",
                final_port=5683,
                timeout=2.0,
            ) as bootstrap,
        ):
            start_dawn()

            request = bootstrap.wait_for_bootstrap(
                endpoint="ntfc-wakaama-bootstrap",
                timeout=15.0,
            )
            registration = server.wait_for_registration(
                endpoint="ntfc-wakaama-bootstrap",
                timeout=15.0,
            )

            assert request.endpoint == "ntfc-wakaama-bootstrap"
            assert "</3/0>" in registration.links
            assert "</1/1>" in registration.links

            assert server.read_path("/1/1/16", timeout=5.0) == b"1"
            assert server.read_path("/1/1/17", timeout=5.0) == b"5"
            assert server.read_path("/1/1/18", timeout=5.0) == b"60"
            server_discovery = server.discover_path("/1/1", timeout=5.0)
            assert b"</1/1/8>" in server_discovery
            assert b"</1/1/17>" in server_discovery
            server.write_path("/1/1/17", b"6", timeout=5.0)
            assert server.read_path("/1/1/17", timeout=5.0) == b"6"
