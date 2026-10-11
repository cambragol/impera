#ifndef IMPERA_CREATION_SCENE_H
#define IMPERA_CREATION_SCENE_H
#include "tiles.h"
#include <stddef.h>
#include <string.h>
/* FONT_04a4 consumes the block at 0x2c0: four 0x80-byte 19x4
   backgrounds (stride 32), then bytecode at +0x200. No text opcode. */
static unsigned U5_CreationStepSize(unsigned op) {
    static const unsigned sizes[16] = {5, 2, 3, 2, 3, 1, 2, 2, 2, 1, 4, 3, 1, 3, 2, 1};
    return op < 16 ? sizes[op] : 1; /* Current engine ignores other single bytes. */
}
typedef struct U5CreationActor {
    int tile, x, y, initialized;
} U5CreationActor;
typedef struct U5CreationPreview {
    unsigned char terrain[76];
    U5CreationActor actors[32];
    int stage, unknown;
    unsigned ticks;
} U5CreationPreview;
/* Execute one restart cycle, or through the first execution of stopOffset.
   Operands and loop registers follow FONT_04a4; visual/audio effects remain coded.
   stopOffset is block-relative, not a relocated runtime pointer. */
static int U5_CreationPreview(const unsigned char *b, size_t size, unsigned stopOffset,
                              U5CreationPreview *state) {
    memset(state, 0, sizeof(*state));
    state->stage = -1;
    if (size <= 512 || size > 2000)
        return 0;
    size_t pc = 512, loop = 0;
    unsigned repeat = 0, iterations = 0;
    int gateX = -1, gateY = -1;
    static const int dx[4] = {0, 1, 0, -1}, dy[4] = {-1, 0, 1, 0};
    while (pc < size && ++iterations < 500000) {
        unsigned op = b[pc], length = U5_CreationStepSize(op);
        size_t next = pc + length;
        if (length > size - pc)
            return 0;
        unsigned a = length > 1 ? b[pc + 1] : 0, c = length > 2 ? b[pc + 2] : 0;
        U5CreationActor *actor = NULL;
        if (op == 0 || op == 1 || op == 2 || op == 7 || op == 8 || op == 13) {
            if (a >= 32)
                return 0;
            actor = &state->actors[a];
        }
        if (state->stage < 0 && op != 6 && op != 12 && op < 16)
            return 0;
        switch (op) {
        case 0:
            if (b[pc + 3] >= 19 || b[pc + 4] >= 4)
                return 0;
            actor->tile = c;
            actor->x = b[pc + 3];
            actor->y = b[pc + 4];
            actor->initialized = 1;
            break;
        case 1:
            if (!actor->initialized)
                return 0;
            actor->tile = 0;
            break;
        case 2:
        case 13:
            if (c >= 4 || !actor->initialized)
                return 0;
            actor->x += dx[c];
            actor->y += dy[c];
            if (actor->x < 0 || actor->x >= 19 || actor->y < 0 || actor->y >= 4)
                return 0;
            if (op == 13)
                state->ticks += 7;
            break;
        case 3:
            if (!a)
                return 0;
            state->ticks += a;
            break;
        case 4:
            if (a >= 19 || c >= 4)
                return 0;
            gateX = a;
            gateY = c;
            state->terrain[c * 19 + a] = TILE_MAP_MOONGATE;
            state->ticks += 17;
            break;
        case 5:
            if (gateX < 0)
                return 0;
            state->terrain[gateY * 19 + gateX] = TILE_MAP_GRASS;
            state->ticks += 17;
            break;
        case 6:
            if (a >= 4)
                return 0;
            state->stage = a;
            for (int y = 0; y < 4; ++y)
                memcpy(state->terrain + y * 19, b + a * 128 + y * 32, 19);
            break;
        case 7:
        case 8:
            if (!actor->initialized)
                return 0;
            break;
        case 9:
            return repeat == 0; /* Restart targets the first step, not an absolute address. */
        case 10:
            if (c >= 19 || b[pc + 3] >= 4)
                return 0;
            state->terrain[b[pc + 3] * 19 + c] = a;
            break;
        case 11:
            if (c >= 32 || !state->actors[c].initialized)
                return 0;
            /* First operand is skipped by engine. Retain it verbatim. */
            state->ticks += 8;
            break;
        case 12:
            for (int i = 0; i < 32; ++i)
                state->actors[i].tile = 0;
            break;
        case 14:
            if (!a || repeat)
                return 0;
            repeat = a;
            loop = next;
            break;
        case 15:
            if (!repeat)
                return 0;
            if (--repeat)
                next = loop;
            break;
        default:
            state->unknown = 1;
            break;
        }
        if (pc == stopOffset)
            return 1;
        pc = next;
    }
    return 0;
}
#endif
