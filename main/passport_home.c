#include "passport_home.h"

#include "bsp_display.h"
#include "passport_nav.h"
#include "profile_images.h"
#include "ui_pixel.h"
#include "lvgl.h"

#include <time.h>

static int s_battery_percent = -1;
static passport_nav_t s_nav;
static lv_obj_t *s_clock_label;
static lv_timer_t *s_clock_timer;

static void replace_screen(lv_obj_t *screen)
{
    lv_obj_t *old_screen = lv_screen_active();
    lv_screen_load(screen);
    if (old_screen != screen) {
        lv_obj_delete(old_screen);
    }
}

static void add_battery(lv_obj_t *screen)
{
    if (s_battery_percent < 0) {
        return;
    }
    lv_obj_t *label = ui_pixel_label(screen, "", &lv_font_montserrat_14, UI_INK);
    lv_label_set_text_fmt(label, "%d%%", s_battery_percent);
    lv_obj_align(label, LV_ALIGN_TOP_RIGHT, -9, 47);
}

static void update_clock(lv_timer_t *timer)
{
    (void)timer;
    if (s_clock_label == NULL) {
        return;
    }
    time_t now = time(NULL);
    struct tm local;
    localtime_r(&now, &local);
    lv_label_set_text_fmt(s_clock_label, "%04d/%02d/%02d  %02d:%02d",
                          local.tm_year + 1900, local.tm_mon + 1, local.tm_mday,
                          local.tm_hour, local.tm_min);
}

static void stop_clock(void)
{
    if (s_clock_timer != NULL) {
        lv_timer_delete(s_clock_timer);
        s_clock_timer = NULL;
    }
    s_clock_label = NULL;
}

static lv_obj_t *add_framed_image(lv_obj_t *parent, const lv_image_dsc_t *source,
                                  int x, int y, int width, int height)
{
    lv_obj_t *shadow = lv_obj_create(parent);
    lv_obj_remove_flag(shadow, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_pos(shadow, x + 4, y + 5);
    lv_obj_set_size(shadow, width, height);
    lv_obj_set_style_radius(shadow, 0, 0);
    lv_obj_set_style_border_width(shadow, 0, 0);
    lv_obj_set_style_bg_color(shadow, lv_color_hex(UI_INK), 0);

    lv_obj_t *image = lv_image_create(parent);
    lv_image_set_src(image, source);
    lv_obj_set_pos(image, x, y);
    lv_obj_set_style_border_width(image, 3, 0);
    lv_obj_set_style_border_color(image, lv_color_hex(UI_INK), 0);
    return image;
}

static void render_home(void)
{
    stop_clock();
    lv_obj_t *screen = ui_pixel_screen_create("AI PASSPORT");
    add_battery(screen);

    s_clock_label = ui_pixel_label(screen, "", &lv_font_montserrat_12, UI_INK);
    lv_obj_set_pos(s_clock_label, 12, 48);
    update_clock(NULL);
    s_clock_timer = lv_timer_create(update_clock, 30000, NULL);

    /* Keep the requested left-right identity layout, but place every label on
     * the screen itself so no nested card can clip the copy. */
    add_framed_image(screen, &profile_avatar_image, 12, 68, 72, 96);

    lv_obj_t *name = ui_pixel_label(screen, "DALEI", &lv_font_montserrat_20, UI_INK);
    lv_obj_set_pos(name, 98, 77);
    lv_obj_t *role = ui_pixel_label(screen, "AI CREATOR", &lv_font_montserrat_14,
                                    UI_INK);
    lv_obj_set_pos(role, 98, 108);
    lv_obj_t *tagline = ui_pixel_label(screen, "LEARN + SHARE",
                                       &lv_font_montserrat_12, UI_INK);
    lv_obj_set_pos(tagline, 98, 136);

    lv_obj_t *apps_label = ui_pixel_label(screen, "MY APPS",
                                           &lv_font_montserrat_12, UI_INK);
    lv_obj_set_pos(apps_label, 15, 172);

    lv_obj_t *app = ui_pixel_panel_create(screen, 13, 190, 214, 88, UI_YELLOW);
    lv_obj_set_style_border_width(app, 4, 0);
    /* App labels also belong to the screen rather than the panel content box.
     * This keeps the full title and description visible on all LVGL themes. */
    lv_obj_t *app_title = ui_pixel_label(screen, "AB-731 EXAM",
                                         &lv_font_montserrat_20, UI_INK);
    lv_obj_align(app_title, LV_ALIGN_TOP_MID, 0, 200);
    lv_obj_t *app_subtitle = ui_pixel_label(screen, "PRACTICE + REVIEW",
                                            &lv_font_montserrat_14, UI_SKY_DARK);
    lv_obj_align(app_subtitle, LV_ALIGN_TOP_MID, 0, 230);
    lv_obj_t *open_label = ui_pixel_label(screen, "OK  OPEN",
                                           &lv_font_montserrat_12, UI_INK);
    lv_obj_align(open_label, LV_ALIGN_TOP_MID, 0, 258);
    replace_screen(screen);
}

void passport_home_init(int battery_percent)
{
    s_battery_percent = battery_percent;
    passport_nav_reset(&s_nav);
}

void passport_home_enter(void)
{
    passport_nav_reset(&s_nav);
    render_home();
}

void passport_home_exit(void)
{
    stop_clock();
}

bool passport_home_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (event != BSP_BTN_CLICK) {
        return false;
    }

    passport_nav_input_t input;
    if (button == BSP_BTN_UP) {
        input = PASSPORT_NAV_UP;
    } else if (button == BSP_BTN_DOWN) {
        input = PASSPORT_NAV_DOWN;
    } else if (button == BSP_BTN_OK) {
        input = PASSPORT_NAV_OK;
    } else {
        return false;
    }

    passport_nav_action_t action = passport_nav_apply(&s_nav, input);
    if (action == PASSPORT_NAV_REDRAW) {
        render_home();
    }
    return action == PASSPORT_NAV_OPEN_AB731;
}
