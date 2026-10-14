#ifndef _DOLPHIN_M2S
#define _DOLPHIN_M2S

#include <dolphin/types.h>

#ifdef __cplusplus
extern "C" {
#endif

#define M2S_SAMPLE_NUM 128
#define M2S_MODE_MANUAL 0
#define M2S_MODE_AUTOSTART (1 << 0)
#define M2S_MODE_AUTOSTOP (1 << 1)
#define M2S_MODE_AUTO (M2S_MODE_AUTOSTART|M2S_MODE_AUTOSTOP)

void M2SInit(void);
void M2SClose(void);
void M2SOpen(void);
void M2SStop(void);
BOOL M2SSetActiveChannel(s32 chan);

BOOL M2SSetBuffer(s16 *buf);
BOOL M2SSetPrerecordSamples(int samples);
BOOL M2SSetMode(u32 mode);
BOOL M2SSetShifts(s32 shift);


#ifdef __cplusplus
}
#endif

#endif // _DOLPHIN_M2S
