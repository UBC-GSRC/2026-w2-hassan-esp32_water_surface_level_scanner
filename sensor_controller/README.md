## Adding / Removing URM14 Sensors
1. Open urm14/urm14.c and change the hexaddresses of `sensor_addresses[]`. 
2. If you need to change the modbus address of a URM14 sensor, use python_scripts/urm14_change_address_prompts.py

## Testing

1. Open a Python terminal with the requirements from the main project directory.
2. Plug in the sensor controller to the host computer
3. run `pytest` in the Python terminal

