#ifndef _GAME_SPRITE_H
#define _GAME_SPRITE_H

#include "dolphin.h"
#include "humath.h"

#define HUSPR_DRAWNO_FRONT 0
#define HUSPR_DRAWNO_BACK 127

#define HUSPR_MAX 400
#define HUSPR_GROUP_MAX 256

#define HUSPR_NONE -1
#define HUSPR_GROUP_NONE -1

#define HUSPR_ATTR_NOANIM 0x1
#define HUSPR_ATTR_LOOP 0x2
#define HUSPR_ATTR_DISPOFF 0x4
#define HUSPR_ATTR_LINEAR 0x8
#define HUSPR_ATTR_FUNC 0x10
#define HUSPR_ATTR_NOPAUSE 0x20
#define HUSPR_ATTR_REVERSE 0x40
#define HUSPR_ATTR_ADDCOL 0x80
#define HUSPR_ATTR_INVCOL 0x100
#define HUSPR_ATTR_3D 0x200
#define HUSPR_ATTR_VTXCOLOR 0x400
#define HUSPR_ATTR_VTXCOLOR_ADD 0x800

#include "game/animdata.h"
#include "game/data.h"

#define HuSprDispOff(grpId, memberNo) HuSprAttrSet(grpId, memberNo, HUSPR_ATTR_DISPOFF)
#define HuSprDispOn(grpId, memberNo) HuSprAttrReset(grpId, memberNo, HUSPR_ATTR_DISPOFF)

#define HuSprAnimDataRead(fileNum) HuSprAnimRead(HuDataSelHeapReadNum(fileNum, HU_MEMNUM_OVL, HEAP_MODEL))

typedef struct HuSprite_s HUSPRITE;
typedef struct HuSprRect_s {
    float x0;
    float y0;
    float x1;
    float y1;
} HUSPR_RECT;

typedef s16 HUSPRID;
typedef s16 HUSPR_GROUPID;

typedef void (*HUSPR_FUNC)(HUSPRITE *sp);
typedef void (*HUSPR_3DHOOK)(HUSPRITE *sp, Mtx *matrix, s16 layerIdx, HUSPR_RECT *rectVtx, HUSPR_RECT *rectST);

typedef struct HuSpr3DData_s {
    int col;
    int row;
    float depthScale;
    HuVecF rot;
    HuVecF *vtx;
    HuVec2f *st;
} HUSPR_3DDATA;

struct HuSprite_s {
    u8 r;
    u8 g;
    u8 b;
    u8 drawNo;
    s16 animNo;
    s16 bank;
    s16 attr;
    s16 dirty;
    s16 prio;
    float time;
    HuVec2f pos;
    float zRot;
    HuVec2f scale;
    float speed;
    float a;
    GXTexWrapMode wrapS;
    GXTexWrapMode wrapT;
    s16 uvScaleX;
    s16 uvScaleY;
    Mtx *groupMtx;
    union {
        ANIMDATA *data;
        HUSPR_FUNC func;
    };
    ANIMPAT *patP;
    ANIMFRAME *frameP;
    s16 work[4];
    ANIMDATA *bg;
    u16 bgBank;
    s16 scissorX;
    s16 scissorY;
    s16 scissorW;
    s16 scissorH;
    GXColor vtxColor[4];
    HUSPR_3DHOOK hook3D;
    HUSPR_3DDATA *data3D;
};

typedef struct HuSprGroup_s {
    s16 sprNum;
    HuVec2f pos;
    float zRot;
    HuVec2f scale;
    HuVec2f center;
    HUSPRID *sprId;
    Mtx mtx;
    s16 work[4];
} HUSPR_GROUP;

extern HUSPRITE *HuSprData;
extern HUSPR_GROUP HuSprGrpData[HUSPR_GROUP_MAX];

void HuSprInit(void);
void HuSprClose(void);
void HuSprExec(s16 drawNo);
void HuSprBegin(void);
HUSPRITE *HuSprCall(void);
void HuSprFinish(void);
void HuSprPauseSet(BOOL value);
ANIMDATA *HuSprAnimRead(void *data);
void HuSprAnimLock(ANIMDATA *anim);
HUSPRID HuSprCreate(ANIMDATA *anim, s16 prio, s16 bank);
HUSPRID HuSprFuncCreate(HUSPR_FUNC func, s16 prio);
HUSPR_GROUPID HuSprGrpCreate(s16 sprNum);
HUSPR_GROUPID HuSprGrpCopy(HUSPR_GROUPID grpId);
void HuSprGrpMemberSet(HUSPR_GROUPID grpId, s16 memberNo, HUSPRID sprId);
void HuSprGrpMemberKill(HUSPR_GROUPID grpId, s16 memberNo);
void HuSprGrpKill(HUSPR_GROUPID grpId);
void HuSprKill(HUSPRID sprId);
void HuSprAnimKill(ANIMDATA *anim);
void HuSprAttrSet(HUSPR_GROUPID grpId, s16 memberNo, s32 attr);
void HuSprAttrReset(HUSPR_GROUPID grpId, s16 memberNo, s32 attr);
void HuSprGrpAttrSet(HUSPR_GROUPID grpId, s32 attr);
void HuSprGrpAttrReset(HUSPR_GROUPID grpId, s32 attr);

void HuSprPosSet(HUSPR_GROUPID grpId, s16 memberNo, float posX, float posY);
void HuSprZRotSet(HUSPR_GROUPID grpId, s16 memberNo, float z_rot);
void HuSprScaleSet(HUSPR_GROUPID grpId, s16 memberNo, float scaleX, float scaleY);
void HuSprTPLvlSet(HUSPR_GROUPID grpId, s16 memberNo, float tpLvl);
void HuSprColorSet(HUSPR_GROUPID grpId, s16 memberNo, u8 r, u8 g, u8 b);
void HuSprSpeedSet(HUSPR_GROUPID grpId, s16 memberNo, float speed);
void HuSprBankSet(HUSPR_GROUPID grpId, s16 memberNo, s16 bank);
void HuSprGrpPosSet(HUSPR_GROUPID grpId, float posX, float posY);
void HuSprGrpCenterSet(HUSPR_GROUPID grpId, float centerX, float centerY);
void HuSprGrpZRotSet(HUSPR_GROUPID grpId, float z_rot);
void HuSprGrpScaleSet(HUSPR_GROUPID grpId, float scaleX, float scaleY);
void HuSprGrpTPLvlSet(HUSPR_GROUPID grpId, float tpLvl);
void HuSprGrpDrawNoSet(HUSPR_GROUPID grpId, s32 drawNo);
void HuSprDrawNoSet(HUSPR_GROUPID grpId, s16 memberNo, s32 drawNo);
void HuSprPriSet(HUSPR_GROUPID grpId, s16 memberNo, s16 prio);
void HuSprGrpScissorSet(HUSPR_GROUPID grpId, s16 x, s16 y, s16 w, s16 h);
void HuSprScissorSet(HUSPR_GROUPID grpId, s16 memberNo, s16 x, s16 y, s16 w, s16 h);
ANIMDATA *HuSprAnimMake(s16 sizeX, s16 sizeY, s16 dataFmt);
void HuSprBGSet(HUSPR_GROUPID grpId, s16 memberNo,  ANIMDATA *bg, s16 bgBank);
void HuSprSprBGSet(HUSPRID sprId, ANIMDATA *bg, s16 bgBank);
void AnimDebug(ANIMDATA *anim);

void HuSprDispInit(void);
void HuSprDisp(HUSPRITE *sp);
//WipeImage breaks with this declaration
//void HuSprTexLoad(ANIMDATA *anim, s16 bmpNo, s16 texMapId, GXTexWrapMode wrapS, GXTexWrapMode wrapT, GXTexFilter filter);
void HuSprExecLayerInit(void);
void HuSprExecLayerCameraSet(s16 drawNo, s16 camera, s16 layer);
void HuSprExecLayerSet(s16 drawNo, s16 layer);
void HuSpr3DDisp(HUSPRITE *sp, Mtx *matrix, s16 layerIdx, HUSPR_RECT *rectVtx, HUSPR_RECT *rectST);
void HuSpr3DHookSet(HUSPRID sprId, HUSPR_3DHOOK hook3D);
HUSPR_3DDATA *HuSpr3DDataCreate(HUSPRID sprId, int size);
void HuSpr3DSet(HUSPRID sprId);
void HuSpr3DRotSet(HUSPRID sprId, float x, float y, float z);
void HuSpr3DFovSet(HUSPRID sprId, float fov);
void HuSprVtxColorSet(HUSPR_GROUPID grpId, s16 memberNo, GXColor *vtxColor);

#endif