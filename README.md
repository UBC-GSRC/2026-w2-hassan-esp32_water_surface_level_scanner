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

## Getting Started

1. Install the python packages using 

1. Determine which COM port the ESP32's are on respectively
2. Compile data_bridge and sensor_controller apps
3. Run the commands in `pytest_command.txt` but change the COM ports
    - This is going to fail because the mac addresses aren't correct. Search the terminal output and write down the mac address of the both the data_bridge and sensor_controller
4. Update and compile data_bridge/main/main.c and sensor_controller/main/main.c with the corresponding peer mac addresses
5. Run the commands in `pytest_command.txt` and you should hear the relays fire and get good distance readings. 

## Setting Up Multiple URM14's

1. Get a USB to RS485 converter such as the DFRobot RainbowLink
2. Plug in the URM14 into A, B, 12V, GND
3. Run the python script `./sensor_controller/python_scripts/urm14_change_address_prompts.py`
4. Change the addresses of the URM14's to start at 1 and end at however many will be attached to a single bus. 
5. Update the sensor_addresses[] `./sensor_controller/components/urm14/urm14.c`
6. Recompile the sensor_controller app
7. Test everything works with the pytest functions in `pytest_command.txt`

## Using ESP32DataBridgeController.py

Use this class to abstract the communication protocol and get convenient python outputs. 

## Function

There are three computers involved in this system. The first is an ESP32S3 8 channel relay board (Sensor controller ESP32) connected directly to the distance sensor / cameras and sends data to the second ESP32. The second is another ESP32S3 (Data Bridge ESP32) which receives the data from the Data Acquisition ESP32. The Data Bridge uses a wired serial connection directly to the host computer. The data bridge is a convenient way to not need to wire another really long wire along the flume. Connecting directly to the sensor controller does not work in it's current state. There would need to be some better serial communication techniques implemented to make this work. 

### References

- [ESP-NOW two way communication](https://randomnerdtutorials.com/esp-now-two-way-communication-esp32/)