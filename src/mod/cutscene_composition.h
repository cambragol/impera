#ifndef IMPERA_CUTSCENE_COMPOSITION_H
#define IMPERA_CUTSCENE_COMPOSITION_H
#include "cutscene_layout.h"
#include <stdint.h>
/* ICUT0003 stores independent native-size RGBA images and bounded Ultima text boxes.
 * Assets are deduplicated snapshots, including images imported from modded game art. */
typedef struct {
    unsigned width, height;
    const unsigned char *rgba, *label;
    unsigned labelSize;
} U5CompositionAsset;
typedef struct {
    int x, y, width, height, flow;
    U5CutsceneLayout layout;
    const char *text;
} U5CompositionText;
typedef struct {
    void *context;
    void (*image)(void *, const U5CompositionAsset *, int, int);
    void (*text)(void *, const U5CompositionText *);
} U5CompositionPainter;
static unsigned U5_CompositionWord(const unsigned char *p) {
    return p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}
static int U5_CompositionRead(const void *data, size_t size, int scene, unsigned page,
                              unsigned *count, const U5CompositionPainter *paint) {
    const unsigned char *b = (const unsigned char *)data;
    if (size < 20 || size > 8u * 1024u * 1024u || memcmp(b, "ICUT0003", 8))
        return 0;
    unsigned counts[2] = {U5_CompositionWord(b + 8), U5_CompositionWord(b + 12)},
             assets = U5_CompositionWord(b + 16);
    if (counts[0] > 128 || counts[1] > 128 || (!counts[0] && !counts[1]) || assets > 256)
        return 0;
    if (count)
        *count = scene >= 0 && scene < 2 ? counts[scene] : 0;
    U5CompositionAsset catalog[256];
    size_t p = 20;
    for (unsigned i = 0; i < assets; i++) {
        if (size - p < 12)
            return 0;
        unsigned w = U5_CompositionWord(b + p), h = U5_CompositionWord(b + p + 4),
                 n = U5_CompositionWord(b + p + 8);
        p += 12;
        if (!w || !h || w > 1024 || h > 1024 || !n || n > 256 || n > size - p ||
            memchr(b + p, 0, n))
            return 0;
        catalog[i].label = b + p;
        catalog[i].labelSize = n;
        p += n;
        size_t pixels = (size_t)w * h * 4;
        if (pixels > size - p)
            return 0;
        catalog[i].width = w;
        catalog[i].height = h;
        catalog[i].rgba = b + p;
        p += pixels;
    }
    for (int s = 0; s < 2; s++)
        for (unsigned i = 0; i < counts[s]; i++) {
            if (size - p < 8)
                return 0;
            unsigned images = U5_CompositionWord(b + p), texts = U5_CompositionWord(b + p + 4);
            p += 8;
            if (images > 64 || texts > 32)
                return 0;
            for (unsigned j = 0; j < images; j++) {
                if (size - p < 12)
                    return 0;
                unsigned asset = U5_CompositionWord(b + p), x = U5_CompositionWord(b + p + 4),
                         y = U5_CompositionWord(b + p + 8);
                p += 12;
                if (asset >= assets || x >= 320 || y >= 200)
                    return 0;
                if (s == scene && i == page && paint && paint->image)
                    paint->image(paint->context, &catalog[asset], (int)x, (int)y);
            }
            for (unsigned j = 0; j < texts; j++) {
                if (size - p < 56)
                    return 0;
                unsigned fields[14];
                for (int k = 0; k < 14; k++)
                    fields[k] = U5_CompositionWord(b + p + k * 4);
                p += 56;
                unsigned x = fields[0], y = fields[1], w = fields[2], h = fields[3],
                         flow = fields[4], n = fields[13];
                if (x >= 320 || y >= 200 || !w || !h || w > 320 - x || h > 200 - y || flow > 1 ||
                    !n || n > 2000 || n > size - p || b[p + n - 1] || memchr(b + p, 0, n - 1))
                    return 0;
                U5CompositionText t;
                memset(&t, 0, sizeof(t));
                t.x = (int)x;
                t.y = (int)y;
                t.width = (int)w;
                t.height = (int)h;
                t.flow = (int)flow;
                t.text = (const char *)b + p;
                int v[8];
                for (int k = 0; k < 8; k++) {
                    int64_t signedValue = fields[5 + k] <= INT32_MAX
                                              ? (int64_t)fields[5 + k]
                                              : (int64_t)fields[5 + k] - 4294967296LL;
                    if (signedValue < -320 || signedValue > 320)
                        return 0;
                    v[k] = (int)signedValue;
                }
                t.layout.left[0] = v[0];
                t.layout.right[0] = v[1];
                t.layout.left[1] = v[2];
                t.layout.right[1] = v[3];
                t.layout.top = v[4];
                t.layout.bottom = v[5];
                t.layout.x = v[6];
                t.layout.y = v[7];
                if (flow && (v[0] >= v[1] || v[2] >= v[3] || v[4] > v[5]))
                    return 0;
                for (unsigned k = 0; k + 1 < n; k++)
                    if (b[p + k] != 10 && b[p + k] != 13 && (b[p + k] < 32 || b[p + k] > 123))
                        return 0;
                if (s == scene && i == page && paint && paint->text)
                    paint->text(paint->context, &t);
                p += n;
            }
        }
    return p == size;
}
static U5CutsceneLayout U5_CompositionTextLayout(const U5CompositionText *text) {
    U5CutsceneLayout layout = text->layout;
    if (!text->flow) {
        memset(&layout, 0, sizeof(layout));
        layout.right[0] = layout.right[1] = text->width;
        layout.top = layout.bottom = 320;
    }
    return layout;
}
static void U5_DrawCompositionText(const U5CompositionText *box,
                                   void (*glyph)(void *, int, int, int), void *context) {
    U5CutsceneLayout layout = U5_CompositionTextLayout(box);
    U5_DrawCutsceneTextKernel(&layout, box->text, 5, 32767, !box->flow, glyph, context);
}
#endif
