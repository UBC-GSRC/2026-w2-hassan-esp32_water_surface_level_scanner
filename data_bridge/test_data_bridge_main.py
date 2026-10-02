# For this to run, 
# 1. you need to have a link set up between this host computer and the data bridge via USB. 
# 2. The sensor controller needs to be powered on but does not need to be connected via usb to the host.
# 3. You also need to make sure that the data bridge and sensor controller's peer mac addresses are set up correctly. 
# If you haven't set up mac addresses, or need to find them, you can run this pytest and scroll through the terminal output and find the printed address. 
from pytest_embedded import Dut
import struct 

start_byte = b'\xAF'
sensor_ids = [1, 2, 3, 4]
struct_rule = "<BB??H"

def test_unity(dut: Dut):

    dut.expect("READY_SERIAL")

    print("pytest: Data bridge is ready.")

    # Send a packet to control the sensor controller for each sensor

    for sensor_id in sensor_ids:
        packet = struct.pack(
            struct_rule,
            0, # node_id
            sensor_id, # sensor_id
            1, # trigger_shutter
            1, # measure_distance
            0 # distance_mm
        )
        dut.write(start_byte)
        dut.write(packet)

        # Expect 
        match = dut.expect(rb"\xAF(.{6})")

        payload = match.group(1)
        assert len(payload) == 6

        node_id, sensor_id, camera_triggered, distance_measured, distance = struct.unpack(struct_rule, payload) 
        print("pytest: Received packet from sensor controller: node_id={}, sensor_id={}, camera_triggered={}, distance_measured={}, distance={}".format(node_id, sensor_id, camera_triggered, distance_measured, distance))

    
    print('pytest: All sensor packets sent and received successfully.')