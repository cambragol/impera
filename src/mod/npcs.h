#ifndef IMPERA_NPCS_H
#define IMPERA_NPCS_H
#include <stddef.h>
#include <stdint.h>
#define IMPERA_NPC_CAPACITY 32768
#define IMPERA_NPC_BASE_SIZE 4608
#define IMPERA_NPC_EXTRA_START 4624
/* Original 4608 bytes followed by INPC0001, count, reserved, then sorted
 * 24-byte entries: settlement, reserved, slot LE16, schedule[16], shape,
 * dialogue, reserved[2]. Slot zero remains reserved for the party. */
static inline unsigned NPCS_Word(const unsigned char *p) { return p[0] | ((unsigned)p[1] << 8); }
#ifdef __cplusplus
extern "C" {
#endif
unsigned NPCS_Count(const void *, size_t, unsigned town);
size_t NPCS_Offset(const void *, size_t, unsigned town, unsigned slot);
size_t NPCS_ShapeOffset(const void *, size_t, unsigned town, unsigned slot);
size_t NPCS_DialogueOffset(const void *, size_t, unsigned town, unsigned slot);
int NPCS_Validate(const void *, size_t);
#ifdef __cplusplus
}
#endif
#endif
