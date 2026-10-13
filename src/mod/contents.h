#ifndef IMPERA_CONTENTS_H
#define IMPERA_CONTENTS_H
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#ifdef __cplusplus
extern "C" {
#endif
#define CONTENTS_MAX 256
#define CONTENTS_ITEMS 16
#define CONTENTS_RECORD 96
#define CONTENTS_HEADER 32
#define CONTENTS_SAVE_MAX (16 + CONTENTS_MAX * 64)
/* Explicit wire encoding; never serialize a native struct. */
/* record: id:u32, map:u16, floor:i8, kind:u8 (0 location,1 BRIT.CBT,2 DUNGEON.CBT),
 x:u8,y:u8,action:u8 (0 Search,1 Open),refill:u8 (0 once,1 daily,2 inherited),
 original:u16 (65535 none),slot:u8 (255 cell),flags:u8 (1 lock,2 trap),
 lockDifficulty:u8,trapDifficulty:u8,trapEffect:u8 (0 vanilla damage),
 itemCount:u8,reserved[12], items[16]: {type:u8,variant:u8,quantity:u16}.
 Item types: 1 gold,2 food,3 torches,4 gems,5 keys,6 skull keys,
 7 equipment (0..47),8 scroll (0..7),9 potion (0..7).
 Empty contents are explicit, not inheritance. */
typedef struct ContentsItem {
    uint8_t type, variant;
    uint16_t quantity;
} ContentsItem;
typedef struct ContentsDef {
    uint8_t space[16];
    uint32_t id;
    uint16_t map, original;
    int8_t floor;
    uint8_t kind, x, y, action, refill, slot, flags, lockDifficulty, trapDifficulty, trapEffect,
        count;
    ContentsItem items[CONTENTS_ITEMS];
} ContentsDef;
typedef struct ContentsState {
    uint8_t space[16];
    uint32_t id;
    uint32_t day;
    uint8_t flags; /* 1 revealed,2 unlocked,4 disarmed,8 exhausted */
    uint16_t remaining[CONTENTS_ITEMS];
} ContentsState;
unsigned CONTENTS_Word(const unsigned char *p);
uint32_t CONTENTS_Dword(const unsigned char *p);
void CONTENTS_PutWord(unsigned char *p, unsigned value);
void CONTENTS_PutDword(unsigned char *p, uint32_t value);
int CONTENTS_Validate(const void *data, size_t size);
void CONTENTS_DecodeRecord(const unsigned char *p, const unsigned char *space, ContentsDef *d);
void CONTENTS_EncodeRecord(unsigned char *p, const ContentsDef *d);
void CONTENTS_Unmount(void);
int CONTENTS_CanMount(const void *data, size_t size);
void CONTENTS_Mount(const void *data, size_t size);
unsigned CONTENTS_Count(void);
const ContentsDef *CONTENTS_Def(unsigned i);
/* NULL means untouched vanilla behavior. Suppressed original sites return a tombstone. */
const ContentsDef *CONTENTS_Find(unsigned kind, unsigned map, int floor, int x, int y);
ContentsState *CONTENTS_State(const ContentsDef *d);
const ContentsState *CONTENTS_Peek(const ContentsDef *d);
void CONTENTS_Reset(void);
void CONTENTS_BeginEncounter(unsigned kind, unsigned map);
void CONTENTS_EndEncounter(void);
unsigned CONTENTS_EncounterKind(void);
unsigned CONTENTS_EncounterMap(void);
uint32_t CONTENTS_GameDay(unsigned year, unsigned month, unsigned day);
int CONTENTS_Available(const ContentsDef *d, ContentsState *s, uint32_t day, const uint8_t *bits,
                       unsigned lastDay, unsigned keys, unsigned questItem);
int CONTENTS_Reveal(const ContentsDef *d, ContentsState *s, uint32_t day);
int CONTENTS_PendingShape(unsigned kind, unsigned map, int floor, int x, int y);
/* Append to the vanilla save, so slot copying and atomic installation retain one snapshot. */
int CONTENTS_WriteState(FILE *stream);
int CONTENTS_ReadState(FILE *stream, int apply);
#ifdef __cplusplus
}
#endif
#endif
