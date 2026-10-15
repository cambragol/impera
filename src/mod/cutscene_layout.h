/* Shared vanilla page layouts, taken from the introduction/ending tables in vars.c. */
#ifndef IMPERA_CUTSCENE_LAYOUT_H
#define IMPERA_CUTSCENE_LAYOUT_H
#include "cutscene_placement.h"
#include <stddef.h>
#include <string.h>
typedef struct {
    int left[2], right[2], top, bottom, x, y;
    int sheet, frame, imageX, imageY, effect, offset;
} U5CutsceneLayout;
static const U5CutsceneLayout U5CutsceneLayouts[2][21] = {
    {
        {{180, 0}, {320, 320}, 180, 200, 180, 128, 0, 0, 0, 0, 1, 0},
        {{0, 172}, {320, 320}, 70, 200, 0, 0, 0, 1, 0, 74, 2, 273},
        {{0, 0}, {132, 320}, 131, 200, 0, 40, 1, 0, 136, 0, 0, 971},
        {{0, 210}, {320, 320}, 32, 160, 0, 0, 1, 1, 0, 38, 0, 1424},
        {{0, 0}, {320, 148}, 70, 200, 0, 9, 1, 2, 152, 76, 0, 2051},
        {{176, 0}, {320, 320}, 133, 200, 176, 0, 1, 2, 0, 0, 0, 2756},
        {{0, 0}, {320, 320}, 200, 200, 32, 9, 1, 2, 72, 38, 3, 0},
        {{188, 0}, {320, 320}, 168, 200, 188, 136, 2, 0, 0, 0, 1, 3437},
        {{0, 0}, {320, 320}, 200, 200, 0, 0, 2, 1, 0, 82, 0, 3619},
        {{0, 0}, {320, 320}, 200, 200, 0, 0, 3, 0, 0, 82, 0, 4022},
        {{0, 0}, {320, 320}, 200, 200, 0, 0, 3, 1, 0, 82, 0, 4467},
        {{0, 0}, {320, 320}, 200, 200, 0, 0, 4, 0, 0, 82, 0, 4906},
        {{0, 0}, {320, 320}, 200, 200, 0, 0, 4, 1, 0, 82, 0, 5310},
        {{0, 0}, {170, 320}, 114, 200, 0, 0, 5, 0, 176, 0, 0, 5678},
        {{184, 0}, {320, 320}, 114, 200, 184, 32, 5, 1, 0, 0, 1, 6443},
        {{0, 0}, {170, 320}, 96, 200, 0, 0, 5, 2, 176, 0, 4, 7066},
        {{0, 148}, {320, 320}, 33, 137, 0, 0, 5, 6, 0, 46, 5, 7803},
        {{0, 0}, {320, 170}, 70, 200, 0, 0, 5, 4, 176, 78, 6, 8562},
        {{148, 0}, {320, 320}, 96, 200, 174, 0, 5, 2, 0, 0, 5, 9291},
        {{0, 0}, {320, 170}, 51, 146, 0, 9, 5, 6, 176, 55, 6, 10112},
        {{0, 156}, {320, 320}, 79, 200, 0, 0, 5, 4, 0, 87, 4, 10903},
    },
    {
        {{172, 0}, {320, 320}, 126, 200, 172, 66, 0, 0, 0, 0, 1, 0},
        {{0, 0}, {320, 320}, 126, 200, 0, 92, 0, 1, 64, 0, 0, 424},
        {{0, 196}, {320, 320}, 42, 148, 0, 9, 0, 2, 0, 52, 0, 956},
        {{179, 0}, {320, 320}, 100, 200, 179, 38, 1, 0, 0, 0, 1, 1530},
        {{0, 161}, {320, 320}, 82, 200, 0, 9, 1, 1, 0, 92, 0, 2280},
        {{0, 0}, {154, 320}, 112, 200, 0, 0, 1, 2, 160, 0, 0, 2932},
    },
};
static U5CutscenePlacement U5_DefaultCutscenePlacement(int scene, unsigned page) {
    const U5CutsceneLayout *l = &U5CutsceneLayouts[scene][page];
    U5CutscenePlacement p = {0};
    p.imageX = l->imageX;
    p.imageY = l->imageY;
    for (int i = 0; i < 2; i++) {
        p.left[i] = l->left[i];
        p.right[i] = l->right[i];
    }
    p.top = l->top;
    p.bottom = l->bottom;
    p.x = l->x;
    p.y = l->y;
    p.secondY = 180;
    return p;
}
static const unsigned char U5CutsceneWidths[90] = {
    2, 6, 7, 7, 7, 7, 3, 4, 4, 5, 6, 3, 3, 2, 7, 6, 5, 6, 6, 6, 6, 6, 6, 6, 6, 2, 3, 5, 5, 5,
    6, 7, 7, 6, 6, 6, 6, 6, 6, 6, 4, 7, 7, 6, 7, 6, 6, 7, 6, 7, 6, 6, 6, 6, 7, 6, 6, 6, 3, 7,
    3, 7, 8, 3, 5, 5, 4, 5, 5, 5, 5, 5, 2, 3, 5, 2, 7, 6, 5, 5, 5, 4, 4, 4, 5, 5, 7, 5, 5, 4};
/* The engine's FONT_0000 wrapping, justification, indents and discretionary hyphens.
 * A callback keeps native game drawing and Workshop preview on identical positions. */
static void U5_DrawCutsceneTextKernel(const U5CutsceneLayout *layout, const char *param_2, int space, int drawBottom, int boxed,
                                void (*draw)(void *, int, int, int), void *context) {
    int x = layout->x, y = layout->y;
    int local_12;
    int local_10;
    int local_e;
    int local_c;
    int local_a;
    int local_8;
    int local_4;
    int local_6;

    local_12 = ((int)y > (int)layout->top && (int)y < (int)layout->bottom) ? 1 : 0;

    local_c = layout->right[local_12] - layout->left[local_12];

    local_e = local_6 = 0;
    local_4 = x - layout->left[local_12];

    while (param_2[local_e] != '\0') {
        int splitWord=0;
        local_a = 0;
        local_10 = local_e;
        while (param_2[local_10] != '\0' && local_4 < local_c && param_2[local_10] != '\n') {
            if (param_2[local_10] <= 0x20) {
                local_4 += space;
                local_a += 1;
            } else if (param_2[local_10] == '{') {
                local_4 += 0xf;
            } else if (param_2[local_10] != '_') {
                local_4 += U5CutsceneWidths[param_2[local_10] - 0x21] + 1;
            }

            local_10++;
        }

        while (1) {
            if ((param_2[local_10] == '\0' && local_4 < local_c))
                break;
            if (param_2[local_10] == '\n')
                break;
            if (--local_10 <= local_e)
                break;

            if (param_2[local_10] == ' ') {
                local_4 -= space;
                local_a--;
                break;
            }

            if (param_2[local_10] == '_') {
                if (local_4 + U5CutsceneWidths[12] + 1 < local_c) {
                    local_4 += U5CutsceneWidths[12] + 1;
                    break;
                }
            }

            if (param_2[local_10] != '_' && param_2[local_10] != '{') {
                local_4 -= U5CutsceneWidths[param_2[local_10] - 0x21] + 1;
            }
        }

        /* Rectangular boxes must not discard an unbreakable word or a glyph
         * wider than the box. Wrap a fitting prefix and clip at the box edge. */
        if(boxed && local_10<=local_e && param_2[local_e]!='\n') {
            int fit=local_e,used=x-layout->left[local_12];
            while(param_2[fit] && param_2[fit]!='\n') {
                int ch=(unsigned char)param_2[fit];
                int advance=ch<=32?space:(ch=='{'?15:(ch=='_'?0:U5CutsceneWidths[ch-33]+1));
                if(fit>local_e && used+advance>=local_c) break;
                used+=advance;fit++;
                if(used>=local_c) break;
            }
            local_10=fit;local_4=used;local_a=0;splitWord=1;
        }
        local_8 = local_c - local_4;
        local_6 = local_10;
        local_10 = local_e;

        for (; local_10 < local_6; local_10++) {
            if (param_2[local_10] <= 0x20) {
                x += space;
                if (local_a != 0 && param_2[local_6] != '\0' && param_2[local_6] != '\n') {
                    x += local_8 / local_a;
                    local_8 -= local_8 / local_a;
                    local_a--;
                }
            } else if (param_2[local_10] == '{') {
                x += 0xf;
            } else if (param_2[local_10] != '_') {
                if (y < drawBottom) {
                    // NOTE: "or" mix
                    draw(context, param_2[local_10] - 0x20, x, y);
                }
                x += U5CutsceneWidths[param_2[local_10] - 0x21] + 1;
            }
        }

        local_e = local_6;
        if (param_2[local_e] != '\0') {
            if (param_2[local_e] == '_' && y < drawBottom) {
                draw(context, 0xd, x, y);
            }

            local_4 = 0;
            y += 9;
            local_12 = ((int)y > (int)layout->top && (int)y < (int)layout->bottom) ? 1 : 0;

            x = layout->left[local_12];
            local_c = layout->right[local_12] - x;
            if(!splitWord) local_e++;
        }
    }
}
static void U5_DrawCutsceneTextBounded(const U5CutsceneLayout *layout,const char *text,int space,int bottom,
    void (*draw)(void *,int,int,int),void *context) {
    U5_DrawCutsceneTextKernel(layout,text,space,bottom,0,draw,context);
}
static void U5_DrawCutsceneText(const U5CutsceneLayout *layout,const char *text,int space,
    void (*draw)(void *,int,int,int),void *context) {
    U5_DrawCutsceneTextBounded(layout,text,space,192,draw,context);
}
static void U5_DrawPlacedCutsceneText(int scene, unsigned page,
                                      const U5CutscenePlacement *placement, const char *text,
                                      void (*draw)(void *, int, int, int), void *context) {
    U5CutsceneLayout layout = U5CutsceneLayouts[scene][page];
    if (placement && placement->custom) {
        for (int i = 0; i < 2; i++) {
            layout.left[i] = placement->left[i];
            layout.right[i] = placement->right[i];
        }
        layout.top = placement->top;
        layout.bottom = placement->bottom;
        layout.x = placement->x;
        layout.y = placement->y;
    }
    if (!scene && page == 6) {
        const char *split = strchr(text, '\n');
        if (split) {
            char first[2000];
            size_t n = (size_t)(split - text);
            if (n >= sizeof(first))
                return;
            memcpy(first, text, n);
            first[n] = 0;
            U5_DrawCutsceneText(&layout, first, 5, draw, context);
            layout.y = placement && placement->custom ? placement->secondY : 180;
            U5_DrawCutsceneText(&layout, split + 1, 5, draw, context);
            return;
        }
    }
    U5_DrawCutsceneText(&layout, text, 5, draw, context);
}
static void U5_DrawCutscenePageText(int scene, unsigned page, const char *text,
                                    void (*draw)(void *, int, int, int), void *context) {
    U5_DrawPlacedCutsceneText(scene, page, NULL, text, draw, context);
}
#endif
