/* Native Impera text rendering. Portable C; no SDL or Qt dependencies. */
#ifndef IMPERA_TEXT_WINDOW_H
#define IMPERA_TEXT_WINDOW_H
#include <stdint.h>
#include <stddef.h>
#ifdef __cplusplus
extern "C" {
#endif
typedef struct TextWindow {
    uint8_t left, top, right, bottom, currentX, currentY, colors, flags;
} TextWindow;
typedef struct ImperaText {
    TextWindow **window;
    uint8_t **font, *savedGlyph;
    uint16_t *advance, *underline, *center, *inverse;
    uint8_t *foreground, *background;
    int hercules, speedFix;
    void *host;
    void (*draw)(void *, uint8_t *, int, uint8_t, uint8_t, int, int);
    void (*clear)(void *, int, int, int, int, uint8_t);
    void (*scroll)(void *, int, int, int, int, int);
    void (*sleep)(void *, int);
} ImperaText;
void IMPERA_TextPrint(ImperaText *, uint16_t);
void IMPERA_TextString(ImperaText *, const char *);
void IMPERA_TextPosition(TextWindow *, int, int);
void IMPERA_TextBackspace(ImperaText *, int);
typedef struct ImperaWords {
    uint8_t *buffer, *length, *lastColumn;
    int matching;
    void *host;
    int (*column)(void *);
    void (*font)(void *, int);
    void (*print)(void *, uint16_t);
} ImperaWords;
void IMPERA_WordFlush(ImperaWords *);
void IMPERA_WordAppend(ImperaWords *, uint8_t);
typedef void (*ImperaPixel)(void *, int, int, uint8_t);
typedef void (*ImperaCopy)(void *, uint8_t *, const uint8_t *, size_t);
void IMPERA_TextGlyph(const uint8_t *, int, uint8_t, uint8_t, int, int, void *, ImperaPixel);
void IMPERA_TextScroll(uint8_t *, int, int, int, int, int, int, void *, ImperaCopy);
extern const uint32_t IMPERA_TextPalette[16];
#ifdef __cplusplus
}
#endif
#endif
