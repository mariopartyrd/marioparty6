#include "game/audio.h"
#include "game/memory.h"
#include "game/armem.h"
#include "game/wipe.h"
#include "game/object.h"
#include "game/process.h"
#include "game/memory.h"
#include "game/gamework.h"

#define HUMSMHEAP_SIZE 0xBFC00

#define HUAUD_STREAM_MAX 16

#define MSM_FILE_PATH "/sound/MP6_SND.msm"
#define PDT_FILE_PATH "/sound/MP6_Str.pdt"

static int HuSePlay(int seId, MSM_SEPARAM *param);
static BOOL HuSeExitCheck(void);

static BOOL CharSeLoadF[GW_CHARA_MAX];
static s32 sndFxBuf[64][2];

static s16 Hu3DAudVol;
static s16 sndGroupBak;
static int auxANoBak;
static int auxBNoBak;
static s8 HuAuxAVol;
static s8 HuAuxBVol;
float Snd3DBackSurDisOffset;
float Snd3DFrontSurDisOffset;
float Snd3DStartDisOffset;
float Snd3DSpeedOffset;
float Snd3DDistOffset;
BOOL musicOffF;
u8 fadeStat;

void HuAudInit(void)
{
    MSM_INIT msmInit;
    MSM_ARAM msmAram;
    s32 result;
    s16 i;
    msmInit.heap = HuMemDirectMalloc(HEAP_SOUND, HUMSMHEAP_SIZE);
    msmInit.heapSize = HUMSMHEAP_SIZE;
    msmInit.msmPath = MSM_FILE_PATH;
    msmInit.pdtPath = PDT_FILE_PATH;
    msmInit.open = NULL;
    msmInit.read = NULL;
    msmInit.close = NULL;
    msmAram.skipARInit = TRUE;
    msmAram.aramEnd = HU_AMEM_BASE;
    result = msmSysInit(&msmInit, &msmAram);
    if(result < 0) {
        OSReport("MSM(Sound Manager) Error:Error Code %d\n", result);
        while(1);
    }
    if(OSGetSoundMode() == OS_SOUND_MODE_MONO) {
        msmSysSetOutputMode(SND_OUTPUTMODE_MONO);
    } else {
        msmSysSetOutputMode(SND_OUTPUTMODE_STEREO);
    }
    for(i=0; i<64; i++) {
        sndFxBuf[i][0] = -1;
    }
    for(i=0; i<GW_CHARA_MAX; i++) {
        CharSeLoadF[i] = FALSE;
    }
    sndGroupBak = MSM_GRP_NONE;
    auxANoBak = MSM_AUX_DEFAULT;
    auxBNoBak = MSM_AUX_DEFAULT;
    HuAuxAVol = HuAuxBVol = -1;
    fadeStat = FALSE;
    musicOffF = FALSE;
}

s32 HuAudStreamPlay(char *name, BOOL flag)
{
    return 0;
}

void HuAudStreamVolSet(s16 vol)
{
    AISetStreamVolLeft(vol);
    AISetStreamVolRight(vol);
    Hu3DAudVol = vol;
}

void HuAudStreamPauseOn(void)
{
    AISetStreamPlayState(AI_STREAM_STOP);
}

void HuAudStreamPauseOff(void)
{
    AISetStreamPlayState(AI_STREAM_START);
}

void HuAudStreamFadeOut(s32 streamNo)
{
    
}

void HuAudAllStop(void)
{
    HuAudSeqAllStop();
    HuAudFXAllStop();
    HuAudSStreamAllStop();
}

void HuAudFadeOut(s32 speed)
{
    HuAudFXAllStop();
    HuAudSeqAllFadeOut(speed);
    HuAudSStreamAllFadeOut(speed);
}

static void dummyfloat(unsigned int i)
{
    float y = i;
}

int HuAudFXPlay(int seId)
{

    if(HuSeExitCheck()) {
        return 0;
    }
    return HuAudFXPlayVolPan(seId, MSM_VOL_MAX, MSM_PAN_CENTER);
}

int HuAudFXPlayVol(int seId, s16 vol)
{
    if(omSysExitReq) {
        return 0;
    }
    return HuAudFXPlayVolPan(seId, vol, MSM_PAN_CENTER);
}

int HuAudFXPlayPan(int seId, s16 pan)
{
    MSM_SEPARAM seParam;
    if(omSysExitReq) {
        return 0;
    }
    seParam.flag = MSM_SEPARAM_PAN;
    seParam.pan = pan;
    return HuSePlay(seId, &seParam);
}

int HuAudFXPlayVolPan(int seId, s16 vol, s16 pan)
{
    MSM_SEPARAM seParam;
    if(omSysExitReq) {
        return 0;
    }
    seParam.flag = MSM_SEPARAM_VOL|MSM_SEPARAM_PAN;
    seParam.vol = vol;
    seParam.pan = pan;
    return HuSePlay(seId, &seParam);
}

static void AudFXPlayDelay(void);

typedef struct HuAudFXDelay_s {
    int seId;
    MSM_SEPARAM param;
    u16 delay;
} HUAUD_FX_DELAY;

void HuAudFXPlayDelay(int seId, u16 delay)
{
    HuAudFXPlayDelayVolPan(seId, MSM_VOL_MAX, MSM_PAN_CENTER, delay);
}

void HuAudFXPlayDelayVol(int seId, u8 vol, u16 delay)
{
    HuAudFXPlayDelayVolPan(seId, vol, MSM_PAN_CENTER, delay);
}

void HuAudFXPlayDelayPan(int seId, u8 pan, u16 delay)
{
    HuAudFXPlayDelayVolPan(seId, MSM_VOL_MAX, pan, delay);
}

void HuAudFXPlayDelayVolPan(int seId, u8 vol, u8 pan, u16 delay)
{
    if(omSysExitReq) {
        return;
    }
    if(delay == 0) {
        HuAudFXPlayVolPan(seId, vol, pan);
        return;
    } else {
        HUPROCESS *process = HuPrcCreate(AudFXPlayDelay, 1, 2304, 0);
        HUAUD_FX_DELAY *work = HuMemDirectMalloc(HEAP_HEAP, sizeof(HUAUD_FX_DELAY));
        process->property = work;
        work->param.flag = MSM_SEPARAM_VOL|MSM_SEPARAM_PAN;
        work->param.vol = vol;
        work->param.pan = pan;
        work->seId = seId;
        work->delay = delay;
    }
}

static void AudFXPlayDelay(void)
{
    HUPROCESS *process = HuPrcCurrentGet();
    HUAUD_FX_DELAY *work = process->property;
    int i;
    for(i=0; i<work->delay; i++) {
        if(omSysExitReq) {
            break;
        }
        HuPrcVSleep();
    }
    if(i == work->delay) {
        HuSePlay(work->seId, &work->param);
    }
    HuMemDirectFree(work);
    HuPrcEnd();
    while(1) {
        HuPrcVSleep();
    }
}


void HuAudFXStop(int seNo)
{
    msmSeStop(seNo, 0);
}

void HuAudFXAllStop(void)
{
    msmSeStopAll(FALSE, 0);
}

void HuAudFXFadeOut(int seNo, s32 speed)
{
    msmSeStop(seNo, speed);
}

void HuAudFXPanning(int seNo, s16 pan)
{
    MSM_SEPARAM param;
    if(omSysExitReq) {
        return;
    }
    param.flag = MSM_SEPARAM_PAN;
    param.pan = pan;
    msmSeSetParam(seNo, &param);
}

void HuAudFXListnerSet(Vec *pos, Vec *heading, float sndDist, float sndSpeed)
{
    if(omSysExitReq) {
      return;
    }
    HuAudFXListnerSetEX(pos, heading,
        sndDist + Snd3DDistOffset,
        sndSpeed + Snd3DSpeedOffset,
        Snd3DStartDisOffset,
        Snd3DFrontSurDisOffset + (0.25 * sndDist + Snd3DStartDisOffset),
        Snd3DBackSurDisOffset + (0.25 * sndDist + Snd3DStartDisOffset));
}

void HuAudFXListnerSetEX(Vec *pos, Vec *heading, float sndDist, float sndSpeed, float startDis, float frontSurDis, float backSurDis)
{
    MSM_SELISTENER listener;
    if(omSysExitReq) {
      return;
    }
    listener.flag = MSM_LISTENER_STARTDIS|MSM_LISTENER_FRONTSURDIS|MSM_LISTENER_BACKSURDIS;
    listener.startDis = startDis + Snd3DStartDisOffset;
    listener.frontSurDis = frontSurDis + Snd3DFrontSurDisOffset;
    listener.backSurDis = backSurDis + Snd3DBackSurDisOffset;
    msmSeSetListener(pos, heading, sndDist + Snd3DDistOffset, sndSpeed + Snd3DSpeedOffset, &listener);
    OSReport("//////////////////////////////////\n");
    OSReport("sndDist %f\n", sndDist);
    OSReport("sndSpeed %f\n", sndSpeed);
    OSReport("startDis %f\n", listener.startDis);
    OSReport("frontSurDis %f\n", listener.frontSurDis);
    OSReport("backSurDis %f\n", listener.backSurDis);
    OSReport("//////////////////////////////////\n");
}

void HuAudFXListnerUpdate(Vec *pos, Vec *heading)
{
    if(omSysExitReq) {
      return;
    }
    msmSeUpdataListener(pos, heading);
}

int HuAudFXEmiterPlay(int seId, Vec *pos)
{
    MSM_SEPARAM seParam;
    if(HuSeExitCheck()) {
      return 0;
    }
    seParam.flag = MSM_SEPARAM_POS;
    seParam.pos.x = pos->x;
    seParam.pos.y = pos->y;
    seParam.pos.z = pos->z;
    return HuSePlay(seId, &seParam);
}

void HuAudFXEmiterUpDate(int seNo, Vec *pos)
{
    MSM_SEPARAM param;
    if(omSysExitReq) {
        return;
    }
    param.flag = MSM_SEPARAM_POS;
    param.pos.x = pos->x;
    param.pos.y = pos->y;
    param.pos.z = pos->z;
    msmSeSetParam(seNo, &param);
}

void HuAudFXListnerKill(void)
{
    msmSeDelListener();
}

void HuAudFXPauseAll(BOOL pauseF)
{
    msmSePauseAll(pauseF, 100);
}

s32 HuAudFXStatusGet(int seNo)
{
    return msmSeGetStatus(seNo);
}

s32 HuAudFXPitchSet(int seNo, s16 pitch)
{
    MSM_SEPARAM param;

    if(omSysExitReq) {
        return 0;
    }
    param.flag = MSM_SEPARAM_PITCH;
    param.pitch = pitch;
    return msmSeSetParam(seNo, &param);
}

s32 HuAudFXVolSet(int seNo, s16 vol)
{
    MSM_SEPARAM param;

    if(omSysExitReq) {
        return 0;
    }
    param.flag = MSM_SEPARAM_VOL;
    param.vol = vol;
    return msmSeSetParam(seNo, &param);
}

int HuAudSeqPlay(s16 musId)
{
    int musNo;
    if(musicOffF || omSysExitReq) {
        return 0;
    }
    musNo = msmMusPlay(musId, NULL);
    return musNo;
}

void HuAudSeqStop(int musNo)
{
    if(musicOffF || omSysExitReq) {
        return;
    }
    msmMusStop(musNo, 0);
}

void HuAudSeqFadeOut(int musNo, s32 speed)
{
    if(musicOffF) {
        return;
    }
    msmMusStop(musNo, speed);
}

void HuAudSeqAllFadeOut(s32 speed)
{
    s16 i;
    for(i=0; i<MSM_MUS_MAX; i++) {
        if(msmMusGetStatus(i) == MSM_MUS_PLAY) {
            msmMusStop(i, speed);
        }
    }
}

void HuAudSeqAllStop(void)
{
    msmMusStopAll(FALSE, 0);
}

void HuAudSeqPauseAll(BOOL pause)
{
    msmMusPauseAll(pause, 100);
}

s32 HuAudSeqMidiCtrlGet(int musNo, s8 channel, s8 ctrl)
{
    if(musicOffF || omSysExitReq) {
        return 0;
    }
    return msmMusGetMidiCtrl(musNo, channel, ctrl);
}

static void SStreamPlay(void);

typedef struct SStreamWork_s {
    int chanNo;
    int streamId;
} SSTREAM_WORK;

static u8 streamVol[HUAUD_STREAM_MAX];

int HuAudSStreamChanPlay(s16 streamId, s16 chanNo)
{
    if(musicOffF || omSysExitReq) {
        return MSM_STREAMNO_NONE;
    }
    if(msmStreamGetStatus(chanNo) != MSM_STREAM_DONE) {
        HUPROCESS *process;
        SSTREAM_WORK *work;
        msmStreamStop(chanNo, 0);
        process = HuPrcCreate(SStreamPlay, 1, 2304, 0);
        process->property = work = HuMemDirectMalloc(HEAP_HEAP, sizeof(SSTREAM_WORK));
        work->chanNo = chanNo;
        work->streamId = streamId;
        return chanNo;
    } else {
        MSM_STREAMPARAM param;
        int streamNo;
        param.flag = MSM_STREAMPARAM_CHAN;
        param.chan = chanNo;
        streamNo = msmStreamPlay(streamId, &param);
        streamVol[chanNo] = MSM_VOL_MAX;
        return streamNo;
    }
}

static void SStreamPlay(void)
{
    HUPROCESS *process = HuPrcCurrentGet();
    SSTREAM_WORK *work = process->property;
    OSTick tick = OSGetTick();
    MSM_STREAMPARAM param;
    while(msmStreamGetStatus(work->chanNo) != MSM_STREAM_DONE && OSTicksToMilliseconds(OSGetTick()-tick) < 1000) {
        if(OSTicksToMilliseconds(OSGetTick()-tick) > 800) {
             msmStreamStop(work->chanNo, 0);
        }
        HuPrcVSleep();
    }
    param.flag = MSM_STREAMPARAM_CHAN;
    param.chan = work->chanNo;
    msmStreamPlay(work->streamId, &param);
    streamVol[work->chanNo] = MSM_VOL_MAX;
    HuMemDirectFree(work);
    HuPrcEnd();
    while(1) {
        HuPrcVSleep();
    }
}

int HuAudSStreamPlay(s16 streamId)
{
    return HuAudSStreamChanPlay(streamId, 0);
}

int HuAudBGMPlay(s16 streamId)
{
    return HuAudSStreamChanPlay(streamId, 0);
}

int HuAudJinglePlay(s16 streamId)
{
    return HuAudSStreamChanPlay(streamId, 2);
}

void HuAudSStreamStop(int streamNo)
{
    if(musicOffF) {
        return;
    }
    msmStreamStop(streamNo, 0);
}

void HuAudSStreamFadeOut(int streamNo, s32 speed)
{
    if(musicOffF) {
        return;
    }
    msmStreamStop(streamNo, speed);
}

void HuAudSStreamAllFadeOut(s32 speed)
{
    msmStreamStopAll(speed);
}

void HuAudSStreamPause(s16 streamNo, BOOL pause)
{
    msmStreamPause(streamNo, (pause) ? TRUE : FALSE, 5);
}

void HuAudSStreamPauseFadeOut(s16 streamNo, BOOL pause, s32 speed)
{
    msmStreamPause(streamNo, (pause) ? TRUE : FALSE, speed);
}

void HuAudSStreamPauseAll(BOOL pause)
{
    msmStreamPauseAll((pause) ? TRUE : FALSE, 5);
}

void HuAudSStreamAllStop(void)
{
    msmStreamStopAll(0);
}

s32 HuAudSStreamStatGet(int streamNo)
{
    return msmStreamGetStatus(streamNo);
}

typedef struct sStreamFadeWork_s {
    u8 volStart;
    u8 volEnd;
    u32 speed;
    int streamNo;
} SSTREAMFADEWORK;

static void SStreamFade(void);

void HuAudSStreamParamSet(int streamNo, u8 vol, u32 speed)
{
    if(musicOffF) {
        return;
    }
    if(speed <= 16) {
        MSM_STREAMPARAM param;
        param.flag = MSM_STREAMPARAM_VOL;
        param.vol = vol;
        msmStreamSetParam(streamNo, &param);
        streamVol[streamNo] = vol;
    } else {
        HUPROCESS *process = HuPrcCreate(SStreamFade, 1, 2304, 0);
        SSTREAMFADEWORK *work = HuMemDirectMalloc(HEAP_HEAP, sizeof(SSTREAMFADEWORK));
        process->property = work;
        work->speed =  speed;
        work->streamNo = streamNo;
        work->volStart = streamVol[streamNo];
        work->volEnd = vol;
    }
}

static void SStreamFade(void)
{
    HUPROCESS *process = HuPrcCurrentGet();
    SSTREAMFADEWORK *work = process->property;
    MSM_STREAMPARAM param;
    float maxTime;
    s16 i;
    param.flag = MSM_STREAMPARAM_VOL;
    maxTime = work->speed/16.666668f;
    for(i=1; i<=maxTime; i++) {
        float vol;
        float time;
        time = i/maxTime;
        vol = (work->volEnd*time)+(work->volStart*(1.0-time));
        param.vol = vol;
        msmStreamSetParam(work->streamNo, &param);
        streamVol[work->streamNo] = vol;
        HuPrcVSleep();
    }
    param.vol = work->volEnd;
    msmStreamSetParam(work->streamNo, &param);
    streamVol[work->streamNo] = work->volEnd;
    HuMemDirectFree(work);
    HuPrcEnd();
    HuPrcSleep(10);
}

SNDGRPTBL sndGrpTable[] = {
    { DLL_m601dll, MSM_GRP_MG601, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m602dll, MSM_GRP_MG602, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m603dll, MSM_GRP_MG603, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m604dll, MSM_GRP_MG604, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m605dll, MSM_GRP_MG605, MSM_GRP_NONE, MSM_AUX_1, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m606dll, MSM_GRP_MG606, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m607dll, MSM_GRP_MG607, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m608dll, MSM_GRP_MG608, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m609dll, MSM_GRP_MG609, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m610dll, MSM_GRP_MG610, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m611dll, MSM_GRP_MG611, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m612dll, MSM_GRP_MG612, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m613dll, MSM_GRP_MG613, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m614dll, MSM_GRP_MG614, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m615dll, MSM_GRP_MG615, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m616dll, MSM_GRP_MG616, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m617dll, MSM_GRP_MG617, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m618dll, MSM_GRP_MG618, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m619dll, MSM_GRP_MG619, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m620dll, MSM_GRP_MG620, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m621dll, MSM_GRP_MG621, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m622dll, MSM_GRP_MG622, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m623dll, MSM_GRP_MG623, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m624dll, MSM_GRP_MG624, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m625dll, MSM_GRP_MG625, MSM_GRP_NONE, MSM_AUX_5, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m626dll, MSM_GRP_MG626, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m627dll, MSM_GRP_MG627, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m628dll, MSM_GRP_MG628, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m629dll, MSM_GRP_MG629, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m630dll, MSM_GRP_MG630, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m631dll, MSM_GRP_MG631, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m632dll, MSM_GRP_MG632, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m633dll, MSM_GRP_MG633, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m634dll, MSM_GRP_MG634, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m635dll, MSM_GRP_MG635, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m636dll, MSM_GRP_MG636, MSM_GRP_NONE, MSM_AUX_6, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m637dll, MSM_GRP_MG637, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m638dll, MSM_GRP_MG638, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m639dll, MSM_GRP_MG639, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m640dll, MSM_GRP_MG640, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m641dll, MSM_GRP_MG641, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m642dll, MSM_GRP_MG642, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m643dll, MSM_GRP_MG643, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m644dll, MSM_GRP_MG644, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m645dll, MSM_GRP_MG645, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m646dll, MSM_GRP_MG646, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m647dll, MSM_GRP_MG647, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m648dll, MSM_GRP_MG648, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m649dll, MSM_GRP_MG649, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m650dll, MSM_GRP_MG650, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m651dll, MSM_GRP_MG651, MSM_GRP_NONE, MSM_AUX_4, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m652dll, MSM_GRP_MG652, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m653dll, MSM_GRP_MG653, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m654dll, MSM_GRP_MG654, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m655dll, MSM_GRP_MG655, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m656dll, MSM_GRP_MG656, MSM_GRP_NONE, MSM_AUX_4, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m657dll, MSM_GRP_MG657, MSM_GRP_NONE, MSM_AUX_4, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m658dll, MSM_GRP_MG658, MSM_GRP_NONE, MSM_AUX_4, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m659dll, MSM_GRP_MG659, MSM_GRP_NONE, MSM_AUX_4, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m660dll, MSM_GRP_MG660, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m661dll, MSM_GRP_MG661, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m662dll, MSM_GRP_MG662, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m663dll, MSM_GRP_MG663, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m664dll, MSM_GRP_MG664, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m665dll, MSM_GRP_MG665, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m666dll, MSM_GRP_MG666, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m667dll, MSM_GRP_MG667, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m668dll, MSM_GRP_MG668, MSM_GRP_NONE, MSM_AUX_1, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m669dll, MSM_GRP_MG669, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m670dll, MSM_GRP_MG670, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m671dll, MSM_GRP_MG671, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m672dll, MSM_GRP_MG672, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m673dll, MSM_GRP_MG673, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m674dll, MSM_GRP_MG674, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m675dll, MSM_GRP_MG675, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m676dll, MSM_GRP_MG676, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m677dll, MSM_GRP_MG677, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m678dll, MSM_GRP_MG678, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m679dll, MSM_GRP_MG679, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m680dll, MSM_GRP_MG680, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m681dll, MSM_GRP_MG681, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_m699dll, MSM_GRP_MG699, MSM_GRP_NONE, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_bootdll, MSM_GRP_NONE, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_openingdll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_fileseldll, MSM_GRP_FILESEL, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdseldll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdminidll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdmicdll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdpartydll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdsingdll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdbankdll, MSM_GRP_BANK, MSM_GRP_FILESEL, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_miraclebookdll, MSM_GRP_BANK, MSM_GRP_FILESEL, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdpresultdll, MSM_GRP_MENU, MSM_GRP_PRESULT, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_optiondll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_endingdll, MSM_GRP_BANK, MSM_GRP_ENDING, MSM_AUX_2, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_staffdll, MSM_GRP_NONE, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmfreedll, MSM_GRP_MGM00, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmdecathlondll, MSM_GRP_MGM00, MSM_GRP_MGM02, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmbattledll, MSM_GRP_MGM00, MSM_GRP_MGM01, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmtournamentdll, MSM_GRP_MGM00, MSM_GRP_MGM03, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmbingodll, MSM_GRP_MGM00, MSM_GRP_MGM05, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mgmrenshodll, MSM_GRP_MGM00, MSM_GRP_MGM04, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_micquizdll, MSM_GRP_MICQUIZ, MSM_GRP_CHARMIC, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_micquizishidll, MSM_GRP_MICQUIZ, MSM_GRP_CHARMIC, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_micquizmyokodll, MSM_GRP_MICQUIZ, MSM_GRP_CHARMIC, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_micquizohdedll, MSM_GRP_MICQUIZ, MSM_GRP_CHARMIC, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mikeactdll, MSM_GRP_MIKEACT, MSM_GRP_CHARMIC, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_instdll, MSM_GRP_NONE, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_resultdll, MSM_GRP_NONE, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w01dll, MSM_GRP_BRD01, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w02dll, MSM_GRP_BRD02, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w03dll, MSM_GRP_BRD03, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w04dll, MSM_GRP_BRD04, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w05dll, MSM_GRP_BRD05, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w06dll, MSM_GRP_BRD06, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w10dll, MSM_GRP_BRDTT, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_mdpresultdll, MSM_GRP_MENU, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_s01dll, MSM_GRP_SBRD, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_s02dll, MSM_GRP_SBRD, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_s03dll, MSM_GRP_SBRD, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_w11dll, MSM_GRP_BRDTT, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, -1, -1 },
    { DLL_NONE, MSM_GRP_NONE, MSM_GRP_NONE, MSM_AUX_DEFAULT, MSM_AUX_DEFAULT, 0, 0 }
};

void HuAudDllSndGrpSet(u16 ovl)
{
    SNDGRPTBL *sndGrpP;
    s16 grpSet;
    if(ovl == DLL_bootdll) {
        return;
    }
    sndGrpP=&sndGrpTable[0];
    while(1) {
        if(sndGrpP->ovl == ovl) {
            grpSet = sndGrpP->grpSet;
            break;
        }
        if(sndGrpP->ovl == DLL_NONE) {
            grpSet = MSM_GRP_MG601;
            break;
        }
        sndGrpP++;
    }
    OSReport("SOUND ##########################\n");
    if(grpSet != MSM_GRP_NONE) {
        HuAudSndGrpSetSet(grpSet);
        if(sndGrpP->loadGrp != MSM_GRP_NONE) {
            HuAudSndGrpSet(sndGrpP->loadGrp);
        }
    }
    if(sndGrpP->auxANo != auxANoBak || sndGrpP->auxBNo != auxBNoBak) {
        msmSysSetAux(sndGrpP->auxANo, sndGrpP->auxBNo);
        OSReport("Change AUX %d,%d\n", sndGrpP->auxANo, sndGrpP->auxBNo);
        auxANoBak = sndGrpP->auxANo;
        auxBNoBak = sndGrpP->auxBNo;
        HuPrcVSleep();
    }
    HuAudAUXVolSet(sndGrpP->auxAVol, sndGrpP->auxBVol);
    OSReport("##########################\n");
}

#define SNDGRP_TIMEOUT 500

#define SNDGRP_WAIT(tickStart) \
    while((msmMusGetNumPlay(TRUE) != 0 || msmSeGetNumPlay(TRUE) != 0) && OSTicksToMilliseconds(OSGetTick()-(tickStart)) < SNDGRP_TIMEOUT)

void HuAudSndGrpSetSet(s16 grpSet)
{
    u32 size;
    void *buf;
    OSTick tickStart;
    s32 result;
    

    if(sndGroupBak != grpSet) {
        msmMusStopAll(TRUE, 0);
        msmSeStopAll(TRUE, 0);
        tickStart = OSGetTick();
        SNDGRP_WAIT(tickStart);
        if(OSTicksToMilliseconds(OSGetTick()-tickStart) >= SNDGRP_TIMEOUT) {
            OSReport("Timed Out! Mus %d:SE %d\n", msmMusGetNumPlay(TRUE), msmSeGetNumPlay(TRUE));
        }
        OSReport("GroupSet %d\n", grpSet);
        sndGroupBak = grpSet;
        result = msmSysDelGroupAll();
        size = msmSysGetSampSize(grpSet);
        if(HuMemMaxMemorySizeGet(HuMemHeapPtrGet(HEAP_MODEL)) > size) {
            buf = HuMemDirectMalloc(HEAP_MODEL, size);
        } else if(HuMemMaxMemorySizeGet(HuMemHeapPtrGet(HEAP_HEAP)) > size) {
            buf = HuMemDirectMalloc(HEAP_HEAP, size);
        } else if(HuMemMaxMemorySizeGet(HuMemHeapPtrGet(HEAP_DVD)) > size) {
            buf = HuMemDirectMalloc(HEAP_DVD, size);
        } else {
            OSReport("Error: Sound GroupSet Error!!\n");
            return;
        }
        
        result = msmSysLoadGroup(grpSet, buf, FALSE);
        if(result) {
            OSReport("***********GroupSet Error %d\n", result);
        }
        HuMemDirectFree(buf);
    }
}

void HuAudSndGrpSet(s16 grp)
{
    void *buf = HuMemDirectMalloc(HEAP_MODEL, msmSysGetSampSize(grp));
    msmSysLoadGroup(grp, buf, FALSE);
    HuMemDirectFree(buf);
}

void HuAudSndCommonGrpSet(s16 grp, BOOL delGrpF) 
{
    OSTick tickStart;
    s16 result;
    void *buf;
    s16 i;
    
    for(i=0; i<GW_CHARA_MAX; i++) {
        CharSeLoadF[i] = 0;
    }
    msmMusStopAll(TRUE, 0);
    msmSeStopAll(TRUE, 0);
    tickStart = OSGetTick();
    SNDGRP_WAIT(tickStart);
    OSReport("CommonGrpSet %d\n", grp);
    if(delGrpF) {
        result = msmSysDelGroupBase(0);
        if(result < 0) {
            OSReport("Del Group Error %d\n", result);
        }
    }
    buf = HuMemDirectMalloc(HEAP_MODEL, msmSysGetSampSize(grp));
    msmSysLoadGroupBase(grp, buf);
    HuMemDirectFree(buf);
    sndGroupBak = MSM_GRP_NONE;
}

void HuAudAUXSet(s32 auxA, s32 auxB)
{
    if(auxA == MSM_AUX_NONE) {
        auxA = MSM_AUX_DEFAULT;
    }
    if(auxB == MSM_AUX_NONE) {
        auxB = MSM_AUX_DEFAULT;
    }
    auxANoBak = auxA;
    auxBNoBak = auxB;
    msmSysSetAux(auxA, auxB);
}

void HuAudAUXVolSet(s8 volA, s8 volB)
{
    HuAuxAVol = volA;
    HuAuxBVol = volB;
}


s32 PlayerFXPlay(s16 playerNo, s16 seId)
{
    s16 charNo = GwPlayerConf[playerNo].charNo;
    return CharFXPlay(charNo, seId);
}

s32 PlayerFXPlayPos(s16 playerNo, s16 seId, Vec *pos)
{
    s16 charNo = GwPlayerConf[playerNo].charNo;
    return CharFXPlayPos(charNo, seId, pos);
}

void PlayerFXStop(s16 playerNo, s16 seId)
{
    s16 charNo = GwPlayerConf[playerNo].charNo;
    CharFXStop(charNo, seId);
}

unsigned int CharSeTable[GW_CHARA_MAX] = {
    MSM_SE_CHAR_MARIO, //GW_CHARA_MARIO
    MSM_SE_CHAR_LUIGI, //GW_CHARA_LUIGI
    MSM_SE_CHAR_PEACH, //GW_CHARA_PEACH
    MSM_SE_CHAR_YOSHI, //GW_CHARA_YOSHI
    MSM_SE_CHAR_WARIO, //GW_CHARA_WARIO
    MSM_SE_CHAR_DAISY, //GW_CHARA_DAISY
    MSM_SE_CHAR_WALUIGI, //GW_CHARA_WALUIGI
    MSM_SE_CHAR_KINOPIO, //GW_CHARA_KINOPIO
    MSM_SE_CHAR_TERESA, //GW_CHARA_TERESA
    MSM_SE_CHAR_MINIKOOPAR, //GW_CHARA_MINIKOOPA
    MSM_SE_CHAR_KINOPIKO, //GW_CHARA_KINOPIKO
    MSM_SE_CHAR_MINIKOOPAR, //GW_CHARA_MINIKOOPAR
    MSM_SE_CHAR_MINIKOOPAG, //GW_CHARA_MINIKOOPAG
    MSM_SE_CHAR_MINIKOOPAB, //GW_CHARA_MINIKOOPAB
};

unsigned int CharVoiceSeTable[GW_CHARA_MAX] = {
    MSM_SE_CHARVOICE_MARIO, //GW_CHARA_MARIO
    MSM_SE_CHARVOICE_LUIGI, //GW_CHARA_LUIGI
    MSM_SE_CHARVOICE_PEACH, //GW_CHARA_PEACH
    MSM_SE_CHARVOICE_YOSHI, //GW_CHARA_YOSHI
    MSM_SE_CHARVOICE_WARIO, //GW_CHARA_WARIO
    MSM_SE_CHARVOICE_DAISY, //GW_CHARA_DAISY
    MSM_SE_CHARVOICE_WALUIGI, //GW_CHARA_WALUIGI
    MSM_SE_CHARVOICE_KINOPIO, //GW_CHARA_KINOPIO
    MSM_SE_CHARVOICE_TERESA, //GW_CHARA_TERESA
    MSM_SE_CHARVOICE_MINIKOOPAR, //GW_CHARA_MINIKOOPA
    MSM_SE_CHARVOICE_KINOPIKO, //GW_CHARA_KINOPIKO
    MSM_SE_CHARVOICE_MINIKOOPAR, //GW_CHARA_MINIKOOPAR
    MSM_SE_CHARVOICE_MINIKOOPAG, //GW_CHARA_MINIKOOPAG
    MSM_SE_CHARVOICE_MINIKOOPAB, //GW_CHARA_MINIKOOPAB
};

unsigned int CharMicSeTable[GW_CHARA_MAX] = {
    MSM_SE_CHARMIC_MARIO, //GW_CHARA_MARIO
    MSM_SE_CHARMIC_LUIGI, //GW_CHARA_LUIGI
    MSM_SE_CHARMIC_PEACH, //GW_CHARA_PEACH
    MSM_SE_CHARMIC_YOSHI, //GW_CHARA_YOSHI
    MSM_SE_CHARMIC_WARIO, //GW_CHARA_WARIO
    MSM_SE_CHARMIC_DAISY, //GW_CHARA_DAISY
    MSM_SE_CHARMIC_WALUIGI, //GW_CHARA_WALUIGI
    MSM_SE_CHARMIC_KINOPIO, //GW_CHARA_KINOPIO
    MSM_SE_CHARMIC_TERESA, //GW_CHARA_TERESA
    MSM_SE_CHARMIC_MINIKOOPAR, //GW_CHARA_MINIKOOPA
    MSM_SE_CHARMIC_KINOPIKO, //GW_CHARA_KINOPIKO
    MSM_SE_CHARMIC_MINIKOOPAR, //GW_CHARA_MINIKOOPAR
    MSM_SE_CHARMIC_MINIKOOPAG, //GW_CHARA_MINIKOOPAG
    MSM_SE_CHARMIC_MINIKOOPAB, //GW_CHARA_MINIKOOPAB
};

static unsigned int *CharSoundList[] = { CharSeTable, CharVoiceSeTable, CharMicSeTable, NULL };

s32 CharFXPlayVolPan(s16 charNo, s16 seId, s16 vol, s16 pan)
{
    MSM_SEPARAM param;
    s16 i;
    unsigned int *seTbl;
    if(HuSeExitCheck()) {
        return 0;
    }
    for(i=0; CharSoundList[i]; i++) {
        seTbl = CharSoundList[i];
        if(seId < seTbl[GW_CHARA_MARIO]) {
            break;
        }
    }
    if(i == 0) {
        return -1;
    }
    seTbl = CharSoundList[i-1];
    seId -= seTbl[GW_CHARA_MARIO];
    seId += seTbl[charNo];
    param.flag = MSM_SEPARAM_NONE;
    if(HuAuxAVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLA;
    }
    if(HuAuxBVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLB;
    }
    param.auxAVol = HuAuxAVol;
    param.auxBVol = HuAuxBVol;
    param.flag |= MSM_SEPARAM_VOL|MSM_SEPARAM_PAN;
    param.vol = vol;
    param.pan = pan;
    return HuSePlay(seId, &param);
}

s32 CharFXPlay(s16 charNo, s16 seId)
{
    return CharFXPlayVolPan(charNo, seId, MSM_VOL_MAX, MSM_PAN_CENTER);
}

s32 CharFXPlayVol(s16 charNo, s16 seId, s16 vol)
{
    return CharFXPlayVolPan(charNo, seId, vol, MSM_PAN_CENTER);
}

s32 CharFXPlayPan(s16 charNo, s16 seId, s16 pan)
{
    return CharFXPlayVolPan(charNo, seId, MSM_VOL_MAX, pan);
}

void CharFXPlayDelayVolPan(s16 charNo, s16 seId, u8 vol, u8 pan, u16 delay)
{
    MSM_SEPARAM param;
    if(HuSeExitCheck()) {
        return;
    }
    if(seId < MSM_SE_CHARVOICE_MARIO) {
        seId -= MSM_SE_CHAR_MARIO;
        seId += CharSeTable[charNo];
    } else {
        seId -= MSM_SE_CHARVOICE_MARIO;
        seId += CharVoiceSeTable[charNo];
    }
    param.flag = MSM_SEPARAM_NONE;
    if(HuAuxAVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLA;
    }
    if(HuAuxBVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLB;
    }
    param.auxAVol = HuAuxAVol;
    param.auxBVol = HuAuxBVol;
    param.flag |= MSM_SEPARAM_VOL|MSM_SEPARAM_PAN;
    param.vol = vol;
    param.pan = pan;
    if(delay == 0) {
        HuSePlay(seId, &param);
        return;
    } else {
        HUPROCESS *process = HuPrcCreate(AudFXPlayDelay, 1, 2304, 0);
        HUAUD_FX_DELAY *work = HuMemDirectMalloc(HEAP_HEAP, sizeof(HUAUD_FX_DELAY));
        process->property = work;
        work->param = param;
        work->seId = seId;
        work->delay = delay;
    }
}

void CharFXPlayDelay(s16 charNo, s16 seId, u16 delay)
{
    CharFXPlayDelayVolPan(charNo, seId, MSM_VOL_MAX, MSM_PAN_CENTER, delay);
}

void CharFXPlayDelayVol(s16 charNo, s16 seId, u8 vol, u16 delay)
{
    CharFXPlayDelayVolPan(charNo, seId, vol, MSM_PAN_CENTER, delay);
}

void CharFXPlayDelayPan(s16 charNo, s16 seId, u8 pan, u16 delay)
{
    CharFXPlayDelayVolPan(charNo, seId, MSM_VOL_MAX, pan, delay);
}

s32 CharFXPlayPos(s16 charNo, s16 seId, Vec *pos)
{
    MSM_SEPARAM param;
    if(omSysExitReq) {
        return 0;
    }
    seId -= MSM_SE_CHAR_MARIO;
    seId += CharSeTable[charNo];
    param.flag = MSM_SEPARAM_POS;
    if(HuAuxAVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLA;
    }
    if(HuAuxBVol != -1) {
        param.flag |= MSM_SEPARAM_AUXVOLB;
    }
    param.auxAVol = HuAuxAVol;
    param.auxBVol = HuAuxBVol;
    param.pos.x = pos->x;
    param.pos.y = pos->y;
    param.pos.z = pos->z;
    return HuSePlay(seId, &param);
}

void CharFXStop(s16 charNo, s16 seId)
{
    int entrySeNo[MSM_ENTRY_SENO_MAX];
    u16 entryNum;
    u16 i;
    seId -= MSM_SE_CHAR_MARIO;
    seId += CharSeTable[charNo];
    entryNum = msmSeGetEntryID(seId, entrySeNo);
    for(i=0; i<entryNum; i++) {
        msmSeStop(entrySeNo[i], 0);
    }
}

static int HuSePlay(int seId, MSM_SEPARAM *param)
{
    int result = msmSePlay(seId, param);
    if(result < 0) {
        OSReport("#########SE Entry Error<SE %d:ErrorNo %d>\n", seId, result);
    }
    return result;
}

static BOOL HuSeExitCheck(void)
{
    WIPEWORK *wipe = &wipeData;
    if(omSysExitReq || (wipeData.mode == WIPE_MODE_OUT && (wipe->time/wipe->maxTime > 0.5))) {
        return TRUE;
    } else {
        return FALSE;
    }
}
