#include "ab731_quiz.h"

#include <string.h>

// Original practice questions aligned with the Microsoft Learn AB-731 skills
// measured from July 22, 2026. They are study prompts, not live exam items.
static const ab731_question_t QUESTIONS[AB731_QUESTION_COUNT] = {
#include "ab731_questions.inc"
};

const ab731_question_t *ab731_question_at(uint8_t index)
{
    return index < AB731_QUESTION_COUNT ? &QUESTIONS[index] : NULL;
}

const char *ab731_domain_name(ab731_domain_t domain)
{
    static const char *const NAMES[] = { "VALUE", "MS AI APPS", "ADOPTION" };
    return domain <= AB731_DOMAIN_ADOPTION ? NAMES[domain] : "UNKNOWN";
}

void ab731_quiz_start(ab731_quiz_t *quiz,
                      const uint32_t wrong_words[AB731_WRONG_WORD_COUNT],
                      bool mistakes_only)
{
    memset(quiz, 0, sizeof(*quiz));
    for (uint8_t index = 0; index < AB731_QUESTION_COUNT; ++index) {
        bool marked_wrong = wrong_words != NULL &&
            (wrong_words[index / 32] & (1UL << (index % 32))) != 0;
        if (!mistakes_only || marked_wrong) {
            quiz->order[quiz->order_count++] = index;
        }
    }
}

void ab731_quiz_move_selection(ab731_quiz_t *quiz, int delta)
{
    if (quiz->answered || quiz->order_count == 0) {
        return;
    }
    int next = (int)quiz->selected + delta;
    while (next < 0) {
        next += AB731_OPTION_COUNT;
    }
    quiz->selected = (uint8_t)(next % AB731_OPTION_COUNT);
}

bool ab731_quiz_submit(ab731_quiz_t *quiz)
{
    if (quiz->answered || ab731_quiz_is_complete(quiz)) {
        return false;
    }
    const ab731_question_t *question =
        ab731_question_at(ab731_quiz_current_index(quiz));
    if (question == NULL) {
        return false;
    }
    quiz->answered = true;
    quiz->last_correct = quiz->selected == question->answer;
    if (quiz->last_correct) {
        ++quiz->session_correct;
    }
    return quiz->last_correct;
}

bool ab731_quiz_advance(ab731_quiz_t *quiz)
{
    if (!quiz->answered) {
        return false;
    }
    ++quiz->position;
    quiz->selected = 0;
    quiz->answered = false;
    quiz->last_correct = false;
    return !ab731_quiz_is_complete(quiz);
}

uint8_t ab731_quiz_current_index(const ab731_quiz_t *quiz)
{
    return quiz->position < quiz->order_count ? quiz->order[quiz->position] : 0;
}

bool ab731_quiz_is_complete(const ab731_quiz_t *quiz)
{
    return quiz->order_count == 0 || quiz->position >= quiz->order_count;
}
