#include "game/EnvelopeExec.h"
#include "game/hsfex.h"

#include "string.h"

static void SetEnvelopMtx(HSF_OBJECT *baseObj, HSF_OBJECT *obj, Mtx *base);
static void SetEnvelopMain(HSF_DATA *hsf);
static void SetEnvelop(HSF_CENV *cenv);
static void SetMtx(HSF_OBJECT *obj, Mtx base);
static void SetRevMtx(void);
static HSF_SKELETON *SearchSklenton(char *name);

Vec *Vertextop;
Mtx *MtxTop;
static u32 nObj;
static u32 nMesh;
static HSF_OBJECT *objtop;
static HSF_DATA *CurHsf;
static Vec *vtxenv;
static Vec *normenv;
static Vec *normtop;
static s32 Meshcnt;
static s32 Meshno;

void InitEnvelope(HSF_DATA *hsf) {
    HSF_BUFFER *vertex;
    HSF_BUFFER *normal;
    HSF_MATRIX *matrix;
    HSF_OBJECT *obj;
    HSF_SKELETON *skeleton;
    Mtx identity;
    s32 i;
    s32 j;

    if (hsf->cenvNum != 0) {
        obj = hsf->object;
        for (Meshcnt = i = 0; i < hsf->objectNum; i++, obj++) {
            if (obj->type == 2) {
                if (obj->mesh.vtxtop) {
                    vertex = obj->mesh.vertex;
                    normal = obj->mesh.normal;
                    Meshcnt++;
                } else {
                    continue;
                }
            }
            skeleton = hsf->skeleton;
            for (j = 0; j < hsf->skeletonNum; j++, skeleton++) {
                if (strcmp(obj->name, skeleton->name) == 0) {
                    obj->mesh.base = skeleton->transform;
                }
            }
            obj->mesh.curr = obj->mesh.base;
        }
        CurHsf = hsf;
        objtop = hsf->object;
        matrix = CurHsf->matrix;
        if (matrix) {
            MtxTop = matrix->data;
            nObj = matrix->count;
            nMesh = matrix->base_idx;
        }
        PSMTXIdentity(identity);
        SetMtx(hsf->root, identity);
        SetRevMtx();
    }
}

static void SetEnvelopMtx(HSF_OBJECT *baseObj, HSF_OBJECT *obj, Mtx *base) {
    Mtx rot;
    Mtx final;
    Mtx trans;
    HSF_CONSTDATA *constData;
    s32 mtxIdx;
    s32 i;
    
    constData = obj->constData;
    if(!constData || !constData->hook) {
        PSMTXTrans(trans, obj->mesh.curr.pos.x, obj->mesh.curr.pos.y, obj->mesh.curr.pos.z);
        PSMTXConcat(*base, trans, final);
        if (obj->mesh.curr.rot.z) {
            PSMTXRotRad(rot, 'z', MTXDegToRad(obj->mesh.curr.rot.z));
            PSMTXConcat(final, rot, final);
        }
        if (obj->mesh.curr.rot.y) {
            PSMTXRotRad(rot, 'y', MTXDegToRad(obj->mesh.curr.rot.y));
            PSMTXConcat(final, rot, final);
        }
        if (obj->mesh.curr.rot.x) {
            PSMTXRotRad(rot, 'x', MTXDegToRad(obj->mesh.curr.rot.x));
            PSMTXConcat(final, rot, final);
        }
        if (obj->mesh.curr.scale.x != 1.0f) {
            final[0][0] *= obj->mesh.curr.scale.x;
            final[1][0] *= obj->mesh.curr.scale.x;
            final[2][0] *= obj->mesh.curr.scale.x;
        }
        if (obj->mesh.curr.scale.y != 1.0f) {
            final[0][1] *= obj->mesh.curr.scale.y;
            final[1][1] *= obj->mesh.curr.scale.y;
            final[2][1] *= obj->mesh.curr.scale.y;
        }
        if (obj->mesh.curr.scale.z != 1.0f) {
            final[0][2] *= obj->mesh.curr.scale.z;
            final[1][2] *= obj->mesh.curr.scale.z;
            final[2][2] *= obj->mesh.curr.scale.z;
        }
    } else {
        HU3D_OBJ_HOOK hook = constData->hook;
        hook(obj, &obj->mesh.curr, base, &final);
    }
    
    mtxIdx = obj - baseObj;
    PSMTXCopy(final, MtxTop[nMesh + mtxIdx]);
    for (i = 0; i < obj->mesh.childNum; i++) {
        SetEnvelopMtx(baseObj, obj->mesh.child[i], &final);
    }
}

void EnvelopeProc(HSF_DATA *hsf) {
    HSF_MATRIX *matrix;
    HSF_OBJECT *obj;
    Mtx rootMtx;

    CurHsf = hsf;
    matrix = CurHsf->matrix;
    MtxTop = matrix->data;
    nObj = matrix->count;
    nMesh = matrix->base_idx;
    obj = hsf->root;
    PSMTXIdentity(rootMtx);
    SetEnvelopMtx(hsf->object, obj, &rootMtx);
    SetEnvelopMain(hsf);
}

void InitVtxParm(HSF_DATA *hsf) {
    HSF_OBJECT *obj;
    s32 i;

    obj = hsf->object;
    for (i = 0; i < hsf->objectNum; i++, obj++) {
        if (obj->type == HSF_OBJ_MESH) {
            obj->mesh.writeNum = 0;
        }
    }
}

static void SetEnvelopMain(HSF_DATA *hsf) {
    void *vtxData;
    void *vtxTop;
    void *vtxData2;
    HSF_BUFFER *normalBuf;
    HSF_BUFFER *vtxBuf;
    HSF_OBJECT *obj;
    s32 i;
    s32 j;
    HSF_CENV *cenv;

    obj = hsf->object;
    for (Meshno = i = 0; i < hsf->objectNum; i++, obj++) {
        if (obj->type == HSF_OBJ_MESH) {
            PSMTXInverse(MtxTop[&obj[nMesh] - hsf->object], MtxTop[Meshno]);
            vtxBuf = obj->mesh.vertex;
            normalBuf = obj->mesh.normal;
            if (obj->mesh.writeNum != 0) {
                Vertextop = vtxBuf->data;
            } else {
                Vertextop = obj->mesh.vtxtop;
            }
            vtxenv = vtxBuf->data;
            normtop = obj->mesh.normtop;
            normenv = normalBuf->data;
            cenv = obj->mesh.cenv;
            for (j = 0; j < obj->mesh.cenvNum; j++, cenv++) {
                SetEnvelop(cenv);
            }
            vtxData = vtxBuf->data;
            vtxTop = obj->mesh.vtxtop;
            vtxData2 = vtxBuf->data;
            DCStoreRangeNoSync(normenv, normalBuf->count * sizeof(Vec));
            DCStoreRangeNoSync(vtxenv, vtxBuf->count * sizeof(Vec));
            Meshno++;
        }
    }
}

void MTXAdd(Mtx *src1, Mtx *src2, Mtx *dst)
{
    Mtx temp;
    Mtx *writePtr;
    if(dst == src1 || dst == src2) {
        writePtr = &temp;
    } else {
        writePtr = dst;
    }
    (*writePtr)[0][0] = (*src1)[0][0]+(*src2)[0][0];
    (*writePtr)[0][1] = (*src1)[0][1]+(*src2)[0][1];
    (*writePtr)[0][2] = (*src1)[0][2]+(*src2)[0][2];
    (*writePtr)[0][3] = (*src1)[0][3]+(*src2)[0][3];
    (*writePtr)[1][0] = (*src1)[1][0]+(*src2)[1][0];
    (*writePtr)[1][1] = (*src1)[1][1]+(*src2)[1][1];
    (*writePtr)[1][2] = (*src1)[1][2]+(*src2)[1][2];
    (*writePtr)[1][3] = (*src1)[1][3]+(*src2)[1][3];
    (*writePtr)[2][0] = (*src1)[2][0]+(*src2)[2][0];
    (*writePtr)[2][1] = (*src1)[2][1]+(*src2)[2][1];
    (*writePtr)[2][2] = (*src1)[2][2]+(*src2)[2][2];
    (*writePtr)[2][3] = (*src1)[2][3]+(*src2)[2][3];
    if(writePtr == &temp) {
        MTXCopy(temp, *dst);
    }
}

static void SetEnvelop(HSF_CENV *cenv) {
    Vec weightVtxOfs;
    Vec vtxOfs;
    Vec weightNormOfs;
    Vec normOfs;
    Vec scale;
    s32 temp;
    u32 target1;
    u32 target2;
    HSF_CENV_DUAL *dualP;
    HSF_CENV_DUAL_WEIGHT *dualWeightP;
    HSF_CENV_MULTI *multiP;
    HSF_CENV_MULTI_WEIGHT *multiWeightP;
    HSF_CENV_SINGLE *singleP;
    Vec *normEnvP;
    Vec *normTopP;
    Vec *vtxEnvP;
    Vec *vtxTopP;
    float weight;
    s32 normalIdx;
    s32 vertexIdx;
    s32 i;
    s32 j;
    Mtx objMtx;
    Mtx invMtx;
    Mtx tempMtx;
    Mtx tempMtx2;
    Mtx invMtx2;
    Mtx objMtx2;
    Mtx weightMtx;

    singleP = cenv->singleData;
    for (i = 0; i < cenv->singleCount; i++, singleP++) {
        normalIdx = singleP->normal;
        vertexIdx = singleP->pos;
        vtxEnvP = &vtxenv[vertexIdx];
        vtxTopP = &Vertextop[vertexIdx];
        normEnvP = &normenv[normalIdx];
        normTopP = &normtop[normalIdx];
        PSMTXConcat(MtxTop[nMesh + singleP->target], MtxTop[nMesh + nObj + nObj * Meshno + singleP->target], tempMtx);
        PSMTXConcat(MtxTop[Meshno], tempMtx, objMtx);
        Hu3DMtxScaleGet(objMtx, &scale);
        if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
            PSMTXScale(invMtx2, 1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
            PSMTXConcat(invMtx2, objMtx, invMtx);
            PSMTXInvXpose(invMtx, invMtx);
        } else {
            PSMTXInvXpose(objMtx, invMtx);
        }
        if (singleP->posNum == 1) {
            PSMTXMultVec(objMtx, vtxTopP, vtxEnvP);
            PSMTXMultVec(invMtx, normTopP, normEnvP);
        } else if (singleP->posNum <= 6) {
            PSMTXMultVecArray(objMtx, vtxTopP, vtxEnvP, singleP->posNum);
            PSMTXMultVecArray(invMtx, normTopP, normEnvP, singleP->normalNum);
        } else {
            PSMTXReorder(objMtx, (ROMtxPtr) tempMtx);
            PSMTXReorder(invMtx, (ROMtxPtr) tempMtx2);
            PSMTXROMultVecArray((ROMtxPtr) tempMtx, vtxTopP, vtxEnvP, singleP->posNum);
            PSMTXROMultVecArray((ROMtxPtr) tempMtx2, normTopP, normEnvP, singleP->normalNum);
        }
    }
    dualP = cenv->dualData;
    for (i = 0; i < cenv->dualCount; i++, dualP++) {
        target1 = dualP->target1;
        target2 = dualP->target2;
        PSMTXConcat(MtxTop[nMesh + target1], MtxTop[nMesh + nObj + nObj * Meshno + target1], tempMtx);
        PSMTXConcat(MtxTop[Meshno], tempMtx, objMtx);
        PSMTXConcat(MtxTop[nMesh + target2], MtxTop[nMesh + nObj + nObj * Meshno + target2], tempMtx);
        PSMTXConcat(MtxTop[Meshno], tempMtx, objMtx2);
        dualWeightP = dualP->weight;
        for (j = 0; j < dualP->weightNum; j++, dualWeightP++) {
            normalIdx = dualWeightP->normal;
            vertexIdx = dualWeightP->pos;
            vtxEnvP = &vtxenv[vertexIdx];
            vtxTopP = &Vertextop[vertexIdx];
            normEnvP = &normenv[normalIdx];
            normTopP = &normtop[normalIdx];
            weight = dualWeightP->weight;
            tempMtx[0][0] = objMtx[0][0] * weight;
            tempMtx[1][0] = objMtx[1][0] * weight;
            tempMtx[2][0] = objMtx[2][0] * weight;
            tempMtx[0][1] = objMtx[0][1] * weight;
            tempMtx[1][1] = objMtx[1][1] * weight;
            tempMtx[2][1] = objMtx[2][1] * weight;
            tempMtx[0][2] = objMtx[0][2] * weight;
            tempMtx[1][2] = objMtx[1][2] * weight;
            tempMtx[2][2] = objMtx[2][2] * weight;
            tempMtx[0][3] = objMtx[0][3] * weight;
            tempMtx[1][3] = objMtx[1][3] * weight;
            tempMtx[2][3] = objMtx[2][3] * weight;
            weight = 1.0f - dualWeightP->weight;
            tempMtx2[0][0] = objMtx2[0][0] * weight;
            tempMtx2[1][0] = objMtx2[1][0] * weight;
            tempMtx2[2][0] = objMtx2[2][0] * weight;
            tempMtx2[0][1] = objMtx2[0][1] * weight;
            tempMtx2[1][1] = objMtx2[1][1] * weight;
            tempMtx2[2][1] = objMtx2[2][1] * weight;
            tempMtx2[0][2] = objMtx2[0][2] * weight;
            tempMtx2[1][2] = objMtx2[1][2] * weight;
            tempMtx2[2][2] = objMtx2[2][2] * weight;
            tempMtx2[0][3] = objMtx2[0][3] * weight;
            tempMtx2[1][3] = objMtx2[1][3] * weight;
            tempMtx2[2][3] = objMtx2[2][3] * weight;
            MTXAdd(&tempMtx2, &tempMtx, &weightMtx);
            Hu3DMtxScaleGet(&weightMtx[0], &scale);
            if (scale.x != 1.0f || scale.y != 1.0f || scale.z != 1.0f) {
                PSMTXScale(invMtx2, 1.0 / scale.x, 1.0 / scale.y, 1.0 / scale.z);
                PSMTXConcat(invMtx2, weightMtx, tempMtx2);
                PSMTXInvXpose(tempMtx2, tempMtx2);
            } else {
                PSMTXInvXpose(weightMtx, tempMtx2);
            }
            if (dualWeightP->posNum == 1) {
                PSMTXMultVec(weightMtx, vtxTopP, vtxEnvP);
            } else if (dualWeightP->posNum <= 6) {
                PSMTXMultVecArray(weightMtx, vtxTopP, vtxEnvP, dualWeightP->posNum);
            } else {
                PSMTXReorder(weightMtx, (ROMtxPtr) tempMtx);
                PSMTXROMultVecArray((ROMtxPtr) tempMtx, vtxTopP, vtxEnvP, dualWeightP->posNum);
            }
            if (dualWeightP->normalNum != 0) {
                if (dualWeightP->normalNum == 1) {
                    PSMTXMultVec(tempMtx2, normTopP, normEnvP);
                } else if (dualWeightP->normalNum <= 6) {
                    PSMTXMultVecArray(tempMtx2, normTopP, normEnvP, dualWeightP->normalNum);
                } else {
                    PSMTXReorder(tempMtx2, (ROMtxPtr) tempMtx);
                    PSMTXROMultVecArray((ROMtxPtr) tempMtx, normTopP, normEnvP, dualWeightP->normalNum);
                }
            }
        }
    }
    multiP = cenv->multiData;
    for (i = 0; i < cenv->multiCount; i++, multiP++) {
        multiWeightP = multiP->weight;
        normalIdx = multiP->normal;
        vertexIdx = multiP->pos;
        vtxEnvP = &vtxenv[vertexIdx];
        vtxTopP = &Vertextop[vertexIdx];
        normEnvP = &normenv[normalIdx];
        normTopP = &normtop[normalIdx];
        vtxOfs.x = vtxOfs.y = vtxOfs.z = 0.0f;
        normOfs.x = normOfs.y = normOfs.z = 0.0f;
        temp = 0;
        for (j = 0; j < multiP->weightNum; j++, multiWeightP++) {
            PSMTXConcat(MtxTop[nMesh + multiWeightP->target], MtxTop[nMesh + nObj + nObj * Meshno + multiWeightP->target], objMtx);
            PSMTXConcat(MtxTop[Meshno], objMtx, objMtx);
            PSMTXInvXpose(objMtx, invMtx);
            PSMTXMultVec(objMtx, vtxTopP, &weightVtxOfs);
            PSMTXMultVec(invMtx, normTopP, &weightNormOfs);
            weightVtxOfs.x = multiWeightP->value * (weightVtxOfs.x - vtxTopP->x);
            weightVtxOfs.y = multiWeightP->value * (weightVtxOfs.y - vtxTopP->y);
            weightVtxOfs.z = multiWeightP->value * (weightVtxOfs.z - vtxTopP->z);
            VECAdd(&vtxOfs, &weightVtxOfs, &vtxOfs);
            weightNormOfs.x = multiWeightP->value * (weightNormOfs.x - normTopP->x);
            weightNormOfs.y = multiWeightP->value * (weightNormOfs.y - normTopP->y);
            weightNormOfs.z = multiWeightP->value * (weightNormOfs.z - normTopP->z);
            VECAdd(&normOfs, &weightNormOfs, &normOfs);
        }
        vtxEnvP->x = vtxTopP->x + vtxOfs.x;
        vtxEnvP->y = vtxTopP->y + vtxOfs.y;
        vtxEnvP->z = vtxTopP->z + vtxOfs.z;
        normEnvP->x = normTopP->x + normOfs.x;
        normEnvP->y = normTopP->y + normOfs.y;
        normEnvP->z = normTopP->z + normOfs.z;
    }
    vertexIdx = cenv->vtxCount;
    vtxEnvP = &vtxenv[vertexIdx];
    vtxTopP = &Vertextop[vertexIdx];
    for (i = 0; i < cenv->copyCount; i++, vtxEnvP++, vtxTopP++) {
        vtxEnvP->x = vtxTopP->x;
        vtxEnvP->y = vtxTopP->y;
        vtxEnvP->z = vtxTopP->z;
    }
}

static void SetMtx(HSF_OBJECT *obj, Mtx base) {
    HSF_SKELETON *skeleton;
    Mtx final;
    Mtx scale;
    Mtx rot;
    s32 meshNum;
    s32 i;

    skeleton = SearchSklenton(obj->name);
    if (skeleton) {
        obj->mesh.base = skeleton->transform;
    }
    PSMTXTrans(final, obj->mesh.base.pos.x, obj->mesh.base.pos.y, obj->mesh.base.pos.z);
    
    PSMTXConcat(base, final, final);
    if(obj->mesh.base.rot.z != 0.0f) {
        MTXRotDeg(rot, 'z', obj->mesh.base.rot.z);
        PSMTXConcat(final, rot, final);
    }
    if(obj->mesh.base.rot.y != 0.0f) {
        MTXRotDeg(rot, 'y', obj->mesh.base.rot.y);
        PSMTXConcat(final, rot, final);
    }
    if(obj->mesh.base.rot.x != 0.0f) {
        MTXRotDeg(rot, 'x', obj->mesh.base.rot.x);
        PSMTXConcat(final, rot, final);
    }
    if(obj->mesh.base.scale.x != 1.0 || obj->mesh.base.scale.y != 1.0 || obj->mesh.base.scale.z != 1.0) {
        PSMTXScale(scale, obj->mesh.base.scale.x, obj->mesh.base.scale.y, obj->mesh.base.scale.z);
        PSMTXConcat(final, scale, final);
    }
    
    meshNum = obj - objtop;
    PSMTXCopy(final, MtxTop[nMesh + meshNum]);
    for (i = 0; i < obj->mesh.childNum; i++) {
        SetMtx(obj->mesh.child[i], final);
    }
}

static void SetRevMtx(void) {
    HSF_OBJECT *obj;
    s32 meshNum;
    s32 i;
    s32 j;
    Mtx revMtx;
    Mtx tempMtx;

    obj = CurHsf->object;
    for (meshNum = i = 0; i < CurHsf->objectNum; i++, obj++) {
        if (obj->type == HSF_OBJ_MESH) {
            PSMTXCopy(MtxTop[nMesh + i], tempMtx);
            for (j = 0; j < CurHsf->objectNum; j++) {
                PSMTXInverse(MtxTop[nMesh + j], revMtx);
                PSMTXConcat(revMtx, tempMtx, MtxTop[nMesh + nObj + nObj * meshNum + j]);
            }
            PSMTXInverse(MtxTop[nMesh + i], tempMtx);
            meshNum++;
        }
    }
}

static HSF_SKELETON *SearchSklenton(char *name) {
    HSF_SKELETON *skeleton;
    s32 i;

    skeleton = CurHsf->skeleton;
    for (i = 0; i < CurHsf->skeletonNum; i++, skeleton++) {
        if (strcmp(name, skeleton->name) == 0) {
            return skeleton;
        }
    }
    return NULL;
}
