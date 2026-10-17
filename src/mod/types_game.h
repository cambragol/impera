#ifndef IMPERA_TYPES_GAME_H
#define IMPERA_TYPES_GAME_H
#include "common/common.h"
#include "types.h"
TypeCell TYPES_GAME_Cell(int x, int y);
uint32_t TYPES_GAME_At(int x, int y);
byte *TYPES_GAME_Address(int x, int y, byte *base);
void TYPES_GAME_Change(int x, int y, unsigned state, uint32_t fallback);
void TYPES_GAME_Write(int x, int y, uint32_t type);
int TYPES_GAME_Push(int sx, int sy, int dx, int dy);
int TYPES_GAME_WalkableView(int actor, int vx, int vy);
int TYPES_GAME_Walkable(int actor, int x, int y);
int TYPES_GAME_Property(int x, int y, int property, int fallback);
void TYPES_GAME_Effects(void);
int TYPES_GAME_CombatEffects(unsigned entity);
void TYPES_GAME_Draw(byte *pixels, int stride, int x, int y, int dx, int dy, int clipTop,
                     int clipBottom);
int TYPES_GAME_Sight(int x, int y, int distance, int fallback);
int TYPES_GAME_Look(int x, int y);
#endif
