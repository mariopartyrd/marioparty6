#include "game/sprite.h"
#include "game/memory.h"
#include "game/init.h"

#include "dolphin/mtx.h"

#define HUSPR_ORDER_MAX 768

#define SPRITE_DIRTY_ATTR 0x1
#define SPRITE_DIRTY_XFORM 0x2
#define SPRITE_DIRTY_COLOR 0x4

typedef struct HuSprOrder_s {
    u16 grpId;
    u16 sprId;
    u16 prio;
    u16 next;
} HUSPR_ORDER;

HUSPR_GROUP HuSprGrpData[HUSPR_GROUP_MAX];
static HUSPR_ORDER HuSprOrder[HUSPR_ORDER_MAX];

static s16 HuSprOrderNum;
static s16 HuSprOrderNo;
static BOOL HuSprPauseF;
HUSPRITE *HuSprData;

static void HuSprOrderEntry(HUSPR_GROUPID grpId, HUSPRID sprId);

void HuSprInit(void)
{
    s16 i;
    HUSPRITE *sp;
    HUSPR_GROUP *gp;
    if(!HuSprData) {
        HuSprData = HuMemDirectMalloc(HEAP_HEAP, sizeof(HUSPRITE)*HUSPR_MAX);
    }
    for(sp = &HuSprData[1], i=1; i<HUSPR_MAX; i++, sp++) {
        sp->data = NULL;
    }
    for(gp = HuSprGrpData, i=0; i<HUSPR_GROUP_MAX; i++, gp++) {
        gp->sprNum = 0;
    }
    HuSprExecLayerInit();
    sp = &HuSprData[0];
    sp->prio = 0;
    sp->data = (void *)1;
    HuSprPauseF = FALSE;
}

void HuSprClose(void)
{
    s16 i;
    HUSPR_GROUP *gp;
    HUSPRITE *sp;
    
    for(gp = HuSprGrpData, i=0; i<HUSPR_GROUP_MAX; i++, gp++) {
        if(gp->sprNum != 0) {
            HuSprGrpKill(i);
        }
    }
    for(sp = &HuSprData[1], i=1; i<HUSPR_MAX; i++, sp++) {
        if(sp->data) {
            HuSprKill(i);
        }
    }
    HuSprExecLayerInit();
    HuSprPauseF = FALSE;
}

void HuSprExec(s16 drawNo)
{
    HUSPRITE *sp;
    while(sp = HuSprCall()) {
        if(!(sp->attr & HUSPR_ATTR_DISPOFF) && sp->drawNo == drawNo) {
            HuSprDisp(sp);
        }
    }
}

void HuSprBegin(void)
{
    Mtx temp, rot;
    s16 i, j;
    Vec axis = {0, 0, 1};
    HUSPR_GROUP *gp;
    gp = HuSprGrpData;
    HuSprOrderNum = 1;
    HuSprOrder[0].next = 0;
    HuSprOrder[0].prio = -1;
    for(i=0; i<HUSPR_GROUP_MAX; i++, gp++) {
        if(gp->sprNum != 0) {
            MTXTrans(temp, gp->center.x*gp->scale.x, gp->center.y*gp->scale.y, 0.0f);
            MTXRotAxisDeg(rot, &axis, gp->zRot);
            MTXConcat(rot, temp, gp->mtx);
            MTXScale(temp, gp->scale.x, gp->scale.y, 1.0f);
            MTXConcat(gp->mtx, temp, gp->mtx);
            mtxTransCat(gp->mtx, gp->pos.x, gp->pos.y, 0);
            for(j=0; j<gp->sprNum; j++) {
                if(gp->sprId[j] != -1) {
                    HuSprOrderEntry(i, gp->sprId[j]);
                }
            }
        }
    }
    HuSprOrderNo = 0;
}

static void HuSprOrderEntry(HUSPR_GROUPID grpId, HUSPRID sprId)
{
    HUSPR_ORDER *order = &HuSprOrder[HuSprOrderNum];
    s16 prio = HuSprData[sprId].prio;
    s16 prev, next;
    if(HuSprOrderNum >= HUSPR_ORDER_MAX) {
        OSReport("Order Max Over!\n");
        return;
    }
    next = HuSprOrder[0].next;
    for(prev = 0; next != 0; prev = next, next = HuSprOrder[next].next) {
        if(HuSprOrder[next].prio < prio) {
            break;
        }
    }
    order->next = HuSprOrder[prev].next;
    HuSprOrder[prev].next = HuSprOrderNum;
    order->prio = prio;
    order->grpId = grpId;
    order->sprId = sprId;
    HuSprOrderNum++;
}

HUSPRITE *HuSprCall(void)
{
    HuSprOrderNo = HuSprOrder[HuSprOrderNo].next;
    if(HuSprOrderNo != 0) {
        HUSPR_ORDER *order = &HuSprOrder[HuSprOrderNo];
        HUSPRITE *sp = &HuSprData[order->sprId];
        sp->groupMtx = &HuSprGrpData[order->grpId].mtx;
        if(sp->attr & HUSPR_ATTR_FUNC) {
            return sp;
        }
        sp->frameP = &sp->data->bank[sp->bank].frame[sp->animNo];
        sp->patP = &sp->data->pat[sp->frameP->pat];
        return sp;
    } else {
        return NULL;
    }
}

static inline void SpriteCalcFrame(HUSPRITE *sp, ANIMBANK *bank, ANIMFRAME **frame, s16 loopF)
{
    if(sp->time >= (*frame)->time) {
        sp->animNo++;
        sp->time -= (*frame)->time;
        if(sp->animNo >= bank->timeNum || (*frame)[1].time == -1) {
            if(loopF) {
                sp->animNo = 0;
            } else {
                sp->animNo = bank->timeNum-1;
            }
        }
        *frame = &bank->frame[sp->animNo];
    } else if(sp->time < 0) {
        sp->animNo--;
        if(sp->animNo < 0) {
            if(loopF) {
                sp->animNo = bank->timeNum-1;
            } else {
                sp->animNo = 0;
            }
        }
        *frame = &bank->frame[sp->animNo];
        sp->time += (*frame)->time;
    }
}

void HuSprFinish(void)
{
    ANIMDATA *anim;
    ANIMBANK *bank;
    ANIMFRAME *frame;
    HUSPRITE *sp;
    s16 i;
    s16 j;
    s16 loopF;
    s16 timeInc;
    
    for(sp = &HuSprData[1], i=1; i<HUSPR_MAX; i++, sp++) {
        if(sp->data && !(sp->attr & HUSPR_ATTR_FUNC)) {
            if(!HuSprPauseF || (sp->attr & HUSPR_ATTR_NOPAUSE)) {
                anim = sp->data;
                bank = &anim->bank[sp->bank];
                frame = &bank->frame[sp->animNo];
                loopF = (sp->attr & HUSPR_ATTR_LOOP) ? 0 : 1;
                if(!(sp->attr & HUSPR_ATTR_NOANIM)) {
                    timeInc = (sp->attr & HUSPR_ATTR_REVERSE) ? -1 : 1;
                    for(j=0; j<(s32)sp->speed*minimumVcount; j++) {
                        sp->time += timeInc;
                        SpriteCalcFrame(sp, bank, &frame, loopF);
                    }
                    sp->time += (sp->speed*(float)minimumVcount)-j;
                    SpriteCalcFrame(sp, bank, &frame, loopF);
                }
                sp->dirty = 0;
            }
        }
    }
}

void HuSprPauseSet(BOOL value)
{
    HuSprPauseF = value;
}

ANIMDATA *HuSprAnimRead(void *data)
{
    s16 i;
    ANIMBMP *bmp;
    ANIMBANK *bank;
    ANIMPAT *pat;
    
    ANIMDATA *anim = (ANIMDATA *)data;
    if((u32)anim->bank & 0xFFFF0000) {
        anim->useNum++;
        return anim;
    }
    bank = (ANIMBANK *)((u32)anim->bank+(u32)data);
    anim->bank = bank;
    pat = (ANIMPAT *)((u32)anim->pat+(u32)data);
    anim->pat = pat;
    bmp = (ANIMBMP *)((u32)anim->bmp+(u32)data);
    anim->bmp = bmp;
    for(i=0; i<anim->bankNum; i++, bank++) {
        bank->frame = (ANIMFRAME *)((u32)bank->frame+(u32)data);
    }
    for(i=0; i<anim->patNum; i++, pat++) {
        pat->layer = (ANIMLAYER *)((u32)pat->layer+(u32)data);
    }
    for(i=0; i<anim->bmpNum; i++, bmp++) {
        bmp->palData = (void *)((u32)bmp->palData+(u32)data);
        bmp->data = (void *)((u32)bmp->data+(u32)data);
    }
    anim->useNum = 0;
    return anim;
}

void HuSprAnimLock(ANIMDATA *anim)
{
    anim->useNum++;
}

HUSPRID HuSprCreate(ANIMDATA *anim, s16 prio, s16 bank)
{
    HUSPRITE *sp;
    s16 i;
    for(sp = &HuSprData[1], i=1; i<HUSPR_MAX; i++, sp++) {
        if(!sp->data) {
            break;
        }
    }
    if(i == HUSPR_MAX) {
        OSReport("Error: Sprite Max Over!\n");
        return HUSPR_NONE;
    }
    sp->data = anim;
    sp->speed = 1.0f;
    sp->animNo = 0;
    sp->bank = bank;
    sp->time = 0.0f;
    sp->attr = HUSPR_ATTR_LINEAR;
    sp->drawNo = 0;
    sp->r = sp->g = sp->b = sp->a = 255;
    sp->pos.x = sp->pos.y = sp->zRot = 0.0f;
    sp->prio = prio;
    sp->scale.x = sp->scale.y = 1.0f;
    sp->wrapS = sp->wrapT = GX_CLAMP;
    sp->uvScaleX = sp->uvScaleY = 1;
    sp->bg = NULL;
    sp->scissorX = sp->scissorY = 0;
    sp->scissorW = 640;
    sp->scissorH = 480;
    sp->hook3D = NULL;
    sp->data3D = NULL;
    if(anim) {
        HuSprAnimLock(anim);
    }
    return i;
}

HUSPRID HuSprFuncCreate(HUSPR_FUNC func, s16 prio)
{
    HUSPRITE *sp;
    HUSPRID sprId = HuSprCreate(NULL, prio, 0);
    if(sprId == HUSPR_NONE) {
        return HUSPR_NONE;
    }
    sp = &HuSprData[sprId];
    sp->func = func;
    sp->attr |= HUSPR_ATTR_FUNC;
    return sprId;
}

HUSPR_GROUPID HuSprGrpCreate(s16 sprNum)
{
    HUSPR_GROUP *gp;
    s16 i, j;
    for(gp = HuSprGrpData, i=0; i<HUSPR_GROUP_MAX; i++, gp++) {
        if(gp->sprNum == 0) {
            break;
        }
    }
    if(i == HUSPR_GROUP_MAX) {
        return HUSPR_GROUP_NONE;
    }
    gp->sprId = HuMemDirectMalloc(HEAP_HEAP, sizeof(HUSPRID)*sprNum);
    for(j=0; j<sprNum; j++) {
        gp->sprId[j] = HUSPR_NONE;
    }
    gp->sprNum = sprNum;
    gp->pos.x = gp->pos.y = gp->zRot = gp->center.x = gp->center.y = 0.0f;
    gp->scale.x = gp->scale.y = 1.0f;
    return i;
}

HUSPR_GROUPID HuSprGrpCopy(HUSPR_GROUPID grpId)
{
    HUSPR_GROUP *newGp;
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HUSPR_GROUPID newGrpId = HuSprGrpCreate(gp->sprNum);
    s16 i;
    if(newGrpId == HUSPR_GROUP_NONE) {
        return HUSPR_GROUP_NONE;
    }
    newGp = &HuSprGrpData[newGrpId];
    newGp->pos.x = gp->pos.x;
    newGp->pos.y = gp->pos.y;
    newGp->zRot = gp->zRot;
    newGp->scale.x = gp->scale.x;
    newGp->scale.y = gp->scale.y;
    newGp->center.x = gp->center.x;
    newGp->center.y = gp->center.y;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HUSPRITE *sp = &HuSprData[gp->sprId[i]];
            s16 sprId = HuSprCreate(sp->data, sp->prio, sp->bank);
            HuSprData[sprId] = *sp;
            HuSprGrpMemberSet(newGrpId, i, sprId);
        }
    }
    return newGrpId;
}

void HuSprGrpMemberSet(HUSPR_GROUPID grpId, s16 memberNo, HUSPRID sprId)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HUSPRITE *sp = &HuSprData[sprId];
    if(gp->sprNum == 0 || gp->sprNum <= memberNo || gp->sprId[memberNo] != HUSPR_NONE) {
        return;
    }
    gp->sprId[memberNo] = sprId;
}

void HuSprGrpMemberKill(HUSPR_GROUPID grpId, s16 memberNo)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    if(gp->sprNum == 0 || gp->sprNum <= memberNo || gp->sprId[memberNo] == HUSPR_NONE) {
        return;
    }
    HuSprKill(gp->sprId[memberNo]);
    gp->sprId[memberNo] = HUSPR_NONE;
}

void HuSprGrpKill(HUSPR_GROUPID grpId)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprKill(gp->sprId[i]);
        }
    }
    gp->sprNum = 0;
    HuMemDirectFree(gp->sprId);
}

void HuSprKill(HUSPRID sprId)
{
    HUSPRITE *sp = &HuSprData[sprId];
    if(!sp->data) {
        return;
    }
    if(!(sp->attr & HUSPR_ATTR_FUNC)) {
        HuSprAnimKill(sp->data);
    }
    if(sp->bg) {
        HuSprAnimKill(sp->bg);
        sp->bg = NULL;
    }
    if(sp->attr & HUSPR_ATTR_3D) {
        if(sp->data3D) {
            HuMemDirectFree(sp->data3D);
        }
    }
    sp->data = NULL;
}

void HuSprAnimKill(ANIMDATA *anim)
{
    if(--anim->useNum <= 0) {
        if(anim->bmpNum & ANIM_BMP_ALLOC) {
            if(anim->bmp->data) {
                HuMemDirectFree(anim->bmp->data);
            }
            if(anim->bmp->palData) {
                HuMemDirectFree(anim->bmp->palData);
            }
        }
        HuMemDirectFree(anim);
    }
}

void HuSprAttrSet(HUSPR_GROUPID grpId, s16 memberNo, s32 attr)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HUSPRITE *sp;
    if(gp->sprNum == 0 || gp->sprNum <= memberNo || gp->sprId[memberNo] == HUSPR_NONE) {
        return;
    }
    sp = &HuSprData[gp->sprId[memberNo]];
    sp->attr |= attr;
    sp->dirty |= SPRITE_DIRTY_ATTR;
}

void HuSprAttrReset(HUSPR_GROUPID grpId, s16 memberNo, s32 attr)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HUSPRITE *sp;
    if(gp->sprNum == 0 || gp->sprNum <= memberNo || gp->sprId[memberNo] == HUSPR_NONE) {
        return;
    }
    sp = &HuSprData[gp->sprId[memberNo]];
    sp->attr &= ~attr;
    sp->dirty |= SPRITE_DIRTY_ATTR;
}

void HuSprGrpAttrSet(HUSPR_GROUPID grpId, s32 attr)
{
    s16 i;
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprAttrSet(grpId, i, attr);
        }
    }
}

void HuSprGrpAttrReset(HUSPR_GROUPID grpId, s32 attr)
{
    s16 i;
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprAttrReset(grpId, i, attr);
        }
    }
}

u16 HuSprAttrGet(HUSPR_GROUPID grpId, s16 memberNo)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HUSPRITE *sp;
    if(gp->sprNum == 0 || gp->sprNum <= memberNo || gp->sprId[memberNo] == HUSPR_NONE) {
        return;
    }
    sp = &HuSprData[gp->sprId[memberNo]];
    return sp->attr;
}

void HuSprPosSet(HUSPR_GROUPID grpId, s16 memberNo, float posX, float posY)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->pos.x = posX;
    sp->pos.y = posY;
    sp->dirty |= SPRITE_DIRTY_XFORM;
}

void HuSprZRotSet(HUSPR_GROUPID grpId, s16 memberNo, float zRot)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->zRot = zRot;
    sp->dirty |= SPRITE_DIRTY_XFORM;
}

void HuSprScaleSet(HUSPR_GROUPID grpId, s16 memberNo, float scaleX, float scaleY)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->scale.x = scaleX;
    sp->scale.y = scaleY;
    sp->dirty |= SPRITE_DIRTY_XFORM;
}

void HuSprTPLvlSet(HUSPR_GROUPID grpId, s16 memberNo, float tpLvl)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->a = tpLvl*255;
    sp->dirty |= SPRITE_DIRTY_COLOR;
}

void HuSprColorSet(HUSPR_GROUPID grpId, s16 memberNo, u8 r, u8 g, u8 b)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->r = r;
    sp->g = g;
    sp->b = b;
    sp->dirty |= SPRITE_DIRTY_COLOR;
}

void HuSprSpeedSet(HUSPR_GROUPID grpId, s16 memberNo, float speed)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    HuSprData[gp->sprId[memberNo]].speed = speed;
}

void HuSprBankSet(HUSPR_GROUPID grpId, s16 memberNo, s16 bank)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    ANIMDATA *anim = sp->data;
    ANIMBANK *bank_ptr = &anim->bank[sp->bank];
    ANIMFRAME *frame_ptr = &bank_ptr->frame[sp->animNo];
    sp->bank = bank;
    if(sp->attr & HUSPR_ATTR_REVERSE) {
        sp->animNo = bank_ptr->timeNum-1;
        frame_ptr = &bank_ptr->frame[sp->animNo];
        sp->time = frame_ptr->time;
    } else {
        sp->time = 0;
        sp->animNo = 0;
    }
}

void HuSprAnimNoSet(HUSPR_GROUPID grpId, s16 memberNo, s16 animNo)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    ANIMDATA *anim = sp->data;
    ANIMBANK *bank_ptr = &anim->bank[sp->bank];
    if(bank_ptr->timeNum <= animNo) {
        OSReport("Error: AnimNoSet Over %d\n", animNo);
        animNo = 0;
    }
    sp->animNo = animNo;
    sp->time = 0;
}

void HuSprAnimNoSetPause(HUSPR_GROUPID grpId, s16 memberNo, s16 animNo)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    HuSprAnimNoSet(grpId, memberNo, animNo);
    sp->attr |= HUSPR_ATTR_NOANIM;
}

void HuSprGrpPosSet(HUSPR_GROUPID grpId, float posX, float posY)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    gp->pos.x = posX;
    gp->pos.y = posY;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != -1) {
            HuSprData[gp->sprId[i]].dirty |= SPRITE_DIRTY_XFORM;
        }
    }
}

void HuSprGrpCenterSet(HUSPR_GROUPID grpId, float centerX, float centerY)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    gp->center.x = centerX;
    gp->center.y = centerY;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprData[gp->sprId[i]].dirty |= SPRITE_DIRTY_XFORM;
        }
    }
}

void HuSprGrpZRotSet(HUSPR_GROUPID grpId, float zRot)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    gp->zRot = zRot;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprData[gp->sprId[i]].dirty |= SPRITE_DIRTY_XFORM;
        }
    }
}

void HuSprGrpScaleSet(HUSPR_GROUPID grpId, float scaleX, float scaleY)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    gp->scale.x = scaleX;
    gp->scale.y = scaleY;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprData[gp->sprId[i]].dirty |= SPRITE_DIRTY_XFORM;
        }
    }
}

void HuSprGrpTPLvlSet(HUSPR_GROUPID grpId, float tpLvl)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprData[gp->sprId[i]].a = tpLvl*255;
            HuSprData[gp->sprId[i]].dirty |= SPRITE_DIRTY_COLOR;
        }
    }
}

void HuSprGrpDrawNoSet(HUSPR_GROUPID grpId, s32 drawNo)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprData[gp->sprId[i]].drawNo = drawNo;
        }
    }
}

void HuSprDrawNoSet(HUSPR_GROUPID grpId, s16 memberNo, s32 drawNo)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->drawNo = drawNo;
}

void HuSprPriSet(HUSPR_GROUPID grpId, s16 memberNo, s16 prio)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->prio = prio;
}

void HuSprGrpScissorSet(HUSPR_GROUPID grpId, s16 x, s16 y, s16 w, s16 h)
{
    HUSPR_GROUP *gp = &HuSprGrpData[grpId];
    s16 i;
    for(i=0; i<gp->sprNum; i++) {
        if(gp->sprId[i] != HUSPR_NONE) {
            HuSprScissorSet(grpId, i, x, y, w, h);
        }
    }
}

void HuSprScissorSet(HUSPR_GROUPID grpId, s16 memberNo, s16 x, s16 y, s16 w, s16 h)
{
    HUSPRITE *sp = &HuSprData[HuSprGrpData[grpId].sprId[memberNo]];
    sp->scissorX = x;
    sp->scissorY = y;
    sp->scissorW = w;
    sp->scissorH = h;
}

static s16 bitSizeTbl[11] = { 32, 24, 16, 8, 4, 16, 8, 8, 4, 8, 4 };

ANIMDATA *HuSprAnimMake(s16 sizeX, s16 sizeY, s16 dataFmt)
{
    ANIMLAYER *layer;
    ANIMBMP *bmp;
    ANIMDATA *anim;
    ANIMPAT *pat;
    ANIMFRAME *frame;
    void *temp;
    ANIMBANK *bank;
    ANIMDATA *newAnim;

    anim = newAnim = HuMemDirectMalloc(HEAP_MODEL, sizeof(ANIMDATA)+sizeof(ANIMBANK)+sizeof(ANIMFRAME)
                                            +sizeof(ANIMPAT)+sizeof(ANIMLAYER)+sizeof(ANIMBMP));

    bank = temp = &newAnim[1];
    anim->bank = bank;
    frame = temp = ((char *)temp+sizeof(ANIMBANK));
    bank->frame = frame;
    pat = temp = ((char *)temp+sizeof(ANIMFRAME));
    anim->pat = pat;
    layer = temp = ((char *)temp+sizeof(ANIMPAT));
    pat->layer = layer;
    bmp = temp = ((char *)temp+sizeof(ANIMLAYER));
    anim->bmp = bmp;
    anim->useNum = 0;
    anim->bankNum = 1;
    anim->patNum = 1;
    anim->bmpNum = (1|ANIM_BMP_ALLOC);
    bank->timeNum = 1;
    bank->unk = 10;
    frame->pat = 0;
    frame->time = 10;
    frame->shiftX = frame->shiftY = frame->flip = 0;
    pat->layerNum = 1;
    pat->centerX = sizeX/2;
    pat->centerY = sizeY/2;
    pat->sizeX = sizeX;
    pat->sizeY = sizeY;
    layer->alpha = 255;
    layer->flip = 0;
    layer->bmpNo = 0;
    layer->startX = layer->startY = 0;
    layer->sizeX = sizeX;
    layer->sizeY = sizeY;
    layer->shiftX = layer->shiftY = 0;
    layer->vtx[0] = layer->vtx[1] = 0;
    layer->vtx[2] = sizeX;
    layer->vtx[3] = 0;
    layer->vtx[4] = sizeX;
    layer->vtx[5] = sizeY;
    layer->vtx[6] = 0;
    layer->vtx[7] = sizeY;
    bmp->pixSize = bitSizeTbl[dataFmt];
    bmp->dataFmt = dataFmt;
    bmp->palNum = 0;
    bmp->sizeX = sizeX;
    bmp->sizeY = sizeY;
    bmp->dataSize = sizeX*sizeY*bitSizeTbl[dataFmt]/8;
    bmp->palData = NULL;
    bmp->data = NULL;
    return anim;
}

void HuSprBGSet(HUSPR_GROUPID grpId, s16 memberNo, ANIMDATA *bg, s16 bgBank)
{
    HUSPRID sprId = HuSprGrpData[grpId].sprId[memberNo];
    HuSprSprBGSet(sprId, bg, bgBank);
}

void HuSprSprBGSet(HUSPRID sprId, ANIMDATA *bg, s16 bgBank)
{
    HUSPRITE *sp = &HuSprData[sprId];
    sp->bg = bg;
    sp->bgBank = bgBank;
    sp->wrapT = sp->wrapS = GX_REPEAT;
    sp->attr &= ~HUSPR_ATTR_LINEAR;
    HuSprAnimLock(bg);
}

void HuSpr3DHookSet(HUSPRID sprId, HUSPR_3DHOOK hook3D)
{
    HUSPRITE *sp = &HuSprData[sprId];
    sp->hook3D = hook3D;
}

HUSPR_3DDATA *HuSpr3DDataCreate(HUSPRID sprId, int size)
{
    HUSPRITE *sp = &HuSprData[sprId];
    sp->data3D = HuMemDirectMalloc(HEAP_HEAP, size);
    sp->attr |= HUSPR_ATTR_3D;
    return sp->data3D;
}

void HuSpr3DSet(HUSPRID sprId)
{
    HUSPR_3DDATA *data3D; //r31
    HUSPRITE *sp; //r30
    int i; //r29
    int j; //r28
    int w; //r26
    int h; //r25
    int gridSize; //r24
    int col; //r23
    int row; //r22
    
    sp = &HuSprData[sprId];
    HuSpr3DHookSet(sprId, HuSpr3DDisp);
    w=h=0;
    for(i=0; i<sp->data->patNum; i++) {
        for(j=0; j<sp->data->pat[i].layerNum; j++) {
            if(w < sp->data->pat[i].layer[j].sizeX) {
                w = sp->data->pat[i].layer[j].sizeX;
            }
            if(h < sp->data->pat[i].layer[j].sizeY) {
                h = sp->data->pat[i].layer[j].sizeY;
            }
        }
    }
    if(w < 16) {
        col = 1;
    } else {
        col = w/16;
    }
    if(h < 16) {
        row = 1;
    } else {
        row = h/16;
    }
    gridSize = (col+1)*(row+1);
    data3D = HuSpr3DDataCreate(sprId, sizeof(HUSPR_3DDATA)+(gridSize*sizeof(HuVecF))+(gridSize*sizeof(HuVec2f)));
    data3D->vtx = (HuVecF *)&data3D[1];
    data3D->st = (HuVec2f *)&data3D->vtx[gridSize];
    data3D->rot.x = data3D->rot.y = data3D->rot.z = 0;
    data3D->col = col;
    data3D->row = row;
    data3D->depthScale = 2*HuSin(30);
}

void HuSpr3DRotSet(HUSPRID sprId, float x, float y, float z)
{
    HUSPRITE *sp = &HuSprData[sprId];
    HUSPR_3DDATA *data3D = sp->data3D;
    if(data3D) {
        data3D->rot.x = x;
        data3D->rot.y = y;
        data3D->rot.z = z;
    }
}

void HuSpr3DFovSet(HUSPRID sprId, float fov)
{
    HUSPRITE *sp = &HuSprData[sprId];
    HUSPR_3DDATA *data3D = sp->data3D;
    if(data3D) {
        data3D->depthScale = 2*HuSin(fov/2);
    }
}

void HuSprVtxColorSet(HUSPR_GROUPID grpId, s16 memberNo, GXColor *vtxColor)
{
    HUSPRID sprId = HuSprGrpData[grpId].sprId[memberNo];
    HUSPRITE *sp = &HuSprData[sprId];
    s16 i;
    sp->attr |= HUSPR_ATTR_VTXCOLOR;
    for(i=0; i<4; i++) {
        sp->vtxColor[i] = vtxColor[i];
    }
}

void HuSprVtxColorReset(HUSPR_GROUPID grpId, s16 memberNo)
{
    HUSPRID sprId = HuSprGrpData[grpId].sprId[memberNo];
    HUSPRITE *sp = &HuSprData[sprId];
    sp->attr &= ~HUSPR_ATTR_VTXCOLOR;
}


void AnimDebug(ANIMDATA *anim)
{
    ANIMPAT *pat;
    ANIMLAYER *layer;
    s16 i;
    s16 j;
    ANIMFRAME *frame;
    ANIMBANK *bank;
    ANIMBMP *bmp;
    
    OSReport("patNum %d,bankNum %d,bmpNum %d\n", anim->patNum, anim->bankNum, anim->bmpNum & ANIM_BMP_NUM_MASK);
    pat = anim->pat;
    for(i=0; i<anim->patNum; i++) {
        OSReport("PATTERN%d:\n", i);
        OSReport("\tlayerNum %d,center (%d,%d),size (%d,%d)\n", pat->layerNum, pat->centerX, pat->centerX, pat->sizeX, pat->sizeY);
        layer = pat->layer;
        for(j=0; j<pat->layerNum; j++) {
            OSReport("\t\tfileNo %d,flip %x\n", layer->bmpNo, layer->flip);
            OSReport("\t\tstart (%d,%d),size (%d,%d),shift (%d,%d)\n", layer->startX, layer->startY, layer->sizeX, layer->sizeY, layer->shiftX, layer->shiftY);
            if(j != pat->layerNum-1) {
                OSReport("\n");
            }
            layer++;
        }
        pat++;
    }
    bank = anim->bank;
    for(i=0; i<anim->bankNum; i++) {
        OSReport("BANK%d:\n", i);
        OSReport("\ttimeNum %d\n", bank->timeNum);
        frame = bank->frame;
        for(j=0; j<bank->timeNum; j++) {
            OSReport("\t\tpat %d,time %d,shift(%d,%d),flip %x\n", frame->pat, frame->time, frame->shiftX, frame->shiftY, frame->flip);
            frame++;
        }
        bank++;
    }
    bmp = anim->bmp;
    for(i=0; i<anim->bmpNum & ANIM_BMP_NUM_MASK; i++) {
        OSReport("BMP%d:\n", i);
        OSReport("\tpixSize %d,palNum %d,size (%d,%d)\n", bmp->pixSize, bmp->palNum, bmp->sizeX, bmp->sizeY);
        bmp++;
    }
}