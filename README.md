# esp32-sense-and-send
An 18m flume in the Flume Lab at UBC can benefit from having better water surface level data during experiments. The flume is already equipped with a Parker linear rail that traverses the length of the flume. This project is to avoid the mess of adding another wire connection to the cart on the linear rail. Another ESP32 is needed for ESP-NOW.

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

### Data Bridge ESP32

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