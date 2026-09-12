#include "ab731_app.h"

#include "ab731_quiz.h"
#include "bsp_display.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/queue.h"
#include "freertos/task.h"
#include "lvgl.h"
#include "nvs.h"
#include "ui_pixel.h"

#include <stdio.h>

#define STATS_VERSION 1
#define STORAGE_NAMESPACE "ab731"
#define STORAGE_KEY "stats"

typedef enum {
    PAGE_HOME = 0,
    PAGE_QUESTION,
    PAGE_FEEDBACK,
    PAGE_DONE,
} page_t;

typedef struct {
    uint8_t version;
    uint8_t last_question;
    uint16_t reserved;
    uint32_t total_answers;
    uint32_t correct_answers;
    uint32_t wrong_mask;
} persisted_stats_t;

static const char *TAG = "ab731_app";
static page_t s_page;
static bool s_mistakes_only;
static int s_battery_percent = -1;
static persisted_stats_t s_stats = { .version = STATS_VERSION };
static ab731_quiz_t s_quiz;
static lv_obj_t *s_screen;
static lv_obj_t *s_mascot;
static QueueHandle_t s_save_queue;

static void render_home(void);
static void render_question(void);
static void render_feedback(void);
static void render_done(void);

static unsigned wrong_count(uint32_t mask)
{
    unsigned count = 0;
    while (mask != 0) {
        count += mask & 1U;
        mask >>= 1;
    }
    return count;
}

static void add_battery(lv_obj_t *screen)
{
    if (s_battery_percent < 0) {
        return;
    }
    lv_obj_t *label = ui_pixel_label(screen, "", &lv_font_montserrat_14, UI_INK);
    lv_label_set_text_fmt(label, "%d%%", s_battery_percent);
    // The clear space below the title avoids the decorative top-right cloud.
    lv_obj_set_pos(label, 190, 32);
}

static lv_obj_t *new_screen(const char *title)
{
    s_screen = ui_pixel_screen_create(title);
    add_battery(s_screen);
    return s_screen;
}

static void replace_screen(lv_obj_t *screen)
{
    lv_obj_t *old_screen = lv_screen_active();
    lv_screen_load(screen);
    if (old_screen != screen) {
        lv_obj_delete(old_screen);
    }
}

static lv_obj_t *wrapped_label(lv_obj_t *parent, const char *text, int width,
                               const lv_font_t *font, uint32_t color)
{
    lv_obj_t *label = ui_pixel_label(parent, text, font, color);
    lv_obj_set_width(label, width);
    lv_label_set_long_mode(label, LV_LABEL_LONG_MODE_WRAP);
    lv_obj_set_style_text_line_space(label, 2, 0);
    return label;
}

// This worker owns all runtime NVS writes. The button callback only overwrites
// a one-element queue with the latest immutable snapshot.
static void save_worker(void *argument)
{
    (void)argument;
    persisted_stats_t pending;
    nvs_handle_t handle;
    if (nvs_open(STORAGE_NAMESPACE, NVS_READWRITE, &handle) != ESP_OK) {
        ESP_LOGW(TAG, "NVS namespace unavailable; progress will not persist");
        vTaskDelete(NULL);
        return;
    }
    while (xQueueReceive(s_save_queue, &pending, portMAX_DELAY) == pdTRUE) {
        esp_err_t result = nvs_set_blob(handle, STORAGE_KEY, &pending, sizeof(pending));
        if (result == ESP_OK) {
            result = nvs_commit(handle);
        }
        if (result != ESP_OK) {
            ESP_LOGW(TAG, "Progress save failed: %s", esp_err_to_name(result));
        }
    }
    nvs_close(handle);
    vTaskDelete(NULL);
}

static void queue_save(void)
{
    if (s_save_queue != NULL && xQueueOverwrite(s_save_queue, &s_stats) != pdTRUE) {
        ESP_LOGW(TAG, "Could not queue progress snapshot");
    }
}

static void load_stats(void)
{
    nvs_handle_t handle;
    size_t size = sizeof(s_stats);
    if (nvs_open(STORAGE_NAMESPACE, NVS_READONLY, &handle) != ESP_OK) {
        return;
    }
    persisted_stats_t loaded = { 0 };
    if (nvs_get_blob(handle, STORAGE_KEY, &loaded, &size) == ESP_OK &&
        size == sizeof(loaded) && loaded.version == STATS_VERSION) {
        s_stats = loaded;
    }
    nvs_close(handle);
}

static void show_no_mistakes(void)
{
    lv_obj_t *panel = ui_pixel_panel_create(s_screen, 16, 210, 208, 48, UI_YELLOW);
    lv_obj_t *label = wrapped_label(panel, "No saved mistakes yet", 184,
                                    &lv_font_montserrat_14, UI_INK);
    lv_obj_center(label);
}

static void start_selected_mode(void)
{
    ab731_quiz_start(&s_quiz, s_stats.wrong_mask, s_mistakes_only);
    if (ab731_quiz_is_complete(&s_quiz)) {
        show_no_mistakes();
        return;
    }
    s_page = PAGE_QUESTION;
    render_question();
}

static void render_home(void)
{
    s_page = PAGE_HOME;
    lv_obj_t *screen = new_screen("AB-731 EXAM");
    lv_obj_t *panel = ui_pixel_panel_create(screen, 14, 54, 212, 148, UI_PAPER);

    lv_obj_t *eyebrow = ui_pixel_label(panel, "POCKET PRACTICE", &lv_font_montserrat_14,
                                       UI_SKY_DARK);
    lv_obj_align(eyebrow, LV_ALIGN_TOP_MID, 0, 0);

    lv_obj_t *mode = ui_pixel_label(panel,
        s_mistakes_only ? "MODE  MISTAKES" : "MODE  ALL 18",
        &lv_font_montserrat_20, UI_INK);
    lv_obj_align(mode, LV_ALIGN_TOP_MID, 0, 28);

    unsigned accuracy = s_stats.total_answers == 0 ? 0 :
        (unsigned)((s_stats.correct_answers * 100U) / s_stats.total_answers);
    lv_obj_t *stats = ui_pixel_label(panel, "", &lv_font_montserrat_14, UI_INK);
    lv_label_set_text_fmt(stats, "Accuracy %u%%\nMistakes %u\n\nUP/DOWN: MODE\nOK: START",
                          accuracy, wrong_count(s_stats.wrong_mask));
    lv_obj_set_style_text_align(stats, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_align(stats, LV_ALIGN_TOP_MID, 0, 57);

    lv_obj_t *note = wrapped_label(screen, "Original study questions - not exam items",
                                   168, &lv_font_montserrat_14, UI_INK);
    lv_obj_set_style_text_align(note, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_set_pos(note, 36, 214);
    s_mascot = ui_pixel_mascot_create(screen, 101, 238);
    replace_screen(screen);
}

static void render_question(void)
{
    const ab731_question_t *question =
        ab731_question_at(ab731_quiz_current_index(&s_quiz));
    lv_obj_t *screen = new_screen("AB-731 EXAM");

    lv_obj_t *meta = ui_pixel_label(screen, "", &lv_font_montserrat_14, UI_INK);
    lv_label_set_text_fmt(meta, "Q %u/%u  %s", (unsigned)s_quiz.position + 1,
                          (unsigned)s_quiz.order_count,
                          ab731_domain_name(question->domain));
    lv_obj_set_pos(meta, 12, 45);

    lv_obj_t *question_panel = ui_pixel_panel_create(screen, 10, 63, 220, 84, UI_PAPER);
    lv_obj_t *prompt = wrapped_label(question_panel, question->prompt, 194,
                                     &lv_font_montserrat_14, UI_INK);
    lv_obj_align(prompt, LV_ALIGN_TOP_LEFT, 0, 0);

    // Keep every choice visible. UP/DOWN now moves one highlight instead of
    // forcing the learner to cycle through hidden answers to compare them.
    for (uint8_t index = 0; index < AB731_OPTION_COUNT; ++index) {
        bool selected = index == s_quiz.selected;
        lv_obj_t *answer_panel = ui_pixel_panel_create(
            screen, 10, 155 + index * 29, 220, 25,
            selected ? UI_YELLOW : UI_PAPER);
        lv_obj_set_style_border_width(answer_panel, selected ? 4 : 2, 0);
        lv_obj_set_style_pad_all(answer_panel, 2, 0);

        char option[160];
        snprintf(option, sizeof(option), "%s%c  %s", selected ? ">" : " ",
                 'A' + index, question->options[index]);
        lv_obj_t *answer = ui_pixel_label(answer_panel, option,
                                           &lv_font_montserrat_14, UI_INK);
        lv_obj_align(answer, LV_ALIGN_LEFT_MID, 1, 0);
    }

    lv_obj_t *help = ui_pixel_label(screen, "UP/DOWN: SELECT  OK: SUBMIT",
                                    &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(help, 9, 273);
    replace_screen(screen);
}

static void render_feedback(void)
{
    const ab731_question_t *question =
        ab731_question_at(ab731_quiz_current_index(&s_quiz));
    lv_obj_t *screen = new_screen(s_quiz.last_correct ? "CORRECT!" : "REVIEW");

    lv_obj_t *panel = ui_pixel_panel_create(screen, 12, 58, 216, 185,
                                             s_quiz.last_correct ? 0xD8F4A8 : 0xFFD3C9);
    char answer[160];
    snprintf(answer, sizeof(answer), "ANSWER %c\n%s", 'A' + question->answer,
             question->options[question->answer]);
    lv_obj_t *answer_label = wrapped_label(panel, answer, 188,
                                           &lv_font_montserrat_20, UI_INK);
    lv_obj_align(answer_label, LV_ALIGN_TOP_LEFT, 0, 0);

    lv_obj_t *reason = wrapped_label(panel, question->reason, 188,
                                     &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(reason, 0, 76);

    lv_obj_t *help = ui_pixel_label(screen, "OK: NEXT   HOLD OK: HOME",
                                    &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(help, 16, 258);
    s_mascot = ui_pixel_mascot_create(screen, 101, 238);
    replace_screen(screen);
}

static void render_done(void)
{
    s_page = PAGE_DONE;
    lv_obj_t *screen = new_screen("SESSION DONE");
    lv_obj_t *panel = ui_pixel_panel_create(screen, 20, 72, 200, 138, UI_PAPER);
    lv_obj_t *score = ui_pixel_label(panel, "", &lv_font_montserrat_20, UI_INK);
    lv_label_set_text_fmt(score, "%u / %u\n\n%u%%",
                          (unsigned)s_quiz.session_correct,
                          (unsigned)s_quiz.order_count,
                          s_quiz.order_count == 0 ? 0 :
                          (unsigned)(s_quiz.session_correct * 100U / s_quiz.order_count));
    lv_obj_set_style_text_align(score, LV_TEXT_ALIGN_CENTER, 0);
    lv_obj_center(score);
    lv_obj_t *help = ui_pixel_label(screen, "OK: HOME", &lv_font_montserrat_14, UI_INK);
    lv_obj_set_pos(help, 84, 226);
    s_mascot = ui_pixel_mascot_create(screen, 101, 238);
    replace_screen(screen);
}

bool ab731_app_init(int battery_percent)
{
    s_battery_percent = battery_percent;
    load_stats();
    s_save_queue = xQueueCreate(1, sizeof(persisted_stats_t));
    if (s_save_queue == NULL ||
        xTaskCreate(save_worker, "ab731_save", 3072, NULL, 3, NULL) != pdPASS) {
        ESP_LOGE(TAG, "Could not create progress persistence worker");
        return false;
    }
    return true;
}

void ab731_app_enter(void)
{
    render_home();
}

bool ab731_app_key(bsp_btn_t button, bsp_btn_ev_t event)
{
    if (button == BSP_BTN_OK && event == BSP_BTN_LONG) {
        return true;
    }
    if (event != BSP_BTN_CLICK) {
        return false;
    }

    if (s_page == PAGE_HOME) {
        if (button == BSP_BTN_UP || button == BSP_BTN_DOWN) {
            s_mistakes_only = !s_mistakes_only;
            render_home();
        } else if (button == BSP_BTN_OK) {
            start_selected_mode();
        }
        return false;
    }

    if (s_page == PAGE_QUESTION) {
        if (button == BSP_BTN_UP || button == BSP_BTN_DOWN) {
            ab731_quiz_move_selection(&s_quiz, button == BSP_BTN_UP ? -1 : 1);
            render_question();
        } else if (button == BSP_BTN_OK) {
            bool correct = ab731_quiz_submit(&s_quiz);
            uint8_t question_index = ab731_quiz_current_index(&s_quiz);
            ++s_stats.total_answers;
            if (correct) {
                ++s_stats.correct_answers;
                s_stats.wrong_mask &= ~(1UL << question_index);
            } else {
                s_stats.wrong_mask |= 1UL << question_index;
            }
            s_stats.last_question = question_index;
            queue_save();
            s_page = PAGE_FEEDBACK;
            render_feedback();
        }
        return false;
    }

    if (s_page == PAGE_FEEDBACK && button == BSP_BTN_OK) {
        if (ab731_quiz_advance(&s_quiz)) {
            s_page = PAGE_QUESTION;
            render_question();
        } else {
            render_done();
        }
        return false;
    }

    if (s_page == PAGE_DONE && button == BSP_BTN_OK) {
        render_home();
    }
    return false;
}
