#include "passport_nav.h"

#include <assert.h>
#include <stdio.h>

int main(void)
{
    passport_nav_t nav;
    passport_nav_reset(&nav);
    assert(nav.app_selected);
    assert(passport_nav_apply(&nav, PASSPORT_NAV_OK) == PASSPORT_NAV_OPEN_AB731);
    assert(passport_nav_apply(&nav, PASSPORT_NAV_UP) == PASSPORT_NAV_STAY);
    assert(passport_nav_apply(&nav, PASSPORT_NAV_DOWN) == PASSPORT_NAV_STAY);
    assert(nav.app_selected);
    puts("AI Passport launcher navigation tests: PASS");
    return 0;
}
