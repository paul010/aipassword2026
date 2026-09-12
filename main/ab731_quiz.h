#pragma once

#include <stdbool.h>
#include <stdint.h>

#define AB731_QUESTION_COUNT 100
#define AB731_OPTION_COUNT 4
#define AB731_WRONG_WORD_COUNT ((AB731_QUESTION_COUNT + 31) / 32)

typedef enum {
    AB731_DOMAIN_VALUE = 0,
    AB731_DOMAIN_APPS,
    AB731_DOMAIN_ADOPTION,
} ab731_domain_t;

typedef struct {
    ab731_domain_t domain;
    const char *prompt;
    const char *options[AB731_OPTION_COUNT];
    uint8_t answer;
    const char *reason;
} ab731_question_t;

typedef struct {
    uint8_t order[AB731_QUESTION_COUNT];
    uint8_t order_count;
    uint8_t position;
    uint8_t selected;
    uint8_t session_correct;
    bool answered;
    bool last_correct;
} ab731_quiz_t;

const ab731_question_t *ab731_question_at(uint8_t index);
const char *ab731_domain_name(ab731_domain_t domain);
void ab731_quiz_start(ab731_quiz_t *quiz,
                      const uint32_t wrong_words[AB731_WRONG_WORD_COUNT],
                      bool mistakes_only);
void ab731_quiz_move_selection(ab731_quiz_t *quiz, int delta);
bool ab731_quiz_submit(ab731_quiz_t *quiz);
bool ab731_quiz_advance(ab731_quiz_t *quiz);
uint8_t ab731_quiz_current_index(const ab731_quiz_t *quiz);
bool ab731_quiz_is_complete(const ab731_quiz_t *quiz);
