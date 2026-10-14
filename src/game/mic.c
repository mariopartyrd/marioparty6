#include "game/mic.h"
#include "game/process.h"
#include "game/sprite.h"
#include "game/window.h"
#include "game/gamework.h"
#include "game/memory.h"
#include "game/init.h"
#include "game/object.h"
#include "game/armem.h"
#include "game/data.h"
#include "game/frand.h"
#include "game/pad.h"
#include "game/hu3d.h"
#include "game/dvd.h"

#include "datanum/win.h"
#include "messnum/micquiz.h"
#include "messnum/saf_test.h"

#include <gsapi.h>
#include <dolphin.h>
#include <string.h>

#define MC_HEAPNUM 0x30000000
#define MC_SESSION_HEAPNUM 0x30000001

#define MIC_BUFFER_SIZE (6144*sizeof(s16))

#define MC_STACK_SIZE 0x4000

#define MC_THREAD_START 0
#define MC_THREAD_START_IDLE 1
#define MC_THREAD_STOP 2
#define MC_THREAD_STOP_IDLE 3
#define MC_THREAD_ERROR 4

#define MC_THREADMSG(x) ((void *)(x))

typedef struct MCContext_s { 
    char path[64]; //0x00
    u32 *pCtxData; //0x40
    u32 *pGcdData; //0x44
    u32 *pWrdData; //0x48
    u16 *pBinData; //0x4C
    GSAPI_HCONTEXT hContext; //0x50
} MC_CONTEXT;

typedef struct MCSelWin_s {
    HUWINID winId; //0x00
    s16 mode; //0x02
    HUPROCESS *proc; //0x04
    float posX; //0x08
    float posY; //0x0C
    u8 *mes; //0x10
    u8 *itemTbl; //0x14
} MC_SELWIN;

static u32 *MCThreadStack;
static u32 *LngData;
static s16 *M2SBuffer;
static s16 *MicBuffer;
static s16 *MCResultData;
static s16 MCResultNum;
static s32 MCResponseLastNo;
static s32 MCResponseNo;
static MC_CONTEXT *MCContextData;
static HUMC_CONTEXTID ContextCallbackId;
static MC_CONTEXT *ContextCur;
static int MCMode;
static HUPROCESS *MCAnswerProc;
static BOOL MCStopListenF;
static HUMC_CONTEXT_CALLBACK MCContextCallback;
static HUSPR_GROUPID MCSprGrpId;
static OSMessage MCMessageArray[2];
static OSThreadQueue MCThreadQueue;
static s32 MC_gsapiEngineError;
static BOOL MicCloseF;
static GSAPI_SESSIONDATA *MCSessionReset;
static s8 MCSessionTimer[HUMC_SESSION_MAX];
static HUMC_CONTEXT_CALLBACK MCListenerCallback;
static s32 MCListenerMaxTime;
static BOOL ValidResultF;
static int LanguageNo;
static BOOL MCWrongDeviceF;
static HUPROCESS *MCListenerProc;
static BOOL MCInitF;
static u32 MCButtonDown;
static u32 MCButton;
static HUMC_RESPONSE *MCResponseBuf;
static GSAPI_HENGINE MC_gsapiEngine;


static OSThread MCThread;
static OSMessageQueue MCMessageQueue;
static GSAPI_SESSIONDATA *MCSession[HUMC_SESSION_MAX];



static int MCThreadState = MC_THREAD_STOP_IDLE;
static HUMC_SESSIONID MCSessionCur = HUMC_SESSION_NULL;
static HUMC_SESSIONID MCSessionPrev = HUMC_SESSION_NULL;
static u32 HeapNum = MC_HEAPNUM;
static HUMC_CONTEXTID MCYesNoCtxId = HUMC_CONTEXT_NULL;
static int MCMicValue = HUMC_FLAG_NULL;
static u32 MCThreshold = 4000;
static s32 M2SShift = -1;

static void InitMCSelWin(void);

void HuMCSysInit(void)
{
    s16 i;
    //TODO: Fix types
    MCResponseBuf = (void *)(MCContextData = NULL);
    MCInitF = FALSE;
    MicCloseF = TRUE;
    MCAnswerProc = NULL;
    MCThreadStack = NULL;
    ContextCallbackId = HUMC_CONTEXT_NULL;
    HeapNum = MC_HEAPNUM;
    InitMCSelWin();
    //TODO: Should this be an inline
    OSSetErrorHandler(OS_ERROR_FPE, NULL);
    __OSFpscrEnableBits &= ~0x10;
    if(HuMCProbe(1) == MIC_RESULT_READY) {
        HuMCMicSet(HUMC_FLAG_USE);
    } else {
        HuMCMicSet(HUMC_FLAG_USE_WIN);
    }
    MCSessionCur = MCSessionPrev = HUMC_SESSION_NULL;
    for(i=0; i<HUMC_SESSION_MAX; i++) {
        MCSession[i] = NULL;
        MCSessionTimer[i] = 0;
    }
}

static void MicResultCallBack(GSAPI_HENGINE hEngine, u32 Action, GSAPI_RESULT *pResult, u32 u32UserData);
static void MicNotifyCallBack(GSAPI_HENGINE hEngine, GSAPI_NOTIFY_TYPE NotifyType, u32 wParam, u32 lParam);
static void *MCThreadFunc(void *arg);
static void *MCDVDRead(char *path);
static void MCAnswerMain(void);

s32 HuMCInit(s16 lang)
{
    s32 gsapiResult; //r30
    s16 i; //r29
    s32 result; //r28
    static char *lngFileTbl[HUWIN_LANG_MAX] = {
        "/mic/lng/asr16v220_jpj200_float_4b.lng",
        "/mic/lng/asr16v220_enu300_float_4b.lng",
        "/mic/lng/asr16v220_enu300_float_4b.lng",
        "/mic/lng/asr16v220_enu300_float_4b.lng",
        "/mic/lng/asr16v220_enu300_float_4b.lng",
        "/mic/lng/asr16v220_enu300_float_4b.lng"
    };

    result = MIC_RESULT_READY;
    if(MCResponseBuf) {
        return;
    }
    MCWrongDeviceF = FALSE;
    if(MCMicValue != HUMC_FLAG_NULL) {
        HuMCMicSet(MCMicValue);
    }
    MCResponseBuf = HuMemDirectMallocNum(HEAP_HEAP, HUMC_RESPONSE_MAX*sizeof(HUMC_RESPONSE), MC_HEAPNUM);
    MCResponseNo = MCResponseLastNo = 0;
    MCResultData = HuMemDirectMallocNum(HEAP_HEAP, sizeof(s16)*64, MC_HEAPNUM);
    MCResultNum = 0;
    MCContextData = HuMemDirectMallocNum(HEAP_HEAP, sizeof(MC_CONTEXT)*HUMC_CONTEXT_MAX, MC_HEAPNUM);
    for(i=0; i<HUMC_CONTEXT_MAX; i++) {
        MCContextData[i].hContext = NULL;
    }
    MCSprGrpId = HUSPR_GROUP_NONE;
    ContextCallbackId = HUMC_CONTEXT_NULL;
    MCYesNoCtxId = HUMC_CONTEXT_NULL;
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return MIC_RESULT_READY;
    }
    M2SShift = -1;
    gsapi_SetHeapUserData(1000);
    if(MC_gsapiEngine) {
        gsapi_EngineClose(MC_gsapiEngine);
    }
    MCThreshold = 4000;
    MICInit();
    M2SInit();
    if(!(M2SBuffer = HuMemDirectMallocNum(HEAP_HEAP, M2S_SAMPLE_NUM*sizeof(s16), MC_HEAPNUM))) {
        OSReport("Mic Error: Not enough Memory\n");
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    if(!M2SSetBuffer(M2SBuffer)) {
        OSReport("Mic Error: M2SSetBuffer()\n");
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    if(!(MicBuffer = HuMemDirectMallocNum(HEAP_HEAP, MIC_BUFFER_SIZE, MC_HEAPNUM))) {
        OSReport("Mic Error: Not enough Memory\n");
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    result = HuMCMount(1);
    M2SSetPrerecordSamples(100);
    M2SSetMode(M2S_MODE_AUTO);
    MICStart(1);
    if((gsapiResult = gsapi_Init(MicResultCallBack, 0)) != 0) {
        OSReport("Mic Error: gsapi_Init() %x\n", gsapiResult);
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    lang = GWLanguageGet();
    LanguageNo = lang;
    if(!(LngData = MCDVDRead(lngFileTbl[LanguageNo]))) {
        OSReport("Mic Error: Read Language Failue\n");
        gsapi_Close();
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    if((gsapiResult = gsapi_LanguageLoadBuffer(LngData, NULL)) != 0) {
        OSReport("Mic Error: gsapi_LanguageLoadBuffer().%x\n", gsapiResult);
        gsapi_Close();
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    if((gsapiResult = gsapi_EngineOpen(0, &MC_gsapiEngine)) != 0) {
        OSReport("Mic Error: gsapi_EngineOpen().%x\n", gsapiResult);
        gsapi_Close();
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    MCSessionReset = NULL;
    if((gsapiResult = gsapi_NotifySetCallback(MicNotifyCallBack)) != 0) {
        OSReport("Mic Error: gsapi_NotifySetCallback().%x\n", gsapiResult);
        gsapi_Close();
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    if((gsapiResult = gsapi_EngineSetMode(MC_gsapiEngine, 1)) != 0) {
        OSReport("Mic Error: gsapi_EngineSetMode().%x\n", gsapiResult);
        gsapi_Close();
        HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
        return MIC_RESULT_FATAL_ERROR;
    }
    gsapi_EngineSetParam(MC_gsapiEngine, GSAPI_PARAM_CONF_THRESHOLD, MCThreshold);
    MCMode = 0;
    MCStopListenF = FALSE;
    MCContextCallback = NULL;
    OSInitMessageQueue(&MCMessageQueue, MCMessageArray, 2);
    MCThreadStack = HuMemDirectMallocNum(HEAP_HEAP, MC_STACK_SIZE, MC_HEAPNUM);
    OSCreateThread(&MCThread, MCThreadFunc, NULL, &MCThreadStack[MC_STACK_SIZE/sizeof(u32)], MC_STACK_SIZE, OS_PRIORITY_MAX, OS_THREAD_ATTR_DETACH);
    OSResumeThread(&MCThread);
    OSInitThreadQueue(&MCThreadQueue);
    MCInitF = TRUE;
    OSReport("HEAP HEAP Malloc Size %x\n", HuMemUsedMallocSizeGet(HEAP_HEAP));
    OSReport("OSAlloc Size Left %dkb(%x)\n", OSCheckHeap(currentHeapHandle)/1024, OSCheckHeap(currentHeapHandle));
    MCAnswerProc = HuPrcCreate(MCAnswerMain, 65000, MC_STACK_SIZE, 0);
    HuPrcSetStat(MCAnswerProc, HU_PRC_STAT_PAUSE_ON|HU_PRC_STAT_UPAUSE_ON);
    MCMicValue = -1;
    return result;
}

void HuMCClose(void)
{
    if(!MCResponseBuf) {
        return;
    }
    MCInitF = FALSE;
    if(MCThreadStack && HuMCMicSaveGet() == HUMC_FLAG_USE) {
        OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
        OSSleepThread(&MCThreadQueue);
    }
    HuMCMicSprKill();
    if(HuMCMicSaveGet() == HUMC_FLAG_USE) {
        MICStop(0);
        MICStop(1);
    }
    HuMCSessionReset();
    if(MCAnswerProc) {
        HuPrcKill(MCAnswerProc);
    }
    MCAnswerProc = NULL;
    if(MCListenerProc) {
        HuPrcKill(MCListenerProc);
    }
    MCListenerProc = NULL;
    if(MCThreadStack) {
        OSCancelThread(&MCThread);
    }
    MCThreadStack = NULL;
    HuMCSelWinKill();
    if(MC_gsapiEngine) {
        gsapi_EngineClose(MC_gsapiEngine);
        gsapi_LanguageUnLoad();
        gsapi_EngineClose(MC_gsapiEngine);
        gsapi_Close();
        MC_gsapiEngine = NULL;
    }
    HuMemDirectFreeNum(HEAP_HEAP, MC_HEAPNUM);
    HuMemDirectFreeNum(HEAP_MODEL, MC_HEAPNUM);
    //TODO: Fix types
    MCResponseBuf = (void *)(MCContextData = NULL);
    MICUnmount(0);
    MICUnmount(1);
    if(MCMicValue != HUMC_FLAG_NULL) {
        HuMCMicSet(MCMicValue);
    }
    MCMicValue = HUMC_FLAG_NULL;
}

static char *MakeMCFilename(char *base, char *ext);

HUMC_CONTEXTID HuMCContextCreate(char *path)
{
    MC_CONTEXT *ctx; //r31
    char *strBuf; //r30
    HUMC_CONTEXTID ctxId; //r29
    s32 result; //r28
    char *file; //r27
    char *slash; //r26
    
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return HUMC_CONTEXT_NULL;
    }
    strBuf = HuMemDirectMalloc(HEAP_HEAP, 256);
    strcpy(strBuf, path);
    if(LanguageNo != HUWIN_LANG_JAPAN) {
        for(slash=strBuf; *slash != '\0' && *slash != '.'; slash++);
        switch(LanguageNo) {
            case HUWIN_LANG_ENGLISH:
                strcat(strBuf, "US");
                break;
            
            default:
                strcat(strBuf, "US");
                break;
        }
    }
    for(ctxId=0; ctxId<HUMC_CONTEXT_MAX; ctxId++) {
        if(!MCContextData[ctxId].hContext) {
            break;
        }
        if(strcmp(path, MCContextData[ctxId].path) == 0) {
            HuMemDirectFree(strBuf);
            return ctxId;
        }
    }
    if(ctxId == HUMC_CONTEXT_MAX) {
        HuMemDirectFree(strBuf);
        return HUMC_CONTEXT_NULL;
    }
    ctx = &MCContextData[ctxId];
    strcpy(ctx->path, strBuf);
    file = MakeMCFilename(strBuf, ".ctx");
    ctx->pCtxData = MCDVDRead(file);
    file = MakeMCFilename(strBuf, ".gcd");
    ctx->pGcdData = MCDVDRead(file);
    file = MakeMCFilename(strBuf, ".wrd");
    ctx->pWrdData = MCDVDRead(file);
    file = MakeMCFilename(strBuf, ".bin");
    ctx->pBinData = MCDVDRead(file);
    result = gsapi_ContextLoadBuffer(ctx->pCtxData, &ctx->hContext);
    if(result != 0) {
        OSReport("Error CTX %x\n", result);
    }
    result = gsapi_ContextSetGcdData(ctx->hContext, ctx->pGcdData);
    if(result != 0) {
        OSReport("Error GCD %x\n", result);
    }
    result = gsapi_ContextSetWrdData(ctx->hContext, ctx->pWrdData);
    if(result != 0) {
        OSReport("Error WRD %x\n", result);
    }
    gsapi_ContextSetParam(ctx->hContext, GSAPI_PARAM_CONF_THRESHOLD, MCThreshold);
    if(omcurovl == DLL_m669dll
        || omcurovl == DLL_m670dll
        || omcurovl == DLL_m665dll
        || omcurovl == DLL_micquizdll
        || omcurovl == DLL_micquizishidll
        || omcurovl == DLL_micquizohdedll
        || omcurovl == DLL_micquizmyokodll) {
            gsapi_ContextSetParam(ctx->hContext, GSAPI_PARAM_TS, 200);
        } else {
            gsapi_ContextSetParam(ctx->hContext, GSAPI_PARAM_TS, 100);
        }
    HuMemDirectFree(strBuf);
    return ctxId;
}

void HuMCContextKill(HUMC_CONTEXTID ctxId)
{
    MC_CONTEXT *ctx;
    if(ctxId >= 0) {
        ctx = &MCContextData[ctxId];
        if(ctx->hContext) {
            gsapi_ContextUnLoad(ctx->hContext);
            HuMemDirectFree(ctx->pCtxData);
            HuMemDirectFree(ctx->pGcdData);
            HuMemDirectFree(ctx->pWrdData);
            ctx->hContext = NULL;
        }
    }
}

void HuMCThresholdSet(s32 threshold)
{
    MCThreshold = threshold;
}

static s32 ActivateContext(HUMC_CONTEXTID ctxId);

BOOL HuMCDelayContextStart(HUMC_CONTEXTID ctxId)
{
    s32 result;
    if(HuMCSelWinCheck()) {
        MCResponseNo = MCResponseLastNo = 0;
        HuMCSelModeSet(HUMC_SEL_MODE_RESPONSE);
        return FALSE;
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return FALSE;
    }
    if(MCMode != GSAPI_MODE_NONE || MicCloseF || ctxId < 0 || !MC_gsapiEngine) {
        return FALSE;
    }
    if((result = ActivateContext(ctxId)) != 0) {
        OSReport("MIC Error: gsapi_ContextActivate() %x\n", result);
        return FALSE;
    }
    ContextCur = &MCContextData[ctxId];
    MCResponseNo = MCResponseLastNo = 0;
    gsapi_EngineSetMode(MC_gsapiEngine, GSAPI_MODE_ONESHOT);
    gsapi_EngineSetParam(MC_gsapiEngine, GSAPI_PARAM_START_ENABLE, FALSE);
    gsapi_EngineSetParam(MC_gsapiEngine, GSAPI_PARAM_TS_ENABLE, FALSE);
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_START), OS_MESSAGE_BLOCK);
    MCMode = GSAPI_MODE_ONESHOT;
    MCStopListenF = FALSE;
    MCContextCallback = NULL;
    return TRUE;
}

s32 HuMCDelayResultGet(void)
{
    if(HuMCSelWinCheck()) {
        while(MCResponseNo == 0) {
            HuPrcVSleep();
        }
        return HuMCDelayResponseResultGet();
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return HUMC_RESPONSE_DROPPED;
    }
    if(MCMode != GSAPI_MODE_ONESHOT || MicCloseF) {
        return HUMC_RESPONSE_ABNORMCOND;
    }
    while(MCResponseNo == 0) {
        if(MCStopListenF) {
            OSReport("Error!!!!!!!!!!!!!!!!!!\n");
            OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
            OSSleepThread(&MCThreadQueue);
            OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_START), OS_MESSAGE_BLOCK);
            MCStopListenF = FALSE;
        }
        HuPrcVSleep();
    } 
    return HuMCDelayResponseResultGet();
    
}

s32 HuMCDelayResponseResultGet(void)
{
    s16 result;
    if(HuMCSelWinCheck()) {
        HuMCSelModeSet(HUMC_SEL_MODE_EXIT);
        if(MCResponseBuf[MCResponseLastNo].nbrWords == 0) {
            return HUMC_RESPONSE_DROPPED;
        } else {
            return MCResponseBuf[MCResponseLastNo].val;
        }
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return HUMC_RESPONSE_DROPPED;
    }
    if(MCMode != GSAPI_MODE_ONESHOT || MicCloseF) {
        return HUMC_RESPONSE_ABNORMCOND;
    }
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
    OSSleepThread(&MCThreadQueue);
    MCMode = GSAPI_MODE_NONE;
    if(MCResponseNo == 0) {
        return HUMC_RESPONSE_DROPPED;
    }
    result = MCResponseBuf[MCResponseLastNo].result;
    if(result != 0) {
        return result;
    }
    if(MCResponseNo == MCResponseLastNo) {
        return HUMC_RESPONSE_DROPPED;
    }
    if(MCResponseBuf[MCResponseLastNo].nbrWords == 0) {
        return HUMC_RESPONSE_DROPPED;
    } else {
        return MCResponseBuf[MCResponseLastNo].val;
    }
}


static inline void HuMCDelayResponseResultGet2(void)
{
    s16 result;
    if(HuMCSelWinCheck()) {
        HuMCSelModeSet(HUMC_SEL_MODE_EXIT);
        (void)MCResponseBuf[MCResponseLastNo].nbrWords;
        return;
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return;
    }
    if(MCMode != GSAPI_MODE_ONESHOT || MicCloseF) {
        return;
    }
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
    OSSleepThread(&MCThreadQueue);
    MCMode = GSAPI_MODE_NONE;
    if(MCResponseNo == 0) {
        return;
    }
    result = MCResponseBuf[MCResponseLastNo].result;
    if(result != 0) {
        return;
    }
    if(MCResponseNo == MCResponseLastNo) {
        return;
    }
    (void)MCResponseBuf[MCResponseLastNo].nbrWords;
}

BOOL HuMCContextStart(HUMC_CONTEXTID ctxId, HUMC_CONTEXT_CALLBACK cb)
{
    s32 result;
    if(HuMCSelWinCheck()) {
        if(cb) {
            MCContextCallback = cb;
        }
        MCResponseNo = MCResponseLastNo = 0;
        HuMCSelModeSet(HUMC_SEL_MODE_RESPONSE);
        return FALSE;
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return FALSE;
    }
    if(MCMode != GSAPI_MODE_NONE || MicCloseF || ctxId < 0 || !MC_gsapiEngine) {
        return FALSE;
    }
    if((result = ActivateContext(ctxId)) != 0) {
        OSReport("MIC Error: gsapi_ContextActivate() %x\n", result);
        return FALSE;
    }
    ContextCallbackId = ctxId;
    ContextCur = &MCContextData[ctxId];
    MCResponseLastNo = MCResponseNo = 0;
    gsapi_EngineSetMode(MC_gsapiEngine, GSAPI_MODE_REPEAT);
    gsapi_EngineSetParam(MC_gsapiEngine, GSAPI_PARAM_START_ENABLE, TRUE);
    gsapi_EngineSetParam(MC_gsapiEngine, GSAPI_PARAM_TS_ENABLE, TRUE);
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_START), OS_MESSAGE_BLOCK);
    if(cb) {
        MCContextCallback = cb;
    } else {
        MCContextCallback = NULL;
    }
    MCMode = GSAPI_MODE_REPEAT;
    return TRUE;
}

s32 HuMCResponseResultGet(void)
{
    s16 result;
    if(HuMCSelWinCheck()) {
        HuMCSelModeSet(HUMC_SEL_MODE_EXIT);
        if(MCResponseBuf[MCResponseLastNo].nbrWords == 0) {
            return HUMC_RESPONSE_DROPPED;
        } else {
            return MCResponseBuf[MCResponseLastNo].val;
        }
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return HUMC_RESPONSE_DROPPED;
    }
    if(MCMode != GSAPI_MODE_REPEAT || MicCloseF) {
        return HUMC_RESPONSE_DROPPED;
    }
    MCContextCallback = NULL;
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
    OSSleepThread(&MCThreadQueue);
    
    result = MCResponseBuf[MCResponseLastNo].result;
    MCMode = GSAPI_MODE_NONE;
    if(result != 0) {
        return result;
    }
    if(MCResponseNo == MCResponseLastNo) {
        return HUMC_RESPONSE_DROPPED;
    }
    return MCResponseBuf[MCResponseLastNo].val;
}

static inline void HuMCResponseResultGet2(void)
{
    s16 result;
    if(HuMCSelWinCheck()) {
        HuMCSelModeSet(HUMC_SEL_MODE_EXIT);
        (void)MCResponseBuf[MCResponseLastNo].nbrWords;
        return;
    }
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return;
    }
    if(MCMode != GSAPI_MODE_REPEAT || MicCloseF) {
        return;
    }
    MCContextCallback = NULL;
    OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
    OSSleepThread(&MCThreadQueue);
    
    result = MCResponseBuf[MCResponseLastNo].result;
    MCMode = GSAPI_MODE_NONE;
    if(result != 0) {
        return;
    }
    if(MCResponseNo == MCResponseLastNo) {
        return;
    }
}


HUMC_RESPONSE *HuMCCurResponseGet(void)
{
    static HUMC_RESPONSE resp;
    if(MCResponseNo > MCResponseLastNo) {
        resp = MCResponseBuf[MCResponseLastNo];
        MCResponseLastNo++;
        if(MCResponseNo <= MCResponseLastNo) {
            MCResponseLastNo = MCResponseNo;
        }
    } else {
        resp.result = HUMC_RESPONSE_DROPPED;
        resp.conf = -1;
        resp.nbrWords = 0;
        resp.itemP = NULL;
        resp.val = -1;
        MCResponseLastNo = MCResponseNo;
    }
    return &resp;
}

s32 HuMCButtonGet(void)
{
    if(HuMCMicSaveGet() != HUMC_FLAG_USE || !MCResponseBuf) {
        return FALSE;
    } else {
        return MCButton;
    }
}

s32 HuMCButtonDownGet(void)
{
    if(HuMCMicSaveGet() != HUMC_FLAG_USE || !MCResponseBuf) {
        return FALSE;
    } else {
        return MCButtonDown;
    }
}

s32 HuMCProbe(s32 chan)
{
    s32 result;
    OSTick tick;
    tick = OSGetTick();
    while(OSTicksToMilliseconds(OSGetTick()-tick) < 500) {
        if((result = MICProbeEx(chan)) != MIC_RESULT_BUSY) {
            break;
        }
    }
    return result;
}

static void MCExtHandler(s32 chan, s32 result);

s32 HuMCMount(s32 chan)
{
    s32 result;
    s32 devResult;
    
    s32 deviceId;
    BOOL m2sResult;
    M2SClose();
    MicCloseF = TRUE;
    MCWrongDeviceF = FALSE;
    if((result = HuMCProbe(chan)) != MIC_RESULT_READY) {
        return result;
    }
    if((result = MICMount(chan, MicBuffer, MIC_BUFFER_SIZE, MCExtHandler)) != MIC_RESULT_READY) {
        if(result != MIC_RESULT_ACTIVE) {
            return result;
        }
        result = MIC_RESULT_READY;
    }
    {
        devResult = MICGetDeviceID(chan, &deviceId);
        if(devResult != MIC_RESULT_READY) {
            return devResult;
        }
        if(deviceId != 0) {
            MCWrongDeviceF = TRUE;
            return MIC_RESULT_WRONGDEVICE;
        }
    }
    {
        if((m2sResult = M2SSetActiveChannel(chan)) == 0) {
            OSReport("MIC Error: M2SSetActiveChannel() %d\n", m2sResult);
            return MIC_RESULT_FATAL_ERROR;
        }
    }
    MICSetGain(chan, 0);
    if(M2SSetShifts(M2SShift) == 0) {
        OSReport("M2SSetShifts Error\n");
    }
    MicCloseF = FALSE;
    MICStart(1);
    M2SOpen();
    return result;
}

static void MCExtHandler(s32 chan, s32 result)
{
    MicCloseF = TRUE;
    M2SStop();
    OSReport("Ouch!\n");
}

void HuMCShiftsSet(s32 shift)
{
    M2SShift = shift;
    M2SSetShifts(shift);
}

void HuMCMicSprCreate(float posX, float posY)
{
    void *data; //r31
    ANIMDATA *anim; //r30
    HUSPRID sprId; //r29
    if(HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return;
    }
    data = HuAR_ARAMtoMRAMFileRead(WIN_ANM_icon_mic, MC_HEAPNUM, HEAP_MODEL);
    anim = HuSprAnimRead(data);
    MCSprGrpId = HuSprGrpCreate(1);
    sprId = HuSprCreate(anim, 1, 0);
    HuSprGrpMemberSet(MCSprGrpId, 0, sprId);
    HuSprGrpPosSet(MCSprGrpId, 0, 0);
    HuSprPosSet(MCSprGrpId, 0, posX, posY);
    HuSprDispOff(MCSprGrpId, 0);
}

void HuMCMicSprKill(void)
{
    if(MCSprGrpId != HUSPR_GROUP_NONE) {
        HuSprGrpKill(MCSprGrpId);
        MCSprGrpId = HUSPR_GROUP_NONE;
    }
}

int HuMCMicGet(void)
{
    if(MCMicValue != HUMC_FLAG_NULL) {
        return MCMicValue;
    } else {
        return GwCommon.mic;
    }
}

int HuMCMicSaveGet(void)
{
    return GwCommon.mic;
}

void HuMCMicSet(int flag)
{
    if(MCResponseBuf) {
        MCMicValue = flag;
    } else {
        GwCommon.mic = flag;
    }
}

static MC_SELWIN MCSelWinWork;

static void InitMCSelWin(void)
{
    MCSelWinWork.winId = HUWIN_NONE;
    MCSelWinWork.proc = NULL;
}

static void MCSelWinFunc(void);

void HuMCSelWinCreate(float posX, float posY)
{
    MCSelWinWork.winId = HUWIN_NONE;
    if(HuMCMicSaveGet() != HUMC_FLAG_USE_WIN) {
        return;
    }
    MCSelWinWork.winId = HuWinCreate(-100, -100, 32, 32, HUWIN_FRAME_DEFAULT);
    MCSelWinWork.posX = posX;
    MCSelWinWork.posY = posY;
    MCSelWinWork.mes = HuMemDirectMallocNum(HEAP_HEAP, 256, MC_HEAPNUM);
    MCSelWinWork.mes[0] = '\0';
    MCSelWinWork.itemTbl = HuMemDirectMallocNum(HEAP_HEAP, 64, MC_HEAPNUM);
    MCSelWinWork.mode = HUMC_SEL_MODE_IDLE;
    MCSelWinWork.proc = HuPrcCreate(MCSelWinFunc, 65000, 8192, 0);
    HuPrcSetStat(MCSelWinWork.proc, HU_PRC_STAT_PAUSE_ON|HU_PRC_STAT_UPAUSE_ON);
}

void HuMCSelWinKill(void)
{
    if(MCSelWinWork.winId != HUWIN_NONE) {
        HuWinKill(MCSelWinWork.winId);
    }
    MCSelWinWork.winId = HUWIN_NONE;
    if(MCSelWinWork.proc) {
        HuPrcKill(MCSelWinWork.proc);
    }
    MCSelWinWork.proc = NULL;
    if(MCSelWinWork.mes) {
        HuMemDirectFree(MCSelWinWork.mes);
    }
    MCSelWinWork.mes = NULL;
    if(MCSelWinWork.itemTbl) {
        HuMemDirectFree(MCSelWinWork.itemTbl);
    }
    MCSelWinWork.itemTbl = NULL;
}

HUWINID HuMCSelWinItemRandSet(u32 mesBase, s16 maxItem, s16 firstItem, s16 numItem, s16 padNo)
{
    s16 i; //r31
    u8 *mes; //r30
    s16 j; //r29
    
    int temp; //r28
    s16 rand; //r25
    
    float posX; //f31
    float posY; //f30
    
    HuVec2f size; //sp+0xC
    
    mes = MCSelWinWork.mes;
    if(HuMCMicSaveGet() != HUMC_FLAG_USE_WIN) {
        return HUWIN_NONE;
    }
    if(MCSelWinWork.winId != HUWIN_NONE) {
        HuWinKill(MCSelWinWork.winId);
    }
    if(maxItem < numItem) {
        OSReport("Error: HuMCSelWinItemRandSet() %d<%d\n", maxItem, numItem);
        return HUWIN_NONE;
    }
    for(i=0; i<maxItem; i++) {
        MCSelWinWork.itemTbl[i] = i;
    }
    temp = MCSelWinWork.itemTbl[0];
    MCSelWinWork.itemTbl[0] = MCSelWinWork.itemTbl[firstItem];
    MCSelWinWork.itemTbl[firstItem] = temp;
    for(j=0; j<HUMC_RESPONSE_WORD_MAX; j++) {
        for(i=1; i<maxItem; i++) {
            rand = frandmod(maxItem-1)+1;
            temp = MCSelWinWork.itemTbl[i];
            MCSelWinWork.itemTbl[i] = MCSelWinWork.itemTbl[rand];
            MCSelWinWork.itemTbl[rand] = temp;
        }
    }
    for(j=0; j<numItem-1; j++) {
        for(i=0; i<numItem-j-1; i++) {
            if(MCSelWinWork.itemTbl[i] > MCSelWinWork.itemTbl[i+1]) {
                temp = MCSelWinWork.itemTbl[i];
                MCSelWinWork.itemTbl[i] = MCSelWinWork.itemTbl[i+1];
                MCSelWinWork.itemTbl[i+1] = temp;
            }
        }
    }
    *mes++ = 0xB;
    for(i=0; i<numItem; i++) {
        *mes++ = 0x10;
        *mes++ = 0xF;
        *mes++ = 0x1F;
        *mes++ = i+1;
        if(i+1 < numItem) {
            *mes++ = 0xA;
        }
        HuWinInsertMesSizeGet(mesBase+MCSelWinWork.itemTbl[i], (int)i);
    }
    *mes++ = 0x0;
    HuWinMesMaxSizeGet(1, &size, MCSelWinWork.mes);
    if(MCSelWinWork.posX == HUMC_SEL_POS_LEFT) {
        posX = 16;
    } else if(MCSelWinWork.posX == HUMC_SEL_POS_RIGHT) {
        posX = 560-(size.x*0.8f);
    } else {
        posX = MCSelWinWork.posX;
    }
    if(MCSelWinWork.posY == HUMC_SEL_POS_TOP) {
        posY = 40;
    } else if(MCSelWinWork.posY == HUMC_SEL_POS_BOTTOM) {
        posY = 440-(size.y*0.8f);
    } else {
        posY = MCSelWinWork.posY;
    }
    MCSelWinWork.winId = HuWinCreate(posX, posY, size.x, size.y, HUWIN_FRAME_DEFAULT);
    HuWinScaleSet(MCSelWinWork.winId, 0.8f, 0.8f);
    for(i=0; i<maxItem; i++) {
        HuWinInsertMesSet(MCSelWinWork.winId, mesBase+MCSelWinWork.itemTbl[i], (int)i);
    }
    HuWinAttrSet(MCSelWinWork.winId, HUWIN_ATTR_NOCANCEL);
    HuWinAttrSet(MCSelWinWork.winId, HUWIN_ATTR_UPAUSE);
    winData[MCSelWinWork.winId].padMask = 1 << padNo;
    HuWinDispOff(MCSelWinWork.winId);
    if(MCSelWinWork.proc) {
        HuPrcKill(MCSelWinWork.proc);
    }
    MCSelWinWork.proc = HuPrcCreate(MCSelWinFunc, 65000, 8192, 0);
    HuPrcSetStat(MCSelWinWork.proc, HU_PRC_STAT_PAUSE_ON|HU_PRC_STAT_UPAUSE_ON);
    return MCSelWinWork.winId;
}

HUWINID HuMCSelWinItemSet(u32 mesBase, s16 numItem, s16 padNo)
{
    return HuMCSelWinItemRandSet(mesBase, numItem, 0, numItem, padNo);
}

BOOL HuMCSelWinCheck(void)
{
    if(MCSelWinWork.winId == HUWIN_NONE) {
        return FALSE;
    } else {
        return TRUE;
    }
}

s16 HuMCSelModeGet(void)
{
    if(!HuMCSelWinCheck()) {
        return HUMC_SEL_MODE_NULL;
    } else {
        return MCSelWinWork.mode;
    }
}

void HuMCSelModeSet(s16 mode)
{
    if(MCSelWinWork.winId != HUWIN_NONE) {
        if(mode == HUMC_SEL_MODE_EXIT) {
            HUWIN *winP = &winData[MCSelWinWork.winId];
            if(winP->stat == HUWIN_STAT_CHOICE) {
                winP->stat = HUWIN_STAT_NONE;
                winP->choice = -1;
            }
        }
        MCSelWinWork.mode = mode;
    }
}

static s32 MicWriteResponse(s16 result, int nbrWords, s16 *itemP, u16 conf, s32 val);

static void MCSelWinFunc(void)
{
    HUWIN *winP; //r31
    s16 choice; //r30
    s16 i; //r29
    s16 item; //sp+0x8
    while(1) {
        switch(MCSelWinWork.mode) {
            case HUMC_SEL_MODE_RESPONSE:
                if(MCSelWinWork.mes[0] == '\0') {
                    MCSelWinWork.mode = HUMC_SEL_MODE_IDLE;
                    break;
                }
                HuWinDispOn(MCSelWinWork.winId);
                HuWinMesSet(MCSelWinWork.winId, MESSNUM_PTR(MCSelWinWork.mes));
                winP = &winData[MCSelWinWork.winId];
                while(winP->stat != HUWIN_STAT_NONE) {
                    if(omUPauseFlag+omPauseChk() != 0) {
                        HuWinDispOff(MCSelWinWork.winId);
                    } else {
                        HuWinDispOn(MCSelWinWork.winId);
                    }
                    HuPrcVSleep();
                }
                HuWinChoiceSet(MCSelWinWork.winId, 0);
                while(winP->stat != HUWIN_STAT_NONE) {
                    if(omUPauseFlag+omPauseChk() != 0) {
                        HuWinDispOff(MCSelWinWork.winId);
                    } else {
                        HuWinDispOn(MCSelWinWork.winId);
                    }
                    HuPrcVSleep();
                }
                choice = winP->choice;
                if(choice >= 0) {
                    item = MCSelWinWork.itemTbl[choice];
                    for(i=0; i<HUMC_RESPONSE_WORD_MAX; i++) {
                        if(omUPauseFlag+omPauseChk() != 0) {
                            HuWinDispOff(MCSelWinWork.winId);
                        } else {
                            HuWinDispOn(MCSelWinWork.winId);
                        }
                        HuPrcVSleep();
                    }
                    MicWriteResponse(HUMC_RESPONSE_READY, 1, &item, 10000, item+1);
                } else if(choice == -1) {
                    MicWriteResponse(HUMC_RESPONSE_ABNORMCOND, 0, NULL, 0, -1);
                }
                if(MCContextCallback) {
                    HUMC_CONTEXT_CALLBACK cb = MCContextCallback;
                    cb(HuMCCurResponseGet());
                }
                MCSelWinWork.mode = HUMC_SEL_MODE_EXIT;
                break;
            
            case HUMC_SEL_MODE_EXIT:
                HuWinDispOff(MCSelWinWork.winId);
                MCSelWinWork.mode = HUMC_SEL_MODE_IDLE;
                break;
        }
        HuPrcVSleep();
    }
}

static void MCListenerMQFunc(void);

void HuMCListenerMQCreate(HUMC_CONTEXTID ctxId, HUMC_CONTEXT_CALLBACK cb, u8 pad)
{
    if(HuMCMicSaveGet() == HUMC_FLAG_USE_OFF) {
        return;
    }
    if(MCListenerProc) {
        HuMCListenerMQKill();
    }
    MCListenerCallback = cb;
    MCListenerProc = HuPrcCreate(MCListenerMQFunc, 65000, 8192, 0);
    MCListenerProc->property = (void *)((pad << 16)|(ctxId & 0xFFFF));
    MCListenerMaxTime = 0;
}

void HuMCListenerMQKill(void)
{
    MicWriteResponse(HUMC_RESPONSE_DROPPED, 0, NULL, 0, -1);
    HuMCDelayResponseResultGet2();
    if(MCListenerProc) {
        HuPrcKill(MCListenerProc);
    }
    MCListenerProc = NULL;
}

void HuMCListenerMQMaxTimeSet(float s)
{
    MCListenerMaxTime = s*60;
}

float HuMCListenerMQMaxTimeGet(void)
{
    return MCListenerMaxTime;
}

int HuMCListenerMQWait(void)
{
    s16 i; //r31
    HUMC_RESPONSE *resp; //r30
    s16 nullResp; //r29
    
    
    MCResponseNo = MCResponseLastNo = nullResp = 0;
    while(1) {
        if(nullResp != MCResponseNo) {
            break;
        }
        HuPrcVSleep();
    }
    for(i=MCResponseNo-1; i>=0; i--) {
        resp = &MCResponseBuf[i];
        if(resp->result == HUMC_RESPONSE_READY) {
            break;
        }
    }
    if(i == -1) {
        return -1;
    }
    return *resp->itemP;
}

static void MCListenerMQFunc(void)
{
    int ctxId; //r31
    HUMC_RESPONSE *resp; //r30
    s16 respNo; //r24
    s16 i; //r23
    HUPROCESS *proc; //r22
    
    u32 property; //r21
    BOOL btn; //r20
    u8 pad; //sp+0x8
    
    proc = HuPrcCurrentGet();
    property = (u32)proc->property;
    pad = property >> 16;
    ctxId = (u16)property;
    btn = HuMCButtonGet();
    while(1) {
        if((HuMCMicSaveGet() == HUMC_FLAG_USE && (HuMCButtonDownGet() || btn)) || (HuMCSelWinCheck() && (HuPadBtnDown[pad] & PAD_TRIGGER_R))) {
            btn = FALSE;
            HuMCDelayContextStart(ctxId);
            MCContextCallback = MCListenerCallback;
            if(HuMCMicSaveGet() == HUMC_FLAG_USE) {
                respNo = MCResponseNo;
                while(HuMCButtonGet()) {
                    if(respNo != MCResponseNo) {
                        resp = &MCResponseBuf[MCResponseNo-1];
                        if(resp->result == HUMC_RESPONSE_READY || resp->result == HUMC_RESPONSE_ABNORMCOND || resp->result == HUMC_RESPONSE_OVERLOAD) {
                            break;
                        }
                        respNo = MCResponseNo;
                        HuMCDelayResponseResultGet2();
                        HuMCDelayContextStart(ctxId);
                        MCContextCallback = MCListenerCallback;
                    }
                    
                    if(MCListenerMaxTime) {
                        MCListenerMaxTime--;
                        if(MCListenerMaxTime == 0) {
                            MicWriteResponse(HUMC_RESPONSE_TIMEOUT, 0, NULL, 0, -1);
                            break;
                        }
                    }
                    if(MCStopListenF) {
                        OSReport("Silence Error!!!!!!!!!!\n");
                        OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_STOP), OS_MESSAGE_BLOCK);
                        OSSleepThread(&MCThreadQueue);
                        OSSendMessage(&MCMessageQueue, MC_THREADMSG(MC_THREAD_START), OS_MESSAGE_BLOCK);
                        MCStopListenF = FALSE;
                    }
                    HuPrcVSleep();
                }
                if(respNo == MCResponseNo) {
                    for(i=0; i<60; i++) {
                        if(respNo != MCResponseNo) {
                            break;
                        }
                        HuPrcVSleep();
                    }
                    if(i == 60) {
                        MicWriteResponse(HUMC_RESPONSE_DROPPED, 0, NULL, 0, -1);
                    }
                }
                HuMCDelayResponseResultGet2();
                break;
            } else {
                while(MCResponseNo == 0) {
                    if(MCListenerMaxTime) {
                        MCListenerMaxTime--;
                        if(MCListenerMaxTime == 0) {
                            MicWriteResponse(HUMC_RESPONSE_TIMEOUT, 0, NULL, 0, -1);
                            break;
                        }
                    }
                    HuPrcVSleep();
                }
                HuMCDelayResponseResultGet2();
                break;
            }
        } else {
            if(MCListenerMaxTime) {
                MCListenerMaxTime--;
                if(MCListenerMaxTime == 0) {
                    MicWriteResponse(HUMC_RESPONSE_TIMEOUT, 0, NULL, 0, -1);
                    break;
                }
            }
        }
        HuPrcVSleep();
    }
    MCListenerProc = NULL;
    HuPrcEnd();
    while(1) {
        HuPrcVSleep();
    }
}

static void MCListenerFunc(void);

void HuMCListenerCreate(HUMC_CONTEXTID ctxId, HUMC_CONTEXT_CALLBACK cb, u8 pad)
{
    if(HuMCMicSaveGet() == HUMC_FLAG_USE_OFF) {
        return;
    }
    MCListenerCallback = cb;
    if(MCListenerProc) {
        HuPrcKill(MCListenerProc);
    }
    
    MCListenerProc = HuPrcCreate(MCListenerFunc, 65000, 8192, 0);
    MCListenerProc->property = (void *)((pad << 16)|(ctxId & 0xFFFF));
}

void HuMCListenerKill(void)
{
    MicWriteResponse(HUMC_RESPONSE_DROPPED, 0, NULL, 0, -1);
    HuMCResponseResultGet2();
    if(HuMCSelWinCheck()) {
        HuMCSelWinKill();
    }
    
    if(MCListenerProc) {
        HuPrcKill(MCListenerProc);
    }
    MCListenerProc = NULL;
}

static void MCListenerFunc(void)
{

    int ctxId; //r28
    s16 respNo; //r26
    s16 i; //r25
    HUPROCESS *proc; //r24
    u32 property; //r23
    u8 pad; //r17
    
    proc = HuPrcCurrentGet();
    property = (u32)proc->property;
    pad = property >> 16;
    ctxId = (u16)property;
    respNo = 100;
    while(1) {
        if((HuMCMicSaveGet() == HUMC_FLAG_USE && HuMCButtonGet()) || (HuMCSelWinCheck() && (HuPadBtnDown[pad] & PAD_TRIGGER_R))) {
            HuMCContextStart(ctxId, MCListenerCallback);
            respNo = MCResponseNo;
            if(HuMCMicSaveGet() == HUMC_FLAG_USE) {
                while(HuMCButtonGet()) {
                    HuPrcVSleep();
                }
                if(respNo == MCResponseNo) {
                    for(i=0; i<60; i++) {
                        if(respNo != MCResponseNo) {
                            break;
                        }
                        HuPrcVSleep();
                    }
                    HuPrcVSleep();
                }
            } else {
                HuPrcVSleep();
            }
            HuMCResponseResultGet2();
        }
        if(ValidResultF && MCListenerCallback) {
            HUMC_CONTEXT_CALLBACK cb = MCListenerCallback;
            cb(HuMCCurResponseGet());
            ValidResultF = FALSE;
        }
        HuPrcVSleep();
        
    }
}

static void MCDeviceMesExec(void);

static void MCAnswerMain(void)
{
    s16 respNo; //r26
    s32 result; //r25
    u32 prevBtn; //r21
    ValidResultF = FALSE;
    while(1) {
        do {
            HuPrcVSleep();
        } while(HuMCSelWinCheck() || HuMCMicSaveGet() != HUMC_FLAG_USE);
        if(MicCloseF && HuMCMicSaveGet() == HUMC_FLAG_USE) {
            if(omcurovl != DLL_mdseldll && omcurovl != DLL_s01dll && omcurovl != DLL_s02dll && omcurovl != DLL_s03dll) {
                result = MIC_RESULT_NOCARD;
                while(WipeCheckIn() || Hu3DPauseF) {
                    if((result = HuMCMount(1)) == MIC_RESULT_READY) {
                        break;
                    }
                    HuPrcVSleep();
                }
                if(result != MIC_RESULT_READY) {
                    MCDeviceMesExec();
                }
                OSReport("Mount OK!\n");
            } else {
                while(HuMCMount(1) != MIC_RESULT_READY) {
                    HuPrcVSleep();
                }
            }
        }
        MICGetButton(1, &MCButton);
        MCButton &= 0x10;
        MCButtonDown = MCButton&(prevBtn^MCButton);
        prevBtn = MCButton;
        if(MCContextCallback && respNo != MCResponseNo) {
            if(respNo < MCResponseNo) {
                HUMC_CONTEXT_CALLBACK cb = MCContextCallback;
                cb(HuMCCurResponseGet());
                if(ValidResultF) {
                    ValidResultF = FALSE;
                }
            }
            respNo = MCResponseNo;
        }
        if(MCThreadState == MC_THREAD_ERROR) {
            OSReport("MIC Error: Engine(%x).\n", MC_gsapiEngineError);
        }
        if(MCSprGrpId != HUSPR_GROUP_NONE) {
            if(MCThreadState == MC_THREAD_START_IDLE) {
                HuSprDispOn(MCSprGrpId, 0);
            } else {
                HuSprDispOff(MCSprGrpId, 0);
            }
        }
    }
    
}

void HuMCSessionSet(HUMC_SESSIONID sessionId)
{
    if(HuMCMicSaveGet() == HUMC_FLAG_USE && sessionId < HUMC_SESSION_MAX) {
        MCSessionCur = sessionId;
    }
}

void HuMCSessionClose(void)
{
    s16 i;
    for(i=0; i<HUMC_SESSION_MAX; i++) {
        if(MCSession[i]) {
            gsapi_EngineSessionDataFree(MCSession[i]);
        }
    }
    MCSessionCur = MCSessionPrev = HUMC_SESSION_NULL;
    for(i=0; i<HUMC_SESSION_MAX; i++) {
        MCSession[i] = NULL;
    }
    HuMemDirectFreeNum(HEAP_HEAP, MC_SESSION_HEAPNUM);
}

void HuMCSessionKill(HUMC_SESSIONID sessionId)
{
    if(HuMCMicSaveGet() != HUMC_FLAG_USE || !MC_gsapiEngine) {
        return;
    }
    if(MCSession[sessionId]) {
        gsapi_EngineSessionDataImport(MC_gsapiEngine, MCSessionReset, FALSE);
        gsapi_EngineSessionDataFree(MCSession[sessionId]);
        MCSession[sessionId] = NULL;
    }
}


static HUMC_RESPONSE MCRetypeResponseData;

BOOL HuMCRetypeResponseCheck(void)
{
    HUMC_RESPONSE *resp; //r31
    int i; //r30
    s16 num; //r29
    int j; //r28
    
    s16 item[3]; //sp+0xE
    s16 conf[3]; //sp+0x8
    
    MCRetypeResponseData.words[0].conf = MCRetypeResponseData.words[1].conf = MCRetypeResponseData.words[2].conf = 0;
    for(i=MCResponseNo-1; i>=0; i--) {
        resp = &MCResponseBuf[i];
        if(resp->result == HUMC_RESPONSE_READY) {
            break;
        }
    }
    if(i == -1) {
        return FALSE;
    }
    if(resp->words[0].conf == 0) {
        return FALSE;
    }
    for(i=num=0; i<3; i++) {
        if(resp->words[i].conf == 0) {
            continue;
        }
        for(j=0; j<num; j++) {
            if(*resp->words[i].itemP == item[j]) {
                break;
            }
        }
        if(j == num) {
            item[num] = *resp->words[i].itemP;
            conf[num] = resp->words[i].conf;
            num++;
        }
    }
    if(num <= 1) {
        return FALSE;
    }
    if(conf[0]-conf[1] <= 1000) {
        MCRetypeResponseData = *resp;
        return TRUE;
    } else {
        return FALSE;
    }
}

int HuMCRetypeResponse(u32 mesBase)
{
    int i; //r30
    HUMC_RESPONSE *resp; //r29
    s16 num; //r28
    int winId; //r27
    int j; //r25
    int result; //r22
    
    HuVec2f size; //sp+0x20
    s16 item[3]; //sp+0x1A
    s16 conf[3]; //sp+0x14
    
    resp = &MCRetypeResponseData;
    if(resp->words[0].conf == 0) {
        return -1;
    }
    if(MCYesNoCtxId == HUMC_CONTEXT_NULL) {
        MCYesNoCtxId = HuMCContextCreate("/mic/ctx/mic_yesno");
        if(MCYesNoCtxId == HUMC_CONTEXT_NULL) {
            return *resp->words[0].itemP;
        }
    }
    for(i=num=0; i<3; i++) {
        if(resp->words[i].conf == 0) {
            continue;
        }
        for(j=0; j<num; j++) {
            if(*resp->words[i].itemP == item[j]) {
                break;
            }
        }
        if(j == num) {
            item[num] = *resp->words[i].itemP;
            conf[num] = resp->words[i].conf;
            num++;
        }
    }
    if(num <= 1) {
        return -1;
    }
    for(i=0; i<num; i++) {
        HuWinInsertMesSizeGet(mesBase+item[i], 0);
        HuWinMesMaxSizeGet(1, &size, MICQUIZ_MESS_RETYPE);
        winId = HuWinExCreateFrame(HUWIN_POS_CENTER, 200, size.x, size.y, HUWIN_SPEAKER_NULL, HUWIN_FRAME_DEFAULT);
        HuWinExOpen(winId);
        HuWinMesSpeedSet(winId, 0);
        HuWinAttrSet(winId, HUWIN_ATTR_ALIGN_CENTER);
        HuWinInsertMesSet(winId, mesBase+item[i], 0);
        HuWinMesSet(winId, MICQUIZ_MESS_RETYPE);
        do {
            HuMCListenerMQCreate(MCYesNoCtxId, NULL, 0);
            result = HuMCListenerMQWait();
        } while(result != 0 && result != 1);
        HuWinExClose(winId);
        HuWinKill(winId);
        if(result == 0) {
            break;
        }
    }
    if(i == num) {
        return -1;
    } else {
        return item[i];
    }
}

void HuMCSessionReset(void)
{
    s32 result;
    if(HuMCMicSaveGet() != HUMC_FLAG_USE || MCSessionCur == HUMC_SESSION_NULL) {
        return;
    }
    if(!MCSessionReset) {
        result = gsapi_EngineSessionDataExport(MC_gsapiEngine, &MCSessionReset);
        if(result != 0) {
            OSReport("Reset Session Export >>>%x\n", result);
        }
    }
    if(MCSessionPrev >= 0) {
        gsapi_EngineSessionDataFree(MCSession[MCSessionPrev]);
        HeapNum = MC_SESSION_HEAPNUM;
        result = gsapi_EngineSessionDataExport(MC_gsapiEngine, &MCSession[MCSessionPrev]);
        if(result != 0) {
            OSReport("Session Export >>>%x:%x\n", result, MCSession[MCSessionPrev]);
        }
        HeapNum = MC_HEAPNUM;
    }
    MCSessionPrev = MCSessionCur;
    if(MCSessionCur == HUMC_SESSION_RESET) {
        result = gsapi_EngineSessionDataImport(MC_gsapiEngine, MCSessionReset, FALSE);
        if(result != 0) {
            OSReport("Session Import >>>%x:%x\n", result, MCSessionReset);
        }
    } else {
        if(MCSession[MCSessionCur]) {
            result = gsapi_EngineSessionDataImport(MC_gsapiEngine, MCSession[MCSessionCur], FALSE);
            if(result != 0) {
                OSReport("Session Import >>>%x\n", result);
            }
        }
    }
    if(MCSessionCur >= 0) {
        MCSessionTimer[MCSessionCur] = 0;
    }
}

static void MCDeviceMesExec(void)
{
    HUWINID winId; //r31
    int i; //r27
    HUSPR_GROUPID gid; //r26
    s32 result; //r25
    char pauseOld; //r19
    u8 pauseEnableOld; //r18
    ANIMDATA *anim; //r17
    
    void *data; //sp+0x28
    HUSPRID sprId; //sp+0x8
    
    pauseEnableOld = omSysPauseEnableFlag;
    pauseOld = omPauseChk();
    omUPauseFlag = TRUE;
    omSysPauseEnable(FALSE);
    omObjManPause(TRUE);
    HuPrcAllPause(TRUE);
    Hu3DPauseSet(TRUE);
    HuSprPauseSet(TRUE);
    HuPadRumbleAllStop();
    
    data = HuAR_ARAMtoMRAMFileRead(WIN_ANM_51, MC_HEAPNUM, HEAP_MODEL);
    anim = HuSprAnimRead(data);
    gid = HuSprGrpCreate(1);
    sprId = HuSprCreate(anim, 0, 0);
    HuSprGrpMemberSet(gid, 0, sprId);
    HuSprTPLvlSet(gid, 0, 0.9f);
    HuSprGrpPosSet(gid, 288, 240);
    HuSprScaleSet(gid, 0, 20, 20);
    
    winId = HuWinWarningCreate(HUWIN_POS_CENTER, HUWIN_POS_CENTER, 412, 120);
    HuWinPriSet(winId, 0);
    HuWinMesSpeedSet(winId, 0);
    HuWinWarningOpen(winId);
    MCButton = MCButtonDown = 0;
    if(MCWrongDeviceF) {
        result = MIC_RESULT_WRONGDEVICE;
    } else {
        result = MIC_RESULT_NOCARD;
    }
    while(result != MIC_RESULT_READY) {
        if(result == MIC_RESULT_WRONGDEVICE) {
            HuWinMesSet(winId, MESS_MIC_WRONGDEVICE);
        } else {
            HuWinMesSet(winId, MESS_MIC_NOCARD);
        }
        HuWinMesWait(winId);
        while(1) {
            for(i=0; i<4; i++) {
                if(HuPadBtnDown[i] & PAD_BUTTON_A) {
                    break;
                }
            }
            if(i != 4) {
                HuPrcVSleep();
                break;
            }
            HuPrcVSleep();
        }
        result = HuMCMount(1);
    }
    HuWinWarningClose(winId);
    HuWinWarningKill(winId);
    HuSprGrpKill(gid);
    MICStart(1);
    if(MCMode == GSAPI_MODE_REPEAT && ContextCallbackId >= 0) {
        MCMode = GSAPI_MODE_NONE;
        HuMCContextStart(ContextCallbackId, MCContextCallback);
    }
    omUPauseFlag = FALSE;
    omSysPauseEnable(pauseEnableOld);
    omObjManPause(FALSE);
    if(!pauseOld) {
        HuPrcAllPause(FALSE);
        Hu3DPauseSet(FALSE);
        HuSprPauseSet(FALSE);
    }
}

static void *MCThreadFunc(void *arg)
{
    OSMessage msg;
    while(1) {
        if(OSReceiveMessage(&MCMessageQueue, &msg, OS_MESSAGE_NOBLOCK)) {
            MCThreadState = (u32)msg;
        }
        if(HuMCSelWinCheck() || HuMCMicSaveGet() != HUMC_FLAG_USE) {
            OSSuspendThread(&MCThread);
            continue;
        }
        if(MicCloseF && MCThreadState != MC_THREAD_STOP) {
            if(MCThreadState == MC_THREAD_START_IDLE) {
                MCThreadState = MC_THREAD_STOP;
            } else {
                MCThreadState = MC_THREAD_STOP_IDLE;
            }
        }
        switch(MCThreadState) {
            case MC_THREAD_START:
                MC_gsapiEngineError = gsapi_EngineStart(MC_gsapiEngine);
                if(MC_gsapiEngineError < 0) {
                    MCThreadState = MC_THREAD_ERROR;
                    OSSuspendThread(&MCThread);
                } else {
                    MCThreadState = MC_THREAD_START_IDLE;
                }
                break;
            
            case MC_THREAD_START_IDLE:
                gsapi_EngineRestart(NULL);
                OSSetThreadPriority(&MCThread, OS_PRIORITY_MAX);
                break;
            
            case MC_THREAD_STOP:
                MC_gsapiEngineError = gsapi_EngineStop(MC_gsapiEngine);
                if(MC_gsapiEngineError < 0) {
                    MCThreadState = MC_THREAD_ERROR;
                    OSSuspendThread(&MCThread);
                } else {
                    MCThreadState = MC_THREAD_STOP_IDLE;
                }
                OSWakeupThread(&MCThreadQueue);
                break;
            
            case MC_THREAD_STOP_IDLE:
                OSSetThreadPriority(&MCThread, OS_PRIORITY_MAX);
                break;
        }
    }
}

static void MCThreadWakeup(OSAlarm* alarm, OSContext* context)
{
    OSSetThreadPriority(&MCThread, OS_PRIORITY_MAX);
}

void HuMCPeriodicProc(void)
{
    static OSAlarm alarm;
    if(!MCInitF || !MCResponseBuf || HuMCMicSaveGet() != HUMC_FLAG_USE) {
        return;
    }
    OSCreateAlarm(&alarm);
    OSSetAlarm(&alarm, OSMillisecondsToTicks(5), MCThreadWakeup);
    OSSetThreadPriority(&MCThread, OS_PRIORITY_MIN);
}

static void MicNotifyCallBack(GSAPI_HENGINE hEngine, GSAPI_NOTIFY_TYPE NotifyType, u32 wParam, u32 lParam)
{
    static char * const errName[6] = {
        "BAD SNR",
        "OVERLOAD",
        "TOO QUIET",
        "NO SIGNAL",
        "GARBLED SOUND",
        "POOR MIC"
    };
    switch(NotifyType) {
        case GSAPI_NOTIFY_ASRRESULT:
            if(lParam < 4000  && MCSessionCur >= 0 && MCSessionTimer[MCSessionCur]++ > 2) {
                HuMCSessionKill(MCSessionCur);
                MCSessionTimer[MCSessionCur] = 0;
            }
        
        case GSAPI_NOTIFY_ABNORMCOND:
            if(NotifyType == GSAPI_NOTIFY_ASRRESULT) {
                OSReport("threshold %d not reached\n", lParam);
                MicWriteResponse(HUMC_RESPONSE_ABNORMCOND, 0, NULL, 0, -1);
            } else {
                if(MCSessionCur >= 0 && MCSessionTimer[MCSessionCur]++ > 2) {
                    HuMCSessionKill(MCSessionCur);
                    MCSessionTimer[MCSessionCur] = 0;
                }
                if(lParam == 1) {
                    MicWriteResponse(HUMC_RESPONSE_OVERLOAD, 0, NULL, 0, -1);
                }
                OSReport("Abnormal condition #%d %s at sample %d\n", lParam, errName[lParam], wParam);
            }
            break;
        
        case GSAPI_NOTIFY_SPEECHDETECTED:
            OSReport("SPEECH DETECTED\n");
            break;
        
        case GSAPI_NOTIFY_STARTLISTENING:
            OSReport("LISTENING\n");
            MCStopListenF = FALSE;
            break;
        
        case GSAPI_NOTIFY_STOPLISTENING:
            OSReport("STOP LISTEN\n");
            MCStopListenF = TRUE;
            break; 
        
        case GSAPI_NOTIFY_TSDETECTED:
            OSReport("SILENCE DETECTED\n");
            break;
    }
}

static void MicResultExec(GSAPI_HENGINE hEngine, u32 Action, GSAPI_RESULT *pResult);

static void MicResultCallBack(GSAPI_HENGINE hEngine, u32 Action, GSAPI_RESULT *pResult, u32 u32UserData)
{
    MicResultExec(hEngine, Action, pResult);
}

static int MicMapWord(int word);
static s32 MicWriteResponseWord(s16 result, HUMC_RESPONSE_WORD *word);


static void MicResultExec(GSAPI_HENGINE hEngine, u32 Action, GSAPI_RESULT *pResult)
{
    GSAPI_RESULT *result; //r31
    s16 i; //r30
    s16 j; //r29
    s16 *respResult; //r27
    HUMC_RESPONSE_WORD words[HUMC_RESPONSE_WORD_MAX]; //sp+0xC
    char *szResult; //sp+0x8
    
    result = pResult;
    if(pResult->u16NbrWords != 0) {
        for(result=pResult, i=0; i<HUMC_RESPONSE_WORD_MAX; i++) {
            if(!result || result->u16NbrWords == 0) {
                words[i].conf = 0;
                words[i].nbrWords = 0;
            } else {
                
                if(-2+result->u16NbrWords+MCResultNum >= 64) {
                    MCResultNum  = 0;
                }
                respResult = &MCResultData[MCResultNum];
                for(j=1; j<pResult->u16NbrWords-1; j++) {
                    MCResultData[MCResultNum++] = MicMapWord(result->pWordIds[j]);
                    gsapi_GetResultString(MC_gsapiEngine, result->pWordIds[j], &szResult);
                    OSReport("%d:%d(%d),%s\n", i, MicMapWord(result->pWordIds[j]), result->u16Conf, szResult);
                }
                words[i].conf = result->u16Conf;
                words[i].nbrWords = -2+result->u16NbrWords;
                words[i].itemP = respResult;
                result=result->pNextBest;
            }
        }
        MicWriteResponseWord(HUMC_RESPONSE_READY, words);
        if(MCSessionCur >= 0) {
            MCSessionTimer[MCSessionCur] = 0;
        }
        ValidResultF = TRUE;
    } else {
        MicWriteResponse(HUMC_RESPONSE_ABNORMCOND, 0, NULL, 0, -1);
    }
}

static int MicMapWord(int word)
{
    u16 *data;
    int i;
    int order;
    int max;
    
    data = ContextCur->pBinData;
    max = *data++;
    for(i=order=0; i<max; data++, i++) {
        order += *data;
        if(order > word) {
            return i;
        }
    }
    return -1;
}

static s32 MicWriteResponse(s16 result, int nbrWords, s16 *itemP, u16 conf, s32 val)
{
    HUMC_RESPONSE *resp;
    s16 i;
    
    resp = &MCResponseBuf[MCResponseNo];
    resp->result = result;
    resp->conf = conf;
    resp->nbrWords = nbrWords;
    resp->itemP = itemP;
    resp->val = val;
    resp->words[0].conf = conf;
    resp->words[0].nbrWords = nbrWords;
    resp->words[0].itemP = itemP;
    for(i=1; i<HUMC_RESPONSE_WORD_MAX; i++) {
        resp->words[i].conf = 0;
    }
    MCResponseNo++;
    if(MCResponseNo >= 256) {
        MCResponseNo = 255;
    }
    return MCResponseNo;
}

static s32 MicWriteResponseWord(s16 result, HUMC_RESPONSE_WORD *word)
{
    HUMC_RESPONSE *resp;
    s16 i;
    
    resp = &MCResponseBuf[MCResponseNo];
    resp->result = result;
    resp->conf = word[0].conf;
    resp->nbrWords = word[0].nbrWords;
    resp->itemP = word[0].itemP;
    resp->val = *word[0].itemP;
    for(i=0; i<HUMC_RESPONSE_WORD_MAX; i++) {
        resp->words[i] = word[i];
    }
    MCResponseNo++;
    if(MCResponseNo >= 256) {
        MCResponseNo = 255;
    }
    return MCResponseNo;
}

static void *MCDVDRead(char *path)
{
    void *dst;
    void *src;
    
    src = HuDvdDataRead(path);
    dst = HuMemDirectMallocNum(HEAP_HEAP, DirDataSize, MC_HEAPNUM);
    DCFlushRange(dst, OSRoundUp32B(DirDataSize));
    memcpy(dst, src, DirDataSize);
    HuMemDirectFree(src);
    return dst;
}

static char *MakeMCFilename(char *base, char *ext)
{
    static char name[64];
    
    char *src;
    char *dst;
    
    for(src=base, dst=name; *src != '\0' && *src != '.'; *dst++=*src++);
    for(src=ext; *src != '\0'; *dst++=*src++);
    *dst = 0;
    return name;
}

static s32 ActivateContext(HUMC_CONTEXTID ctxId)
{
    s32 result;
    if((result = gsapi_ContextSetParam(MCContextData[ctxId].hContext, GSAPI_PARAM_MAXNBEST, 10)) != 0) {
        OSReport("NBEST Error %x\n", result);
    }
    result = gsapi_ContextActivate(MC_gsapiEngine, MCContextData[ctxId].hContext);
    HuMCSessionReset();
    return result;
}

s32 heap_Open(GSAPI_HHEAP * phHeap, u32 MaxHeapDebugElem)
{
    *phHeap = NULL;
    return 0;
}

s32 heap_Close(GSAPI_HHEAP hHeap)
{
    HuMemDirectFreeNum(HEAP_HEAP, HeapNum);
    return 0;
}

void *heap_Alloc(GSAPI_HHEAP hHeap, u32 u32Size)
{
    void *out = HuMemDirectTailMallocNum(HEAP_HEAP, u32Size, HeapNum);
    return out;
}

void *heap_Calloc(GSAPI_HHEAP hHeap, u32 u32Num, u32 u32Size) {
    void *out = heap_Alloc(hHeap, u32Num*u32Size);
    if(!out) {
        return NULL;
    } else {
        memset(out, 0, u32Num*u32Size);
        return out;
    }
}

void *heap_Realloc(GSAPI_HHEAP hHeap, void *pMem, u32 u32Size)
{
    return (!pMem) ? heap_Alloc(hHeap, u32Size) : HuMemDirectRealloc(HEAP_HEAP, pMem, u32Size);
}

void heap_Free(GSAPI_HHEAP hHeap, void *pMem)
{
    HuMemDirectFree(pMem);
}

#define MCLEVEL_MAX_SAMPLES 44100

static struct {
    s16 *buffer;
    s32 currTop;
    s32 currButtom;
} lvlBlock;


void HuMCLevelInit(void)
{
    lvlBlock.buffer = HuMemDirectMallocNum(HEAP_HEAP, MCLEVEL_MAX_SAMPLES*sizeof(s16), MC_HEAPNUM);
    memset(lvlBlock.buffer, 0, MCLEVEL_MAX_SAMPLES*sizeof(s16));
    lvlBlock.currTop = 0;
    lvlBlock.currButtom = 0;
}

s32 HuMCLevelGet(s32 minLevel, u32 n)
{
    BOOL old; //r25
    s16 *buffer; //r27
    s32 ii; //r26
    s32 currTop; //r31
    s32 currButtom; //r30
    s32 oldTop; //r23
    s32 gotSamples; //r29
    s32 stmp32; //r24
    float facc; //f31
    float flevel; //f30
    s32 slevel; //r28
    s32 rsize; //r22
    old = OSDisableInterrupts();
    if(!MICIsActive(1)) {
        memset(lvlBlock.buffer, 0, MCLEVEL_MAX_SAMPLES*sizeof(s16));
        lvlBlock.currTop = 0;
        lvlBlock.currButtom = 0;
        OSRestoreInterrupts(old);
        return -1;
    } else {
        buffer = lvlBlock.buffer;
        currTop = lvlBlock.currTop;
        currButtom = lvlBlock.currButtom;
        oldTop = currTop;
        rsize = MCLEVEL_MAX_SAMPLES-currTop;
        gotSamples = MICGetSamples(1, buffer+currTop, currButtom, rsize);
        currTop += gotSamples;
        currButtom = MICUpdateIndex(1, currButtom, gotSamples);
        if(currTop >= MCLEVEL_MAX_SAMPLES) {
            gotSamples = MICGetSamples(1, buffer, currButtom, oldTop);
            currTop = gotSamples;
            currButtom = MICUpdateIndex(1, currButtom, gotSamples);
        }
        lvlBlock.currTop = currTop;
        lvlBlock.currButtom = currButtom;
        OSRestoreInterrupts(old);
        currTop--;
        if(currTop < 0) {
            currTop += MCLEVEL_MAX_SAMPLES;
        }
        for(facc=0, ii=0; ii<1024; ii++) {
            stmp32 = buffer[currTop];
            facc += stmp32*stmp32;
            currTop--;
            if(currTop < 0) {
                currTop += MCLEVEL_MAX_SAMPLES;
            }
        }
        if(facc > 0) {
            flevel = 10*log10(facc/1099511600000.0f);
        } else {
            flevel = -100;
        }
        flevel = (flevel*n)/(-minLevel);
        slevel = flevel;
        slevel += n;
        if(slevel < 0) {
            slevel = 0;
        }
        return slevel;
    }
}

static s32 lbl_80287554[66];