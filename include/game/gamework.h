#ifndef _GAME_GAMEWORK_H
#define _GAME_GAMEWORK_H

#include "dolphin.h"
#include "game/gamework_enum.h"
#include "game/flag.h"

typedef struct GwPlayerConf_s {
    s16 charNo; //0x00
    s16 padNo; //0x02
    s16 comDif; //0x04
    s16 grpNo; //0x06
    s16 type; //0x08
} GW_PLAYER_CONF; //size=0x0A

typedef struct GwPlayer_s {
    u16 comDif : 2; //0x00 Bit 7
    u16 comF : 1; //0x00 Bit 5
    u16 charNo : 4; //0x00 Bit 1-4
    u16 autoSize : 2; //0x01 Bit 7 and 0x00 Bit 0
    u16 deadF : 1; //0x01 Bit 6
    u16 diceMode : 6; //0x01 Bits  0-5
    u16 team : 1; //0x02 Bit 7
    u16 skipEventF : 1;  //0x02 Bit 6
    u16 circuitOrder : 2;  //0x02 Bits 4-5
    s8 handicap; //0x03
    s8 padNo; //0x04
    s8 capsule[3]; //0x05-0x07
    u16 statusColor : 3; //0x08 Bits 5-7
    u16 moveF : 1; //0x08 Bit 4
    u16 jumpF : 1; //0x08 Bit 3
    u16 dispLightF : 1; //0x08 Bit 2
    u16 orderNo : 2; //0x08 Bits 0-1
    u16 diceNum : 2; //0x09 Bits 6-7
    u16 rank : 2; //0x09 Bits 4-5
    u16 koopaSuit : 1; //0x09 Bit 3
    u16 teamBackup : 1; //0x09 Bit 2
    s8 moveNum; //0x0A
    s16 masuId; //0x0C
    s16 masuIdPrev; //0x0E
    s16 masuIdNext; //0x10
    s16 capsuleUse; //0x12
    s8 plusMasuNum; //0x14
    s8 minusMasuNum; //0x15
    s8 capsuleMasuNum; //0x16
    s8 hatenaMasuNum; //0x17
    s8 koopaMasuNum;  //0x18
    s8 miracleMasuNum;  //0x19
    s8 kettouMasuNum;  //0x1A
    s8 donkeyMasuNum;  //0x1B
    s16 coin; //0x1C
    s16 coinTotalMg; //0x1E
    s16 coinTotal; //0x20
    s16 coinMax; //0x22
    s16 coinBattle; //0x24
    s16 mgCoin; //0x26
    s16 mgCoinBonus; //0x28
    s32 mgScore; //0x2C
    s16 star; //0x30
    s16 starMax; //0x32
    s16 capsuleUseNum; //0x34
    s16 starGraph[GW_PLAYER_GRAPH_SIZE]; //0x36
    s16 coinGraph[GW_PLAYER_GRAPH_SIZE]; //0x9E
} GW_PLAYER;

typedef struct GwDecaScore_s {
	s8 charNo; //0x00
    u16 mgScore[10]; //0x02
    u16 finalScore; //0x16
} GW_DECA_SCORE; //size=0x18

typedef struct GwSystem_s {
	//Board Game Type
	u8 partyF : 1; //0x00 Bit 7
    u8 tagF : 1; //0x00 Bit 6
	u8 storyComDif; //0x01
	//Board Menu Config
    u16 bonusStarF : 1; //0x02 Bit 7
    u16 mgInstDispF : 1; //0x02 Bit 6
    u16 mgComDispF : 1; //0x02 Bit 5
    u16 mgPack : 3; //0x02 Bits 2-4
    u16 messSpeed : 2; //0x02 Bits 0-1
    u16 saveMode : 2; //0x03 Bits 6-7
	u8 turnNo; //0x04
	u8 turnMax; //0x05
	u8 starFlag; //0x06
    u8 starTotal; //0x07
    u8 starPos : 3; //0x08 Bits 5-7
    u8 boardNo : 5; //0x08 Bits 0-4
	s8 last5Effect; //0x09
    s8 turnPlayerNo; //0x0A
	u8 storyCharBit; //0x0B
	s8 storyChar; //0x0C
	s16 hiddenBlockMasuId; //0x0E
	//Time Values
	u8 nextTime : 1; //0x10 Bit 7
	u8 curTime : 1; //0x10 Bit 6
	u8 timeTurn; //0x11
	u8 timeTurnMax; //0x12
	u32 boardWork[GW_BOARD_WORK_SIZE/4]; //0x14
	u8 comKeyDelay; //0x34
    u8 mgEvent : 4; //0x35 Bits 4-7
    s8 playerMode; //0x36
    u16 mgNo; //0x38
    s16 subGameNo; //0x3A
	u16 bankCoin; //0x3C
	s16 masuCapsule[256]; //0x3E
    u8 flag[8][16]; //0x23E
} GW_SYSTEM; //Size=0x2C0

typedef struct GwCommon_s {
	char magic[4]; //0x00
    u16 unk4;
	//General Options
	u8 languageNo : 3; //0x06 Bits 5-7
	u8 outputMode : 2; //0x06 Bits 3-4
	u8 vibrateF : 1; //0x06 Bit 2
	u8 mic : 2; //0x06 Bits 0-1
	OSTime time; //0x08
	char name[17]; //0x10
	u32 mgUnlock[4]; //0x24
	u32 record[GW_RECORD_MAX]; //0x34
	u16 charPlayNum[GW_BOARD_MAX][GW_CHARA_MAX]; //0x54
    u16 boardPlayNum[GW_BOARD_MAX]; //0x188
    u16 boardMaxStar[GW_BOARD_MAX][GW_CHARA_MAX]; //0x19E
    u16 boardMaxCoin[GW_BOARD_MAX][GW_CHARA_MAX]; //0x2D2
	u8 singleMgWinNum[GW_PLAYER_COM_DIF_MAX]; //0x406
	u8 singleBoardPlayNum[3]; //0x40A
	u64 singleBoardFlag[3]; //0x410
	//General Flags
	u8 saveEnableF : 1; //0x428 Bit 7
	u8 map7Unlock : 1; //0x428 Bit 6
	u8 veryHardUnlock : 1; //0x428 Bit 4
	u8 m562VeryHardUnlock : 1; //0x428 Bit 4
	u8 : 1;
	u8 unkFlag4 : 1; //0x428 Bit 2
	u8 viewOpening : 1; //0x428 Bit 1
	u8 viewEnding : 1; //0x428 Bit 0
	//Pause Config
	u8 partyMgInstDispF : 1; //0x429 Bit 7
    u8 partyMgComDispF : 1; //0x429 Bit 6
    u8 partyMgPack : 3; //0x429 Bits 3-5
    u8 partyMessSpeed : 2; //0x429 Bits 1-2
    u8 partySaveMode : 2; //0x42A Bits 6-7
	u8 storyMgInstDispF : 1; //0x42A Bit 5
    u8 storyMgComDispF : 1; //0x42A Bit 4
    u8 storyMgPack : 3; //0x42A Bits 1-3
    u8 storyMessSpeed : 2; //0x42B Bits 6-7
    u8 storySaveMode : 2; //0x42B Bits 4-5
	//Game Config
	u8 confTurnNum : 7; //0x42C Bits 1-7
	u8 confBonusStar : 1; //0x42C Bit 0
	u8 confTag : 1; //0x42D Bit 7
	u8 confSingleDiff : 2; //0x42D Bits 5-6
	s8 lastBoard; //0x42E
	GW_DECA_SCORE decaScore[GW_DECA_SCORE_MAX]; //0x430
	u16 decaMgRecord[10]; //0x520
	u8 renshoMgRecordNum; //0x534
	u8 renshoMgRecord[GW_RENSHO_MG_MAX]; //0x535
	u16 bankStar; //0x59A
	u16 bankStarAward; //0x59C
	u32 bankFlag[2]; //0x5A0
	u32 miracleBookFlag[2]; //0x5A8
	u32 mikeActRecord[3]; //0x5B0
	u8 unk5BC;
	u32 singlePrizeFlag[2]; //0x5C0
    u8 unk5C8[8];
} GW_COMMON; //size=0x5D0

extern GW_DECA_SCORE GwMgDecaScore[GW_PLAYER_MAX];
extern u32 GwSingleMgFlag[3];
extern GW_COMMON GwCommonOrig;
extern GW_COMMON GwCommon;
extern GW_SYSTEM GwSystem;
extern GW_PLAYER GwPlayer[GW_PLAYER_MAX];
extern GW_PLAYER_CONF GwPlayerConf[GW_PLAYER_MAX];

extern u32 GwSinglePrizeFlag[2];
extern s16 GwMgNightF;

extern s16 GwLanguage;
extern s16 GwLanguageSave;

static inline void GWLanguageSet(int languageNo)
{
    GwCommon.languageNo = languageNo;
}

static inline int GWLanguageGet(int languageNo)
{
    return GwCommon.languageNo;
}

static inline void GWVibrateSet(BOOL value)
{
    //HACK: to prevent prototype errors
    extern void HuPadRumbleAllStop(void);
    GwCommon.vibrateF = value;
	if(value == FALSE) {
		HuPadRumbleAllStop();
	}
}

static inline BOOL GWVibrateGet()
{
    return GwCommon.vibrateF;
}

static inline void GWBonusStarSet(BOOL flag)
{
    GwSystem.bonusStarF = flag;
}

static inline BOOL GWBonusStarGet(void)
{
    return GwSystem.bonusStarF;
}

static inline void GWMgInstDispSet(BOOL flag)
{
    GwSystem.mgInstDispF = flag;
}

static inline BOOL GWMgInstDispGet(void)
{
    return GwSystem.mgInstDispF;
}

static inline void GWMgComDispSet(BOOL flag)
{
    GwSystem.mgComDispF = flag;
}

static inline BOOL GWMgComDispGet(void)
{
    return GwSystem.mgComDispF;
}


static inline GW_MESS_SPEED GWMessSpeedGet(void)
{
    if (GwSystem.messSpeed == GW_MESS_SPEED_MAX) {
        GwSystem.messSpeed = GW_MESS_SPEED_NORMAL;
    }
    return GwSystem.messSpeed;
}

static inline void GWMessSpeedSet(GW_MESS_SPEED value)
{
	GwSystem.messSpeed = value;
	switch(value) {
		case GW_MESS_SPEED_FAST:
			GwSystem.comKeyDelay = 16;
			break;
			
		case GW_MESS_SPEED_SLOW:
			GwSystem.comKeyDelay = 48;
			break;
			
		default:
			GwSystem.comKeyDelay = 32;
			break;
	}
}

static inline void GWSaveModeSet(GW_SAVE_MODE value)
{
    GwSystem.saveMode = value;
}

static inline GW_SAVE_MODE GWSaveModeGet(void)
{
    if (GwSystem.saveMode == GW_SAVE_MODE_MAX) {
		GWSaveModeSet(GW_SAVE_MODE_NEVER);
    }
    return GwSystem.saveMode;
}

static inline BOOL GWPartyGet(void)
{
    return GwSystem.partyF;
}

static inline void GWPartySet(BOOL flag)
{
    GwSystem.partyF = flag;
}

static inline GW_PLAYER_COM_DIF GWStoryComDifGet(void)
{
    return GwSystem.storyComDif;
}

static inline void GWSingleMgWinInc(GW_PLAYER_COM_DIF comDif)
{
    if(GwCommon.singleMgWinNum[comDif] < GW_SINGLE_MGWIN_MAX) {
        GwCommon.singleMgWinNum[comDif]++;
    }
}

void GWInit(void);
void GWCommonInit(void);
int GWComKeyDelayGet(void);
void GWRecordSet(GW_RECORD_NO recordNo, u32 recordVal);
u32 GWRecordGet(GW_RECORD_NO recordNo);
void GWCharColorGet(GW_CHARA_ID charNo, GXColor *colorP);
void GWBoardPlayNumSet(GW_BOARD_NO boardNo, u8 num);
void GWBoardPlayNumAdd(GW_BOARD_NO boardNo, u8 num);
u16 GWBoardPlayNumGet(GW_BOARD_NO boardNo);
void GWBoardMaxStarSet(GW_BOARD_NO boardNo, s32 value, u8 charNo);
u16 GWBoardMaxStarGet(GW_BOARD_NO boardNo, u8 charNo);
void GWBoardMaxCoinSet(GW_BOARD_NO boardNo, s32 value, u8 charNo);
u16 GWBoardMaxCoinGet(GW_BOARD_NO boardNo, u8 charNo);
int GWCharPlayNumInc(GW_CHARA_ID charNo, GW_BOARD_NO boardNo);
int GWCharPlayNumGet(GW_CHARA_ID charNo, GW_BOARD_NO boardNo);
void GWCharPlayNumSet(GW_CHARA_ID charNo, GW_BOARD_NO boardNo, int value);
BOOL GWMgUnlockGet(int mgNo);
void GWMgUnlockSet(int mgNo);

s16 GWPlayerCoinGet(GW_PLAYER_ID playerNo);
void GWPlayerCoinSet(GW_PLAYER_ID playerNo, s16 coin);
void GWPlayerCoinAdd(GW_PLAYER_ID playerNo, s16 coin);
void GWPlayerStarSet(GW_PLAYER_ID playerNo, s16 star);
void GWPlayerStarAdd(GW_PLAYER_ID playerNo, s16 star);
s16 GWPlayerStarGet(GW_PLAYER_ID playerNo);

void GWSinglePrizeFlagSet(GW_SINGLE_PRIZE_FLAG flag);
BOOL GWSinglePrizeFlagGet(GW_SINGLE_PRIZE_FLAG flag);
void GWSingleDataInit(void);
void GWSinglePrizeSaveFlagSet(void);
BOOL GWSinglePrizeSaveFlagGet(GW_SINGLE_PRIZE_FLAG flag);
void GWSingleMgFlagSet(int mgNo);
BOOL GWSingleMgFlagGet(int mgNo);
void GWBankStarAdd(u16 num);
void GWBankStarSub(u16 num);
u16 GWBankStarGet(void);
u16 GWBankStarAwardGet(void);
void GWBankStarAwardReset(void);
void GWBankFlagSet(GW_BANK_FLAG flag);
void GWBankFlagReset(GW_BANK_FLAG flag);
BOOL GWBankFlagGet(GW_BANK_FLAG flag);
void GWMiracleBookFlagSet(GW_MIRACLE_BOOK_FLAG flag);
BOOL GWMiracleBookFlagGet(GW_MIRACLE_BOOK_FLAG flag);
void GWMgPlayNumSet(u16 num);
u16 GWMgPlayNumGet(void);
void GWMgPlayNumAdd(s16 num);
void GWMikeActRecordSet(s16 recordNo, u32 recordVal);
u32 GWMikeActRecordGet(s16 recordNo);
u16 GWSingleMgWinNumGet(void);
void GWSingleMgWinNumSet(u16 val);
u16 GWSingleMgRecordNumGet(void);
void GWSingleMgRecordNumSet(u16 val);
void GWSaveDebugSet(void);

#endif