"""Two-board ESP-NOW test. Both DUTs run the SAME build.

Example:
  idf.py build
  pytest pytest_espnow_link.py --embedded-services esp,idf \
      --app-path ".|." --port "/dev/ttyACM0|/dev/ttyACM1"
(adjust to however you already point pytest-embedded at apps and ports)
"""
import pytest


@pytest.mark.parametrize('count', [2], indirect=True)
def test_espnow_link(dut):
    a, b = dut[0], dut[1]

    for d in (a, b):
        d.expect(r'ESPNOW_INIT_OK mac=', timeout=30)

    # Both boards enter the receive test; each beacons while listening
    for d in (a, b):
        d.expect_exact('READY_ESPNOW_RX', timeout=90)

    # Each must have heard the other
    for d in (a, b):
        d.expect(r'ESPNOW_RX peer_packets=(\d+)', timeout=60)

    # Unity summary on each board: 4 tests, 0 failures
    for d in (a, b):
        d.expect_unity_test_output(timeout=60)