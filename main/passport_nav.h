#pragma once

#include <stdbool.h>

typedef enum {
    PASSPORT_NAV_UP = 0,
    PASSPORT_NAV_DOWN,
    PASSPORT_NAV_OK,
} passport_nav_input_t;

typedef enum {
    PASSPORT_NAV_STAY = 0,
    PASSPORT_NAV_REDRAW,
    PASSPORT_NAV_OPEN_AB731,
} passport_nav_action_t;

typedef struct {
    bool app_selected;
} passport_nav_t;

void passport_nav_reset(passport_nav_t *nav);
passport_nav_action_t passport_nav_apply(passport_nav_t *nav,
                                         passport_nav_input_t input);
