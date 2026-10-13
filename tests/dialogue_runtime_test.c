#undef NDEBUG
#include <assert.h>
#include <stdio.h>
#include <string.h>
/* Exercise the actual interpreter, including its private word buffer, while
 * capturing the renderer calls instead of needing an interactive conversation. */
#include "../src/talk.c"

static char output[4096];
static int styles[4096], length, column, charset;
void __wrap_ULTIMA_16ba_PrintChar(uint ch) {
    assert(length < (int)sizeof(output) - 1);
    output[length] = (char)ch;
    styles[length++] = charset;
    output[length] = 0;
    column = ch == '\n' ? 0 : column + 1;
}
void __wrap_ULTIMA_1c9e_SelectCharset(int value) { charset = value; }
int __wrap_ULTIMA_1f12_GetCurrentTextX(void) { return column; }
static void reset(void) {
    length = column = charset = 0;
    output[0] = 0;
    D_4aee = D_4aef = D_4af1 = D_4af3 = D_4af4 = D_4af5 = 0;
    D_4af0 = 255;
}
static void text(const char *s) {
    while (*s)
        assert(TALK_0f32_ProcessChar((byte)*s++ | 128) == 0);
}
int main(void) {
    for (int pattern = 0; pattern < 16; ++pattern) {
        for (int i = 0; i < 4; ++i)
            D_5d5e[5].time[i] = (byte)(pattern * 19 + i * 73);
        for (int hour = 0; hour < 256; ++hour)
            assert(IMPERA_GameSchedule((const uint8_t *)&D_5d5e[5], (uint8_t)hour) ==
                   NPC_12e0(5, (char)hour));
    }
    reset();
    text("Hi lastword.");
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    assert(!strcmp(output, "Hi lastword.") && D_4af1 == 0);
    reset();
    D_5888 = 10;
    text("complete");
    assert(TALK_0f32_ProcessChar(0x89) == 0);
    assert(!strcmp(output, "complete") && D_5888 == 11);
    text(" again");
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    assert(!strcmp(output, "complete again"));
    reset();
    text("old");
    assert(TALK_0f32_ProcessChar(0x8e) == 0);
    text("new");
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    assert(!strcmp(output, "oldnew"));
    for (int i = 0; i < 6; ++i)
        assert(styles[i] == (i < 3 ? 0 : 1));
    reset();
    text("1234567890 lastword");
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    assert(!strcmp(output, "1234567890 \nlastword"));
    reset();
    text("abcdefghijklmnopqrst");
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    char letters[32];
    int count = 0;
    for (int i = 0; i < length; ++i)
        if (output[i] != '\n')
            letters[count++] = output[i];
    letters[count] = 0;
    assert(!strcmp(letters, "abcdefghijklmnopqrst"));
    reset();
    D_5893_map_id = 13;
    D_bcdc = 5;
    memset(D_5bda, 0, sizeof(D_5bda));
    D_5888 = 10;
    text("before branch");
    assert(TALK_0f32_ProcessChar(0x8c) == 0);
    assert(!strcmp(output, "before branch"));
    assert(TALK_0f32_ProcessChar(0x89) == 0); /* Label operand, not another action. */
    assert(D_5888 == 10 && !D_4aee && !strcmp(output, "before branch"));
    reset();
    assert(TALK_0f32_ProcessChar(1) == 0);
    assert(TALK_0f32_ProcessChar(0x82) == 1);
    assert(!strcmp(output, D_24ea[0]));
    /* Workshop's mixed paragraphs are ordinary native text and quote bytes. */
    reset();
    TALK_04da_PrintQuoteMark();
    text("I speak.\"");
    TALK_04d2_PrintCr();
    TALK_04d2_PrintCr();
    text("He gestures.");
    TALK_04d2_PrintCr();
    TALK_04d2_PrintCr();
    text("\"Listen.\"");
    /* Normal spoken dispatch supplies another closing quote; the native guard suppresses it. */
    TALK_04da_PrintQuoteMark();
    TALK_04e2_PrintWord();
    assert(!strcmp(output, "\"I speak.\"\n\nHe gestures.\n\n\"Listen.\""));
    puts("PASS dialogue flushes literal/dictionary words before actions, preserves operands and "
         "rune style, and retains long words");
    return 0;
}
