// main/main.c - AB-731 pocket practice application entry point.
#include "ab731_app.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "passport_home.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "ab731_main";
static bool s_ab731_active;
/* Opening the launcher app on PRESS makes OK feel immediate. The button
 * component later emits CLICK for the same physical press, so consume that
 * one event instead of accidentally starting the quiz as well. */
static bool s_consume_entry_ok_click;

/* The Passport has no external real-time clock and this offline app does not
 * connect to Wi-Fi. Seed the software clock from the firmware build time so
 * the home screen can still show a useful local date and time after boot. */
static void initialize_offline_clock(void)
{
    time_t now = time(NULL);
    if (now >= 1704067200) {
        return;
    }

    static const char *months = "JanFebMarAprMayJunJulAugSepOctNovDec";
    char month_name[4] = { __DATE__[0], __DATE__[1], __DATE__[2], '\0' };
    const char *month = strstr(months, month_name);
    struct tm build_time = {
        .tm_year = atoi(__DATE__ + 7) - 1900,
        .tm_mon = month ? (int)((month - months) / 3) : 0,
        .tm_mday = atoi(__DATE__ + 4),
        .tm_hour = atoi(__TIME__),
        .tm_min = atoi(__TIME__ + 3),
        .tm_sec = atoi(__TIME__ + 6),
        .tm_isdst = -1,
    };

    setenv("TZ", "CST-8", 1);
    tzset();
    time_t compiled_at = mktime(&build_time);
    if (compiled_at > 0) {
        struct timeval value = { .tv_sec = compiled_at, .tv_usec = 0 };
        settimeofday(&value, NULL);
    }
}

// Button callbacks run in the button timer task. Keep them short and hold the
// LVGL lock while the application changes widgets.
static void on_key(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    if (!bsp_lvgl_lock(250)) {
        return;
    }
    if (s_ab731_active) {
        if (s_consume_entry_ok_click && button == BSP_BTN_OK) {
            if (event == BSP_BTN_CLICK) {
                s_consume_entry_ok_click = false;
                bsp_lvgl_unlock();
                return;
            }
            if (event == BSP_BTN_LONG) {
                s_consume_entry_ok_click = false;
            }
        }
        if (ab731_app_key(button, event)) {
            s_ab731_active = false;
            passport_home_enter();
        }
    } else if (passport_home_key(button, event)) {
        passport_home_exit();
        s_ab731_active = true;
        s_consume_entry_ok_click = event == BSP_BTN_PRESS;
        ab731_app_enter();
    }
    bsp_lvgl_unlock();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting AB-731 pocket practice");
    initialize_offline_clock();

    esp_err_t nvs_result = nvs_flash_init();
    if (nvs_result == ESP_ERR_NVS_NO_FREE_PAGES ||
        nvs_result == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        nvs_result = nvs_flash_init();
    }
    ESP_ERROR_CHECK(nvs_result);

    // I2C only serves the optional battery gauge here. A gauge failure must not
    // prevent the display-and-buttons quiz flow from starting.
    (void)bsp_i2c_init();
    if (bsp_display_init() != ESP_OK || bsp_lvgl_init() == NULL) {
        ESP_LOGE(TAG,
                 "Display/LVGL init failed (MOSI=%d SCLK=%d CS=%d DC=%d BL=%d)",
                 BSP_LCD_MOSI, BSP_LCD_SCLK, BSP_LCD_CS, BSP_LCD_DC, BSP_LCD_BL);
        return;
    }
    bsp_display_backlight(100);

    int battery_percent = -1;
    if (bsp_battery_init() == ESP_OK) {
        battery_percent = bsp_battery_soc();
    }

    if (!ab731_app_init(battery_percent)) {
        ESP_LOGE(TAG, "AB-731 application initialization failed");
        return;
    }
    if (!bsp_lvgl_lock(1000)) {
        ESP_LOGE(TAG, "Could not lock LVGL for passport home");
        return;
    }
    passport_home_init(battery_percent);
    passport_home_enter();
    bsp_lvgl_unlock();
    ESP_ERROR_CHECK(bsp_button_init(on_key, NULL));
    ESP_LOGI(TAG, "AI Passport home ready; AB-731 available as a sub-app");
}
