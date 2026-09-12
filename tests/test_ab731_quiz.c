#include "ab731_quiz.h"

#include <assert.h>
#include <stdio.h>

static void test_all_questions(void)
{
    ab731_quiz_t quiz;
    ab731_quiz_start(&quiz, 0, false);
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
    ab731_quiz_start(&quiz, (1UL << 2) | (1UL << 7), true);
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
    ab731_quiz_start(&quiz, 1UL, true);
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
    ab731_quiz_start(&quiz, 0, true);
    assert(quiz.order_count == 0);
    assert(ab731_quiz_is_complete(&quiz));
    assert(!ab731_quiz_submit(&quiz));
}

int main(void)
{
    test_all_questions();
    test_mistakes_filter();
    test_completion_and_double_submit();
    test_empty_mistakes();
    puts("AB-731 quiz tests: PASS");
    return 0;
}
