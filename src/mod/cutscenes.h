#ifndef IMPERA_CUTSCENES_H
#define IMPERA_CUTSCENES_H
#include "cutscene_layout.h"
#include "cutscene_composition.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* ICUT0001: magic, two u32 page counts (0 inherits vanilla), then introduction
 * and ending records: u32 vanilla layout, u32 text size, NUL-terminated ASCII.
 * ICUT0002 adds twelve u32 values after each record header: custom flag,
 * artwork X/Y, text left/right outside and inside the wrap band, band top/bottom,
 * text start X/Y and lower portal paragraph Y. v1 remains supported. */
#define U5_CUTSCENE_MAX_PAGES 128
static unsigned U5_CutsceneWord(const unsigned char *p) {
    return p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}
static int U5_CutsceneReadPlacement(const void *data, size_t size, int scene, unsigned index,
                                    unsigned *layout, const char **text, unsigned *count,
                                    U5CutscenePlacement *placement) {
    const unsigned char *b = (const unsigned char *)data;
    if(size>=8 && !memcmp(b,"ICUT0003",8)) {
        if(text) *text=NULL;
        if(placement) memset(placement,0,sizeof(*placement));
        return U5_CompositionRead(data,size,scene,index,count,NULL);
    }
    int v2 = size >= 8 && !memcmp(b, "ICUT0002", 8);
    if (size < 16 || (!v2 && memcmp(b, "ICUT0001", 8)))
        return 0;
    unsigned counts[2] = {U5_CutsceneWord(b + 8), U5_CutsceneWord(b + 12)};
    if (counts[0] > U5_CUTSCENE_MAX_PAGES || counts[1] > U5_CUTSCENE_MAX_PAGES ||
        (!counts[0] && !counts[1]))
        return 0;
    size_t p = 16;
    if (count)
        *count = scene >= 0 && scene < 2 ? counts[scene] : 0;
    if (text)
        *text = NULL;
    for (int s = 0; s < 2; s++)
        for (unsigned i = 0; i < counts[s]; i++) {
            if (size - p < 8)
                return 0;
            unsigned l = U5_CutsceneWord(b + p), n = U5_CutsceneWord(b + p + 4);
            p += 8;
            U5CutscenePlacement position = {0};
            if (v2) {
                if (size - p < 48)
                    return 0;
                int fields[12];
                for (int j = 0; j < 12; j++) {
                    unsigned value = U5_CutsceneWord(b + p + j * 4);
                    if (value > 320)
                        return 0;
                    fields[j] = (int)value;
                }
                position.custom = fields[0];
                position.imageX = fields[1];
                position.imageY = fields[2];
                position.left[0] = fields[3];
                position.right[0] = fields[4];
                position.left[1] = fields[5];
                position.right[1] = fields[6];
                position.top = fields[7];
                position.bottom = fields[8];
                position.x = fields[9];
                position.y = fields[10];
                position.secondY = fields[11];
                if (!U5_ValidCutscenePlacement(&position))
                    return 0;
                p += 48;
            }
            if (l >= (s ? 6u : 21u) || !n || n > 2000 || n > size - p || b[p + n - 1] ||
                memchr(b + p, 0, n - 1))
                return 0;
            for (unsigned j = 0; j + 1 < n; j++)
                if (b[p + j] != 10 && b[p + j] != 13 && (b[p + j] < 32 || b[p + j] > 123))
                    return 0;
            if (s == scene && i == index) {
                if (placement)
                    *placement = position;
                if (layout)
                    *layout = l;
                if (text)
                    *text = (const char *)b + p;
            }
            p += n;
        }
    return p == size;
}
static int U5_CutsceneRead(const void *data, size_t size, int scene, unsigned index,
                           unsigned *layout, const char **text, unsigned *count) {
    return U5_CutsceneReadPlacement(data, size, scene, index, layout, text, count, NULL);
}
static int U5_ValidateCutscenes(const void *data, size_t size) {
    return U5_CutsceneRead(data, size, -1, 0, NULL, NULL, NULL);
}
#endif
