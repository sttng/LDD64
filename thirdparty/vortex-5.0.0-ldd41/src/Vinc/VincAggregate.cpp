// Vortex 5.0.0 build 800 "vortexldd41" - aggregate (composite) models: a model whose geometry is
// the aggregate geometry of the space item (V3.8, Mac 0x54b028...) holding element models
// (restored; Mac 557c 0x4bfbdd-0x4bfd82, Win 0x7746a0-0x7747e0).
// Used by Vx::VxCompositeCollisionGeometry (Mac 0x43b67c ... 0x43bec8).
#include "Vinc/VincIntersect.h"
#include "Me/VinPool.h"

#include "VincInternal.h"

extern "C" {

// Mac 0x4bfbdd, Win 0x7747c0 (name inferred)
int MEAPI VincModelIsAggregate(VincModelID m)
{
    return m->geometry->type == VincAggregateGetTypeId();
}

// Mac 0x4bfc00, Win 0x774730 (name inferred)
VincModelID MEAPI VincAggregateGetElementModel(VincModelID m, int i)
{
    return VincAggregateGetElement(m->geometry, i);
}

// Mac 0x4bfc1c (name inferred): the setVelocityPtrsFn of aggregate models
void MEAPI VincAggregateSetVelocityPtrs(VincModelID m, MeReal* v, MeReal* w)
{
    m->linearVelocity = v;
    m->angularVelocity = w;
    for (int i = 0; i < VincAggregateGetElementCount(m->geometry); ++i) {
        VincModelID e = VincAggregateGetElement(m->geometry, i);
        e->setVelocityPtrsFn(e, v, w);
    }
}

// Mac 0x4bfc71, Win 0x7746e0 (name inferred)
void MEAPI VincAggregateModelUpdate(VincModelID m)
{
    VincAggregateGeometryUpdate(m->geometry);
}

// Mac 0x4bfc86, Win 0x774820 (name inferred)
int MEAPI VincAggregateRemoveElement(VincModelID m, int i)
{
    VincModelID e = VincAggregateGetElementModel(m, i);
    int r = VincAggregateRemoveElementGeometry(m->geometry, i);
    e->parent = 0;
    e->bodyTransform = 0;
    e->setVelocityPtrsFn(e, 0, 0);
    return r;
}

// Mac 0x4bfcea, Win 0x7746a0 (name inferred)
int MEAPI VincAggregateAddElement(VincModelID m, VincModelID element, MeReal* relTM)
{
    element->parent = m;
    element->bodyTransform = m->transform;
    int r = VincAggregateAddElementGeometry(m->geometry, element, relTM);
    if (r != 0)
        element->setVelocityPtrsFn(element, m->linearVelocity, m->angularVelocity);
    return r;
}

// Mac 0x4bfd44, Win 0x774650 (name inferred)
void MEAPI VincAggregateModelDestroy(VincModelID m)
{
    VincAggregateGeometryDestroy(m->geometry);
    VinPoolPutStruct(&VincTheFramework->modelPool, m);
}

// Win 0x774680 (name inferred)
void MEAPI VincAggregateModelDestroyToPool(VincModelID m, VinPool* pool)
{
    VincAggregateGeometryDestroy(m->geometry);
    VinPoolPutStruct(pool, m);
}

// Win 0x7747e0 (name inferred)
VincModelID MEAPI VincModelGetParent(VincModelID m)
{
    return m->parent;
}

// Mac 0x4bfd82 (name inferred)
VincModelID MEAPI VincAggregateModelCreate(int maxElements)
{
    VincModelID m = VincModelCreate(VincAggregateGeometryCreate(maxElements));
    m->updateTransformFn = 0;
    m->setVelocityPtrsFn = VincAggregateSetVelocityPtrs;
    return m;
}

// ---- Win-only model wrappers (dead-stripped on the Mac) ----

// Win 0x774700 (name inferred): the element transforms after the aggregate model moved
void MEAPI VincAggregateModelUpdateTransforms(VincModelID m)
{
    if (m->relTransform != 0)
        VincModelComposeRelativeTransform(m);
    VincAggregateUpdateElementTransforms(m->geometry, m->transform);
}

// Win 0x774750 (name inferred): the relative transform of element i
MeReal* MEAPI VincAggregateGetElementTransform(VincModelID m, int i)
{
    VincModelID e;
    MeReal* relTM;
    VincAggregateGetElementAndTransform(m->geometry, i, &e, &relTM);
    return relTM;
}

// Win 0x774780 (name inferred)
int MEAPI VincAggregateModelGetElementCount(VincModelID m)
{
    return VincAggregateGetElementCount(m->geometry);
}

// Win 0x7747a0 (name inferred)
int MEAPI VincAggregateModelGetMaxElementCount(VincModelID m)
{
    return VincAggregateGetMaxElementCount(m->geometry);
}

// Win 0x7747f0 (name inferred): update the element transforms, then release all elements (their
// parent cleared); returns the element array
VincModelID* MEAPI VincAggregateModelReleaseElements(VincModelID m)
{
    if (m->relTransform != 0)
        VincModelComposeRelativeTransform(m);
    VincAggregateUpdateElementTransforms(m->geometry, m->transform);
    return VincAggregateRemoveAllElements(m->geometry);
}

} // extern "C"
