#include "game/ShapeExec.h"
#include "game/EnvelopeExec.h"

static void SetShapeMain(HSF_OBJECT *obj) {
    HSF_BUFFER *shapeNext;
    HSF_BUFFER *shape;
    float totalWeight;
    float t;
    s32 shapeIdx;
    s32 j;
    s32 shapeIdxNext;
    s32 i;

    if (obj->mesh.shapeType == 2) {
        totalWeight = 0.0f;
        for (i = 0; i < obj->mesh.shapeNum; i++) {
            totalWeight += obj->mesh.mesh.morphWeight[i];
        }
        shape = *obj->mesh.shape;
        for (i = 0; i < shape->count; i++) {
            Vertextop[i].x = ((Vec*) shape->data)[i].x;
            Vertextop[i].y = ((Vec*) shape->data)[i].y;
            Vertextop[i].z = ((Vec*) shape->data)[i].z;
        }
        for (i = 0; i < obj->mesh.shapeNum; i++) {
            shape = obj->mesh.shape[i];
            t = obj->mesh.mesh.morphWeight[i];
            if (t < 0.0f) {
                t = 0.0f;
            } else if (totalWeight > 1.0f) {
                t /= totalWeight;
            }
            for (j = 0; j < shape->count; j++) {
                Vertextop[j].x += t * (((Vec*) shape->data)[j].x - Vertextop[j].x);
                Vertextop[j].y += t * (((Vec*) shape->data)[j].y - Vertextop[j].y);
                Vertextop[j].z += t * (((Vec*) shape->data)[j].z - Vertextop[j].z);
            }
        }
    } else {
        shapeIdx = obj->mesh.mesh.baseMorph;
        shapeIdxNext = shapeIdx + 1;
        if (shapeIdxNext >= obj->mesh.shapeNum) {
            shapeIdxNext = shapeIdx;
        }
        t = obj->mesh.mesh.baseMorph - shapeIdx;
        shape = obj->mesh.shape[shapeIdx];
        shapeNext = obj->mesh.shape[shapeIdxNext];
        for (i = 0; i < shape->count; i++) {
            Vertextop[i].x = ((Vec*) shape->data)[i].x + t * (((Vec*) shapeNext->data)[i].x - ((Vec*) shape->data)[i].x);
            Vertextop[i].y = ((Vec*) shape->data)[i].y + t * (((Vec*) shapeNext->data)[i].y - ((Vec*) shape->data)[i].y);
            Vertextop[i].z = ((Vec*) shape->data)[i].z + t * (((Vec*) shapeNext->data)[i].z - ((Vec*) shape->data)[i].z);
        }
    }
}

void ShapeProc(HSF_DATA *hsf) {
    HSF_OBJECT *obj;
    s32 i;

    obj = hsf->object;
    for (i = 0; i < hsf->objectNum; i++, obj++) {
        if (obj->type == HSF_OBJ_MESH && obj->mesh.shapeNum != 0) {
            Vertextop = obj->mesh.vertex->data;
            SetShapeMain(obj);
            DCStoreRange(Vertextop, obj->mesh.vertex->count * sizeof(Vec));
            obj->mesh.writeNum++;
        }
    }
}
