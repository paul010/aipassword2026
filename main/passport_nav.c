#include "passport_nav.h"

void passport_nav_reset(passport_nav_t *nav)
{
    /* There is one app, so one deliberate OK press opens it from home. */
    nav->app_selected = true;
}

passport_nav_action_t passport_nav_apply(passport_nav_t *nav,
                                         passport_nav_input_t input)
{
    if (input == PASSPORT_NAV_OK && nav->app_selected) {
        return PASSPORT_NAV_OPEN_AB731;
    }
    return PASSPORT_NAV_STAY;
}
