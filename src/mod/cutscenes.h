#ifndef IMPERA_CUTSCENES_H
#define IMPERA_CUTSCENES_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* ICUT0001: magic, two u32 page counts (0 inherits vanilla), then introduction
 * and ending records: u32 vanilla layout, u32 text size, NUL-terminated ASCII.
 * Layouts retain the original artwork, exclusions, effects and font settings. */
#define U5_CUTSCENE_MAX_PAGES 128
static unsigned U5_CutsceneWord(const unsigned char *p) {
    return p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}
static int U5_CutsceneRead(const void *data, size_t size, int scene, unsigned index,
                           unsigned *layout, const char **text, unsigned *count) {
    const unsigned char *b = (const unsigned char *)data;
    if (size < 16 || memcmp(b, "ICUT0001", 8))
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
            if (l >= (s ? 6u : 21u) || !n || n > 2000 || n > size - p || b[p + n - 1] ||
                memchr(b + p, 0, n - 1))
                return 0;
            for (unsigned j = 0; j + 1 < n; j++)
                if (b[p + j] != 10 && b[p + j] != 13 && (b[p + j] < 32 || b[p + j] > 123))
                    return 0;
            if (s == scene && i == index) {
                if (layout)
                    *layout = l;
                if (text)
                    *text = (const char *)b + p;
            }
            p += n;
        }
    return p == size;
}
static int U5_ValidateCutscenes(const void *data, size_t size) {
    return U5_CutsceneRead(data, size, -1, 0, NULL, NULL, NULL);
}
#endif
