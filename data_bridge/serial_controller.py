# This controller needs to do a few things
# 1. Send serial messages packaged in a struct for the data_bridge ESP32 to read and relay to data_acquisition
# 2. Listen to the serial messages from the data_bridge and get the data 
# 3. Be used by another program to perform a bedscan by calling all the functions in this controller
