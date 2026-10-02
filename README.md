# Water Surface Scanner Electronics V1.0 
This system is made to create a system which can control modbus ultrasonic distance sensors and cameras using relays.  

## Testing

We ran this device on the 18m flume for a two month long experiment. Around a span of 120 experiment hours and every 15 minutes or so we have a scan. 
Manual measurements were coordinated to be taken at the same time the sensor took the scans for us to compare. Manual measurements are very difficult to take as the water oscillates multiple cm while you're trying to pick the right number. 

## Components
- Waveshare ESP32S3 Zero (data_bridge)
- Waveshare ESP32S3 ETH-8DI-8RO (sensor_controller)
- URM14 Ultrasonic Distance Sensor DFRobot
- TRS cables to trigger the shutter of a camera 
- 12V 750mA power supply

## Examples for ESP-IDF
There are a few basic apps which test small functionalities. These are not that useful once the final app is built, but this helped us make incremental upgrades to the main app along the way. 
- examples/

## Function

There are three computers involved in this system. The first is an ESP32S3 8 channel relay board (Sensor controller ESP32) connected directly to the distance sensor / cameras and sends data to the second ESP32. The second is another ESP32S3 (Data Bridge ESP32) which receives the data from the Data Acquisition ESP32. The Data Bridge uses a wired serial connection directly to the host computer. The data bridge is a convenient way to not need to wire another really long wire along the flume. Connecting directly to the sensor controller does not work in it's current state. There would need to be some better serial communication techniques implemented to make this work. 

### Data Acquisition ESP32

The data acquisiton esp32 reads data from the RS485 port when prompted to from the data bridge. It just reads the sensor and sends it back to the host when commanded to.

### Data Bridge ESP32

The data bridge esp32 controls the data acquisition board over ESPNOW. It basically just asks for data and gets data in return. 

### Camera Cart Computer

- ~host_python/main.py 
- This Python script is responsible for 
    1. Receiving data through a wired UART connection
    2. Transforming the data into a cartesian coordinate system relative to the sensor (see /docs/sensor_fov_description.pdf)
    3. Transforming the data into a new coordinate reference system relative to the flume (see /docs/water_surface_level_side_view.pdf)
    4. Tracking the cart position along the flume 
    5. Appending data to a .csv file 

### References

- [ESP-NOW two way communication](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/)