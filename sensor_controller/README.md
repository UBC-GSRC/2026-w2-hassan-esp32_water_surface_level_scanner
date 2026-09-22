## Adding / Removing URM14 Sensors

1. Open urm14/urm14.c and change the hexaddresses of `sensor_addresses[]`. 
2. If you need to change the modbus address of a URM14 sensor, use python_scripts/urm14_change_address_prompts.py

## Testing

1. Open a Python terminal with the requirements from the main project directory.
2. Plug in the sensor controller to the host computer
3. run `pytest` in the Python terminal

[!IMPORTANT]
If you are like me and barely work with firmware... and want to update/change test source code, make sure to change your ESP-IDF directory to each of the test apps (i.e. urm14/test_apps) and full clean / build after you make changes. 