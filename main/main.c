// main/main.c - AB-731 pocket practice application entry point.
#include "ab731_app.h"
#include "bsp_battery.h"
#include "bsp_button.h"
#include "bsp_display.h"
#include "bsp_i2c.h"
#include "bsp_pins.h"
#include "esp_log.h"
#include "esp_sleep.h"
#include "esp_timer.h"
#include "idle_power.h"
#include "nvs_flash.h"
#include "passport_home.h"

#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"

#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>

static const char *TAG = "ab731_main";
#define IDLE_SLEEP_MS (5U * 60U * 1000U)

static bool s_ab731_active;
static volatile uint32_t s_last_activity_ms;
static bool s_accept_buttons;
static QueueHandle_t s_button_queue;

typedef struct {
    bsp_btn_t button;
    bsp_btn_ev_t event;
} button_message_t;

static uint32_t now_ms(void)
{
    return (uint32_t)(esp_timer_get_time() / 1000);
}

static void note_activity(void)
{
    s_last_activity_ms = now_ms();
}

static void enter_idle_sleep(void)
{
    if (bsp_button_any_pressed()) {
        note_activity();
        return;
    }

    esp_err_t result = esp_deep_sleep_enable_gpio_wakeup(
        1ULL << BSP_BTN_GPIO, ESP_GPIO_WAKEUP_GPIO_LOW);
    if (result != ESP_OK) {
        ESP_LOGE(TAG, "Could not arm button wake source: %s",
                 esp_err_to_name(result));
        note_activity();
        return;
    }

    if (bsp_lvgl_lock(1000)) {
        result = bsp_display_sleep();
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "LCD sleep returned: %s", esp_err_to_name(result));
        }
        bsp_display_backlight(0);
        bsp_lvgl_unlock();
    } else {
        bsp_display_backlight(0);
    }

    result = bsp_battery_sleep();
    if (result != ESP_OK && result != ESP_ERR_INVALID_STATE) {
        ESP_LOGW(TAG, "Battery-gauge sleep returned: %s",
                 esp_err_to_name(result));
    }
    result = bsp_button_deinit();
    if (result != ESP_OK) {
        ESP_LOGW(TAG, "Button shutdown returned: %s", esp_err_to_name(result));
    }
    ESP_LOGI(TAG, "Five minutes idle; entering deep sleep (any key wakes)");
    esp_deep_sleep_start();
}

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

// Button callbacks run in the esp_timer task and must never redraw LVGL. Queue
// an immutable event for app_main, which owns all navigation and screen work.
static void button_callback(bsp_btn_t button, bsp_btn_ev_t event, void *user)
{
    (void)user;
    button_message_t message = { .button = button, .event = event };
    if (s_button_queue != NULL) {
        (void)xQueueSend(s_button_queue, &message, 0);
    }
}

static void handle_key(const button_message_t *message)
{
    bsp_btn_t button = message->button;
    bsp_btn_ev_t event = message->event;
    note_activity();
    if (!s_accept_buttons) {
        return;
    }
    if (event == BSP_BTN_PRESS) {
        ESP_LOGI(TAG, "Button press classified as key=%d, ADC=%d mV",
                 (int)button, bsp_button_read_mv());
    }
    if (!bsp_lvgl_lock(250)) {
        return;
    }
    if (s_ab731_active) {
        if (ab731_app_key(button, event)) {
            s_ab731_active = false;
            passport_home_enter();
        }
    } else if (passport_home_key(button, event)) {
        passport_home_exit();
        s_ab731_active = true;
        ab731_app_enter();
    }
    bsp_lvgl_unlock();
}

void app_main(void)
{
    ESP_LOGI(TAG, "Starting AB-731 pocket practice");
    bool woke_from_button =
        esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_GPIO;
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
    s_button_queue = xQueueCreate(8, sizeof(button_message_t));
    if (s_button_queue == NULL) {
        ESP_LOGE(TAG, "Could not create button event queue");
        return;
    }
    ESP_ERROR_CHECK(bsp_button_init(button_callback, NULL));
    s_accept_buttons = !woke_from_button;
    note_activity();
    ESP_LOGI(TAG, "AI Passport home ready; AB-731 available as a sub-app");

    for (;;) {
        bool pressed = bsp_button_any_pressed();
        if (!s_accept_buttons && !pressed) {
            s_accept_buttons = true;
            note_activity();
        }

        button_message_t message;
        if (xQueueReceive(s_button_queue, &message,
                          pdMS_TO_TICKS(100)) == pdTRUE) {
            handle_key(&message);
        }
        if (s_accept_buttons &&
            idle_power_expired(s_last_activity_ms, now_ms(), IDLE_SLEEP_MS)) {
            enter_idle_sleep();
        }
    }
}
