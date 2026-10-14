#ifndef _GAME_MIC_H
#define _GAME_MIC_H

#include <dolphin/types.h>
#include <dolphin/mic.h>
#include <dolphin/m2s.h>

#include "game/window.h"

#define HUMC_CONTEXT_MAX 8
#define HUMC_CONTEXT_NULL -1

#define HUMC_SESSION_MAX 4
#define HUMC_SESSION_NULL -1
#define HUMC_SESSION_RESET -2

#define HUMC_RESPONSE_MAX 256
#define HUMC_RESPONSE_WORD_MAX 10

#define HUMC_RESPONSE_READY 0
#define HUMC_RESPONSE_DROPPED -1
#define HUMC_RESPONSE_ABNORMCOND -2
#define HUMC_RESPONSE_TIMEOUT -3
#define HUMC_RESPONSE_OVERLOAD -4

#define HUMC_FLAG_NULL -1
#define HUMC_FLAG_USE_OFF 0
#define HUMC_FLAG_USE 1
#define HUMC_FLAG_USE_WIN 2

#define HUMC_SEL_POS_LEFT (-10000.0)
#define HUMC_SEL_POS_RIGHT (-10001.0)
#define HUMC_SEL_POS_TOP (-10002.0)
#define HUMC_SEL_POS_BOTTOM (-10003.0)

#define HUMC_SEL_MODE_NULL -1
#define HUMC_SEL_MODE_IDLE 0
#define HUMC_SEL_MODE_RESPONSE 1
#define HUMC_SEL_MODE_EXIT 2



typedef struct HuMCResponseWord_s {
    u16 conf; //0x00
    s16 nbrWords; //0x02
    s16 *itemP; //0x04
} HUMC_RESPONSE_WORD;

typedef struct HuMCResponse_s {
    s16 result; //0x00
    u16 conf; //0x02
    s16 nbrWords; //0x04
    s16 *itemP; //0x08
    s32 val; //0x0C
    HUMC_RESPONSE_WORD words[HUMC_RESPONSE_WORD_MAX];
} HUMC_RESPONSE;

typedef void (*HUMC_CONTEXT_CALLBACK)(HUMC_RESPONSE *resp);
typedef s16 HUMC_CONTEXTID;
typedef s16 HUMC_SESSIONID;

void HuMCSysInit(void);
s32 HuMCInit(s16 lang);
void HuMCClose(void);

HUMC_CONTEXTID HuMCContextCreate(char *path);
void HuMCContextKill(HUMC_CONTEXTID ctxId);

BOOL HuMCDelayContextStart(HUMC_CONTEXTID ctxId);
s32 HuMCDelayResultGet(void);
s32 HuMCDelayResponseResultGet(void);

BOOL HuMCContextStart(HUMC_CONTEXTID ctxId, HUMC_CONTEXT_CALLBACK cb);
s32 HuMCResponseResultGet(void);
HUMC_RESPONSE *HuMCCurResponseGet(void);

s32 HuMCButtonGet(void);
s32 HuMCButtonDownGet(void);

s32 HuMCProbe(s32 chan);
s32 HuMCMount(s32 chan);

void HuMCShiftsSet(s32 shift);
void HuMCMicSprCreate(float posX, float posY);
void HuMCMicSprKill(void);

int HuMCMicGet(void);
int HuMCMicSaveGet(void);
void HuMCMicSet(int flag);

void HuMCSelWinCreate(float posX, float posY);
void HuMCSelWinKill(void);
HUWINID HuMCSelWinItemRandSet(u32 mesBase, s16 maxItem, s16 firstItem, s16 numItem, s16 padNo);
HUWINID HuMCSelWinItemSet(u32 mesBase, s16 numItem, s16 padNo);
BOOL HuMCSelWinCheck(void);

s16 HuMCSelModeGet(void);
void HuMCSelModeSet(s16 mode);

void HuMCListenerMQCreate(HUMC_CONTEXTID ctxId, HUMC_CONTEXT_CALLBACK cb, u8 pad);
void HuMCListenerMQKill(void);

void HuMCListenerMQMaxTimeSet(float s);
float HuMCListenerMQMaxTimeGet(void);
int HuMCListenerMQWait(void);

BOOL HuMCSelWinCheck(void);
void HuMCSelWinKill(void);
s16 HuMCSelWinWaitResponse(void);

void HuMCSessionSet(HUMC_SESSIONID sessionId);
void HuMCSessionClose(void);
void HuMCSessionKill(HUMC_SESSIONID sessionId);
BOOL HuMCRetypeResponseCheck(void);
int HuMCRetypeResponse(u32 mesBase);

void HuMCSessionReset(void);
void HuMCThresholdSet(s32 threshold);

void HuMCPeriodicProc(void);

void HuMCLevelInit(void);
s32 HuMCLevelGet(s32 minLevel, u32 n);


#endif