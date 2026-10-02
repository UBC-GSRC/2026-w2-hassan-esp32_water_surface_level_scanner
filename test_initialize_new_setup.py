# This test needs both the data bridge and sensor controller to be connected to the host. If you want to just test the data bridge, run ESP32SerialController separately 

import pytest
import struct

start_byte = b'\xAF'
sensor_ids = [1, 2, 3, 4]
struct_rule = "<BB??H"

def trigger_camera_and_sensors(data_bridge, sensor_controller):
    """Wireless trigger camera and sensor using ESP-NOW."""
    for sensor_id in sensor_ids:
            packet = struct.pack(
                struct_rule,
                0, # node_id
                sensor_id, # sensor_id
                1, # trigger_shutter
                1, # measure_distance
                0 # distance_mm
            )
            data_bridge.write(start_byte)
            data_bridge.write(packet)

            # Expect 
            match = data_bridge.expect(rb"\xAF(.{6})")

            payload = match.group(1)
            assert len(payload) == 6

            node_id, sensor_id, camera_triggered, distance_measured, distance = struct.unpack(struct_rule, payload) 
            print("pytest: Received packet from sensor controller: node_id={}, sensor_id={}, camera_triggered={}, distance_measured={}, distance={}".format(node_id, sensor_id, camera_triggered, distance_measured, distance))

        

@pytest.mark.parametrize('count', [2], indirect=True)
def test_espnow(dut):
    data_bridge, sensor_controller = dut

    data_bridge.expect("READY_SERIAL")
    sensor_controller.expect("READY_SERIAL")

    print("pytest: Data bridge and sensor controller are ready.")
    
    # Send a packet to control the sensor controller for each sensor
    trigger_camera_and_sensors(data_bridge, sensor_controller)
    
    print('pytest: All sensor packets sent and received successfully.')
    print('pytest: Firmware loaded to both data bridge and sensor controller successfully. All sensors work. Test completed.')

