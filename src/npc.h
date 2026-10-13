#ifndef _NPC_H
#define _NPC_H

void NPC_0000_LoadNpcFile(void);
/* Refresh a new settlement game from mounted NPC definitions, without entering gameplay. */
int NPC_InitializeNewGame(void);
void NPC_00d6(int param_1);
void NPC_0db4(int param_1);
int NPC_12e0(int param_1, char param_2);

#endif
