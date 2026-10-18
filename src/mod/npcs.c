#include "npcs.h"
#include <string.h>
int NPCS_Validate(const void *bytes, size_t n) {
    const unsigned char *p = bytes;
    if (n == IMPERA_NPC_BASE_SIZE)
        return 1;
    if (n < IMPERA_NPC_EXTRA_START || memcmp(p + 4608, "INPC0001", 8) ||
        (n - IMPERA_NPC_EXTRA_START) % 24 || p[4620] || p[4621] || p[4622] || p[4623])
        return 0;
    unsigned count = (unsigned)((n - IMPERA_NPC_EXTRA_START) / 24);
    unsigned stored =
        p[4616] | ((unsigned)p[4617] << 8) | ((unsigned)p[4618] << 16) | ((unsigned)p[4619] << 24);
    if (count != stored)
        return 0;
    unsigned previous = 0, next[8] = {32, 32, 32, 32, 32, 32, 32, 32};
    for (size_t off = IMPERA_NPC_EXTRA_START; off < n; off += 24) {
        unsigned slot = NPCS_Word(p + off + 2), key = p[off] * IMPERA_NPC_CAPACITY + slot;
        if (p[off] >= 8 || p[off + 1] || slot < 32 || slot >= IMPERA_NPC_CAPACITY ||
            slot != next[p[off]]++ || (off > IMPERA_NPC_EXTRA_START && key <= previous) ||
            p[off + 22] || p[off + 23])
            return 0;
        previous = key;
    }
    return 1;
}

unsigned NPCS_Count(const void *bytes, size_t size, unsigned town) {
    const unsigned char *p = (const unsigned char *)bytes;
    if (size < IMPERA_NPC_EXTRA_START || town >= 8)
        return 32;
    size_t lo = 0, hi = (size - IMPERA_NPC_EXTRA_START) / 24;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2;
        if (p[IMPERA_NPC_EXTRA_START + mid * 24] <= town)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo && p[IMPERA_NPC_EXTRA_START + (lo - 1) * 24] == town)
        return NPCS_Word(p + IMPERA_NPC_EXTRA_START + (lo - 1) * 24 + 2) + 1;
    return 32;
}
size_t NPCS_Offset(const void *bytes, size_t size, unsigned town, unsigned slot) {
    const unsigned char *p = (const unsigned char *)bytes;
    if (town >= 8)
        return size;
    if (slot < 32)
        return town * 576 + slot * 16;
    if (size < IMPERA_NPC_EXTRA_START || slot >= IMPERA_NPC_CAPACITY)
        return size;
    size_t lo = 0, hi = (size - IMPERA_NPC_EXTRA_START) / 24;
    unsigned key = town * IMPERA_NPC_CAPACITY + slot;
    while (lo < hi) {
        size_t mid = lo + (hi - lo) / 2, off = IMPERA_NPC_EXTRA_START + mid * 24;
        unsigned current = p[off] * IMPERA_NPC_CAPACITY + NPCS_Word(p + off + 2);
        if (current < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    size_t off = IMPERA_NPC_EXTRA_START + lo * 24;
    return off + 24 <= size && p[off] == town && NPCS_Word(p + off + 2) == slot ? off + 4 : size;
}
size_t NPCS_ShapeOffset(const void *p, size_t n, unsigned town, unsigned slot) {
    return slot < 32 ? town * 576 + 512 + slot : NPCS_Offset(p, n, town, slot) + 16;
}
size_t NPCS_DialogueOffset(const void *p, size_t n, unsigned town, unsigned slot) {
    return slot < 32 ? town * 576 + 544 + slot : NPCS_Offset(p, n, town, slot) + 17;
}
