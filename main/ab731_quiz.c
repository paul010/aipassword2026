#include "ab731_quiz.h"

#include <string.h>

// Original practice questions aligned with the Microsoft Learn AB-731 skills
// measured from July 22, 2026. They are study prompts, not live exam items.
static const ab731_question_t QUESTIONS[AB731_QUESTION_COUNT] = {
    { AB731_DOMAIN_VALUE,
      "A support bot must answer only from approved manuals. What helps most?",
      { "Higher temperature", "RAG grounding", "More colors", "No system prompt" },
      1, "RAG retrieves approved content and grounds the response." },
    { AB731_DOMAIN_VALUE,
      "Which factor commonly drives generative AI usage cost?",
      { "Office count", "Token volume", "Screen size", "Employee age" },
      1, "Input and output tokens are common usage-based cost drivers." },
    { AB731_DOMAIN_VALUE,
      "You predict invoice categories from labeled history. Best approach?",
      { "Supervised ML", "Image generation", "Random rules", "Text-to-speech" },
      0, "Labeled examples make this a supervised classification task." },
    { AB731_DOMAIN_VALUE,
      "When is a pretrained model the best first choice?",
      { "Common task, fast pilot", "Unique behavior required", "Fixed legal phrase", "Large labeled set" },
      0, "Start with a pretrained model when common capabilities meet the need." },
    { AB731_DOMAIN_VALUE,
      "A useful prompt should clearly state what?",
      { "Task and output format", "Only a broad topic", "Model internals", "Unrelated examples" },
      0, "Task, context, constraints, and output format improve consistency." },
    { AB731_DOMAIN_VALUE,
      "A training set underrepresents one customer group. Main risk?",
      { "Lower monitor brightness", "Biased performance", "Free tokens", "Stronger login" },
      1, "Poor representation can produce unfair or unreliable outcomes." },
    { AB731_DOMAIN_VALUE,
      "What is the best first ROI measure for an AI pilot?",
      { "Business outcome baseline", "Model name", "Logo quality", "Prompt length" },
      0, "Compare a measurable business outcome with a pre-pilot baseline." },
    { AB731_DOMAIN_APPS,
      "Leaders need a sourced report across files and the web. Choose?",
      { "Analyst", "Researcher", "Designer", "Clipchamp" },
      1, "Researcher supports multi-step, source-grounded research." },
    { AB731_DOMAIN_APPS,
      "Finance needs trends and explainable calculations from tables. Choose?",
      { "Analyst", "Researcher", "Designer", "Azure DNS" },
      0, "Analyst is designed for data reasoning and calculations." },
    { AB731_DOMAIN_APPS,
      "Build a low-code agent that calls workflows across channels. Use?",
      { "Paint", "Copilot Studio", "Intune", "Windows Backup" },
      1, "Copilot Studio builds and connects custom agents and actions." },
    { AB731_DOMAIN_APPS,
      "Which API accesses permitted Microsoft 365 people, mail, and files?",
      { "Microsoft Graph", "Azure Firewall", "Windows Registry", "PowerShell Gallery" },
      0, "Microsoft Graph is the unified API for Microsoft 365 data." },
    { AB731_DOMAIN_APPS,
      "Which service indexes enterprise content for a RAG solution?",
      { "Azure AI Search", "Azure DNS", "Intune", "Virtual Desktop" },
      0, "Azure AI Search provides indexing and retrieval for grounding." },
    { AB731_DOMAIN_APPS,
      "Copilot must query an internal order system. Best decision?",
      { "Extend with a connector", "Remove authentication", "Copy data by hand", "Train a base model" },
      0, "Extend the existing experience with governed data and actions." },
    { AB731_DOMAIN_APPS,
      "Why combine Microsoft AI services in one governed solution?",
      { "Remove all reviews", "Shared security controls", "Avoid business goals", "Hide usage" },
      1, "Integrated controls can improve security, safety, and governance." },
    { AB731_DOMAIN_ADOPTION,
      "Who oversees cross-functional AI strategy and major risks?",
      { "One intern", "AI council", "Only the vendor", "Each user alone" },
      1, "A cross-functional AI council aligns strategy and oversight." },
    { AB731_DOMAIN_ADOPTION,
      "What is the main role of an AI champions program?",
      { "Replace security", "Buy every license", "Help peers adopt", "Hide metrics" },
      2, "Champions share practices, help peers, and return feedback." },
    { AB731_DOMAIN_ADOPTION,
      "Copilot pilot usage is low. What should a leader do first?",
      { "Scale immediately", "Find adoption barriers", "Remove policies", "Raise temperature" },
      1, "Identify workflow, trust, skill, and data barriers before scaling." },
    { AB731_DOMAIN_ADOPTION,
      "Which is a Microsoft responsible AI principle?",
      { "Vendor lock-in", "Opacity", "Accountability", "Maximum automation" },
      2, "Accountability is one of Microsoft's responsible AI principles." },
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

void ab731_quiz_start(ab731_quiz_t *quiz, uint32_t wrong_mask,
                      bool mistakes_only)
{
    memset(quiz, 0, sizeof(*quiz));
    for (uint8_t index = 0; index < AB731_QUESTION_COUNT; ++index) {
        if (!mistakes_only || (wrong_mask & (1UL << index)) != 0) {
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
