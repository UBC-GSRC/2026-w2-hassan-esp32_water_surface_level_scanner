#pragma once

#include <stdbool.h>
#include <stdint.h>

void urm14_init(void);

uint16_t urm14_read_distance(void);

bool urm14_self_test(void);

bool urm14_set_address(uint16_t new_address);

void urm14_scan(void);