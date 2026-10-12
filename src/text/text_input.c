/* Native input, cursor and key normalization shared by engine and Workshop. */
#include "text_input.h"

#define U5_KEY_LEFT 1
#define U5_KEY_RIGHT 2
#define U5_KEY_UP 3
#define U5_KEY_DOWN 4
#define U5_KEY_BACKSPACE 8
#define U5_KEY_ENTER 13
#define U5_KEY_ESC 27
#define U5_KEY_HOME 211
#define U5_KEY_END 212
#define U5_KEY_PGUP 213
#define U5_KEY_PGDN 214

static int cancelled(ImperaInput *s) { return s->cancelled && s->cancelled(s->host); }
static int upper(uint8_t x) { return x >= 'a' && x <= 'z' ? x - 32 : x; }
int IMPERA_InputCursor(ImperaInput *s) {
    uint16_t saved = *s->advance;
    *s->advance = 0;
    s->print(s->host, s->cursorBase + (*s->cycle)++);
    int key = s->poll(s->host);
    if (*s->cycle >= s->cursorCount)
        *s->cycle = 0;
    if (key)
        s->print(s->host, ' ');
    else if (!cancelled(s))
        s->waitTicks(s->host, 1);
    *s->advance = saved;
    return key;
}
int IMPERA_InputChar(ImperaInput *s) {
    int local_4;

    if (s->driver == 2) {
        (*s->animate) = 0xff;
    }

    do {
        local_4 = upper(IMPERA_InputCursor(s));
        if ((local_4 == 0) && (s->map < 0x21 || 0x7f < s->map)) {
            s->frame(s->host);
        }
    } while (!cancelled(s) && local_4 == 0);

    if ((*s->numpad) != 0) {
        // numpad
        switch (local_4) {
        case 0x31:
            local_4 = U5_KEY_END;
            break;
        case 0x32:
            local_4 = U5_KEY_DOWN;
            break;
        case 0x33:
            local_4 = U5_KEY_PGDN;
            break;
        case 0x34:
            local_4 = U5_KEY_LEFT;
            break;
        case 0x36:
            local_4 = U5_KEY_RIGHT;
            break;
        case 0x37:
            local_4 = U5_KEY_HOME;
            break;
        case 0x38:
            local_4 = U5_KEY_UP;
            break;
        case 0x39:
            local_4 = U5_KEY_PGUP;
        }
    } else {
        if (local_4 < 5)
            local_4 += 0xfa;
    }

    return local_4;
}

void IMPERA_InputString(ImperaInput *s, char *param_1, int param_2) {
    int local_8;
    int local_6;
    int local_4;

    local_4 = 0; // di
    local_8 = (*s->buffering);
    (*s->buffering) = 0;
    do {
        local_6 = s->readChar ? s->readChar(s->host) : IMPERA_InputChar(s);
        if ((local_6 == U5_KEY_BACKSPACE || local_6 == U5_KEY_LEFT) && local_4 != 0) {
            // 3b43
            s->backspace(s->host, 1);
            local_4--;
        } else if ((local_6 == U5_KEY_ESC) && (local_4 != 0)) {
            s->backspace(s->host, local_4);
            local_4 = 0;
        } else if (0x1f < local_6 && local_6 < 0x80 && local_4 < param_2) {
            param_1[local_4++] = (char)local_6;
            s->print(s->host, local_6);
        }
    } while (!cancelled(s) && local_6 != U5_KEY_ENTER);
    param_1[local_4] = 0;
    (*s->buffering) = local_8;
}
