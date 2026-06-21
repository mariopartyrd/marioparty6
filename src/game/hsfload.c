#include "game/hsfload.h"
#include "string.h"
#include "ctype.h"

GXColor rgba[100];
HSF_HEADER head;
HSF_DATA Model;

static BOOL MotionOnly;
static HSF_DATA *MotionModel;
static void *VertexDataTop;
static void *NormalDataTop;
void *fileptr;
char *StringTable;
char *DicStringTable;
void **NSymIndex;
HSF_OBJECT *objtop;
HSF_BUFFER *vtxtop;
HSF_CLUSTER *ClusterTop;
HSF_ATTRIBUTE *AttributeTop;
HSF_MATERIAL *MaterialTop;

static void FileLoad(void *data);
static HSF_DATA *SetHsfModel(void);
static void MaterialLoad(void);
static void AttributeLoad(void);
static void SceneLoad(void);
static void ColorLoad(void);
static void VertexLoad(void);
static void NormalLoad(void);
static void STLoad(void);
static void FaceLoad(void);
static void ObjectLoad(void);
static void CenvLoad(void);
static void SkeletonLoad(void);
static void PartLoad(void);
static void ClusterLoad(void);
static void ShapeLoad(void);
static void MapAttrLoad(void);
static void PaletteLoad(void);
static void BitmapLoad(void);
static void MotionLoad(void);
static void MatrixLoad(void);

static s32 SearchObjectSetName(HSF_DATA *data, char *name);
static HSF_BUFFER *SearchVertexPtr(s32 id);
static HSF_BUFFER *SearchNormalPtr(s32 id);
static HSF_BUFFER *SearchStPtr(s32 id);
static HSF_BUFFER *SearchColorPtr(s32 id);
static HSF_BUFFER *SearchFacePtr(s32 id);
static HSF_CENV *SearchCenvPtr(s32 id);
static HSF_PART *SearchPartPtr(s32 id);
static HSF_PALETTE *SearchPalettePtr(s32 id);

static HSF_BITMAP *SearchBitmapPtr(s32 id);
static char *GetString(u32 *strOfs);
static char *GetMotionString(u16 *strOfs);

HSF_DATA *LoadHSF(void *data)
{
    HSF_DATA *hsf;
    Model.root = NULL;
    objtop = NULL;
    FileLoad(data);
    SceneLoad();
    ColorLoad();
    PaletteLoad();
    BitmapLoad();
    MaterialLoad();
    AttributeLoad();
    VertexLoad();
    NormalLoad();
    STLoad();
    FaceLoad();
    ObjectLoad();
    CenvLoad();
    SkeletonLoad();
    PartLoad();
    ClusterLoad();
    ShapeLoad();
    MapAttrLoad();
    MotionLoad();
    MatrixLoad();
    hsf = SetHsfModel();
    InitEnvelope(hsf);
    objtop = NULL;
    return hsf;
    
}

void ClusterAdjustObject(HSF_DATA *model, HSF_DATA *srcModel)
{
    HSF_CLUSTER *cluster;
    s32 i;
    if(!srcModel) {
        return;
    }
    if(srcModel->clusterNum == 0) {
        return;
    }
    cluster = srcModel->cluster;
    if(cluster->adjusted) {
        return;
    }
    cluster->adjusted = 1;
    for(i=0; i<srcModel->clusterNum; i++, cluster++) {
        char *name = cluster->targetName;
        cluster->target = SearchObjectSetName(model, name);
    }
}

static void FileLoad(void *data)
{
    fileptr = data;
    memcpy(&head, fileptr, sizeof(HSF_HEADER));
    memset(&Model, 0, sizeof(HSF_DATA));
    NSymIndex = (void **)((u32)fileptr+head.symbol.ofs);
    StringTable = (char *)((u32)fileptr+head.string.ofs);
    ClusterTop = (HSF_CLUSTER *)((u32)fileptr+head.cluster.ofs);
    AttributeTop = (HSF_ATTRIBUTE *)((u32)fileptr+head.attribute.ofs);
    MaterialTop = (HSF_MATERIAL *)((u32)fileptr+head.material.ofs);
}

static HSF_DATA *SetHsfModel(void)
{
    HSF_DATA *data = fileptr;
    data->scene = Model.scene;
    data->sceneNum = Model.sceneNum;
    data->attribute = Model.attribute;
    data->attributeNum = Model.attributeNum;
    data->bitmap = Model.bitmap;
    data->bitmapNum = Model.bitmapNum;
    data->cenv = Model.cenv;
    data->cenvNum = Model.cenvNum;
    data->skeleton = Model.skeleton;
    data->skeletonNum = Model.skeletonNum;
    data->face = Model.face;
    data->faceNum = Model.faceNum;
    data->material = Model.material;
    data->materialNum = Model.materialNum;
    data->motion = Model.motion;
    data->motionNum = Model.motionNum;
    data->normal = Model.normal;
    data->normalNum = Model.normalNum;
    data->root = Model.root;
    data->objectNum = Model.objectNum;
    data->object = objtop;
    data->matrix = Model.matrix;
    data->matrixNum = Model.matrixNum;
    data->palette = Model.palette;
    data->paletteNum = Model.paletteNum;
    data->st = Model.st;
    data->stNum = Model.stNum;
    data->vertex = Model.vertex;
    data->vertexNum = Model.vertexNum;
    data->cenv = Model.cenv;
    data->cenvNum = Model.cenvNum;
    data->cluster = Model.cluster;
    data->clusterNum = Model.clusterNum;
    data->part = Model.part;
    data->partNum = Model.partNum;
    data->shape = Model.shape;
    data->shapeNum = Model.shapeNum;
    data->mapAttr = Model.mapAttr;
    data->mapAttrNum = Model.mapAttrNum;
    return data;
}

char *SetName(u32 *strOfs)
{
    char *ret = GetString(strOfs);
    return ret;
}

static inline char *SetMotionName(u16 *strOfs)
{
    char *ret = GetMotionString(strOfs);
    return ret;
}

static void MaterialLoad(void)
{
    s32 i;
    s32 j;
    if(head.material.num) {
        HSF_MATERIAL *fileMat = (HSF_MATERIAL *)((u32)fileptr+head.material.ofs);
        HSF_MATERIAL *currMat;
        HSF_MATERIAL *newMat;
        for(i=0; i<head.material.num; i++) {
            currMat = &fileMat[i];
        }
        newMat = fileMat;
        Model.material = newMat;
        Model.materialNum = head.material.num;
        fileMat = (HSF_MATERIAL *)((u32)fileptr+head.material.ofs);
        for(i=0; i<head.material.num; i++, newMat++) {
            currMat = &fileMat[i];
            newMat->name = SetName((u32 *)&currMat->name);
            newMat->pass = currMat->pass;
            newMat->vtxMode = currMat->vtxMode;
            newMat->litColor[0] = currMat->litColor[0];
            newMat->litColor[1] = currMat->litColor[1];
            newMat->litColor[2] = currMat->litColor[2];
            newMat->color[0] = currMat->color[0];
            newMat->color[1] = currMat->color[1];
            newMat->color[2] = currMat->color[2];
            newMat->shadowColor[0] = currMat->shadowColor[0];
            newMat->shadowColor[1] = currMat->shadowColor[1];
            newMat->shadowColor[2] = currMat->shadowColor[2];
            newMat->hiliteScale = currMat->hiliteScale;
            newMat->unk18 = currMat->unk18;
            newMat->invAlpha = currMat->invAlpha;
            newMat->unk20[0] = currMat->unk20[0];
            newMat->unk20[1] = currMat->unk20[1];
            newMat->refAlpha = currMat->refAlpha;
            newMat->unk2C = currMat->unk2C;
            newMat->attrNum = currMat->attrNum;
            newMat->attr = (s32 *)(NSymIndex+((u32)currMat->attr));
            rgba[i].r = newMat->litColor[0];
            rgba[i].g = newMat->litColor[1];
            rgba[i].b = newMat->litColor[2];
            rgba[i].a = 255;
            for(j=0; j<newMat->attrNum; j++) {
                newMat->attr[j] = newMat->attr[j];
            }
        }
    }
}

static void AttributeLoad(void)
{
    HSF_ATTRIBUTE *fileAttr;
    HSF_ATTRIBUTE *newAttr;
    HSF_ATTRIBUTE *tempAttr;
    s32 i;
    if(head.attribute.num) {
        tempAttr = fileAttr = (HSF_ATTRIBUTE *)((u32)fileptr+head.attribute.ofs);
        newAttr = tempAttr;
        Model.attribute = newAttr;
        Model.attributeNum = head.attribute.num;
        for(i=0; i<head.attribute.num; i++, newAttr++) {
            if((u32)fileAttr[i].name != -1) {
                newAttr->name = SetName((u32 *)&fileAttr[i].name);
            } else {
                newAttr->name = NULL;
            }
            newAttr->bitmap = SearchBitmapPtr((s32)fileAttr[i].bitmap);
        }
    }
}

static void SceneLoad(void)
{
    HSF_SCENE *fileScene;
    HSF_SCENE *newScene;
    if(head.scene.num) {
        fileScene = (HSF_SCENE *)((u32)fileptr+head.scene.ofs);
        newScene = fileScene;
        newScene->fogEnd = fileScene->fogEnd;
        newScene->fogStart = fileScene->fogStart;
        Model.scene = newScene;
        Model.sceneNum = head.scene.num;
    }
}

static void ColorLoad(void)
{
    s32 i;
    HSF_BUFFER *fileColor;
    HSF_BUFFER *newColor;
    void *data;
    u32 colorOfs;
    HSF_BUFFER *tempColor;
    
    if(head.color.num) {
        tempColor = fileColor = (HSF_BUFFER *)((u32)fileptr+head.color.ofs);
        data = &fileColor[head.color.num];
        for(i=0; i<head.color.num; i++, fileColor++);
        newColor = tempColor;
        Model.color = newColor;
        Model.colorNum = head.color.num;
        fileColor = (HSF_BUFFER *)((u32)fileptr+head.color.ofs);
        data = &fileColor[head.color.num];
        for(i=0; i<head.color.num; i++, newColor++, fileColor++) {
            colorOfs = (u32)fileColor->data;
            newColor->name = SetName((u32 *)&fileColor->name);
            newColor->data = (void *)((u32)data+colorOfs);
        }
    }
}

static void VertexLoad(void)
{
    s32 i, j;
    HSF_BUFFER *fileVertex;
    HSF_BUFFER *newVertex;
    void *data;
    HuVecF *dataPtr;
    u32 dataOfs;
    
    if(head.vertex.num) {
        vtxtop = fileVertex = (HSF_BUFFER *)((u32)fileptr+head.vertex.ofs);
        data = (void *)&fileVertex[head.vertex.num];
        for(i=0; i<head.vertex.num; i++, fileVertex++) {
            for(j=0; j<(u32)fileVertex->count; j++) {
                dataPtr = (HuVecF *)(((u32)data)+((u32)fileVertex->data)+(j*sizeof(HuVecF)));
            }
        }
        newVertex = vtxtop;
        Model.vertex = newVertex;
        Model.vertexNum = head.vertex.num;
        fileVertex = (HSF_BUFFER *)((u32)fileptr+head.vertex.ofs);
        VertexDataTop = data = (void *)&fileVertex[head.vertex.num];
        for(i=0; i<head.vertex.num; i++, newVertex++, fileVertex++) {
            dataOfs = (u32)fileVertex->data;
            newVertex->count = fileVertex->count;
            newVertex->name = SetName((u32 *)&fileVertex->name);
            newVertex->data = (void *)((u32)data+dataOfs);
            for(j=0; j<newVertex->count; j++) {
                dataPtr = (HuVecF *)(((u32)data)+dataOfs+(j*sizeof(HuVecF)));
                HuCopyVecF(&((HuVecF *)newVertex->data)[j], dataPtr);
            }
        }
    }
}

static void NormalLoad(void)
{
    s32 i, j;
    u32 dataOfs;
    HSF_BUFFER *fileNormal;
    HSF_BUFFER *newNormal;
    HSF_BUFFER *tempNormal;
    void *data;
    
    
    if(head.normal.num) {
        s32 cenv_count = head.cenv.num;
        tempNormal = fileNormal = (HSF_BUFFER *)((u32)fileptr+head.normal.ofs);
        data = (void *)&fileNormal[head.normal.num];
        newNormal = tempNormal;
        Model.normal = newNormal;
        Model.normalNum = head.normal.num;
        fileNormal = (HSF_BUFFER *)((u32)fileptr+head.normal.ofs);
        NormalDataTop = data = (void *)&fileNormal[head.normal.num];
        for(i=0; i<head.normal.num; i++, newNormal++, fileNormal++) {
            dataOfs = (u32)fileNormal->data;
            newNormal->count = fileNormal->count;
            newNormal->name = SetName((u32 *)&fileNormal->name);
            newNormal->data = (void *)((u32)data+dataOfs);
        }
    }
}

static void STLoad(void)
{
    s32 i, j;
    HSF_BUFFER *fileST;
    HSF_BUFFER *tempST;
    HSF_BUFFER *newST;
    void *data;
    HuVec2f *dataElem;
    u32 dataOfs;
    
    if(head.st.num) {
        tempST = fileST = (HSF_BUFFER *)((u32)fileptr+head.st.ofs);
        data = (void *)&fileST[head.st.num];
        for(i=0; i<head.st.num; i++, fileST++) {
            for(j=0; j<(u32)fileST->count; j++) {
                dataElem = (HuVec2f *)(((u32)data)+((u32)fileST->data)+(j*sizeof(HuVec2f)));
            }
        }
        newST = tempST;
        Model.st = newST;
        Model.stNum = head.st.num;
        fileST = (HSF_BUFFER *)((u32)fileptr+head.st.ofs);
        data = (void *)&fileST[head.st.num];
        for(i=0; i<head.st.num; i++, newST++, fileST++) {
            dataOfs = (u32)fileST->data;
            newST->count = fileST->count;
            newST->name = SetName((u32 *)&fileST->name);
            newST->data = (void *)((u32)data+dataOfs);
            for(j=0; j<newST->count; j++) {
                dataElem = (HuVec2f *)(((u32)data)+dataOfs+(j*sizeof(HuVec2f)));
                HuCopyVec2F(&((HuVec2f *)newST->data)[j], dataElem);
            }
        }
    }
}

static void FaceLoad(void)
{
    HSF_BUFFER *fileFace;
    HSF_BUFFER *newFace;
    HSF_BUFFER *tempFace;
    u32 dataOfs;
    HSF_FACE *data;
    HSF_FACE *fileFaceStrip;
    HSF_FACE *newFaceStrip;
    u8 *strip;
    s32 i;
    s32 j;
    
    if(head.face.num) {
        tempFace = fileFace = (HSF_BUFFER *)((u32)fileptr+head.face.ofs);
        data = (HSF_FACE *)&fileFace[head.face.num];
        newFace = tempFace;
        Model.face = newFace;
        Model.faceNum = head.face.num;
        fileFace = (HSF_BUFFER *)((u32)fileptr+head.face.ofs);
        data = (HSF_FACE *)&fileFace[head.face.num];
        for(i=0; i<head.face.num; i++, newFace++, fileFace++) {
            dataOfs = (u32)fileFace->data;
            newFace->name = SetName((u32 *)&fileFace->name);
            newFace->count = fileFace->count;
            newFace->data = (void *)((u32)data+dataOfs);
            strip = (u8 *)(&((HSF_FACE *)newFace->data)[newFace->count]);
        }
        newFace = tempFace;
        for(i=0; i<head.face.num; i++, newFace++) {
            fileFaceStrip = newFaceStrip = newFace->data;
            for(j=0; j<newFace->count; j++, newFaceStrip++, fileFaceStrip++) {
                if(fileFaceStrip->typeSrc == HSF_FACE_TRISTRIP) {
                    newFaceStrip->strip.data = (HSF_FACE_INDEX *)(strip+(u32)fileFaceStrip->strip.data*(sizeof(HSF_FACE_INDEX)));
                }
            }
        }
    }
}

static void DispObject(HSF_OBJECT *parent, HSF_OBJECT *object)
{
    u32 i;
    HSF_OBJECT *childObj;
    HSF_OBJECT *tempObj;
    struct {
        HSF_OBJECT *parent;
        HSF_BUFFER *shape;
        HSF_CLUSTER *cluster;
    } work;
    
    work.parent = parent;
    object->type = object->type;
    switch(object->type) {
        case HSF_OBJ_MESH:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            newObj->mesh.parent = parent;
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            newObj->type = HSF_OBJ_MESH;
            newObj->mesh.vertex = SearchVertexPtr((s32)data->vertex);
            newObj->mesh.normal = SearchNormalPtr((s32)data->normal);
            newObj->mesh.st = SearchStPtr((s32)data->st);
            newObj->mesh.color = SearchColorPtr((s32)data->color);
            newObj->mesh.face = SearchFacePtr((s32)data->face);
            newObj->mesh.shape = (HSF_BUFFER **)&NSymIndex[(u32)data->shape];
            for(i=0; i<newObj->mesh.shapeNum; i++) {
                work.shape = &vtxtop[(u32)newObj->mesh.shape[i]];
                newObj->mesh.shape[i] = work.shape;
            }
            newObj->mesh.cluster = (HSF_CLUSTER **)&NSymIndex[(u32)data->cluster];
            for(i=0; i<newObj->mesh.clusterNum; i++) {
                work.cluster = &ClusterTop[(u32)newObj->mesh.cluster[i]];
                newObj->mesh.cluster[i] = work.cluster;
            }
            newObj->mesh.cenv = SearchCenvPtr((s32)data->cenv);
            newObj->mesh.material = Model.material;
            if((s32)data->attribute >= 0) {
                newObj->mesh.attribute = Model.attribute;
            } else {
                newObj->mesh.attribute = NULL;
            }
            newObj->mesh.vtxtop = (void *)((u32)fileptr+(u32)data->vtxtop);
            newObj->mesh.normtop = (void *)((u32)fileptr+(u32)data->normtop);
            newObj->mesh.base.pos.x = data->base.pos.x;
            newObj->mesh.base.pos.y = data->base.pos.y;
            newObj->mesh.base.pos.z = data->base.pos.z;
            newObj->mesh.base.rot.x = data->base.rot.x;
            newObj->mesh.base.rot.y = data->base.rot.y;
            newObj->mesh.base.rot.z = data->base.rot.z;
            newObj->mesh.base.scale.x = data->base.scale.x;
            newObj->mesh.base.scale.y = data->base.scale.y;
            newObj->mesh.base.scale.z = data->base.scale.z;
            newObj->mesh.mesh.min.x = data->mesh.min.x;
            newObj->mesh.mesh.min.y = data->mesh.min.y;
            newObj->mesh.mesh.min.z = data->mesh.min.z;
            newObj->mesh.mesh.max.x = data->mesh.max.x;
            newObj->mesh.mesh.max.y = data->mesh.max.y;
            newObj->mesh.mesh.max.z = data->mesh.max.z;
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
            
        case HSF_OBJ_NULL1:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
        
        case HSF_OBJ_REPLICA:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            newObj->mesh.replica = &objtop[(u32)newObj->mesh.replica];
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;

        case HSF_OBJ_ROOT:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
        
        case HSF_OBJ_JOINT:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
        
        case HSF_OBJ_NULL2:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
        
        case HSF_OBJ_MAP:
        {
            HSF_MESH *data;
            HSF_OBJECT *newObj;
            data = &object->mesh;
            newObj = tempObj = object;
            newObj->mesh.parent = parent;
            newObj->mesh.childNum = data->childNum;
            newObj->mesh.child = (HSF_OBJECT **)&NSymIndex[(u32)data->child];
            for(i=0; i<newObj->mesh.childNum; i++) {
                childObj = &objtop[(u32)newObj->mesh.child[i]];
                newObj->mesh.child[i] = childObj;
            }
            if(Model.root == NULL) {
                Model.root = tempObj;
            }
            for(i=0; i<data->childNum; i++) {
                DispObject(newObj, newObj->mesh.child[i]);
            }
        }
        break;
        
        default:
            break;
    }
}

static inline void FixupObject(HSF_OBJECT *object)
{
    HSF_LIGHT *light;
    HSF_CAMERA *camera;
    
    s32 type = object->type;
    switch(type) {
        case HSF_OBJ_LIGHT:
        {
            light = &object->light;
            object->type = HSF_OBJ_LIGHT;
        }
        break;
        
        case HSF_OBJ_CAMERA:
        {
            camera = &object->camera;
            object->type = HSF_OBJ_CAMERA;
        }
        break;
        
        default:
            break;
            
    }
}

static void ObjectLoad(void)
{
    s32 i;
    HSF_OBJECT *object;
    HSF_OBJECT *newObj;

    if(head.object.num) {
        objtop = object = (HSF_OBJECT *)((u32)fileptr+head.object.ofs);
        for(i=0; i<head.object.num; i++, object++) {
            newObj = object;
            newObj->name = SetName((u32 *)&object->name);
        }
        object = objtop;
        for(i=0; i<head.object.num; i++, object++) {
            if((s32)object->mesh.parent == -1) {
                break;
            }
        }
        DispObject(NULL, object);
        Model.objectNum = head.object.num;
        object = objtop;
        for(i=0; i<head.object.num; i++, object++) {
            FixupObject(object);
        }
    }
}

static void CenvLoad(void)
{
    HSF_CENV_MULTI *multiFile;
    HSF_CENV_MULTI *multiNew;
    HSF_CENV_SINGLE *singleNew;
    HSF_CENV_SINGLE *singleFile;
    HSF_CENV_DUAL *dualFile;
    HSF_CENV_DUAL *dualNew;

    HSF_CENV *cenvNew;
    HSF_CENV *cenvFile;
    void *dataP;
    void *weightP;
    
    s32 j;
    s32 i;
    
    if(head.cenv.num) {
        cenvFile = (HSF_CENV *)((u32)fileptr+head.cenv.ofs);
        dataP = &cenvFile[head.cenv.num];
        weightP = dataP;
        cenvNew = cenvFile;
        Model.cenvNum = head.cenv.num;
        Model.cenv = cenvFile;
        for(i=0; i<head.cenv.num; i++) {
            cenvNew[i].singleData = (HSF_CENV_SINGLE *)((u32)cenvFile[i].singleData+(u32)dataP);
            cenvNew[i].dualData = (HSF_CENV_DUAL *)((u32)cenvFile[i].dualData+(u32)dataP);
            cenvNew[i].multiData = (HSF_CENV_MULTI *)((u32)cenvFile[i].multiData+(u32)dataP);
            cenvNew[i].singleCount = cenvFile[i].singleCount;
            cenvNew[i].dualCount = cenvFile[i].dualCount;
            cenvNew[i].multiCount = cenvFile[i].multiCount;
            cenvNew[i].copyCount = cenvFile[i].copyCount;
            cenvNew[i].vtxCount = cenvFile[i].vtxCount;
            weightP = (void *)((u32)weightP+(cenvNew[i].singleCount*sizeof(HSF_CENV_SINGLE)));
            weightP = (void *)((u32)weightP+(cenvNew[i].dualCount*sizeof(HSF_CENV_DUAL)));
            weightP = (void *)((u32)weightP+(cenvNew[i].multiCount*sizeof(HSF_CENV_MULTI)));
        }
        for(i=0; i<head.cenv.num; i++) {
            singleNew = singleFile = cenvNew[i].singleData;
            for(j=0; j<cenvNew[i].singleCount; j++) {
                singleNew[j].target = singleFile[j].target;
                singleNew[j].posNum = singleFile[j].posNum;
                singleNew[j].pos = singleFile[j].pos;
                singleNew[j].normalNum = singleFile[j].normalNum;
                singleNew[j].normal = singleFile[j].normal;
                
            }
            dualNew = dualFile = cenvNew[i].dualData;
            for(j=0; j<cenvNew[i].dualCount; j++) {
                dualNew[j].target1 = dualFile[j].target1;
                dualNew[j].target2 = dualFile[j].target2;
                dualNew[j].weightNum = dualFile[j].weightNum;
                dualNew[j].weight = (HSF_CENV_DUAL_WEIGHT *)((u32)weightP+(u32)dualFile[j].weight);
            }
            multiNew = multiFile = cenvNew[i].multiData;
            for(j=0; j<cenvNew[i].multiCount; j++) {
                multiNew[j].weightNum = multiFile[j].weightNum;
                multiNew[j].pos = multiFile[j].pos;
                multiNew[j].posNum = multiFile[j].posNum;
                multiNew[j].normal = multiFile[j].normal;
                multiNew[j].normalNum = multiFile[j].normalNum;
                multiNew[j].weight = (HSF_CENV_MULTI_WEIGHT *)((u32)weightP+(u32)multiFile[j].weight);
            }
            dualNew = dualFile = cenvNew[i].dualData;
            for(j=0; j<cenvNew[i].dualCount; j++) {
                HSF_CENV_DUAL_WEIGHT *discard = dualNew[j].weight;
            }
            multiNew = multiFile = cenvNew[i].multiData;
            for(j=0; j<cenvNew[i].multiCount; j++) {
                HSF_CENV_MULTI_WEIGHT *weight = multiNew[j].weight;
                s32 k;
                for(k=0; k<multiNew[j].weightNum; k++, weight++);
            }
        }
    }
}

static void SkeletonLoad(void)
{
    HSF_SKELETON *skeletonFile;
    HSF_SKELETON *skeletonNew;
    s32 i;
    
    if(head.skeleton.num) {
        skeletonNew = skeletonFile = (HSF_SKELETON *)((u32)fileptr+head.skeleton.ofs);
        Model.skeletonNum = head.skeleton.num;
        Model.skeleton = skeletonFile;
        for(i=0; i<head.skeleton.num; i++) {
            skeletonNew[i].name = SetName((u32 *)&skeletonFile[i].name);
            skeletonNew[i].transform.pos.x = skeletonFile[i].transform.pos.x;
            skeletonNew[i].transform.pos.y = skeletonFile[i].transform.pos.y;
            skeletonNew[i].transform.pos.z = skeletonFile[i].transform.pos.z;
            skeletonNew[i].transform.rot.x = skeletonFile[i].transform.rot.x;
            skeletonNew[i].transform.rot.y = skeletonFile[i].transform.rot.y;
            skeletonNew[i].transform.rot.z = skeletonFile[i].transform.rot.z;
            skeletonNew[i].transform.scale.x = skeletonFile[i].transform.scale.x;
            skeletonNew[i].transform.scale.y = skeletonFile[i].transform.scale.y;
            skeletonNew[i].transform.scale.z = skeletonFile[i].transform.scale.z;
        }
    }
}

static void PartLoad(void)
{
    HSF_PART *partFile;
    HSF_PART *partNew;
    
    u16 *data;
    s32 i, j;
    
    if(head.part.num) {
        partNew = partFile = (HSF_PART *)((u32)fileptr+head.part.ofs);
        Model.partNum = head.part.num;
        Model.part = partFile;
        data = (u16 *)&partFile[head.part.num];
        for(i=0; i<head.part.num; i++, partNew++) {
            partNew->name = SetName((u32 *)&partFile[i].name);
            partNew->num = partFile[i].num;
            partNew->vertex = &data[(u32)partFile[i].vertex];
            for(j=0; j<partNew->num; j++) {
                partNew->vertex[j] = partNew->vertex[j];
            }
        }
    }
}

static void ClusterLoad(void)
{
    HSF_CLUSTER *clusterFile;
    HSF_CLUSTER *clusterNew;
    
    s32 i, j;
    
    if(head.cluster.num) {
        clusterNew = clusterFile = (HSF_CLUSTER *)((u32)fileptr+head.cluster.ofs);
        Model.clusterNum = head.cluster.num;
        Model.cluster = clusterFile;
        for(i=0; i<head.cluster.num; i++) {
            HSF_BUFFER *vertex;
            u32 vertexSym;
            clusterNew[i].name[0] = SetName((u32 *)&clusterFile[i].name[0]);
            clusterNew[i].name[1] = SetName((u32 *)&clusterFile[i].name[1]);
            clusterNew[i].targetName = SetName((u32 *)&clusterFile[i].targetName);
            clusterNew[i].part = SearchPartPtr((s32)clusterFile[i].part);
            clusterNew[i].unk95 = clusterFile[i].unk95;
            clusterNew[i].type = clusterFile[i].type;
            clusterNew[i].vertexNum = clusterFile[i].vertexNum;
            vertexSym = (u32)clusterFile[i].vertex;
            clusterNew[i].vertex = (HSF_BUFFER **)&NSymIndex[vertexSym];
            for(j=0; j<clusterNew[i].vertexNum; j++) {
                vertex = SearchVertexPtr((s32)clusterNew[i].vertex[j]);
                clusterNew[i].vertex[j] = vertex;
            }
        }
    }
}

static void ShapeLoad(void)
{
    s32 i, j;
    HSF_SHAPE *shapeNew;
    HSF_SHAPE *shapeFile;

    if(head.shape.num) {
        shapeNew = shapeFile = (HSF_SHAPE *)((u32)fileptr+head.shape.ofs);
        Model.shapeNum = head.shape.num;
        Model.shape = shapeFile;
        for(i=0; i<Model.shapeNum; i++) {
            u32 vertexSym;
            HSF_BUFFER *vertex;

            shapeNew[i].name = SetName((u32 *)&shapeFile[i].name);
            shapeNew[i].num16[0] = shapeFile[i].num16[0];
            shapeNew[i].num16[1] = shapeFile[i].num16[1];
            vertexSym = (u32)shapeFile[i].vertex;
            shapeNew[i].vertex = (HSF_BUFFER **)&NSymIndex[vertexSym];
            for(j=0; j<shapeNew[i].num16[1]; j++) {
                vertex = &vtxtop[(u32)shapeNew[i].vertex[j]];
                shapeNew[i].vertex[j] = vertex;
            }
        }
    }
}

static void MapAttrLoad(void)
{
    s32 i;
    HSF_MAPATTR *mapAttrBase;
    HSF_MAPATTR *mapAttrFile;
    HSF_MAPATTR *mapAttrNew;
    u16 *data;
    
    if(head.mapAttr.num) {
        mapAttrFile = mapAttrBase = (HSF_MAPATTR *)((u32)fileptr+head.mapAttr.ofs);
        mapAttrNew = mapAttrBase;
        Model.mapAttrNum = head.mapAttr.num;
        Model.mapAttr = mapAttrBase;
        data = (u16 *)&mapAttrBase[head.mapAttr.num];
        for(i=0; i<head.mapAttr.num; i++, mapAttrFile++, mapAttrNew++) {
            mapAttrNew->data = &data[(u32)mapAttrFile->data];
        }
    }
}

static void BitmapLoad(void)
{
    HSF_BITMAP *bitmapFile;
    HSF_BITMAP *bitmapTemp;
    HSF_BITMAP *bitmapNew;
    HSF_PALETTE *palette;
    void *dataP;
    s32 i;
    
    if(head.bitmap.num) {
        bitmapTemp = bitmapFile = (HSF_BITMAP *)((u32)fileptr+head.bitmap.ofs);
        dataP = &bitmapFile[head.bitmap.num];
        for(i=0; i<head.bitmap.num; i++, bitmapFile++);
        bitmapNew = bitmapTemp;
        Model.bitmap = bitmapFile;
        Model.bitmapNum = head.bitmap.num;
        bitmapFile = (HSF_BITMAP *)((u32)fileptr+head.bitmap.ofs);
        dataP = &bitmapFile[head.bitmap.num];
        for(i=0; i<head.bitmap.num; i++, bitmapFile++, bitmapNew++) {
            bitmapNew->name = SetName((u32 *)&bitmapFile->name);
            bitmapNew->dataFmt = bitmapFile->dataFmt;
            bitmapNew->pixSize = bitmapFile->pixSize;
            bitmapNew->sizeX = bitmapFile->sizeX;
            bitmapNew->sizeY = bitmapFile->sizeY;
            bitmapNew->palSize = bitmapFile->palSize;
            palette = SearchPalettePtr((u32)bitmapFile->palData);
            if(palette) {
                bitmapNew->palData = palette->data;
            }
            bitmapNew->data = (void *)((u32)dataP+(u32)bitmapFile->data);
        }
    }
}

static void PaletteLoad(void)
{
    s32 i;
    s32 j;
    HSF_PALETTE *paletteFile;
    HSF_PALETTE *paletteTemp;
    HSF_PALETTE *paletteNew;
    
    void *dataBase;
    u16 *dataTemp;
    u16 *data;
    
    if(head.palette.num) {
        paletteTemp = paletteFile = (HSF_PALETTE *)((u32)fileptr+head.palette.ofs);
        dataBase = (u16 *)&paletteFile[head.palette.num];
        for(i=0; i<head.palette.num; i++, paletteFile++) {
            dataTemp = (u16 *)((u32)dataBase+(u32)paletteFile->data);
        }
        Model.palette = paletteTemp;
        Model.paletteNum = head.palette.num;
        paletteNew = paletteTemp;
        paletteFile = (HSF_PALETTE *)((u32)fileptr+head.palette.ofs);
        dataBase = (u16 *)&paletteFile[head.palette.num];
        for(i=0; i<head.palette.num; i++, paletteFile++, paletteNew++) {
            dataTemp = (u16 *)((u32)dataBase+(u32)paletteFile->data);
            data = dataTemp;
            paletteNew->name = SetName((u32 *)&paletteFile->name);
            paletteNew->data = data;
            paletteNew->palSize = paletteFile->palSize;
            for(j=0; j<paletteFile->palSize; j++) {
                data[j] = data[j];
            }
        }
    }
}

char *MakeObjectName(s8 *name)
{
    static char buf[768];
    s32 index, numSeparate;
    char *nameP;
    numSeparate = 0;
    index = 0;
    nameP = (char *)name;
    while(*nameP) {
        if(*nameP == '-') {
            name = (s8 *)nameP+1;
            break;
        }
        nameP++;
    }
    while(*name) {
        if(numSeparate != 0) {
            break;
        }
        if(*name == '_' && !isalpha(name[1])) {
            numSeparate++;
            break;
        }
        buf[index] = *name;
        name++;
        index++;
    }
    buf[index] = '\0';
    return buf;
}

s32 CmpObjectName(char *name1, char *name2)
{
    s32 temp = 0;
    return strcmp(name1, name2);
}

static inline char *MotionGetName(HSF_TRACK *track)
{
    char *ret;
    if(DicStringTable) {
        ret = &DicStringTable[track->target];
    } else {
        ret = GetMotionString(&track->target);
    }
    return ret;
}

static inline s32 FindObjectName(char *name)
{
    s32 i;
    HSF_OBJECT *object;
    
    object = objtop;
    for(i=0; i<head.object.num; i++, object++) {
        if(!CmpObjectName(object->name, name)) {
            return i;
        }
    }
    return -1;
}

static inline s32 FindClusterName(char *name)
{
    s32 i;
    HSF_CLUSTER *cluster;
    
    cluster = ClusterTop;
    for(i=0; i<head.cluster.num; i++, cluster++) {
        if(!strcmp(cluster->name[0], name)) {
            return i;
        }
    }
    return -1;
}

static inline s32 FindMotionClusterName(char *name)
{
    s32 i;
    HSF_CLUSTER *cluster;
    
    cluster = MotionModel->cluster;
    for(i=0; i<MotionModel->clusterNum; i++, cluster++) {
        if(!strcmp(cluster->name[0], name)) {
            return i;
        }
    }
    return -1;
}

static inline s32 FindAttributeName(char *name)
{
    s32 i;
    HSF_ATTRIBUTE *attribute;
    
    attribute = AttributeTop;
    for(i=0; i<head.attribute.num; i++, attribute++) {
        if(!attribute->name) {
            continue;
        }
        if(!strcmp(attribute->name, name)) {
            return i;
        }
    }
    return -1;
}

static inline s32 FindMotionAttributeName(char *name)
{
    s32 i;
    HSF_ATTRIBUTE *attribute;
    
    attribute = MotionModel->attribute;
    for(i=0; i<MotionModel->attributeNum; i++, attribute++) {
        if(!attribute->name) {
            continue;
        }
        if(!strcmp(attribute->name, name)) {
            return i;
        }
    }
    return -1;
}

static inline void MotionLoadTransform(HSF_TRACK *track, void *data)
{
    float *stepData;
    float *linearData;
    float *bezierData;
    HSF_TRACK *outTrack;
    char *name;
    s32 dataNum;
    outTrack = track;
    name = MotionGetName(track);
    if(objtop) {
        outTrack->target = FindObjectName(name);
    }
    dataNum = track->dataNum;
    switch(track->curveType) {
        case HSF_CURVE_STEP:
        {
            stepData = (float *)((u32)data+(u32)track->data);
            outTrack->data = stepData;
        }
        break;
        
        case HSF_CURVE_LINEAR:
        {
            linearData = (float *)((u32)data+(u32)track->data);
            outTrack->data = linearData;
        }
        break;
        
        case HSF_CURVE_BEZIER:
        {
            bezierData = (float *)((u32)data+(u32)track->data);
            outTrack->data = bezierData;
        }
        break;
        
        case HSF_CURVE_CONST:
            break;
    }
}

static inline void MotionLoadCluster(HSF_TRACK *track, void *data)
{
    s32 dataNum;
    float *stepData;
    float *linearData;
    float *bezierData;
    HSF_TRACK *outTrack;
    char *name;
    
    outTrack = track;
    name = SetMotionName(&track->target);
    if(!MotionOnly) {
        outTrack->cluster = FindClusterName(name);
    } else {
        outTrack->cluster = FindMotionClusterName(name);
    }
    dataNum = track->dataNum;
    (void)outTrack;
    switch(track->curveType) {
        case HSF_CURVE_STEP:
        {
            stepData = (float *)((u32)data+(u32)track->data);
            outTrack->data = stepData;
        }
        break;
        
        case HSF_CURVE_LINEAR:
        {
            linearData = (float *)((u32)data+(u32)track->data);
            outTrack->data = linearData;
        }
        break;
        
        case HSF_CURVE_BEZIER:
        {
            bezierData = (float *)((u32)data+(u32)track->data);
            outTrack->data = bezierData;
        }
        break;
        
        case HSF_CURVE_CONST:
            break;
    }
}

static inline void MotionLoadClusterWeight(HSF_TRACK *track, void *data)
{
    s32 dataNum;
    float *stepData;
    float *linearData;
    float *bezierData;
    HSF_TRACK *outTrack;
    char *name;
    
    outTrack = track;
    name = SetMotionName(&track->target);
    if(!MotionOnly) {
        outTrack->cluster = FindClusterName(name);
    } else {
        outTrack->cluster = FindMotionClusterName(name);
    }
    dataNum = track->dataNum;
    (void)outTrack;
    switch(track->curveType) {
        case HSF_CURVE_STEP:
        {
            stepData = (float *)((u32)data+(u32)track->data);
            outTrack->data = stepData;
        }
        break;
        
        case HSF_CURVE_LINEAR:
        {
            linearData = (float *)((u32)data+(u32)track->data);
            outTrack->data = linearData;
        }
        break;
        
        case HSF_CURVE_BEZIER:
        {
            bezierData = (float *)((u32)data+(u32)track->data);
            outTrack->data = bezierData;
        }
        break;
        
        case HSF_CURVE_CONST:
            break;
    }
}

static inline void MotionLoadMaterial(HSF_TRACK *track, void *data)
{
    float *stepData;
    float *linearData;
    float *bezierData;
    s32 dataNum;
    HSF_TRACK *outTrack;
    outTrack = track;
    dataNum = track->dataNum;
    switch(track->curveType) {
        case HSF_CURVE_STEP:
        {
            stepData = (float *)((u32)data+(u32)track->data);
            outTrack->data = stepData;
        }
        break;
        
        case HSF_CURVE_LINEAR:
        {
            linearData = (float *)((u32)data+(u32)track->data);
            outTrack->data = linearData;
        }
        break;
        
        case HSF_CURVE_BEZIER:
        {
            bezierData = (float *)((u32)data+(u32)track->data);
            outTrack->data = bezierData;
        }
        break;
        
        case HSF_CURVE_CONST:
            break;
    }
}

static inline void MotionLoadAttribute(HSF_TRACK *track, void *data)
{
    HSF_BITMAP_KEY *fileBitmap;
    HSF_BITMAP_KEY *newBitmap;
    s32 i;
    float *stepData;
    float *linearData;
    float *bezierData;
    HSF_TRACK *outTrack;
    char *name;
    outTrack = track;
    if(outTrack->cluster != -1) {
        name = SetMotionName(&track->target);
        if(!MotionOnly) {
            outTrack->attrIdx = FindAttributeName(name);
        } else {
            outTrack->attrIdx = FindMotionAttributeName(name);
        }
    }
    
    switch(track->curveType) {
        case HSF_CURVE_STEP:
        {
            stepData = (float *)((u32)data+(u32)track->data);
            outTrack->data = stepData;
        }
        break;
        
        case HSF_CURVE_LINEAR:
        {
            linearData = (float *)((u32)data+(u32)track->data);
            outTrack->data = linearData;
        }
        break;
        
        case HSF_CURVE_BEZIER:
        {
            bezierData = (float *)((u32)data+(u32)track->data);
            outTrack->data = bezierData;
        }
        break;
        
        case HSF_CURVE_BITMAP:
        {
            newBitmap = fileBitmap = (HSF_BITMAP_KEY *)((u32)data+(u32)track->data);
            outTrack->data = fileBitmap;
            for(i=0; i<outTrack->numKeyframes; i++, fileBitmap++, newBitmap++) {
                newBitmap->data = SearchBitmapPtr((s32)fileBitmap->data);
            }
        }
        break;
        case HSF_CURVE_CONST:
            break;
    }
}

static void MotionLoad(void)
{
    HSF_MOTION *fileMotion;
    HSF_MOTION *tempMotion;
    HSF_MOTION *newMotion;
    HSF_TRACK *trackStart;
    void *trackData;
    s32 i;
    
    MotionOnly = FALSE;
    MotionModel = NULL;
    if(head.motion.num) {
        tempMotion = fileMotion = (HSF_MOTION *)((u32)fileptr+head.motion.ofs);
        newMotion = tempMotion;
        Model.motion = newMotion;
        Model.motionNum = fileMotion->numTracks;
        trackStart = (HSF_TRACK *)&fileMotion[head.motion.num];
        trackData = &trackStart[fileMotion->numTracks];
        newMotion->track = trackStart;
        for(i=0; i<(s32)fileMotion->numTracks; i++) {
            switch(trackStart[i].type) {
                case HSF_TRACK_TRANSFORM:
                case HSF_TRACK_MORPH:
                    MotionLoadTransform(&trackStart[i], trackData);
                    break;
                    
                case HSF_TRACK_CLUSTER:
                    MotionLoadCluster(&trackStart[i], trackData);
                    break;
                    
                case HSF_TRACK_CLUSTER_WEIGHT:
                    MotionLoadClusterWeight(&trackStart[i], trackData);
                    break;
                    
                case HSF_TRACK_MATERIAL:
                    MotionLoadMaterial(&trackStart[i], trackData);
                    break;
                    
                case HSF_TRACK_ATTRIBUTE:
                    MotionLoadAttribute(&trackStart[i], trackData);
                    break;
                    
                default:
                    break;
            }
        }
    }
    //HACK: Bump register of i to r31
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
    (void)i;
}

static void MatrixLoad(void)
{
    HSF_MATRIX *matrixFile;
    
    if(head.matrix.num) {
        matrixFile = (HSF_MATRIX *)((u32)fileptr+head.matrix.ofs);
        matrixFile->data = (Mtx *)((u32)fileptr+head.matrix.ofs+sizeof(HSF_MATRIX));
        Model.matrix = matrixFile;
        Model.matrixNum = head.matrix.num;
    }
}

static s32 SearchObjectSetName(HSF_DATA *data, char *name)
{
    HSF_OBJECT *object = data->object;
    s32 i;
    for(i=0; i<data->objectNum; i++, object++) {
        if(!CmpObjectName(object->name, name)) {
            return i;
        }
    }
    OSReport("Search Object Error %s\n", name);
    return -1;
}

static HSF_BUFFER *SearchVertexPtr(s32 id)
{
    HSF_BUFFER *vertex; 
    if(id == -1) {
        return NULL;
    }
    vertex = (HSF_BUFFER *)((u32)fileptr+head.vertex.ofs);
    vertex += id;
    return vertex;
}

static HSF_BUFFER *SearchNormalPtr(s32 id)
{
    HSF_BUFFER *normal; 
    if(id == -1) {
        return NULL;
    }
    normal = (HSF_BUFFER *)((u32)fileptr+head.normal.ofs);
    normal += id;
    return normal;
}

static HSF_BUFFER *SearchStPtr(s32 id)
{
    HSF_BUFFER *st; 
    if(id == -1) {
        return NULL;
    }
    st = (HSF_BUFFER *)((u32)fileptr+head.st.ofs);
    st += id;
    return st;
}

static HSF_BUFFER *SearchColorPtr(s32 id)
{
    HSF_BUFFER *color; 
    if(id == -1) {
        return NULL;
    }
    color = (HSF_BUFFER *)((u32)fileptr+head.color.ofs);
    color += id;
    return color;
}

static HSF_BUFFER *SearchFacePtr(s32 id)
{
    HSF_BUFFER *face; 
    if(id == -1) {
        return NULL;
    }
    face = (HSF_BUFFER *)((u32)fileptr+head.face.ofs);
    face += id;
    return face;
}

static HSF_CENV *SearchCenvPtr(s32 id)
{
    HSF_CENV *cenv; 
    if(id == -1) {
        return NULL;
    }
    cenv = (HSF_CENV *)((u32)fileptr+head.cenv.ofs);
    cenv += id;
    return cenv;
}

static HSF_PART *SearchPartPtr(s32 id)
{
    HSF_PART *part; 
    if(id == -1) {
        return NULL;
    }
    part = (HSF_PART *)((u32)fileptr+head.part.ofs);
    part += id;
    return part;
}

static HSF_PALETTE *SearchPalettePtr(s32 id)
{
    HSF_PALETTE *palette; 
    if(id == -1) {
        return NULL;
    }
    palette = Model.palette;
    palette += id;
    return palette;
}

static HSF_BITMAP *SearchBitmapPtr(s32 id)
{
    HSF_BITMAP *bitmap; 
    if(id == -1) {
        return NULL;
    }
    bitmap = (HSF_BITMAP *)((u32)fileptr+head.bitmap.ofs);
    bitmap += id;
    return bitmap;
}

static char *GetString(u32 *strOfs)
{
    char *ret = &StringTable[*strOfs];
    return ret;
}

static char *GetMotionString(u16 *strOfs)
{
    char *ret = &StringTable[*strOfs];
    return ret;
}