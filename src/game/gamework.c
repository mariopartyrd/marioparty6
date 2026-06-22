#include "game/gamework.h"
#include "game/object.h"
#include "game/pad.h"
#include "game/flag.h"


GW_DECA_SCORE GwMgDecaScore[GW_PLAYER_MAX];
u32 GwSingleMgFlag[3];
GW_COMMON GwCommonOrig;
GW_COMMON GwCommon;
GW_SYSTEM GwSystem;
GW_PLAYER GwPlayer[GW_PLAYER_MAX];
GW_PLAYER_CONF GwPlayerConf[GW_PLAYER_MAX];

u32 GwSinglePrizeFlag[2];
static u16 SingleMgWinNum;
static u16 SingleMgRecordNum;
static u16 MgPlayNum;
s16 GwMgNightF;

s16 GwLanguage = 1; //HUWIN_LANG_ENGLISH
s16 GwLanguageSave = -1;

void GWInit(void)
{
    int i;
    GWCommonInit();
    InitFlag();
    memset(&GwPlayerConf[0], 0, sizeof(GwPlayerConf));
    memset(&GwPlayer[0], 0, sizeof(GwPlayer));
    memset(&GwSystem, 0, sizeof(GwSystem));
    for(i=0; i<GW_PLAYER_MAX; i++) {
        GW_PLAYER_CONF *conf = &GwPlayerConf[i];
        conf->charNo = i;
        conf->padNo = i;
        conf->comDif = GW_PLAYER_COM_DIF_EASY;
        conf->grpNo = i;
        if(!omPadDisableChk(i) || SIProbe(i) == 0x00040000) {
            conf->type = GW_PLAYER_TYPE_MAN;
        } else {
            conf->type = GW_PLAYER_TYPE_COM;
        }
    }
    GWLanguageSet(GwLanguage);
    GWVibrateSet(TRUE);
    GWMgInstDispSet(TRUE);
    GWMgComDispSet(TRUE);
    GWMessSpeedSet(GW_MESS_SPEED_NORMAL);
    GWSaveModeSet(GW_SAVE_MODE_ALWAYS);
    GWPartySet(TRUE);
    for(i=0; i<GW_PLAYER_MAX; i++) {
        GwCommon.singleMgWinNum[i] = 0;
    }
    for(i=0; i<3; i++) {
        GwCommon.singleBoardPlayNum[i] = 0;
        GwCommon.singleBoardFlag[i] = 0;
    }
}

void GWCommonInit(void)
{
    GW_COMMON *commonP = &GwCommonOrig;
    int i;
    int j;
    int charNo;
    memset(commonP, 0, sizeof(GW_COMMON));
    commonP->magic[0] = 0;
    commonP->unk4 = 0;
    commonP->languageNo = GwLanguage;
    commonP->outputMode = 1; //Stereo
    commonP->vibrateF = TRUE;
    commonP->mic = 1; //HUMC_FLAG_USE
    commonP->time = 0;
    commonP->mgUnlock[0] = 0;
    commonP->mgUnlock[1] = 0;
    commonP->mgUnlock[2] = 0;
    commonP->mgUnlock[3] = 0;
    //Pre-Unlock Mic Minigames
    commonP->mgUnlock[2] |= 0x1; //M665
    commonP->mgUnlock[2] |= 0x2; //M666
    commonP->mgUnlock[2] |= 0x4; //M667
    commonP->mgUnlock[2] |= 0x10; //M669
    commonP->mgUnlock[2] |= 0x20; //M670
    commonP->record[GW_RECORD_M606] = 60*60;
    commonP->record[GW_RECORD_M608] = 1080;
    commonP->record[GW_RECORD_M618] = 120*60;
    commonP->record[GW_RECORD_M638] = 60*60;
    commonP->record[GW_RECORD_M648] = 60*60;
    commonP->record[GW_RECORD_M650] = 60*60;
    commonP->record[GW_RECORD_M652] = 60*60;
    commonP->record[GW_RECORD_M678] = 0;
    for(j=0; j<GW_BOARD_MAX; j++) {
        for(charNo=0; charNo<GW_CHARA_MAX; charNo++) {
            commonP->charPlayNum[j][charNo] = 0;
        }
        commonP->boardPlayNum[j] = 0;
        for(i=0; i<GW_CHARA_MAX; i++) {
            commonP->boardMaxStar[j][i] = 0;
            commonP->boardMaxCoin[j][i] = 0;
        }
    }
    commonP->saveEnableF = FALSE;
    commonP->map7Unlock = FALSE;
    commonP->veryHardUnlock = FALSE;
    commonP->m562VeryHardUnlock = FALSE;
    commonP->unkFlag4 = FALSE;
    commonP->viewOpening = FALSE;
    commonP->viewEnding = FALSE;
    commonP->storyMgInstDispF = commonP->partyMgInstDispF = TRUE;
    commonP->storyMgComDispF = commonP->partyMgComDispF = TRUE;
    commonP->storyMgPack = commonP->partyMgPack = GW_MINIGAME_PACK_ALL;
    commonP->storyMessSpeed = commonP->partyMessSpeed = GW_MESS_SPEED_NORMAL;
    commonP->storySaveMode = commonP->partySaveMode = GW_SAVE_MODE_ALWAYS;
    commonP->confTurnNum = 20;
    commonP->confBonusStar = TRUE;
    commonP->confTag = FALSE;
    commonP->confSingleDiff = GW_PLAYER_COM_DIF_NORMAL;
    for(i=0; i<GW_DECA_SCORE_MAX; i++) {
        commonP->decaScore[i].charNo = GW_CHARA_NULL;
    }
    //Setup Decathlon minigame records
    commonP->decaMgRecord[0] = 10;
    commonP->decaMgRecord[1] = 100;
    commonP->decaMgRecord[2] = 60*60;
    commonP->decaMgRecord[3] = 1080;
    commonP->decaMgRecord[4] = 10;
    commonP->decaMgRecord[5] = 15;
    commonP->decaMgRecord[6] = 10;
    commonP->decaMgRecord[7] = 60*60;
    commonP->decaMgRecord[8] = 500;
    commonP->decaMgRecord[9] = 10;
    commonP->renshoMgRecordNum = 0;
    commonP->singlePrizeFlag[0] = commonP->singlePrizeFlag[1] = 0;
    commonP->bankStar = 0;
    commonP->bankStarAward = 0;
    commonP->bankFlag[0] = commonP->bankFlag[1] = 0;
    commonP->miracleBookFlag[0] = commonP->miracleBookFlag[1] = 0;
    commonP->mikeActRecord[0] = commonP->mikeActRecord[1] = commonP->mikeActRecord[2] = 300*60;
    commonP->unk5BC = 0;
    commonP->lastBoard = GW_BOARD_NULL;
    memcpy(&GwCommon, &GwCommonOrig, sizeof(GW_COMMON));
    GWVibrateSet(TRUE);
    GWMgInstDispSet(TRUE);
    GWMgComDispSet(TRUE);
    GWMessSpeedSet(GW_MESS_SPEED_NORMAL);
    GWSaveModeSet(GW_SAVE_MODE_ALWAYS);
    for(i=0; i<GW_PLAYER_COM_DIF_MAX; i++) {
        GwCommon.singleMgWinNum[i] = 0;
    }
    for(i=0; i<3; i++) {
        GwCommon.singleBoardPlayNum[i] = 0;
        GwCommon.singleBoardFlag[i] = 0;
    }
}

int GWComKeyDelayGet(void)
{
    if(GwSystem.comKeyDelay > 48) {
        GWMessSpeedSet(GW_MESS_SPEED_NORMAL);
    }
    return GwSystem.comKeyDelay;
}

void GWRecordSet(GW_RECORD_NO recordNo, u32 recordVal)
{
    if(!CheckFlag(FLAG_MG_PRACTICE)) {
        GwCommon.record[recordNo] = recordVal;
    }
}

u32 GWRecordGet(GW_RECORD_NO recordNo)
{
    return GwCommon.record[recordNo];
}

void GWCharColorGet(GW_CHARA_ID charNo, GXColor *colorP)
{
    GXColor color[GW_CHARA_MAX] = {
        { 227, 67, 67, 255 },
        { 68, 67, 227, 255 },
        { 241, 158, 220, 255 },
        { 67, 228, 68, 255 },
        { 138, 60, 180, 255 },
        { 146, 85, 55, 255 },
        { 227, 228, 68, 255 },
        { 40, 40, 40, 255 }
    };
    *colorP = color[charNo];
}

void GWBoardPlayNumSet(GW_BOARD_NO boardNo, u8 num)
{
    if(num > 99) {
        num = 99;
    }
    GwCommon.boardPlayNum[boardNo] = num;
}

void GWBoardPlayNumAdd(GW_BOARD_NO boardNo, u8 num)
{
    num += GwCommon.boardPlayNum[boardNo];
    if(num > 99) {
        num = 99;
    }
    GwCommon.boardPlayNum[boardNo] = num;
}

u16 GWBoardPlayNumGet(GW_BOARD_NO boardNo)
{
    return GwCommon.boardPlayNum[boardNo];
}

void GWBoardMaxStarSet(GW_BOARD_NO boardNo, s32 value, u8 charNo)
{
    GwCommon.boardMaxStar[boardNo][charNo] = value;
}

u16 GWBoardMaxStarGet(GW_BOARD_NO boardNo, u8 charNo)
{
    return GwCommon.boardMaxStar[boardNo][charNo];
}

void GWBoardMaxCoinSet(GW_BOARD_NO boardNo, s32 value, u8 charNo)
{
    GwCommon.boardMaxCoin[boardNo][charNo] = value;
}

u16 GWBoardMaxCoinGet(GW_BOARD_NO boardNo, u8 charNo)
{
    return GwCommon.boardMaxCoin[boardNo][charNo];
}

int GWCharPlayNumInc(GW_CHARA_ID charNo, GW_BOARD_NO boardNo)
{
    int playNum = GwCommon.charPlayNum[boardNo][charNo]+1;
    if(playNum > 99) {
        playNum = 99;
    }
    GwCommon.charPlayNum[boardNo][charNo] = playNum;
    return playNum;
}

int GWCharPlayNumGet(GW_CHARA_ID charNo, GW_BOARD_NO boardNo)
{
    return GwCommon.charPlayNum[boardNo][charNo];
}

void GWCharPlayNumSet(GW_CHARA_ID charNo, GW_BOARD_NO boardNo, int value)
{
    GwCommon.charPlayNum[boardNo][charNo] = value;
}

BOOL GWMgUnlockGet(int mgNo)
{
    int word;
    int bit;
    mgNo -= GW_MGNO_BASE;
    word = mgNo >> 5;
    bit = mgNo % 32;
    if(GwCommon.mgUnlock[word] & (1 << bit)) {
        return TRUE;
    } else {
        return FALSE;
    }
}

void GWMgUnlockSet(int mgNo)
{
    int word;
    int bit;
    mgNo -= GW_MGNO_BASE;
    word = mgNo >> 5;
    bit = mgNo % 32;
    GwCommon.mgUnlock[word] |= (1 << bit);
}

BOOL GWMgCustomGet(int mgNo)
{
    
}

void GWMgCustomSet(int mgNo)
{
    
}

void GWMgCustomReset(int mgNo)
{
    
}

s16 GWPlayerCoinGet(GW_PLAYER_ID playerNo)
{
    return GwPlayer[playerNo].coin;
}

void GWPlayerCoinSet(GW_PLAYER_ID playerNo, s16 coin)
{
    if(_CheckFlag(FLAG_MG_PRACTICE)) {
        return;
    }
    if(coin < 0) {
        coin = 0;
    }
    if(coin > GW_PLAYER_COIN_MAX) {
        coin = GW_PLAYER_COIN_MAX;
    }
    if(coin > GwPlayer[playerNo].coinMax) {
        GwPlayer[playerNo].coinMax = coin;
    }
    GwPlayer[playerNo].coin = coin;
}

void GWPlayerCoinAdd(GW_PLAYER_ID playerNo, s16 coin)
{
    GWPlayerCoinSet(playerNo, coin+GwPlayer[playerNo].coin);
}

void GWPlayerStarSet(GW_PLAYER_ID playerNo, s16 star)
{
    if(star < 0) {
        star = 0;
    }
    if(star > GW_PLAYER_STAR_MAX) {
        star = GW_PLAYER_STAR_MAX;
    }
    if(star > GwPlayer[playerNo].starMax) {
        GwPlayer[playerNo].starMax = star;
    }
    GwPlayer[playerNo].star = star;
}

void GWPlayerStarAdd(GW_PLAYER_ID playerNo, s16 star)
{
    GWPlayerStarSet(playerNo, star+GwPlayer[playerNo].star);
}

s16 GWPlayerStarGet(GW_PLAYER_ID playerNo)
{
    return GwPlayer[playerNo].star;
}

void GWSinglePrizeFlagSet(GW_SINGLE_PRIZE_FLAG flag)
{
    if(flag > GW_SINGLE_PRIZE_FLAG_MAX) {
        return;
    }
    GwSinglePrizeFlag[flag >> 5] |= 1 << (flag & 0x1F);
}

BOOL GWSinglePrizeFlagGet(GW_SINGLE_PRIZE_FLAG flag)
{
    if(flag > GW_SINGLE_PRIZE_FLAG_MAX) {
        return FALSE;
    }
    return GwSinglePrizeFlag[flag >> 5] & (1 << (flag & 0x1F));
}

void GWSingleDataInit(void)
{
    GwSinglePrizeFlag[0] = GwSinglePrizeFlag[1] = 0;
    GwSingleMgFlag[0] = GwSingleMgFlag[1] = GwSingleMgFlag[2] = 0;
    SingleMgWinNum = SingleMgRecordNum = 0;
}

void GWSinglePrizeSaveFlagSet(void)
{
    GwCommon.singlePrizeFlag[0] |= GwSinglePrizeFlag[0];
    GwCommon.singlePrizeFlag[1] |= GwSinglePrizeFlag[1];
}

BOOL GWSinglePrizeSaveFlagGet(GW_SINGLE_PRIZE_FLAG flag)
{
    if(flag > GW_SINGLE_PRIZE_FLAG_MAX) {
        return FALSE;
    }
    return GwCommon.singlePrizeFlag[flag >> 5] & (1 << (flag & 0x1F));
}

void GWSingleMgFlagSet(int mgNo)
{
    mgNo -= GW_MGNO_BASE;
    if(mgNo >= 96 || mgNo < 0) {
        return;
    }
    GwSingleMgFlag[mgNo >> 5] |= 1 << (mgNo & 0x1F);
}

BOOL GWSingleMgFlagGet(int mgNo)
{
    mgNo -= GW_MGNO_BASE;
    if(mgNo >= 96 || mgNo < 0) {
        return FALSE;
    }
    return GwSingleMgFlag[mgNo >> 5] & (1 << (mgNo & 0x1F));
}

void GWBankStarAdd(u16 num)
{
    if(GwCommon.bankStar+num > GW_BANK_STAR_MAX) {
        GwCommon.bankStar = GW_BANK_STAR_MAX;
    } else {
        GwCommon.bankStar += num;
    }
    if(GwCommon.bankStarAward+num > GW_BANK_STAR_MAX) {
        GwCommon.bankStarAward = GW_BANK_STAR_MAX;
    } else {
        GwCommon.bankStarAward += num;
    }
}

void GWBankStarSub(u16 num)
{
    if(GwCommon.bankStar < num) {
        GwCommon.bankStar = 0;
    } else {
        GwCommon.bankStar -= num;
    }
}

u16 GWBankStarGet(void)
{
    return GwCommon.bankStar;
}

u16 GWBankStarAwardGet(void)
{
    return GwCommon.bankStarAward;
}

void GWBankStarAwardReset(void)
{
    GwCommon.bankStarAward = 0;
}

void GWBankFlagSet(GW_BANK_FLAG flag)
{
    if(flag > GW_BANK_FLAG_MAX) {
        return;
    }
    GwCommon.bankFlag[flag >> 5] |= 1 << (flag & 0x1F);
}

void GWBankFlagReset(GW_BANK_FLAG flag)
{
    if(flag > GW_BANK_FLAG_MAX) {
        return;
    }
    GwCommon.bankFlag[flag >> 5] &= ~(1 << (flag & 0x1F));
}

BOOL GWBankFlagGet(GW_BANK_FLAG flag)
{
    if(flag > GW_BANK_FLAG_MAX) {
        return FALSE;
    }
    return GwCommon.bankFlag[flag >> 5] & (1 << (flag & 0x1F));
}

void GWMiracleBookFlagSet(GW_MIRACLE_BOOK_FLAG flag)
{
    if(flag > GW_MIRACLE_BOOK_FLAG_MAX) {
        return;
    }
    GwCommon.miracleBookFlag[flag >> 5] |= 1 << (flag & 0x1F);
}

BOOL GWMiracleBookFlagGet(GW_MIRACLE_BOOK_FLAG flag)
{
    if(flag > GW_MIRACLE_BOOK_FLAG_MAX) {
        return FALSE;
    }
    return GwCommon.miracleBookFlag[flag >> 5] & (1 << (flag & 0x1F));
}

void GWMgPlayNumSet(u16 num)
{
    MgPlayNum = num;
}

u16 GWMgPlayNumGet(void)
{
    return MgPlayNum;
}

void GWMgPlayNumAdd(s16 num)
{
    if(num < 0 && MgPlayNum < -num) {
        MgPlayNum = 0;
        return;
    }
    MgPlayNum += num;
    if(MgPlayNum > GW_MG_PLAY_NUM_MAX) {
        MgPlayNum = GW_MG_PLAY_NUM_MAX;
    }
}

void GWMikeActRecordSet(s16 recordNo, u32 recordVal)
{
    GwCommon.mikeActRecord[recordNo] = recordVal;
}

u32 GWMikeActRecordGet(s16 recordNo)
{
    return GwCommon.mikeActRecord[recordNo];
}

u16 GWSingleMgWinNumGet(void)
{
    return SingleMgWinNum;
}

void GWSingleMgWinNumSet(u16 val)
{
    SingleMgWinNum = val;
}

u16 GWSingleMgRecordNumGet(void)
{
    return SingleMgRecordNum;
}

void GWSingleMgRecordNumSet(u16 val)
{
    SingleMgRecordNum = val;
}

void GWSaveDebugSet(void)
{
    s16 i;
    for(i=0; i<4; i++) {
        GwCommon.mgUnlock[i] = 0xFFFFFFFF;
    }
    GwCommon.map7Unlock = TRUE;
    GwCommon.veryHardUnlock = TRUE;
    GwCommon.m562VeryHardUnlock = TRUE;
    GwCommon.bankStar = 1000;
}