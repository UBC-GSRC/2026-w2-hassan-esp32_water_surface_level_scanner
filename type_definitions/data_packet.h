#pragma once
#include <stdint.h>

// Structure to pass data through ESP-NOW

typedef struct {
    uint8_t node_id;
    bool trigger_shutter;
    bool measure_distance; 
    uint16_t distance_mm; // value * 10 to preserve one decimal place
 } __attribute__((packed)) data_packet_t;