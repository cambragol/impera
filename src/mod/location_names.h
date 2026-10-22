#ifndef IMPERA_LOCATION_NAMES_H
#define IMPERA_LOCATION_NAMES_H
#include <stddef.h>
#include <string.h>
/* ILOC0001: 42 fixed 64-byte, NUL-padded ASCII names. Empty slots inherit.
   Slots 1..40 are map IDs; 0 is Britannia and 41 is the Underworld. */
#define LOCATION_NAME_SLOTS 42
#define LOCATION_NAME_BYTES 64
#define LOCATION_NAMES_SIZE (8 + LOCATION_NAME_SLOTS * LOCATION_NAME_BYTES)
static inline int LOCATIONS_Validate(const void *data, size_t size) {
    const unsigned char *b = (const unsigned char *)data;
    if (size != LOCATION_NAMES_SIZE || memcmp(b, "ILOC0001", 8))
        return 0;
    for (unsigned slot = 0; slot < LOCATION_NAME_SLOTS; ++slot) {
        const unsigned char *name = b + 8 + slot * LOCATION_NAME_BYTES;
        int ended = 0;
        for (unsigned i = 0; i < LOCATION_NAME_BYTES; ++i) {
            unsigned ch = name[i];
            if (!ch)
                ended = 1;
            else if (ended || ch < 32 || ch > 126)
                return 0;
        }
        if (!ended || name[0] == ' ')
            return 0;
        size_t length = strlen((const char *)name);
        if (length && name[length - 1] == ' ')
            return 0;
    }
    return 1;
}
static inline const char *LOCATIONS_Name(const void *data, size_t size, unsigned map) {
    if (map >= LOCATION_NAME_SLOTS || size != LOCATION_NAMES_SIZE)
        return NULL;
    const char *name = (const char *)data + 8 + map * LOCATION_NAME_BYTES;
    return *name ? name : NULL;
}
#endif
