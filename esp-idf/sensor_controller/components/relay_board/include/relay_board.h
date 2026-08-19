#pragma once

#include <stdbool.h>
#include <stdint.h>

void relay_board_init(void);

void relay_all_off(void);

void relay_set(uint8_t relay, bool on);

bool relay_board_self_test(void);