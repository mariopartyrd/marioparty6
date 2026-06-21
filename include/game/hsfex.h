#ifndef _GAME_HSFEX_H
#define _GAME_HSFEX_H

#include "dolphin.h"
#include "humath.h"
#include "game/hu3d.h"

typedef struct CamMotionWork_s {
    float time;
    float dist;
    float fov;
    HuVecF pos;
    HuVecF up;
    HuVecF target;
} CAM_MOTION_WORK;

void CamMotionEx2(HU3D_MODELID modelId, s16 cameraBit, float maxTime, s16 mode);
void CamMotionEx(HU3D_MODELID modelId, s16 cameraBit, HuVecF *pos, HuVecF *up, HuVecF *target, float maxTime, s16 mode);
int CamMotionExPathGet(HU3D_MODELID modelId, s16 stepSpeed, float maxTime, s16 mode, CAM_MOTION_WORK **workBuf, int *stepMax);
void CamMotionExPath(s16 cameraBit, CAM_MOTION_WORK *workBuf, int num);
float InterpolateBMLine(float *p1, float *p2, float time);
void Hu3D2Dto3D(HuVecF *src, s16 cameraBit, HuVecF *dst);
void Hu3D3Dto2D(HuVecF *src, s16 cameraBit, HuVecF *dst);
void Hu3DMtxTransGet(Mtx src, HuVecF *dst);
void Hu3DMtxRotGet(Mtx src, HuVecF *dst);
void Hu3DMtxScaleGet(Mtx src, HuVecF *dst);

#endif
