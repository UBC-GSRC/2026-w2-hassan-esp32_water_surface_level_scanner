import struct 
import serial 
import time 

struct_rule = '<B??H' # 1 + 1 + 1 + 2 = 5 bytes: 1 unsigned char, 2 bools, 1 unsigned short see format character table https://docs.python.org/3/library/struct.html#format-characters
buffer_size = 1024 

def get_distance(self)->int:
    self.port.reset_input_buffer()

    msg = struct.pack(self.struct_rule, self.node_id, False, True, 0)
    self.port.write(msg)
    time.sleep(0.1)

    # Clear any garbage data in the input buffer before reading the response

    # wait for response from data_bridge for distance measurement
    res_bytes = self.port.read(5) # read 5 bytes for the response

    try:
        node_id, camera_triggered, distance_measured, distance = struct.unpack(self.struct_rule, res_bytes) 
        print(f"Received response - Node ID: {node_id}, Camera Triggered: {camera_triggered}, Distance Measured: {distance_measured}, Distance: {distance} mm")
    except struct.error as e :
        print(f"Error unpacking serial data: {e}")
        print(f"Received bytes: {res_bytes}")
        return -1

    time.sleep(0.1)

    if distance != 65535:
        return distance / 10 # decimal unpacking
    else:
        return -1 # error value for distance measurement failure