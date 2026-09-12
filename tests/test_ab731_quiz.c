#include "ab731_quiz.h"

#include <assert.h>
#include <stdio.h>

static void test_all_questions(void)
{
    ab731_quiz_t quiz;
    uint32_t wrong_words[AB731_WRONG_WORD_COUNT] = { 0 };
    ab731_quiz_start(&quiz, wrong_words, false);
    assert(quiz.order_count == AB731_QUESTION_COUNT);
    assert(!ab731_quiz_is_complete(&quiz));

    ab731_quiz_move_selection(&quiz, -1);
    assert(quiz.selected == 3);
    ab731_quiz_move_selection(&quiz, 1);
    assert(quiz.selected == 0);
}

static void test_mistakes_filter(void)
{
    ab731_quiz_t quiz;
    uint32_t wrong_words[AB731_WRONG_WORD_COUNT] = { 0 };
    wrong_words[0] = (1UL << 2) | (1UL << 7);
    ab731_quiz_start(&quiz, wrong_words, true);
    assert(quiz.order_count == 2);
    assert(ab731_quiz_current_index(&quiz) == 2);
    quiz.selected = ab731_question_at(2)->answer;
    assert(ab731_quiz_submit(&quiz));
    assert(quiz.answered);
    assert(ab731_quiz_advance(&quiz));
    assert(ab731_quiz_current_index(&quiz) == 7);
}

static void test_completion_and_double_submit(void)
{
    ab731_quiz_t quiz;
    uint32_t wrong_words[AB731_WRONG_WORD_COUNT] = { 1UL };
    ab731_quiz_start(&quiz, wrong_words, true);
    const ab731_question_t *question = ab731_question_at(0);
    quiz.selected = question->answer;
    assert(ab731_quiz_submit(&quiz));
    assert(!ab731_quiz_submit(&quiz));
    assert(!ab731_quiz_advance(&quiz));
    assert(ab731_quiz_is_complete(&quiz));
    assert(quiz.session_correct == 1);
}

static void test_empty_mistakes(void)
{
    ab731_quiz_t quiz;
    uint32_t wrong_words[AB731_WRONG_WORD_COUNT] = { 0 };
    ab731_quiz_start(&quiz, wrong_words, true);
    assert(quiz.order_count == 0);
    assert(ab731_quiz_is_complete(&quiz));
    assert(!ab731_quiz_submit(&quiz));
}

static void test_mistakes_above_32(void)
{
    ab731_quiz_t quiz;
    uint32_t wrong_words[AB731_WRONG_WORD_COUNT] = { 0 };
    wrong_words[1] = 1UL << (37 - 32);
    wrong_words[3] = 1UL << (99 - 96);
    ab731_quiz_start(&quiz, wrong_words, true);
    assert(quiz.order_count == 2);
    assert(ab731_quiz_current_index(&quiz) == 37);
    quiz.selected = ab731_question_at(37)->answer;
    assert(ab731_quiz_submit(&quiz));
    assert(ab731_quiz_advance(&quiz));
    assert(ab731_quiz_current_index(&quiz) == 99);
}

static void test_question_bank_integrity(void)
{
    unsigned domain_counts[3] = { 0 };
    for (uint8_t index = 0; index < AB731_QUESTION_COUNT; ++index) {
        const ab731_question_t *question = ab731_question_at(index);
        assert(question != NULL);
        assert(question->domain <= AB731_DOMAIN_ADOPTION);
        assert(question->prompt != NULL && question->prompt[0] != '\0');
        assert(question->answer < AB731_OPTION_COUNT);
        assert(question->reason != NULL && question->reason[0] != '\0');
        for (uint8_t option = 0; option < AB731_OPTION_COUNT; ++option) {
            assert(question->options[option] != NULL);
            assert(question->options[option][0] != '\0');
        }
        ++domain_counts[question->domain];
    }
    assert(ab731_question_at(AB731_QUESTION_COUNT) == NULL);
    assert(domain_counts[AB731_DOMAIN_VALUE] == 40);
    assert(domain_counts[AB731_DOMAIN_APPS] == 38);
    assert(domain_counts[AB731_DOMAIN_ADOPTION] == 22);
}

int main(void)
{
    test_all_questions();
    test_mistakes_filter();
    test_completion_and_double_submit();
    test_empty_mistakes();
    test_mistakes_above_32();
    test_question_bank_integrity();
    puts("AB-731 quiz tests: PASS");
    return 0;
}
