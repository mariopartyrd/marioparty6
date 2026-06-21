#ifndef _GAME_ENVELOPE_EXEC_H
#define _GAME_ENVELOPE_EXEC_H

#include "game/hsfformat.h"

void InitEnvelope(HSF_DATA *hsf);
void EnvelopeProc(HSF_DATA *hsf);
void InitVtxParm(HSF_DATA *hsf);

extern HuVecF *Vertextop;

#endif
