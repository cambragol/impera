#ifndef IMPERA_ACTORS_H
#define IMPERA_ACTORS_H
#include "common/common.h"
#include "structs.h"
#include "types.h"
/* Combat indices 0..47 retain their vanilla meaning; extended IDs are independent. */
TypeDef ACTORS_Def(unsigned type);
EnemyStats ACTORS_Stats(unsigned type);
unsigned ACTORS_Flags(unsigned type);
unsigned ACTORS_Range(unsigned type);
unsigned ACTORS_Projectile(unsigned type);
char *ACTORS_Name(unsigned type, int plural);
unsigned ACTORS_Profile(unsigned type);
unsigned ACTORS_Target(unsigned type, unsigned action, unsigned fallback);
unsigned ACTORS_Instance(unsigned actor);
unsigned ACTORS_Sprite(unsigned actor, unsigned fallback);
unsigned ACTORS_AtCell(int x, int y, unsigned fallback);
unsigned ACTORS_MoveShape(unsigned actor, unsigned fallback);
unsigned ACTORS_Encounter(void);
void ACTORS_SetEncounter(unsigned actor);
int ACTORS_Transform(unsigned entity);
unsigned ACTORS_PlacedEncounter(unsigned slot, unsigned fallback);
#endif
