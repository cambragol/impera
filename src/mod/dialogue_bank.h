#ifndef IMPERA_DIALOGUE_BANK_H
#define IMPERA_DIALOGUE_BANK_H
#include <stddef.h>
#include "../text/dialogue_format.h"
#include <stdint.h>
#include <string.h>
/* Extended TLK: magic, u32 count, then {u32 id, u32 offset, u32 size}.
 * Ordinary DOS banks retain their original bytes and 16-bit table. */
#define DIALOGUE_BANK_MAGIC "ITLK0001"
#define DIALOGUE_BANK_MAX (8u * 1024u * 1024u)
static inline uint32_t DIALOGUE_U16(const uint8_t *p) { return p[0] | (uint32_t)p[1] << 8; }
static inline uint32_t DIALOGUE_U32(const uint8_t *p) {
    return DIALOGUE_U16(p) | DIALOGUE_U16(p + 2) << 16;
}
static inline int DIALOGUE_Extended(const void *data, size_t size) {
    return size >= 8 && !memcmp(data, DIALOGUE_BANK_MAGIC, 8);
}
static inline int DIALOGUE_BankEntry(const void *data, size_t size, unsigned index, unsigned *id,
                                     size_t *offset, size_t *length) {
    const uint8_t *b = (const uint8_t *)data;
    int extended = DIALOGUE_Extended(data, size);
    size_t header = extended ? 12 : 2, stride = extended ? 12 : 4;
    if (size < header || size > DIALOGUE_BANK_MAX)
        return 0;
    unsigned count = extended ? DIALOGUE_U32(b + 8) : DIALOGUE_U16(b);
    if (count > 127 || index >= count || count * stride > size - header)
        return 0;
    const uint8_t *record = b + header + index * stride;
    *id = extended ? DIALOGUE_U32(record) : DIALOGUE_U16(record);
    *offset = extended ? DIALOGUE_U32(record + 4) : DIALOGUE_U16(record + 2);
    size_t end = extended            ? *offset + (size_t)DIALOGUE_U32(record + 8)
                 : index + 1 < count ? DIALOGUE_U16(record + 6)
                                     : size;
    if (*id > 65535 || *offset < header + count * stride || *offset >= end || end > size)
        return 0;
    *length = end - *offset;
    return 1;
}
static inline int DIALOGUE_ValidateBank(const void *data, size_t size) {
    const uint8_t *b = (const uint8_t *)data;
    int extended = DIALOGUE_Extended(data, size);
    size_t header = extended ? 12 : 2, stride = extended ? 12 : 4;
    if (size < header || size > DIALOGUE_BANK_MAX)
        return 0;
    unsigned count = extended ? DIALOGUE_U32(b + 8) : DIALOGUE_U16(b);
    if (count > 127 || count * stride > size - header)
        return 0;
    size_t previous = header + count * stride;
    for (unsigned i = 0; i < count; ++i) {
        unsigned id;
        size_t offset, length;
        if (!DIALOGUE_BankEntry(data, size, i, &id, &offset, &length) || offset < previous)
            return 0;
        previous = offset + length;
        for (unsigned j = 0; j < i; ++j) {
            const uint8_t *record = b + header + j * stride;
            if (id == (extended ? DIALOGUE_U32(record) : DIALOGUE_U16(record)))
                return 0;
        }
    }
    return 1;
}
static inline int DIALOGUE_SkipsOpening(const void *data, size_t size) {
    return (IMPERA_TalkConversationFlags(data, size) & IMPERA_TALK_FLAG_SKIP_OPENING) != 0;
}
static inline int DIALOGUE_GargoyleSpeech(const void *data, size_t size) {
    return (IMPERA_TalkConversationFlags(data, size) & IMPERA_TALK_FLAG_GARGOYLE_SPEECH) != 0;
}
#endif
