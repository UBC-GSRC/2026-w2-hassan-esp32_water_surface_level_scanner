"""
This module is a high level abstraction of the water surface scanning system.
It integrates the CartLogger and ESP32Logger to coordinate data collection and processing. 
"""
import ESP32Logger
import CartLogger
import numpy as np

from pytransform3d import rotations as pr
from pytransform3d import transformations as pt
from pytransform3d.transform_manager import (
    TemporalTransformManager,
    NumpyTimeseriesTransform,
)


class WaterSurfaceScanner:
    def __init__(self):
        self._esp_logger = ESP32Logger.ESP32Logger(port="COM3")
        self._cart_logger = CartLogger.CartLogger()

        # Offsets are in meters and rotations are in radians
        self._cart2flume_x_offset = 0.0
        self._cart2flume_z_offset = 800.0 #mm

        self._sensor2cart_x_offset = 0.0
        self._sensor2cart_y_offset = 500.0 #mm 
        self._sensor2cart_z_offset = 0.0 
        self._sensor2cart_x_rotation = np.pi
        self._sensor2cart_y_rotation = 0.0
        self._sensor2cart_z_rotation = np.pi

        self.tm = TemporalTransformManager()

        self.transforms = {}

    def define_transforms(self,):
        """Abstraction to define the necessary transforms throughout time. This should be done after all the data is recorded as per pytransform3d's capabilities."""
        # Translation between the flume origin and the camera cart 

        cart_data = self._cart_logger.get_data()
        self.define_cart2flume_transform(cart_data[:,0], # timestamps
                                         cart_data[:,1], # y positions
                                         self._cart2flume_x_offset,
                                         self._cart2flume_z_offset
        )

        # Translation and rotation between the sensor and the camera cart
        artificial_sensor2cart_timestamps = np.linspace(0.0, np.max(self._cart_logger._timestamps), len(self._cart_logger._timestamps)) # there needs to be an artificial set of timestamps for the constant transform to keep NumpyTimeseriesTransform happy 
        self.define_sensor2cart_transform(artificial_sensor2cart_timestamps,
                                          self._sensor2cart_x_offset,
                                          self._sensor2cart_y_offset,
                                          self._sensor2cart_z_offset,
                                          self._sensor2cart_x_rotation, 
                                          self._sensor2cart_y_rotation,
                                          self._sensor2cart_z_rotation
        )

        # Translation between each distance measurement from the sensor to the sensor origin for each sensor zone
        sensor_data = self._esp_logger.get_data()
        self.define_measurement_i_2sensor_transforms(sensor_data)

    def define_cart2flume_transform(self, timestamps:np.array, y_positions:np.array, x_offset:float, z_offset:float):
        """The cart will have constant x and z offsets but the y position will vary over time."""
        # There is no rotation for any of the recorded datapoints
        R_const = pr.active_matrix_from_extrinsic_euler_zyx([0.0, 0.0, 0.0])

        # Create a position quaternion (pq) for each timestamp
        pqs = []
        for y_i in y_positions:
            T_i = pt.transform_from(R_const, np.array([x_offset, y_i, z_offset]))
            pqs.append(pt.pq_from_transform(T_i))

        self.transforms["cart2flume"] = NumpyTimeseriesTransform(timestamps, np.array(pqs))
        

    def define_sensor2cart_transform(self, timestamps:np.array, x_offset:float, y_offset:float, z_offset:float, x_rotation:float, y_rotation:float, z_rotation:float):
        """The sensor is a constant translation and rotation relative to the cart."""
        
        # first make a transform using a rotation matrix and translation vector
        sensor2cart_const = pt.transform_from(
            pr.active_matrix_from_intrinsic_euler_xyz([x_rotation, y_rotation, z_rotation]),
            np.array([x_offset, y_offset, z_offset]),
        )

        # then convert this transform into a position quaterion (pq) representation and repeat it for each timestamp
        pqs_sensor2cart = np.stack((pt.pq_from_transform(sensor2cart_const),) * len(timestamps), axis=0)

        self.transforms["sensor2cart"] = NumpyTimeseriesTransform(timestamps, pqs_sensor2cart)
         

    def define_measurement_i_2sensor_transforms(self, sensor_data: np.ndarray):
        """Each distance measurement will have a variable distance to the sensor origin. There will be 64 time varying transforms, one for each sensor zone."""
        
        for idx in range(64):
            data_filtered = sensor_data[sensor_data[:,1] == idx] # filter the data for the current index
            # TODO: There should be some form of error handling (filtering out bad readings) using the 'status' column to make sure the data is valid. I think there is going to be a lot of random bad readings
            self.define_measurement2sensor_transform(data_filtered[:,0], # timestamps
                                                     idx,
                                                     data_filtered[:,3], # x positions
                                                     data_filtered[:,4], # y positions
                                                     data_filtered[:,5]  # z positions
                                                     )

    def define_measurement2sensor_transform(self, timestamps:np.array, idx:int, x_positions:np.array, y_positions:np.array, z_positions:np.array):
        # There is no rotation for any of the recorded datapoints
        R_const = pr.active_matrix_from_extrinsic_euler_zyx([0.0, 0.0, 0.0])

        # Create a position quaternion (pq) for each timestamp
        # NOTE: it is possible that because the way the rotation matrices are defined, we may need to flip the sign of x values here. Will need to look at the data later.
        pqs = []
        for i in range(len(x_positions)):
            T_i = pt.transform_from(R_const, np.array([x_positions[i], y_positions[i], z_positions[i]]))
            pqs.append(pt.pq_from_transform(T_i))

        try:
            self.transforms[f"measurement{idx}2sensor"] = NumpyTimeseriesTransform(timestamps, np.array(pqs))
        except:
            self.transforms[f"measurement{idx}2sensor"] = None

    def start_recording(self,):
        """Start recording data from both the ESP32 sensor and the camera cart."""
        self._esp_logger.start_recording()
        self._cart_logger.start_recording()

    def stop_recording(self,):
        """Stop recording data from both the ESP32 sensor and the camera cart."""
        self._esp_logger.stop_recording()
        self._cart_logger.stop_recording()
    
    def add_transforms(self):
        """Create a TemporalTransformManager from the defined transforms."""
        self.tm.add_transform("cart", "flume", self.transforms["cart2flume"])
        self.tm.add_transform("sensor", "cart", self.transforms["sensor2cart"])
        for i in range(64): 
            if self.transforms[f"measurement{i}2sensor"] is not None:
                self.tm.add_transform(f"measurement{i}", "sensor", self.transforms[f'measurement{i}2sensor'])

    def get_water_surface_data(self,):
        """Get the transformed water surface data as a numpy array at the sensor recording timestamps."""
        origin_const = pt.vector_to_point([0.0, 0.0, 0.0]) # using the origin of the frame to transform
        timestamps_unique = np.unique(self._esp_logger.get_data()[:, 0])
        
        times = []
        idxs = []
        x_transformed = []
        y_transformed = []
        z_transformed = []

        # Iterate through each measurement zone
        for i in range(64):
            # Get all of the recording times for that measurement zone and transform them into the correct reference frame
            if self.transforms[f"measurement{i}2sensor"] is not None:
                timestamps = self.transforms[f'measurement{i}2sensor'].time
                for timestamp in timestamps:
                    if timestamp > self.transforms["cart2flume"].time[-1]: # ignore all data points from the sensor that came after the cart stopped moving
                        continue
                    elif len(timestamps) <= 1:
                        continue
                    elif timestamp == timestamps[-1]:
                        transform_i = self.tm.get_transform_at_time(f"measurement{i}", "flume", timestamp-1) # Weird error when you use the exact final timestamp... hopefully interpolation at 1 ms before works fine
                    else:
                        transform_i = self.tm.get_transform_at_time(f"measurement{i}", "flume", timestamp) # Use the recorded datapoint
                    pos = pt.transform(transform_i, origin_const)[:-1]
                    times.append(timestamp)
                    x_transformed.append(pos[0])
                    y_transformed.append(pos[1])
                    z_transformed.append(pos[2])
                    idxs.append(i)
        
        return np.hstack((
            np.array(times)[:, np.newaxis],
            np.array(idxs)[:, np.newaxis],
            np.array(x_transformed)[:, np.newaxis],
            np.array(y_transformed)[:, np.newaxis],
            np.array(z_transformed)[:, np.newaxis]
        ))

    def visualize_data(self,):
        """Use a 3d plot to visualize the recorded data."""
        pass

