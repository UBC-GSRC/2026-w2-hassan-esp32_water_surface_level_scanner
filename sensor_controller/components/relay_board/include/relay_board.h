#pragma once

#include <stdbool.h>
#include <stdint.h>

bool relay_board_init(void);

bool relay_all_off(void);

bool relay_set(uint8_t relay, bool on);

bool relay_board_self_test(void);