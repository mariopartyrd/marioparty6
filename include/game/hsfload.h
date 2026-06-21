#ifndef _GAME_HSFLOAD_H
#define _GAME_HSFLOAD_H

#include "game/hsfformat.h"

HSF_DATA *LoadHSF(void *data);
void ClusterAdjustObject(HSF_DATA *model, HSF_DATA *srcModel);
char *SetName(u32 *strOfs);
char *MakeObjectName(s8 *name);
s32 CmpObjectName(char *name1, char *name2);

#endif