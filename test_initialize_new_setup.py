# Two DUTs, same command you used before (bridge first, sensor controller second):
#   pytest --count 2 --port 'COM31|<SENSOR_PORT>' --embedded-services esp,idf \
#          --app-path '<bridge_app>|<sensor_app>' --skip-autoflash y
# dut[0] = data bridge, dut[1] = sensor controller. pytest-embedded owns both ports and
# the controller class talks to the bridge THROUGH dut[0], so nothing fights over COM31.
import pytest
import time
from data_bridge_controller.ESP32DataBridgeControllerDut import ESP32DataBridgeControllerDut

SENSOR_IDS = [1, 2, 3, 4]
MAX_COUNT = 500


@pytest.fixture
def bridge(dut):
    bridge_dut, _sensor_dut = dut
    return ESP32DataBridgeControllerDut(bridge_dut)


def test_read_sensors(dut, bridge):
    _bridge_dut, sensor_dut = dut
    sensor_dut.expect("READY", timeout=10)
    print("pytest: Sensor controller is ready.")

    assert bridge.trigger_camera()
    time.sleep(0.5)
    assert bridge.trigger_camera()

    count = 0
    while count < MAX_COUNT:
        for sensor_id in SENSOR_IDS:
            count += 1
            d = bridge.get_distance(sensor_id)
            print(f"pytest: {sensor_id} : {d} mm")
            assert d > 0, f"request {count}: sensor {sensor_id} returned {d}"

    print(f"pytest: {count} distance requests completed.")

    sensor_dut.expect("Triggering camera shutter", timeout=5)

    print("pytest: All sensor packets sent and received successfully.")
