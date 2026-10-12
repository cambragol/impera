#include "text_window.h"
#include <string.h>
const uint32_t IMPERA_TextPalette[16] = {
    0xff000000, 0xff0000aa, 0xff00aa00, 0xff00aaaa, 0xffaa0000, 0xffaa00aa, 0xffaa5500, 0xffaaaaaa,
    0xff555555, 0xff5555ff, 0xff55ff55, 0xff55ffff, 0xffff5555, 0xffff55ff, 0xffffff55, 0xffffffff};
static void applyEffects(ImperaText *, uint8_t *, int);
static void pixelRect(TextWindow *w, int *x1, int *y1, int *x2, int *y2) {
    *x1 = w->left * 8;
    *y1 = w->top * 8;
    *x2 = w->right * 8 + 7;
    *y2 = w->bottom * 8 + 7;
}
// ASM
void IMPERA_TextPrint(ImperaText *s, uint16_t ch) {
    int iVar4;
    uint8_t bVar6;
    int uVar7;
    TextWindow *text_window;

    bVar6 = ch;
    text_window = (*s->window);
    if (ch > 0x7f) {
        if (ch == 255) // clear text window
        {
            int ax, bx, cx, dx;

            text_window->currentX = 0;
            text_window->currentY = 0;

            pixelRect(text_window, &ax, &bx, &cx, &dx);
            s->clear(s->host, ax, bx, cx, dx, (text_window->colors >> 4) & 15);

            return;
        }
        if (ch == 254) // toggle underline
        {
            (*s->underline) ^= 1;
            text_window->flags ^= 1; // underline
            return;
        }
        if (ch == 253) // toggle inversion
        {
            (*s->inverse) ^= 1;
            text_window->flags ^= 4; // inverse
            return;
        }
        if (ch == 252) // center text
        {
            (*s->center) = 1;
            text_window->flags |= 2;
            return;
        }
        if (ch == 251) // uncenter text
        {
            (*s->center) = 0;
            text_window->flags &= 0xfd;
            return;
        }
        ch &= 0x7f;
    }

    if (ch != 0xa) {
        if (ch == 0xd) {
            goto LAB_1000_1745;
        }

        // Render Character
        uVar7 = bVar6 * 8;
        if (s->hercules) {
            uVar7 = bVar6 * 24;
        }
        applyEffects(s, (*s->font), uVar7);

        // es = 5398
        // di = uVar7
        // dl = [(*s->foreground)]
        // dh = [(*s->background)]
        // al = [text_window + current_x] + [text_window + left]
        // ah = 0
        // bl = [text_window + current_y] + [text_window + top]
        // bh = 0
        s->draw(s->host, (*s->font), uVar7, (*s->foreground), (*s->background),
                text_window->currentX + text_window->left,
                text_window->currentY + text_window->top);

        iVar4 = 4;
        if (s->hercules) {
            iVar4 = 12;
        }

        memcpy((*s->font) + uVar7, s->savedGlyph, iVar4 * 2);

        text_window = (*s->window);
        if ((*s->advance) == 0) {
            return;
        }

        text_window->currentX++;
        if ((int8_t)(text_window->currentX + text_window->left) <= text_window->right) {
            return;
        }
    }

    text_window->currentY++;
    if (s->speedFix && s->sleep)
        s->sleep(s->host, 10);

LAB_1000_1745:
    text_window->currentX = 0;
    if (text_window->bottom < text_window->currentY + text_window->top) {
        int ax, bx, cx, dx;
        pixelRect(text_window, &ax, &bx, &cx, &dx);
        text_window->currentY--;
        if (s->speedFix && s->sleep)
            s->sleep(s->host, 10);
        s->scroll(s->host, ax, bx, cx, dx, -8);
    }
}

// ASM
// ptr = (es:)di
static void applyEffects(ImperaText *s, uint8_t *es, int di) {
    int size;
    uint8_t *ptr = es + di;

    size = 4;
    if (s->hercules) {
        size = 12;
    }

    memcpy(s->savedGlyph, ptr, size * 2);

    if ((*s->underline) != 0) {
        if (s->hercules) {
            // herc
            *(uint16_t *)&ptr[0x16] = 0xffff;
        } else {
            // ega/cga/t1k
            ptr[7] = 0xff;
        }
    }

    if ((*s->inverse) != 0) {
        size = 4;
        if (s->hercules) {
            // herc
            size = 12;
        }

        size *= 2;
        for (; size > 0; size--) {
            *ptr = ~*ptr;
            ptr++;
        }
    }
}

// CHECKED
void IMPERA_WordFlush(ImperaWords *s) {
    uint8_t local_6;
    int local_4;

    local_4 = 0;

    // c469
    if ((*s->length) != 0) {
        while (local_4 < (*s->length)) // c4c9
        {
            // c4d5 -> c47c
            s->font(s->host, (s->buffer[local_4] & 0x80) == 0);
            local_6 = s->buffer[local_4] & 0x7f;
            local_4++;

            if (s->column(s->host) != 0 || local_6 != 0x20) {
                // c49a
                if ((*s->lastColumn) < 0xf || local_6 != 10) {
                    // c4a7
                    if (local_6 != 10) {
                        (*s->lastColumn) = s->column(s->host);
                    }

                    // c4b3
                    s->print(s->host, local_6);
                } else if (local_6 == 10) // c4be
                {
                    (*s->lastColumn) = 0;
                }
            }
        }

        // c4e0
        (*s->length) = 0;
        s->font(s->host, 0);
    }
}

void IMPERA_WordAppend(ImperaWords *s, uint8_t param_1) {
    if (!s->matching) {
        /* Keep the incoming character when a long literal fills the buffer. */
        if ((*s->length) == 16)
            IMPERA_WordFlush(s);
    }
    if ((*s->length) != 0x10) {
        s->buffer[(*s->length)++] = param_1;

        if (param_1 != 0x8a && param_1 != 0xa0) // 0xa / 0x20
        {
            return;
        }

        if (s->column(s->host) + (uint16_t)(*s->length) >= 0x12) {
            s->print(s->host, '\n');
        }
    }

    IMPERA_WordFlush(s);
}

void IMPERA_TextPosition(TextWindow *w, int x, int y) {
    if ((uint8_t)x + w->left < 40 && (uint8_t)y + w->top < 25) {
        w->currentX = (uint8_t)x;
        w->currentY = (uint8_t)y;
    }
}
void IMPERA_TextString(ImperaText *s, const char *param_1) {
    int local_44;
    char local_42[40];
    uint16_t local_1a;
    int local_18;
    int local_16;
    int local_14;
    TextWindow *local_12;
    int local_10;
    int local_e;
    int local_c;
    int local_a;
    int local_8;
    int local_6;
    int local_4;

    // debug("GRAP_PrintString(%s)", param_1);

    local_8 = 0;
    local_a = 0;
    local_4 = 0;
    if (*param_1 != '\0') {
        // 1872
        local_12 = (*s->window);
        local_1a = local_12->flags & 2;
        local_44 =
            (uint16_t)local_12->right - (uint16_t)local_12->left; // text_window_width = r - l;

        // 189a
        do {
            if (local_4 != 0) {
                IMPERA_TextPrint(s, '\n');
            }

            // 18a7 (NOT MATCHING: si register)
            local_4 = local_6 = local_14 = 0;
            local_10 =
                local_44 - (*s->window)->currentX; // remaining_length = text_window_length - column

            for (local_c = 0; param_1[local_8] != '\n' && param_1[local_8] != '\r' &&
                              param_1[local_8] != '\0' && local_c <= local_10;
                 local_8++, local_c++) {
                local_42[local_c] = param_1[local_8];
            }

            // 18f4 (after loop)
            if (local_c == 0) {
                if (param_1[local_8] != '\0') {
                    IMPERA_TextPrint(s, (uint8_t)param_1[local_8++]);
                } else {
                    local_a++;
                }
            } else {
                // 191c
                if (local_c > local_10) {
                    // 1927
                    local_18 = local_c;
                    local_16 = local_8;

                    // 1939 (NOT MATCHING: optimization)
                    for (; param_1[local_8] != ' ' && local_c != 0 && param_1[local_8] != '\n' &&
                           param_1[local_8] != '\0' && param_1[local_8] != '\r';
                         local_c--, local_8--) {
                    }

                    // 1968
                    if (local_c == 0) {
                        local_c = local_18;
                        local_8 = local_16;

                        if (local_12->currentX != 0) {
                            IMPERA_TextPrint(s, '\n');
                            local_6 = 1;
                        }
                    } else {
                        // 1996
                        do {
                            local_c--;
                        } while (local_42[local_c] == ' ' || local_42[local_c] == '\r' ||
                                 local_42[local_c] == '\n');

                        if (param_1[local_8] != '\0')
                            local_8++;

                        // 19bf
                        if (local_c < local_10)
                            local_4++;
                    }

                    // 19ca
                    if (local_c > local_10)
                        local_c--;
                } else {
                    local_c--;
                }

                // 19d5
                if (local_1a != 0) {
                    if (local_c > local_44) {
                        IMPERA_TextPrint(s, '\n');
                    }

                    // 19ea
                    IMPERA_TextPosition(*s->window, (local_10 - local_c) / 2,
                                        (*s->window)->currentY);
                }

                // 19fd (NOT MATCHING: loop optimization)
                if (local_6 != 0) {
                    // 1a03
                    for (; local_42[local_14] == ' '; local_14++) {
                    }
                }

                // 1a14 (NOT MATCHING: loop optimization)
                for (; local_14 <= local_c; local_14++) {
                    // 1a1c
                    IMPERA_TextPrint(s, (uint8_t)local_42[local_14]);
                }
            }
        } while (local_a == 0);
    }
    return;
}

void IMPERA_TextBackspace(ImperaText *s, int count) {
    TextWindow *w = *s->window;
    if ((w->currentX || w->currentY) && count) {
        uint16_t advance = *s->advance;
        *s->advance = 0;
        for (int i = 0; i < count; ++i) {
            int x = w->currentX, y = w->currentY;
            IMPERA_TextPrint(s, ' ');
            x = x ? x - 1 : w->right - w->left;
            if (!w->currentX)
                --y;
            /* Same unsigned argument and screen bounds as SetTextPosition. */
            IMPERA_TextPosition(w, x, y);
        }
        *s->advance = advance;
    }
}
void IMPERA_TextGlyph(const uint8_t *font, int offset, uint8_t fg, uint8_t bg, int x, int y,
                      void *host, ImperaPixel pixel) {
    for (int row = 0; row < 8; ++row)
        for (int col = 0; col < 8; ++col)
            pixel(host, x + col, y + row, (font[offset + row] & (128 >> col)) ? fg : bg);
}
void IMPERA_TextScroll(uint8_t *pixels, int stride, int left, int top, int right, int bottom,
                       int amount, void *host, ImperaCopy copy) {
    int distance = amount < 0 ? -amount : amount, width = right - left + 1;
    for (int n = 0; n <= bottom - top; ++n) {
        int y = amount < 0 ? top + n : bottom - n;
        int source = amount < 0 ? y + distance : y - distance;
        uint8_t *dst = pixels + y * stride + left;
        const uint8_t *src =
            source >= top && source <= bottom ? pixels + source * stride + left : NULL;
        if (copy)
            copy(host, dst, src, (size_t)width);
        if (src)
            memcpy(dst, src, (size_t)width);
        else
            memset(dst, 0, (size_t)width);
    }
}
