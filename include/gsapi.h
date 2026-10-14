#ifndef __GSAPI_H__
#define __GSAPI_H__

#include <dolphin/types.h>

#define GSAPI_MODE_NONE 0
#define GSAPI_MODE_ONESHOT (1 << 0)
#define GSAPI_MODE_REPEAT (1 << 1)

typedef struct _GSAPI_RESULT GSAPI_RESULT;

typedef enum _GSAPI_RESULTTYPE {
    GSAPI_RESULTTYPE_FINAL = 0,
    GSAPI_RESULTTYPE_PREMATURE = 1,
    GSAPI_RESULTTYPE_TIMEOUT = 2,
    GSAPI_RESULTTYPE_REJECTED = 3,
    GSAPI_RESULTTYPE_FORCE_DWORD = 134217727,
} GSAPI_RESULTTYPE;


typedef enum _GSAPI_NOTIFY_TYPE {
    GSAPI_NOTIFY_ASRRESULT = 0,
    GSAPI_NOTIFY_WAVEDATA = 1,
    GSAPI_NOTIFY_STARTLISTENING = 2,
    GSAPI_NOTIFY_STOPLISTENING = 3,
    GSAPI_NOTIFY_ABNORMCOND = 4,
    GSAPI_NOTIFY_ENERGYLEVEL = 5,
    GSAPI_NOTIFY_SNRLEVEL = 6,
    GSAPI_NOTIFY_SPEECHDETECTED = 7,
    GSAPI_NOTIFY_TSDETECTED = 8,
    GSAPI_NOTIFY_FORCE_DWORD = 134217727,
} GSAPI_NOTIFY_TYPE;


typedef enum _GSAPI_PARAMID {
    GSAPI_PARAM_TS_ENABLE = 0,
    GSAPI_PARAM_START_ENABLE = 1,
    GSAPI_PARAM_TIMEOUT = 2,
    GSAPI_PARAM_SENSITIVITY = 3,
    GSAPI_PARAM_MINSPEECH = 4,
    GSAPI_PARAM_MAXNBEST = 5,
    GSAPI_PARAM_FARTALK = 6,
    GSAPI_PARAM_AGCON = 7,
    GSAPI_PARAM_SELECTGENDER = 8,
    GSAPI_PARAM_CONF_THRESHOLD = 9,
    GSAPI_PARAM_TS = 10,
    GSAPI_PARAM_ACCURACY = 11,
    GSAPI_PARAM_REJECTION = 12,
    GSAPI_PARAM_GARBAGE = 13,
    GSAPI_PARAM_SECONDPASS_ENABLE = 14,
    GSAPI_PARAM_ABSOLUTE_THRESHOLD = 15,
    GSAPI_PARAM_USETREEDP = 16,
    GSAPI_PARAM_EXTRAEVENT_ENABLE = 17,
    GSAPI_PARAM_LAST = 18,
    GSAPI_PARAM_FORCE_DWORD = 134217727,
} GSAPI_PARAMID;

typedef struct _GSAPI_TIMEINFO {
    // total size: 0x8
    u32 TimeBegin; // offset 0x0, size 0x4
    u32 TimeEnd; // offset 0x4, size 0x4
} GSAPI_TIMEINFO;

typedef struct _GSAPI_DETAILEDWORD {
    // total size: 0x10
    u16 u16Conf; // offset 0x0, size 0x2
    GSAPI_TIMEINFO TimeInfo; // offset 0x4, size 0x8
    u32 WordId; // offset 0xC, size 0x4
} GSAPI_DETAILEDWORD;

typedef struct _GSAPI_DETAILEDRESULT {
    // total size: 0x10
    GSAPI_TIMEINFO TimeInfo; // offset 0x0, size 0x8
    u16 u16NbrWords; // offset 0x8, size 0x2
    GSAPI_DETAILEDWORD * pWords; // offset 0xC, size 0x4
} GSAPI_DETAILEDRESULT;

struct _GSAPI_RESULT {
    // total size: 0x14
    GSAPI_RESULTTYPE Type; // offset 0x0, size 0x4
    u16 u16NbrWords; // offset 0x4, size 0x2
    u16 u16Conf; // offset 0x6, size 0x2
    u32 *pWordIds; // offset 0x8, size 0x4
    GSAPI_RESULT * pNextBest; // offset 0xC, size 0x4
    GSAPI_DETAILEDRESULT * pDetails; // offset 0x10, size 0x4
};

typedef struct _GSAPI_SESSIONDATA {
    // total size: 0x8
    void *pData; // offset 0x0, size 0x4
    u32 DataSize; // offset 0x4, size 0x4
} GSAPI_SESSIONDATA;

typedef void *GSAPI_HENGINE;
typedef void *GSAPI_HCONTEXT;
typedef void *GSAPI_HHEAP;

typedef void (*GSAPI_CBRESULT)(GSAPI_HENGINE hEngine, u32 Action, GSAPI_RESULT *pResult, u32 u32UserData);
typedef void (*GSAPI_CBNOTIFY)(GSAPI_HENGINE hEngine, GSAPI_NOTIFY_TYPE NotifyType, u32 wParam, u32 lParam);


s32 gsapi_Init(GSAPI_CBRESULT cbResult, u32 u32UserData);
s32 gsapi_Close(void);
s32 gsapi_NotifySetCallback(GSAPI_CBNOTIFY cbNotify);
s32 gsapi_EngineOpen(s32 s32DevNum, GSAPI_HENGINE phEngine);
s32 gsapi_EngineSetMode(GSAPI_HENGINE hEngine, u32 Mode);
s32 gsapi_EngineSetParam(GSAPI_HENGINE hEngine, GSAPI_PARAMID ParamId, u32 ParamValue);
s32 gsapi_EngineClose(GSAPI_HENGINE hEngine);
s32 gsapi_EngineSessionDataImport(GSAPI_HENGINE hEngine, GSAPI_SESSIONDATA *pSessionData, BOOL bFreeMemory);
s32 gsapi_EngineSessionDataFree(GSAPI_SESSIONDATA *pSessionData);
s32 gsapi_EngineSessionDataExport(GSAPI_HENGINE hEngine, GSAPI_SESSIONDATA **ppSessionData);
s32 gsapi_EngineStart(GSAPI_HENGINE hEngine);
s32 gsapi_EngineRestart(GSAPI_HENGINE hEngine);
s32 gsapi_EngineStop(GSAPI_HENGINE hEngine);
s32 gsapi_EngineGetWordString(GSAPI_HENGINE hEngine, u32 WordId, char **WordString);

s32 gsapi_LanguageLoadBuffer(const u32 *pu32LngData, const u32 *pu32GcdData);
s32 gsapi_LanguageUnLoad();

s32 gsapi_ContextActivate(GSAPI_HENGINE hEngine, GSAPI_HCONTEXT hContext);
s32 gsapi_ContextOpen(const u32 *pu32CtxData, GSAPI_HCONTEXT phContext);
s32 gsapi_ContextSetGcdData(GSAPI_HCONTEXT hContext, const u32 *pu32GcdData);
s32 gsapi_ContextSetWrdData(GSAPI_HCONTEXT hContext, const u32 *pu32WrdData);
s32 gsapi_ContextSetParam(GSAPI_HCONTEXT hContext, GSAPI_PARAMID ParamId, u32 Value);
s32 gsapi_ContextDeActivate(GSAPI_HCONTEXT hContext);
s32 gsapi_SetHeapUserData(u32 u32HeapUserData);


#endif