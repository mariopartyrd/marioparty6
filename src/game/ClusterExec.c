#include "game/ClusterExec.h"
#include "game/EnvelopeExec.h"
#include "game/hu3d.h"
#include "game/sprite.h"

float GetClusterCurve(HSF_TRACK *track, float time) {
    float *value;

    switch (track->curveType) {
        case HSF_CURVE_LINEAR:
            return GetLinear(track->numKeyframes, track->data, time);
        case HSF_CURVE_BEZIER:
            return GetBezier(track->numKeyframes, track, time);
        case HSF_CURVE_CONST:
            value = &track->value;
            return *value;
    }
    return 0.0f;
}

float GetClusterWeightCurve(HSF_TRACK *track, float time) {
    float *value;

    switch (track->curveType) {
        case HSF_CURVE_LINEAR:
            return GetLinear(track->numKeyframes, track->data, time);
        case HSF_CURVE_BEZIER:
            return GetBezier(track->numKeyframes, track, time);
        case HSF_CURVE_CONST:
            value = &track->value;
            return *value;
    }
    return 0.0f;
}

void SetClusterMain(HSF_CLUSTER *cluster) {
    float weightTotal;
    float t;
    s32 idx;
    s32 vtxIdx;
    u16 *vtxIdxP;
    s32 idxNext;
    s32 i;
    s32 j;
    HSF_BUFFER *vtxBufNext;
    HSF_PART *part;
    HSF_BUFFER *vtxBuf;

    part = cluster->part;
    if (cluster->vertexNum != 0) {
        if (cluster->type == 2) {
            vtxIdxP = part->vertex;
            vtxBuf = *cluster->vertex;
            weightTotal = 0.0f;
            for (i = 0; i < cluster->vertexNum; i++) {
                weightTotal += cluster->weight[i];
            }
            for (i = 0; i < part->num; i++, vtxIdxP++) {
                vtxIdx = *vtxIdxP;
                Vertextop[vtxIdx].x = ((Vec*) vtxBuf->data)[i].x;
                Vertextop[vtxIdx].y = ((Vec*) vtxBuf->data)[i].y;
                Vertextop[vtxIdx].z = ((Vec*) vtxBuf->data)[i].z;
            }
            for (i = 1; i < cluster->vertexNum; i++) {
                vtxBuf = cluster->vertex[i];
                vtxIdxP = part->vertex;
                t = cluster->weight[i];
                if (t < 0.0f) {
                    t = 0.0f;
                } else if (weightTotal > 1.0f) {
                    t /= weightTotal;
                }
                for (j = 0; j < part->num; j++, vtxIdxP++) {
                    vtxIdx = *vtxIdxP;
                    Vertextop[vtxIdx].x += t * (((Vec*) vtxBuf->data)[j].x - Vertextop[vtxIdx].x);
                    Vertextop[vtxIdx].y += t * (((Vec*) vtxBuf->data)[j].y - Vertextop[vtxIdx].y);
                    Vertextop[vtxIdx].z += t * (((Vec*) vtxBuf->data)[j].z - Vertextop[vtxIdx].z);
                }
            }
            return;
        }
        idx = cluster->index;
        idxNext = idx + 1;
        if (idxNext >= cluster->vertexNum) {
            idxNext = idx;
        }
        t = cluster->index - idx;
        vtxBuf = cluster->vertex[idx];
        vtxBufNext = cluster->vertex[idxNext];
        vtxIdxP = part->vertex;
        for (i = 0; i < part->num; i++, vtxIdxP++) {
            vtxIdx = *vtxIdxP;
            Vertextop[vtxIdx].x = ((Vec*) vtxBuf->data)[i].x + t * (((Vec*) vtxBufNext->data)[i].x - ((Vec*) vtxBuf->data)[i].x);
            Vertextop[vtxIdx].y = ((Vec*) vtxBuf->data)[i].y + t * (((Vec*) vtxBufNext->data)[i].y - ((Vec*) vtxBuf->data)[i].y);
            Vertextop[vtxIdx].z = ((Vec*) vtxBuf->data)[i].z + t * (((Vec*) vtxBufNext->data)[i].z - ((Vec*) vtxBuf->data)[i].z);
        }
    }
}

void ClusterProc(HU3D_MODEL *modelP) {
    int motId;
    s32 i;
    s32 j;
    s32 k;
    HSF_DATA *hsfMotion;
    HSF_DATA *hsf;
    HU3D_MOTION *motionP;
    HSF_CLUSTER *cluster;
    HSF_OBJECT *obj;

    for (i = 0; i < HU3D_CLUSTER_MAX; i++) {
        motId = modelP->motIdCluster[i];
        if (motId != HU3D_MOTIONID_NONE) {
            motionP = &Hu3DMotion[motId];
            hsfMotion = motionP->hsf;
            hsf = modelP->hsf;
            cluster = hsfMotion->cluster;
            for (j = 0; j < hsfMotion->clusterNum; j++, cluster++) {
                if (cluster->target != -1) {
                    obj = hsf->object;
                    obj += cluster->target;
                    Vertextop = obj->mesh.vertex->data;
                    if (obj->mesh.cenvNum) {
                        for (k = 0; k < obj->mesh.vertex->count; k++) {
                            Vertextop[k].x = ((Vec*) obj->mesh.vtxtop)[k].x;
                            Vertextop[k].y = ((Vec*) obj->mesh.vtxtop)[k].y;
                            Vertextop[k].z = ((Vec*) obj->mesh.vtxtop)[k].z;
                        }
                    }
                    SetClusterMain(cluster);
                    DCStoreRangeNoSync(Vertextop, obj->mesh.vertex->count * sizeof(Vec));
                    obj->mesh.writeNum++;
                }
            }
        }
    }
}

void ClusterMotionExec(HU3D_MODEL *modelP) {
    float t;
    s32 i;
    s32 j;
    HU3D_MOTIONID motId;
    HSF_CLUSTER *cluster;
    HSF_DATA *hsf;
    HSF_MOTION *hsfMotion;
    HSF_TRACK *track;
    HSF_TRACK *weightTrack;
    HU3D_MOTION *motionP;
    hsf = modelP->hsf;
    hsfMotion = hsf->motion;
    track = hsfMotion->track;
    for (i = 0; i < HU3D_CLUSTER_MAX; i++) {
        if (modelP->motIdCluster[i] != HU3D_MOTIONID_NONE) {
            motId = modelP->motIdCluster[i];
            motionP = &Hu3DMotion[motId];
            hsf = motionP->hsf;
            hsfMotion = hsf->motion;
            track = hsfMotion->track;
            t = modelP->clusterTime[i];
            for (j = 0; j < hsfMotion->numTracks; j++, track++) {
                switch (track->type) {
                    case HSF_TRACK_CLUSTER:
                        cluster = &hsf->cluster[track->cluster];
                        cluster->index = GetClusterCurve(track, t);
                        break;
                    case HSF_TRACK_CLUSTER_WEIGHT:
                        weightTrack = track;
                        cluster = &hsf->cluster[weightTrack->cluster];
                        cluster->weight[weightTrack->clusterWeight] = GetClusterCurve(weightTrack, t);
                        break;
                }
            }
        }
    }
}
