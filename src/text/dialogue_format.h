#ifndef IMPERA_DIALOGUE_FORMAT_H
#define IMPERA_DIALOGUE_FORMAT_H
#include <stddef.h>
#include <stdint.h>
#include <string.h>
/* Description-entry prefixes, shared by every NPC using the conversation. */
#define IMPERA_TALK_SKIP_OPENING "\xfc\xfd\xfc\xfa"
#define IMPERA_TALK_SKIP_OPENING_SIZE 4
#define IMPERA_TALK_GARGOYLE_SPEECH "\xfc\xfd\xfc\xf9"
#define IMPERA_TALK_FLAG_SKIP_OPENING 1u
#define IMPERA_TALK_FLAG_GARGOYLE_SPEECH 2u
static inline unsigned IMPERA_TalkFlagPrefix(const void *data, size_t size, size_t *length) {
    const uint8_t *b = (const uint8_t *)data;
    size_t i = 0;
    unsigned flags = 0;
    while (size - i >= 4) {
        if (!memcmp(b + i, IMPERA_TALK_SKIP_OPENING, 4))
            flags |= IMPERA_TALK_FLAG_SKIP_OPENING;
        else if (!memcmp(b + i, IMPERA_TALK_GARGOYLE_SPEECH, 4))
            flags |= IMPERA_TALK_FLAG_GARGOYLE_SPEECH;
        else
            break;
        i += 4;
    }
    if (length)
        *length = i;
    return flags;
}
static inline unsigned IMPERA_TalkConversationFlags(const void *data, size_t size) {
    const uint8_t *b = (const uint8_t *)data;
    size_t i = 0;
    while (i < size) {
        unsigned code = b[i++];
        if (!code)
            return IMPERA_TalkFlagPrefix(b + i, size - i, NULL);
        unsigned operands = code == 133 ? 3 : code == 134 || code == 140 ? 1 : code == 254 ? 2 : 0;
        if (operands > size - i)
            return 0;
        i += operands;
    }
    return 0;
}
#endif
