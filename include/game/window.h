#ifndef _GAME_WINDOW_H
#define _GAME_WINDOW_H

#include "game/animdata.h"
#include "game/sprite.h"
#include "game/process.h"
#include "game/window_enum.h"

#include "dolphin.h"

#define MESSNUM_PTR(ptr) ((u32)ptr)

#define HUWIN_NONE -1
#define HUWIN_MAX 32

#define HUWIN_POS_CENTER -10000.0f

#define HUWIN_SPR_BEGIN 3
#define HUWIN_SPR_END 10
#define HUWIN_SPCFONT_BEGIN 11
#define HUWIN_SPCFONT_END 31

#define HUWIN_SPR_MAX HUWIN_SPCFONT_END

#define HUWIN_INSERTMES_MAX 16
#define HUWIN_CHOICE_MAX 16

#define HUWIN_CHOICE_NONE -1

#define HUWIN_COMKEY_NONE -1

#define HUWIN_PLAYER_NONE 0
#define HUWIN_PLAYER_1 (1 << 0)
#define HUWIN_PLAYER_2 (1 << 1)
#define HUWIN_PLAYER_3 (1 << 2)
#define HUWIN_PLAYER_4 (1 << 3)
#define HUWIN_PLAYER_ALL 0xF

#define HUWIN_ATTR_NONE 0
#define HUWIN_ATTR_OUTLINE (1 << 0)
#define HUWIN_ATTR_KEYWAIT_CLEAR (1 << 1)
#define HUWIN_ATTR_KEYWAIT_MESS (1 << 2)
#define HUWIN_ATTR_DISPOFF (1 << 3)
#define HUWIN_ATTR_NOCANCEL (1 << 4)
#define HUWIN_ATTR_TEXTDISABLE (1 << 5)
#define HUWIN_ATTR_CHOICEON (1 << 6)
#define HUWIN_ATTR_SETCOLOR (1 << 7)
#define HUWIN_ATTR_NOCR (1 << 8)
#define HUWIN_ATTR_WHITESPACE (1 << 9)
#define HUWIN_ATTR_SKIP_KEYWAIT (1 << 10)
#define HUWIN_ATTR_ALIGN_CENTER (1 << 11)
#define HUWIN_ATTR_ALIGN_RIGHT (1 << 12)
#define HUWIN_ATTR_CHOICE (1 << 13)
#define HUWIN_ATTR_UPAUSE (1 << 14)

#define HUWIN_MESCOL_BLACK 0
#define HUWIN_MESCOL_BLUE 1
#define HUWIN_MESCOL_RED 2
#define HUWIN_MESCOL_MAGENTA 3
#define HUWIN_MESCOL_GREEN 4
#define HUWIN_MESCOL_CYAN 5
#define HUWIN_MESCOL_ORANGE 6
#define HUWIN_MESCOL_WHITE 7
#define HUWIN_MESCOL_DARKGRAY 8
#define HUWIN_MESCOL_LIGHTGRAY 9
#define HUWIN_MESCOL_MAX 10

#define HUWIN_STAT_NONE 0
#define HUWIN_STAT_DRAWMES 1
#define HUWIN_STAT_KEYWAIT 2
#define HUWIN_STAT_CHOICE 3



typedef s16 HUWINID;

typedef void (*HUWIN_CALLBACK)(HUWINID winId, u32 mess, char c);

typedef struct WinCharEntry_s {
    u8 color;
    u8 fade;
    s16 x;
    s16 y;
    s16 charNo;
} WINCHARENTRY;

typedef struct WinChoice_s {
    u8 stat;
    s16 x;
    s16 y;
} WINCHOICE;

typedef struct HuWinWarning_s {
    HUWINID winId; //0x00
    u8 unk02[4];
    s16 scaleTimer[2]; //0x06
    ANIMDATA *glowAnim; //0x0C
    ANIMDATA *bgAnim[2]; //0x10
    HUPROCESS *process; //0x18
} HUWIN_WARNING;

typedef struct HuWin_s {
    u8 stat; //0x00
    u8 padMask; //0x01
    u8 disablePlayer; //0x02
    u8 bgPalNum; //0x03
    HUSPR_GROUPID grpId; //0x04
    HUSPRID sprId[HUWIN_SPR_MAX]; //0x06
    s16 mesSpeed; //0x44
    s16 mesTime; //0x46
    s16 keyWaitSprNo; //0x48
    s16 prio; //0x4A
    s16 drawNo; //0x4C
    u32 attr; //0x50
    ANIMDATA *animFrame[2]; //0x54
    s16 mesRectX; //0x5C
    s16 mesRectW; //0x5E
    s16 mesRectY; //0x60
    s16 mesRectH; //0x62
    s16 mesX; //0x64
    s16 mesY; //0x66
    s16 mesCol; //0x68
    s16 mesColShadow; //0x6A
    s16 charPadX; //0x6C
    s16 charPadY; //0x6E
    s16 winW; //0x70
    s16 winH; //0x72
    HuVec2f pos; //0x74
    HuVec2f scale; //0x7C
    float zRot; //0x84
    s16 charEntryNum; //0x88
    s16 charEntryMax; //0x8A
    WINCHARENTRY *charEntry; //0x8C
    s16 messSp; //0x90
    s32 unk94; //0x94
    char *messData; //0x98
    char *messDataStack[HUWIN_INSERTMES_MAX]; //0x9C
    char *messDataInsert[HUWIN_INSERTMES_MAX]; //0xDC
    char *mesCopy; //0x11C
    s16 choiceNum; //0x120
    s16 choice; //0x122
    s16 cursorSprNo; //0x124
    u8 choiceDisable[HUWIN_CHOICE_MAX]; //0x126
    WINCHOICE choiceData[HUWIN_CHOICE_MAX]; //0x136
    s16 scissorX; //0x196
    s16 scissorY; //0x198
    s16 scissorW; //0x19A
    s16 scissorH; //0x19C
    s16 tabW; //0x19E
    s16 pushKey; //0x1A0
    s16 activePadKey; //0x1A2
    s16 choiceEndSe; //0x1A4
    u8 ATTRIBUTE_ALIGN(32) mesPal[HUWIN_MESCOL_MAX][3]; //0x1C0
    HUWIN_WARNING *warning; //0x1E0
    HUWIN_CALLBACK callback; //0x1E4
    u32 origMes; //0x1E8
} HUWIN;

extern HUWIN winData[HUWIN_MAX];
extern void *messDataPtr;

void HuWindowInit(void);
void HuWinInit(s32 _messDataNo);
HUWINID HuWinCreate(float posX, float posY, s16 winW, s16 winH, s16 frame);
void HuWinFrameCreate(HUWIN *winP, s16 frame, s16 prio);
void HuWinFrameSet(HUWINID winId, s16 frame);

void HuWinKill(HUWINID winId);
void HuWinAllKill(void);
char *HuWinMesPtrGet(u32 messNum);
u32 HuWinMesMaxDirGet(void);
u32 HuWinMesMaxNumGet(u32 dirNum);
void *HuWinMesDataPtrGet(void *messData, u32 messNum);

void HuWinHomeClear(HUWINID winId);
void HuWinKeyWaitEntry(HUWINID winId);
void HuWinPosSet(HUWINID winId, float posX, float posY);
void HuWinScaleSet(HUWINID winId, float scaleX, float scaleY);
void HuWinZRotSet(HUWINID winId, float zRot);
void HuWinCenterPosSet(HUWINID winId, float centerX, float centerY);
void HuWinDrawNoSet(HUWINID winId, s16 drawNo);
void HuWinScissorSet(HUWINID winId, s16 x, s16 y, s16 w, s16 h);
void HuWinPriSet(HUWINID winId, s16 prio);
void HuWinAttrSet(HUWINID winId, u32 attr);
void HuWinAttrReset(HUWINID winId, u32 attr);
s16 HuWinStatGet(HUWINID winId);
void HuWinMesColSet(HUWINID winId, u8 mesCol);
void HuWinMesPalSet(HUWINID winId, u8 mesCol, u8 r, u8 g, u8 b);
void HuWinBGTPLvlSet(HUWINID winId, float tpLvl);
void HuWinBGColSet(HUWINID winId, GXColor *bgCol);
void HuWinMesSpeedSet(HUWINID winId, s16 mesSpeed);
void HuWinMesRead(void);
void HuWinMesLanguageSet(HUWIN_LANG lang);
void HuWinMesSet(HUWINID winId, u32 messNum);
void HuWinInsertMesSet(HUWINID winId, u32 messNum, s16 insertMesNo);
int HuWinAtoi(char *str);

s16 HuWinChoiceSet(HUWINID winId, s16 choiceNo);
s16 HuWinChoiceGet(HUWINID winId, s16 choiceNo);
s16 HuWinChoiceNumGet(HUWINID winId);
void HuWinChoiceDisable(HUWINID winId, s16 choiceNo);
s16 HuWinChoiceNowGet(HUWINID winId);
void HuWinMesWait(HUWINID winId);
s16 HuWinAnimSet(HUWINID winId, ANIMDATA *anim, s16 animBank, float posX, float posY);
s16 HuWinSprSet(HUWINID winId, HUSPRID sprId, float posX, float posY);
void HuWinSprPosSet(HUWINID winId, s16 sprNo, float posX, float posY);
void HuWinSprPriSet(HUWINID winId, s16 sprNo, s16 prio);
HUSPRID HuWinSprIDGet(HUWINID winId, s16 sprNo);
void HuWinSprKill(HUWINID winId, s16 sprNo);
void HuWinDispOff(HUWINID winId);
void HuWinDispOn(HUWINID winId);
void HuWinComKeyWait(s32 keyP1, s32 keyP2, s32 keyP3, s32 keyP4, s16 time);
void HuWinComKeySet(s32 keyP1, s32 keyP2, s32 keyP3, s32 keyP4);
void _HuWinComKeySet(s32 keyP1, s32 keyP2, s32 keyP3, s32 keyP4, s16 time);
void HuWinComKeyGet(HUWINID winId, u32 *key);
void HuWinComKeyReset(void);
void HuWinPadMaskSet(HUWINID winId, s16 mask);

void HuWinMesMaxSizeGet(s16 messCnt, HuVec2f *maxSize, ...);
void HuWinInsertMesSizeGet(u32 messNum, s16 insertMesNo);
void HuWinMesSizeCancelCRSet(s32 cancelCR);
void HuWinMesMaxSizeBetGet(HuVec2f *maxSize, u32 messStart, u32 messEnd);
s16 HuWinKeyWaitNumGet(u32 messNum);
void HuWinPushKeySet(HUWINID winId, s16 pushKey);
void HuWinDisablePlayerSet(HUWINID winId, u8 playerBit);
void HuWinDisablePlayerReset(HUWINID winId, u8 playerBit);
void HuWinCallbackStub(HUWINID winId, u32 mess, char c);
void HuWinCallbackSet(HUWINID winId, HUWIN_CALLBACK cb);

HUWINID HuWinExCreate(float x, float y, s16 w, s16 h, s16 speakerNo);
HUWINID HuWinExCreateFrame(float x, float y, s16 w, s16 h, s16 speakerNo, s16 frame);
void HuWinExOpen(HUWINID winId);
void HuWinExClose(HUWINID winId);
void HuWinExKill(HUWINID winId);
void HuWinExSpeakerSet(HUWINID winId, s16 speakerNo);

HUWINID HuWinWarningCreate(float posX, float posY, s16 winW, s16 winH);
void HuWinWarningOpen(HUWINID winId);
void HuWinWarningClose(HUWINID winId);
void HuWinWarningKill(HUWINID winId);

#endif
