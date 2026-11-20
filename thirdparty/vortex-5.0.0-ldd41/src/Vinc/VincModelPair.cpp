// Vortex 5.0.0 build 800 "vortexldd41" - VincRequest, VincModelPair and the model pair container
// (restored; Mac 557c 0x4c248d-0x4c2715).
#include "Vinc/VincIntersect.h"
#include "Me/MeMemory.h"

#include "VincInternal.h"

namespace {
VincRequest s_defaultRequest;          // Mac 0x77ce40, Win 0xaf2028
}

extern "C" {

VincRequest* VincDefaultRequestPtr = 0; // Mac 0x77ce74, Win 0xaf2060

// Mac 0x4c248d, Win 0x754ec0 (name inferred)
void MEAPI VincRequestInit(VincRequest* r)
{
    r->contactMaxCount = 4;
    r->faceNormalsFirst = 0;
    r->m08 = 0.1;
    r->m10 = 0.1;
    r->contactGroupMaxCount = 4;
    r->m1c = 0.5;
    r->m28 = 0;
    r->m24 = 0;
    r->m2c = 0.001;
}

// Mac 0x4c24eb (inlined on Win: 0x754f00) (name inferred)
void MEAPI VincRequestInitDefault(void)
{
    VincDefaultRequestPtr = &s_defaultRequest;
    VincRequestInit(&s_defaultRequest);
}

// Mac 0x4c2512, Win 0x754f70 (name inferred)
void MEAPI VincSetDefaultRequestPtr(VincRequest* r)
{
    VincDefaultRequestPtr = r;
}

// Mac 0x4c2526 (name inferred)
void MEAPI VincRequestCopy(const VincRequest* src, VincRequest* dst)
{
    dst->contactMaxCount = src->contactMaxCount;
    dst->faceNormalsFirst = src->faceNormalsFirst;
    dst->m10 = src->m10;
    dst->m08 = src->m08;
    dst->contactGroupMaxCount = src->contactGroupMaxCount;
    dst->m1c = src->m1c;
    dst->m28 = src->m28;
    dst->m24 = src->m24;
    dst->m2c = src->m2c;
}

// Mac 0x4c2575 (name inferred). The pair key is type2 * 13 + type1, or 0xaa when a Convex (9) or
// type 12 geometry is involved.
void MEAPI VincModelPairReset(VincModelPair* p, VincModelID m1, VincModelID m2)
{
    p->model1 = m1;
    p->model2 = m2;
    p->request = VincDefaultRequestPtr;
    p->m0c = 0;
    p->intersectFn = 0;
    p->sweptIntersectFn = 0;
    p->safeTimeFn = 0;
    p->goodbyeFn = 0;
    p->pairData = 0;
    p->m24 = 0;
    p->flags = 0;
    if (m1 == 0 || m2 == 0) {
        p->pairKey = 0;
    } else {
        MeI16 t1 = m1->geometry->type;
        if (t1 == 0xc || t1 == 9) {
            p->pairKey = 0xaa;
        } else {
            MeI16 t2 = m2->geometry->type;
            if (t2 == 0xc || t2 == 9)
                p->pairKey = 0xaa;
            else
                p->pairKey = t2 * 0xd + t1;
        }
    }
}

// Mac 0x4c261d (name inferred)
void MEAPI VincModelPairGetModels(VincModelPair* p, VincModelID* m1, VincModelID* m2)
{
    *m1 = p->model1;
    *m2 = p->model2;
}

// Mac 0x4c2634 (name inferred)
VincRequest* MEAPI VincModelPairGetRequestPtr(VincModelPair* p)
{
    return p->request;
}

// Mac 0x4c263f (name inferred)
void MEAPI VincModelPairSetFlag0(VincModelPair* p, int on)
{
    if (on == 0)
        p->flags &= ~1u;
    else
        p->flags |= 1u;
}

// Mac 0x4c2657 (name inferred)
unsigned int MEAPI VincModelPairGetFlag0(VincModelPair* p)
{
    return p->flags & 1u;
}

// Mac 0x4c2665 (name inferred)
void MEAPI VincModelPairContainerReset(VincModelPairContainer* c)
{
    c->goodbyeFirst = 0;
    c->goodbyeEnd = 0;
    c->stayingEnd = c->size;
    c->stayingFirst = c->size;
    c->helloFirst = c->middle;
    c->helloEnd = c->middle;
}

// Mac 0x4c268d (name inferred)
void MEAPI VincModelPairContainerInit(VincModelPairContainer* c, VincModelPair** array, int size)
{
    c->array = array;
    c->size = size;
    c->goodbyeFirst = 0;
    c->middle = size / 2;
    VincModelPairContainerReset(c);
}

// Mac 0x4c26be (name inferred)
VincModelPairContainer* MEAPI VincModelPairContainerCreate(int size)
{
    VincModelPairContainer* c = (VincModelPairContainer*)MeMemoryAPI.create(sizeof(VincModelPairContainer));
    VincModelPair** array = (VincModelPair**)MeMemoryAPI.create(size * sizeof(VincModelPair*));
    VincModelPairContainerInit(c, array, size);
    return c;
}

// Mac 0x4c2715 (name inferred)
void MEAPI VincModelPairContainerDestroy(VincModelPairContainer* c)
{
    MeMemoryAPI.destroy(c->array);
    MeMemoryAPI.destroy(c);
}

} // extern "C"
