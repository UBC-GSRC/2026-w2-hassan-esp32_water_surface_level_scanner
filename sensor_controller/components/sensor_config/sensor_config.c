#include "sensor_config.h"

const uint16_t sensor_addresses[] = {
    0x01,
    0x02,
    0x03,
    0x04
};

const size_t sensor_count =
    sizeof(sensor_addresses) /
    sizeof(sensor_addresses[0]);