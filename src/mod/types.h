#ifndef IMPERA_TYPES_H
#define IMPERA_TYPES_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define TYPES_MAX 1024
#define TYPES_CELLS 65536
#define TYPES_FIRST_CUSTOM 512u
#define TYPES_LAST_CUSTOM 65535u
#define TYPES_HEADER 32u
#define TYPES_RECORD 256u
#define TYPES_MAP_RECORD 16u
#define TYPES_INFER_GROUND UINT32_MAX
#define TYPES_SPRITE 1u
#define TYPES_MASONRY 2u
#define TYPES_POISON 1u
#define TYPES_BURN 2u
/* Portable wire format; fields are explicitly encoded, never memcpy a struct.
 * ITYPES01 header: UUID[16] at 8, count:u32 at 24, reserved[4].
 * Each record: length:u32, id:u32, kind:u8 (0 tile,1 actor), base:u8,
 * filters:u8, render:u8, movement:u16, sight:u8, projectile:u8, travel:u8,
 * effects:u8, damageMin:u8, damageMax:u8, ground:u32,
 * result[4]:u32 (open,closed,locked,broken), stats[8], actorFlags:u16,
 * moveProfile:u8,range:u8,projectileAppearance:u8,companion:u8, effectModes:u16,
 * name[64],description[128],frames:u16,skiffDirections:u8,humanoidSeat:u8,combatMap:u8,reserved[3].
 * Frames: variant:u8 (0 default,1..5),side:u8 (16/32),duration:u16 (10..60000 ms),
 * followed by width*height RGBA bytes. Width 14 means Apple II 14x16;
 * widths 16 and 32 are square. Each variant has the same frame count.
 * ITYMAP01 header: UUID[16], count:u32,reserved[4]. Sorted cells:
 * kind:u8 (0 map,1 BRIT.CBT,2 DUNGEON.CBT), map:u8,floor:i8,x:u8,y:u8,
 * reserved[3],type:u32,reserved[4]. Save records reuse the final word
 * for a temporary door’s restore type. */
typedef struct TypeDef {
    uint32_t id, ground, result[4];
    uint16_t movement, actorFlags, frames, effectModes;
    uint8_t kind, base, filters, render, sight, projectile, travel, effects, damageMin, damageMax;
    uint8_t stats[8], moveProfile, range, projectileAppearance, companion;
    uint8_t skiffDirections, humanoidSeat, combatMap;
    char name[64], description[128];
} TypeDef;
typedef struct TypeCell {
    uint8_t kind, map;
    int8_t floor;
    uint8_t x, y;
    uint32_t type, restoreType;
} TypeCell;
uint32_t TYPES_U32(const void *);
void TYPES_Put32(void *, uint32_t);
void TYPES_Decode(const unsigned char *, TypeDef *);
void TYPES_Encode(unsigned char *, const TypeDef *);
void TYPES_Vanilla(unsigned kind, unsigned id, TypeDef *);
int TYPES_Walkable(unsigned actorShape, unsigned terrain);
int TYPES_DefWalkable(unsigned actorShape, const TypeDef *);
int TYPES_Validate(const void *, size_t);
int TYPES_ValidateMap(const void *, size_t, const void *, size_t);
const unsigned char *TYPES_Find(const void *, size_t, uint32_t);
static inline unsigned TYPES_FrameHeight(unsigned width) { return width == 14 ? 16 : width; }
const unsigned char *TYPES_Frame(const unsigned char *, unsigned variant, uint64_t ms,
                                 unsigned *side);
void TYPES_CellDecode(const unsigned char *, TypeCell *);
void TYPES_CellEncode(unsigned char *, const TypeCell *);
uint64_t TYPES_CellKey(const TypeCell *);
/* Runtime snapshot and mutable terrain state, shared with save validation. */
void TYPES_Mount(const void *, size_t, const void *, size_t);
void TYPES_Reset(void);
void TYPES_BeginEncounter(unsigned kind, unsigned map);
int TYPES_Get(uint32_t, TypeDef *);
uint32_t TYPES_At(TypeCell, uint32_t fallback);
int TYPES_State(TypeCell, TypeCell *);
int TYPES_Defined(uint32_t);
int TYPES_Set(TypeCell, uint32_t);
int TYPES_WriteState(FILE *);
int TYPES_ReadState(FILE *, int apply);
unsigned TYPES_ArtSlot(uint32_t);
const unsigned char *TYPES_SampleArt(unsigned slot, unsigned variant, uint64_t ms, unsigned *side);
const unsigned char *TYPES_Art(uint32_t, unsigned variant, uint64_t ms, unsigned *side);
#ifdef __cplusplus
}
#endif
#endif
