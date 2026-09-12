#pragma once

#include "bsp_button.h"
#include <stdbool.h>

void passport_home_init(int battery_percent);
void passport_home_enter(void);
void passport_home_exit(void);
bool passport_home_key(bsp_btn_t button, bsp_btn_ev_t event);
