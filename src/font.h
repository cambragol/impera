#ifndef _FONT_H
#define _FONT_H
#include <stddef.h>
#include "mod/cutscene_placement.h"

void FONT_0000(byte* param_1, char* param_2);
int FONT_02fc(int param_1);
void FONT_04a4(void);
void FONT_0b0a(void);

void FONT_ComposeCutscene(byte *font,const void *bytes,size_t size,int scene,unsigned page);
void FONT_PlacedCutsceneText(byte *font, int scene, unsigned page, const U5CutscenePlacement *placement, const char *text);
void FONT_CutsceneText(byte *font, int scene, unsigned page, const char *text);

#endif
