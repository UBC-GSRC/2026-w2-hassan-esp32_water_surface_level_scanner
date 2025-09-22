"""
This module is to handle the data logging from the cart system's position.
"""
import threading
import CameraCart
import numpy as np
import time

class CartLogger:
    def __init__(self, sample_rate_hz = 10):
        self._cc = CameraCart.CameraCart()
        self._flag_record = False
        self._thread = None
        self._timestamps = []
        self._y_positions = []
        self._start_timestamp = None
        self.sample_rate_hz = sample_rate_hz

    def start_recording(self):
        self._flag_record = True
        self._thread = threading.Thread(target=self.thread_record_cart_position)
        self._thread.start()

    def thread_record_cart_position(self):
        while self._flag_record:
            query_time, position = self._cc.get_position()
            self.record_data_point(query_time, position)
            time.sleep(1 / self.sample_rate_hz)
           
    def stop_recording(self):
        self._flag_record = False
        self._thread.join()

    def get_data(self):
        return np.hstack((
            np.array(self._timestamps)[:, np.newaxis],
            np.array(self._y_positions)[:, np.newaxis]
        ))


    def record_data_point(self, timestamp, y_position):
        # Make sure the timestamps are relative to the start of recording
        if self._start_timestamp is None:
            self._start_timestamp = timestamp
            self._timestamps.append(0)
            self._y_positions.append(y_position)      
        else:
            self._timestamps.append(timestamp - self._start_timestamp)
            self._y_positions.append(y_position)


def main():
    cl = CartLogger()
    cl.start_recording()
    input("Press Enter to stop recording...")
    cl.stop_recording()
    data = cl.get_data()
    print("Recorded data shape:", data.shape)
    print(data)

if __name__ == "__main__":
    main()
