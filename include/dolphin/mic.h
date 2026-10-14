#ifndef _DOLPHIN_MIC
#define _DOLPHIN_MIC

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef void (*MICExtHandler)(s32 chan, s32 result);

#define MIC_RESULT_READY 0
#define MIC_RESULT_BUSY -1
#define MIC_RESULT_WRONGDEVICE -2
#define MIC_RESULT_NOCARD -3
#define MIC_RESULT_ACTIVE -4
#define MIC_RESULT_FATAL_ERROR -128

void MICInit(void);
s32 MICStart(s32 chan);
s32 MICStop(s32 chan);
s32 MICSetGain(s32 chan, s32 gain);

s32 MICMount(s32 chan, s16 *buffer, s32 bufferSize, MICExtHandler extHandler);
s32 MICGetDeviceID(s32 chan, s32 *deviceId);
s32 MICGetButton(s32 chan, u32 *button);
s32 MICProbeEx(s32 chan);
s32 MICUnmount(s32 chan);

BOOL MICIsActive(s32 chan);
s32 MICGetSamples(s32 chan, s16 *buffer, s32 ofs, s32 nSamples);
s32 MICUpdateIndex(s32 chan, s32 ofs, s32 nSamples);

#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_MIC
