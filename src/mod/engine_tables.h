#ifndef IMPERA_ENGINE_TABLES_H
#define IMPERA_ENGINE_TABLES_H
#include "engine_tables_data.h"
#include <stddef.h>
#include <string.h>
/* RULES.TBL v1: IRULES01, u16 count, then sorted {u8 table,u16 index,u16 value}.
 * IDs refer to verified concepts, never native pointer offsets. Little endian. */
static inline unsigned U5_TableWord(const unsigned char *p) { return p[0] | ((unsigned)p[1] << 8); }
static inline const U5EngineTable *U5_TableSpec(unsigned id) {
    return id >= 1 && id <= U5_ENGINE_TABLE_COUNT ? &U5_EngineTables[id - 1] : NULL;
}
static inline int U5_TableEntryValid(unsigned id, unsigned index, unsigned value) {
    const U5EngineTable *s = U5_TableSpec(id);
    if (!s || index >= s->rows * s->columns || value > s->maximum)
        return 0;
    if (id == 4) {
        if (index % 8 == 7)
            return value == 255; /* Retain the final stock terminator. */
        return value < 48 || value == 255;
    }
    if (id == 5 && (index >= 12 || index % 4 == 3))
        return 0; /* Unused guild supply column; no false runtime claim. */
    if (id == 6 && index >= 3)
        return 0; /* Unassigned stable row. */
    return 1;
}
static inline int U5_ValidateTables(const unsigned char *data, size_t size) {
    if (!data || size < 10 || memcmp(data, "IRULES01", 8))
        return 0;
    unsigned count = U5_TableWord(data + 8);
    if (count > 512 || size != 10 + (size_t)count * 5)
        return 0;
    unsigned previous = 0;
    for (unsigned i = 0; i < count; ++i) {
        const unsigned char *p = data + 10 + i * 5;
        unsigned id = p[0], index = U5_TableWord(p + 1), value = U5_TableWord(p + 3),
                 key = (id << 16) | index;
        if (!U5_TableEntryValid(id, index, value) || (i && key <= previous))
            return 0;
        previous = key;
    }
    return 1;
}
/* Mounted data has already passed the shared validator; bounded binary lookup. */
static inline unsigned U5_TableValue(const unsigned char *data, size_t size, unsigned id,
                                     unsigned index, unsigned fallback) {
    if (!data || size < 10 || memcmp(data, "IRULES01", 8))
        return fallback;
    unsigned count = U5_TableWord(data + 8), key = (id << 16) | index, lo = 0, hi = count;
    if (count > 512 || size != 10 + (size_t)count * 5)
        return fallback;
    while (lo < hi) {
        unsigned mid = lo + (hi - lo) / 2;
        const unsigned char *p = data + 10 + mid * 5;
        unsigned found = ((unsigned)p[0] << 16) | U5_TableWord(p + 1);
        if (found < key)
            lo = mid + 1;
        else
            hi = mid;
    }
    if (lo < count) {
        const unsigned char *p = data + 10 + lo * 5;
        if ((((unsigned)p[0] << 16) | U5_TableWord(p + 1)) == key)
            return U5_TableWord(p + 3);
    }
    return fallback;
}
#endif
