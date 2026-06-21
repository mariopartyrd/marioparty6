#ifndef _GAME_CLUSTER_EXEC_H
#define _GAME_CLUSTER_EXEC_H

#include "game/hu3d.h"

float GetClusterCurve(HSF_TRACK *track, float time);
float GetClusterWeightCurve(HSF_TRACK *track, float time);
void SetClusterMain(HSF_CLUSTER *cluster);
void ClusterProc(HU3D_MODEL *modelP);
void ClusterMotionExec(HU3D_MODEL *modelP);

#endif
