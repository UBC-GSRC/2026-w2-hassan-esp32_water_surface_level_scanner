"""
This module is to handle the data logging from the ESP32 device over serial communication. 
The data is expected to be valid JSON strings and will be parsed and recorded.
"""
import serial
import numpy as np
import threading
import json

class ESP32Logger:
    def __init__(self, port="COM1", baud_rate=115200):
        self._COM = port
        self._baud = baud_rate
        self._timestamps = []
        self._statuses = []
        self._index = []
        self._x = []
        self._y = []
        self._z = []
        self._flag_record = False
        self._thread = None
        self._start_timestamp = None
        
    def start_recording(self):
        self._flag_record = True
        self._thread = threading.Thread(target=self.thread_serial_monitor)
        self._thread.start()

    def thread_serial_monitor(self):
            try:
                with serial.Serial(self._COM, self._baud, timeout=1) as ser:
                    while self._flag_record:
                        line = ser.readline().decode().strip()
                        if line:
                            # print(line)
                            if line.startswith('{') and line.endswith('}'):
                                try:
                                    data_point = json.loads(str.replace(line, "'", '"'))
                                    self.record_data_point(data_point)
                                except json.JSONDecodeError as e:
                                    print(f"JSON decode error: {e}")
            except serial.SerialException as e:
                print(f"Serial exception: {e}")
            except Exception as e:
                print(f"Unexpected error: {e}")

    def stop_recording(self):
        self._flag_record = False
        self._thread.join()

    def get_data(self, filtered = True):
        data = np.hstack((
            np.array(self._timestamps)[:, np.newaxis],
            np.array(self._index)[:, np.newaxis],
            np.array(self._statuses)[:, np.newaxis],
            np.array(self._x)[:, np.newaxis],
            np.array(self._y)[:, np.newaxis],
            np.array(self._z)[:, np.newaxis]
        ))

        if filtered:
            return data[np.logical_and(data[:,2] != 255, data[:,5] != 0)]
        else:
            return data

    def record_data_point(self, data_point:dict):
        # Make sure the timestamps are relative to the start of recording

        if self._start_timestamp is None:
            self._start_timestamp = data_point["T"]
            data_point["T"] = 0
        else:
            data_point["T"] -= self._start_timestamp
            
        self._timestamps.append(data_point["T"])
        self._statuses.append(data_point["S"])
        self._index.append(data_point["I"])
        self._x.append(data_point["X"]) 
        self._y.append(data_point["Y"])
        self._z.append(data_point["Z"])


def main():
    logger = ESP32Logger(port="COM3", baud_rate=115200)
    logger.start_recording()
    input("Press Enter to stop recording...\n")
    logger.stop_recording()
    data = logger.get_data()
    print("Recorded data shape:", data.shape)
    print(data)

    

if __name__ == "__main__":
    main()