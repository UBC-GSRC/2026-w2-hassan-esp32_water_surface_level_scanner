#pragma once

#include <stdbool.h>
#include <stdint.h>

typedef struct
{
    uint16_t slave_addr;
    uint16_t control_register;
} urm14_t;

bool urm14_init(urm14_t *sensor);

uint16_t urm14_read_distance(urm14_t *sensor);

bool urm14_self_test(urm14_t *sensor);

bool urm14_set_address(
    urm14_t *sensor,
    uint16_t new_address);