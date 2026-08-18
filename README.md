# Water Surface Scanner Electronics V1.0 
An 18m flume in the Flume Lab at UBC can benefit from having better water surface level data during experiments. The flume is already equipped with a Parker linear rail that traverses the length of the flume. This project is to avoid the mess of adding another wire connection to the cart on the linear rail. Another ESP32 is needed for ESP-NOW.

## Testing

We ran this device on the 18m flume for a two month long experiment. Around a span of 120 experiment hours and every 15 minutes or so we have a scan. 
Manual measurements were coordinated to be taken at the same time the sensor took the scans for us to compare. Manual measurements are very difficult to take as the water oscillates multiple cm while you're trying to pick the right number. 

## Components
- ESP32C5 x 2
- URM14 Ultrasonic Distance Sensor DFRobot
- RS485 Transceiver module Sparkfun
- 12V 750mA power supply
-

## Examples for ESP-IDF
- examples/

## Function

There are three computers involved in this system. The first computer is an ESP32C5 (Data Acquisition ESP32) connected directly to the distance sensor / cameras and sends data to the second ESP32. The second is another ESP32C5 (Data Bridge ESP32) which receives the data from the Data Acquisition ESP32. The Data Bridge uses a wired serial connection directly to the host computer. The data bridge is a convenient way to not need to wire another really long wire along the flume. The host computer (Camera Cart Computer) gets the data and uses Python to convert the data into the desired coordinate reference system.

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