#pragma once

#include "bsp_button.h"
#include <stdbool.h>

bool ab731_app_init(int battery_percent);
void ab731_app_enter(void);
bool ab731_app_key(bsp_btn_t button, bsp_btn_ev_t event);
