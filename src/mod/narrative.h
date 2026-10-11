#ifndef IMPERA_NARRATIVE_H
#define IMPERA_NARRATIVE_H
#include "narrative_tables.h"
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* TEXT.OVR v1: magic, u32 count, then u32 ID length, ID, u32 text length,
   text including NUL. IDs are verified against the engine-derived catalog.
   Limits include the terminator and match the consumers' scratch buffers. */
static const U5NarrativeEntry *U5_NarrativeEntry(const char *resource, unsigned offset) {
    for (unsigned i = 0; i < U5_NARRATIVE_COUNT; ++i)
        if (!strcmp(resource, U5NarrativeEntries[i].resource) &&
            offset == U5NarrativeEntries[i].offset)
            return &U5NarrativeEntries[i];
    return NULL;
}
static unsigned U5_TextWord(const unsigned char *b) {
    return b[0] | ((unsigned)b[1] << 8) | ((unsigned)b[2] << 16) | ((unsigned)b[3] << 24);
}
static int U5_NarrativeTextValid(const U5NarrativeEntry *entry, const unsigned char *text,
                                 size_t length) {
    int shop = !strcmp(entry->resource, "SHOPPE.DAT");
    if (!length || length > (shop ? 1500u : 2000u) || text[length - 1] ||
        memchr(text, 0, length - 1))
        return 0;
    for (size_t i = 0; i + 1 < length; ++i) {
        unsigned c = text[i];
        if (c >= 128) {
            if (!shop || c - 128 >= sizeof(U5ShopPhrases) / sizeof(U5ShopPhrases[0]) ||
                !U5ShopPhrases[c - 128])
                return 0;
        } else if (c != 10 && c != 13 && (c < 32 || c > 126))
            return 0;
        if ((!strcmp(entry->resource, "QUESTION.DAT") || !strcmp(entry->resource, "END.DAT")) &&
            c > 123)
            return 0; /* Proportional width table ends at z; { is a special indent. */
    }
    return 1;
}
/* Returns NULL for an absent entry. Validation is all-or-nothing before mounting. */
static const char *U5_FindNarrativeText(const unsigned char *b, size_t size, const char *id,
                                        int *valid) {
    unsigned seen[U5_NARRATIVE_COUNT];
    memset(seen, 0, sizeof(seen));
    const char *found = NULL;
    *valid = 0;
    if (size < 12 || memcmp(b, "ITXT0001", 8))
        return NULL;
    unsigned count = U5_TextWord(b + 8);
    size_t p = 12;
    if (count > U5_NARRATIVE_COUNT)
        return NULL;
    for (unsigned i = 0; i < count; ++i) {
        if (size - p < 4)
            return NULL;
        unsigned n = U5_TextWord(b + p);
        p += 4;
        if (!n || n > 100 || n > size - p)
            return NULL;
        unsigned index = 0;
        for (; index < U5_NARRATIVE_COUNT; ++index)
            if (strlen(U5NarrativeEntries[index].id) == n &&
                !memcmp(b + p, U5NarrativeEntries[index].id, n))
                break;
        if (index == U5_NARRATIVE_COUNT || seen[index]++)
            return NULL;
        p += n;
        if (size - p < 4)
            return NULL;
        unsigned length = U5_TextWord(b + p);
        p += 4;
        if (length > size - p || !U5_NarrativeTextValid(&U5NarrativeEntries[index], b + p, length))
            return NULL;
        if (id && !strcmp(id, U5NarrativeEntries[index].id))
            found = (const char *)b + p;
        p += length;
    }
    if (p != size)
        return NULL;
    *valid = 1;
    return found;
}
/* The engine and Workshop use exactly the same substitution and spacing rules. */
static int U5_ExpandShopText(const unsigned char *input, char *output, size_t capacity,
                             const char *const values[7]) {
    size_t p = 0;
    const char *symbols = "#$@%^&*";
    while (*input) {
        unsigned c = *input++;
        const char *text = NULL;
        int phrase = c >= 128;
        if (phrase) {
            if (c - 128 >= sizeof(U5ShopPhrases) / sizeof(U5ShopPhrases[0]) ||
                !U5ShopPhrases[c - 128])
                return 0;
            text = U5ShopPhrases[c - 128];
        } else {
            const char *at = strchr(symbols, c);
            if (at)
                text = values[at - symbols] ? values[at - symbols] : "";
        }
        if (text) {
            size_t n = strlen(text), spaces = phrase ? 1 + (*input && *input < 128) : 0;
            if (p >= capacity || n + spaces >= capacity - p)
                return 0;
            if (phrase)
                output[p++] = ' ';
            memcpy(output + p, text, n);
            p += n;
            if (phrase && *input && *input < 128)
                output[p++] = ' ';
        } else {
            if (p + 1 >= capacity)
                return 0;
            output[p++] = (char)c;
        }
    }
    if (p >= capacity)
        return 0;
    output[p] = 0;
    return 1;
}
#endif
