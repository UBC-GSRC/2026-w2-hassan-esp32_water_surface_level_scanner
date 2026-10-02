# This controller needs to do a few things
# 1. Send serial messages packaged in a struct for the data_bridge ESP32 to read and relay to data_acquisition
# 2. Listen to the serial messages from the data_bridge and get the data 
# 3. Be used by another program to perform a bedscan by calling all the functions in this controller
# Resources on struct packing & serial in python: https://alknemeyer.github.io/embedded-comms-with-python/

# From a higher level this controller will allow for 
# 1. request camera trigger returning an acknowledgement
# 2. request distance measurement returning a distance value 

# This means the data_bridge will listen to a UART, decode the message into a struct, and act accordingly
import serial
import struct
import time

NODE_ID_SELF = 0
NODE_ID_PEER = 1
START_BYTE = b'\xAF'

class ESP32DataBridgeController:
    def __init__(self,com = "COM4"):
        self.node_id = NODE_ID_SELF # see type_definition/data_packet.h for type definition
        self.struct_rule = '<BB??H' # 2 + 1 + 1 + 2 = 5 bytes: 2 unsigned char, 2 bools, 1 unsigned short see format character table https://docs.python.org/3/library/struct.html#format-characters
        self.buffer_size = 1024 
        
        try:
            self.port = serial.Serial(com, baudrate=115200, timeout=None)
            self.port.open()
            print("Successfully opened port")
        except serial.SerialException as e:
            print(f"Error opening serial port: {e}")

    def __del__(self):
        if self.port is not None:
            self.port.close()

    def trigger_camera(self)->bool:
        self.port.reset_input_buffer()

        msg = struct.pack(self.struct_rule, self.node_id, 0, True, False, 0)
        print(f"Writing: {msg.hex()}")
        self.port.write(START_BYTE)
        self.port.write(msg)
        time.sleep(0.1)

        # wait for response from data_bridge for camera trigger acknowledgement
        node_id, sensor_id, camera_triggered, distance_measured, distance = self.read_response()

        time.sleep(1) 

        return camera_triggered # return the camera trigger acknowledgement from the data_bridge
    
    def get_distance(self, sensor_id = 0)->int:
        self.port.reset_input_buffer()

        msg = struct.pack(self.struct_rule, self.node_id, sensor_id, False, True, 0)
        print(f"Writing: {msg.hex()}")
        self.port.write(START_BYTE)
        self.port.write(msg)
        time.sleep(0.1)

        # Clear any garbage data in the input buffer before reading the response

        node_id, sensor_id, camera_triggered, distance_measured, distance = self.read_response()

        time.sleep(0.1)

        if distance != 65535:
            return distance / 10 # decimal unpacking
        else:
            return -1 # error value for distance measurement failure

    def read_response(self):
        # wait for response from data_bridge for distance measurement
        res_bytes = self.port.read(1 + 6) # read 6 bytes for the response and 1 for start byte

        try:
            if (res_bytes[0] == START_BYTE[0]):
                node_id, sensor_id, camera_triggered, distance_measured, distance = struct.unpack(self.struct_rule, res_bytes[1:]) # Skip starting byte 
                print(res_bytes)
                print(f"len={len(res_bytes)} {' '.join(f'{b:02X}' for b in res_bytes)}")
                print(f"Received response - Node ID: {node_id}, Sensor ID: {sensor_id}, Camera Triggered: {camera_triggered}, Distance Measured: {distance_measured}, Distance: {distance} mm")
                return node_id, sensor_id, camera_triggered, distance_measured, distance
            else:
                print(f"Invalid start byte received: {res_bytes[0]}")
        except struct.error as e :
            print(f"Error unpacking serial data: {e}")
            print(f"Received bytes: {res_bytes}")
            return -1
def main():
    try:
        controller = ESP32DataBridgeController("COM31")
        # Example usage
        print("Press Enter to call function")

        urm14_sensors = [1, 2, 3, 4]
        while (1):
            input()
            camera_shutter = controller.trigger_camera()
            print(f"Camera shutter triggered: {camera_shutter}")

            for id in urm14_sensors:
                distance = controller.get_distance(sensor_id=id)
                print(f"Distance measured: {distance} mm")
                # time.sleep(0.05)
    except KeyboardInterrupt:
        if controller.port.is_open:
            controller.port.close()


if __name__ == "__main__":
    main()