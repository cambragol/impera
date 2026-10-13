/* Differential oracle: frozen pre-extraction functions, compared after EVERY operation. */
#undef NDEBUG
#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include "text/text_window.h"
typedef uint8_t byte;
typedef uint16_t u16;
typedef uint16_t uint;
enum {
    CTRL_CHAR_CLEAR_WINDOW = 255,
    CTRL_CHAR_TOGGLE_UNDERLINE = 254,
    CTRL_CHAR_TOGGLE_INVERSION = 253,
    CTRL_CHAR_CENTER_TEXT = 252,
    CTRL_CHAR_UNCENTER_TEXT = 251,
    VDP_VIDEO_HERC = 3
};
typedef struct State {
    TextWindow window, *current;
    uint8_t fonts[2][3072], *font, saved[24], pixels[320 * 200], word[16], length, last;
    uint16_t advance, underline, center, inverse;
    uint8_t fg, bg;
    int fontIndex, sleeps, scrolls, pen;
} State;
static State old, shared;
static struct {
    int _52c8_videoDriverSelection;
} D_52ba_vdp;
#define D_539a_currentTextWindow old.current
#define D_5398_currentCharset old.font
#define D_53ea old.saved
#define D_538e old.advance
#define D_53a4_underline old.underline
#define D_53a6 old.center
#define D_53a8_inverse old.inverse
#define D_53aa_text_fg_color old.fg
#define D_53ab_text_bg_color old.bg
#define D_52da_pen_color old.pen
#define D_4af1 old.length
#define D_4af3 old.last
#define D_bce4 old.word
#define D_5386_current_text_window_idx 0
#define D_535e_textWindows (&old.window)
static const byte s_bitMask[8] = {128, 64, 32, 16, 8, 4, 2, 1};
static const byte s_colorTable[16] = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15};
static byte *g_linearEgaBuffer0 = old.pixels;
enum { loresWidth = 320 };
static int s_textPixelOffset, s_dirty;
enum { true = 1 };
static void TILESET_Copy(byte *dst, const byte *src, int count) {
    (void)dst;
    (void)src;
    (void)count;
}
static void GrPutPixel(int page, int x, int y, byte color) {
    (void)page;
    assert(x >= 0 && x < 320 && y >= 0 && y < 200);
    old.pixels[y * 320 + x] = color;
}
/* Frozen raster accesses the native page member. */
#define _52d8_page _52c8_videoDriverSelection
#include "native_graphics_reference.inc"
static void ULTIMA_1f77_ConvertCharCoordToPixel(TextWindow *w, int *x, int *y, int *r, int *b) {
    *x = w->left * 8;
    *y = w->top * 8;
    *r = w->right * 8 + 7;
    *b = w->bottom * 8 + 7;
}
static void ULTIMA_17f4_ApplyCharEffects(byte *, int);
static void DRV_2d(int color) { old.pen = color; }
static void DRV_3f(int x, int y, int r, int b, int xorMode) {
    (void)xorMode;
    for (int row = y; row <= b; ++row)
        memset(old.pixels + row * 320 + x, old.pen, (size_t)(r - x + 1));
}
static void DRV_5d(byte *font, int offset, byte fg, byte bg, int x, int y) {
    GRAP_BUF_PrintChar(font, offset, fg, bg, x, y);
}
static void DRV_27(int x, int y, int r, int b, int amount) {
    ++old.scrolls;
    GRAP_BUF_ScrollWindow(x, y, r, b, amount);
}
static void TIME_SleepMs(int ms) { old.sleeps += ms; }
#define ENABLE_SPEED_FIX 1
#include "native_text_reference.inc"
static int ULTIMA_1f12_GetCurrentTextX(void) { return old.window.currentX; }
static int ULTIMA_1cee_GetCurrentTextY(void) { return old.window.currentY; }
static void ULTIMA_1bf2_SetTextPosition(int x, int y) {
    if ((byte)x + old.window.left < 40 && (byte)y + old.window.top < 25) {
        old.window.currentX = (byte)x;
        old.window.currentY = (byte)y;
    }
}
static void ULTIMA_1c9e_SelectCharset(int index) {
    old.fontIndex = index;
    old.font = old.fonts[index];
}
#include "native_words_reference.inc"
#include "native_backspace_reference.inc"
#include "native_string_reference.inc"
static void pixel(void *host, int x, int y, byte color) {
    State *s = host;
    assert(x >= 0 && x < 320 && y >= 0 && y < 200);
    s->pixels[y * 320 + x] = color;
}
static void draw(void *host, byte *font, int offset, byte fg, byte bg, int x, int y) {
    IMPERA_TextGlyph(font, offset, fg, bg, x * 8, y * 8, host, pixel);
}
static void clear(void *host, int x, int y, int r, int b, byte color) {
    State *s = host;
    for (int row = y; row <= b; ++row)
        memset(s->pixels + row * 320 + x, color, (size_t)(r - x + 1));
}
static void scroll(void *host, int x, int y, int r, int b, int amount) {
    State *s = host;
    ++s->scrolls;
    IMPERA_TextScroll(s->pixels, 320, x, y, r, b, amount, NULL, NULL);
}
static void sleepMs(void *host, int ms) { ((State *)host)->sleeps += ms; }
static ImperaText text;
static ImperaWords words;
static int column(void *host) { return ((State *)host)->window.currentX; }
static void font(void *host, int index) {
    State *s = host;
    s->fontIndex = index;
    s->font = s->fonts[index];
}
static void print(void *host, uint16_t ch) {
    (void)host;
    IMPERA_TextPrint(&text, ch);
}
static unsigned comparisons;
static void equal(void) {
    assert(!memcmp(old.pixels, shared.pixels, sizeof(old.pixels)));
    assert(!memcmp(&old.window, &shared.window, sizeof(TextWindow)));
    assert(!memcmp(old.fonts, shared.fonts, sizeof(old.fonts)));
    assert(!memcmp(old.word, shared.word, 16));
    assert(old.length == shared.length && old.last == shared.last);
    assert(old.advance == shared.advance && old.underline == shared.underline);
    assert(old.inverse == shared.inverse && old.center == shared.center);
    assert(old.fontIndex == shared.fontIndex && old.sleeps == shared.sleeps &&
           old.scrolls == shared.scrolls);
    ++comparisons;
}
static void init(int row, int x) {
    memset(&old, 0, sizeof(old));
    memset(&shared, 0, sizeof(shared));
    old.window = shared.window = (TextWindow){24, 11, 39, 23, (byte)x, (byte)row, 0x1f, 0};
    old.current = &old.window;
    shared.current = &shared.window;
    old.font = old.fonts[0];
    shared.font = shared.fonts[0];
    old.fg = shared.fg = 15;
    old.bg = shared.bg = 1;
    old.advance = shared.advance = 1;
    for (int f = 0; f < 2; ++f)
        for (int i = 0; i < 3072; ++i)
            old.fonts[f][i] = shared.fonts[f][i] = (byte)(i * 37 + f * 53);
    memset(old.pixels, 3, sizeof(old.pixels));
    memset(shared.pixels, 3, sizeof(shared.pixels));
    text = (ImperaText){0};
    text.window = &shared.current;
    text.font = &shared.font;
    text.savedGlyph = shared.saved;
    text.advance = &shared.advance;
    text.underline = &shared.underline;
    text.center = &shared.center;
    text.inverse = &shared.inverse;
    text.foreground = &shared.fg;
    text.background = &shared.bg;
    text.speedFix = 1;
    text.host = &shared;
    text.draw = draw;
    text.clear = clear;
    text.scroll = scroll;
    text.sleep = sleepMs;
    words = (ImperaWords){0};
    words.buffer = shared.word;
    words.length = &shared.length;
    words.lastColumn = &shared.last;
    words.host = &shared;
    words.column = column;
    words.font = font;
    words.print = print;
}
static void character(uint16_t ch) {
    ULTIMA_16ba_PrintChar(ch);
    IMPERA_TextPrint(&text, ch);
    equal();
}
int main(void) {
    uint32_t random = 42;
    for (int row = 0; row < 13; ++row)
        for (int x = 0; x < 16; ++x) {
            init(row, x);
            for (int n = 0; n < 400; ++n) {
                random = random * 1664525u + 1013904223u;
                byte ch = (byte)(0x80 | ((random >> 8) & 127));
                if (n % 7 == 0)
                    ch = 0xa0;
                if (n % 13 == 0)
                    ch = 0x8a;
                TALK_0574_BuildOrPrintWord(ch);
                IMPERA_WordAppend(&words, ch);
                equal();
            }
            TALK_04e2_PrintWord();
            IMPERA_WordFlush(&words);
            equal();
            character(254);
            character('A');
            character(253);
            character('B');
            character(252);
            character('C');
            character(251);
            character('\r');
            character('\n');
            character(255);
            character('D');
            old.advance = shared.advance = 0;
            for (int i = 5; i < 9; ++i)
                character((uint16_t)i);
            old.advance = shared.advance = 1;
            ULTIMA_1fa0_Backspace(2);
            IMPERA_TextBackspace(&text, 2);
            equal();
            character('E');
            char prompt[] = "Your interest?\r\n: a very long word abcdefghijklmnopqrstuvwxyz";
            ULTIMA_1850_PrintString(prompt);
            IMPERA_TextString(&text, prompt);
            equal();
        }
    init(12, 15);
    for (int i = 0; i < 100; ++i)
        character('x');
    // Scroll in either direction preserves pixels outside the window.
    DRV_27(192, 88, 319, 191, 8);
    scroll(&shared, 192, 88, 319, 191, 8);
    equal();
    DRV_27(192, 88, 319, 191, -8);
    scroll(&shared, 192, 88, 319, 191, -8);
    equal();
    printf("PASS shared/native pixels, cursor, effects, buffer, fonts and delays: %u boundaries\n",
           comparisons);
    return 0;
}
