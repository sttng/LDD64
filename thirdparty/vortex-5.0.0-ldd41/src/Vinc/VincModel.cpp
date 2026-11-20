// Vortex 5.0.0 build 800 "vortexldd41" - VincModel (restored; Mac 557c
// 0x4c189a-0x4c2042, Win 0x74f620-0x74fd10). Win-only (dead-stripped on the Mac): SetPadding,
// Get/SetSweep, Get/SetUpdateTransformFn, GetEffectiveTransformPtr, DestroyToPool, UpdateAABB.
#include "Vinc/VincIntersect.h"
#include "Vinc/VincMath.h"
#include "Me/MeMath.h"
#include "Me/VinPool.h"

#include "VincInternal.h"

namespace {
// Mac 0x780340: default velocity (zero vector) of a new model
MeReal s_zeroVelocity[3] = {0.0, 0.0, 0.0};
}

extern "C" {

// Mac 0x4c189a, Win 0x74f720 (name inferred)
void MEAPI VincSetDefaultPadding(const MeReal* padding)
{
    VincTheFramework->defaultPadding = *padding;
}

// Mac 0x4c18b9, Win 0x74f7c0 (name inferred)
int MEAPI VincModelGetState(VincModelID m)
{
    return m->state;
}

// Mac 0x4c18c7, Win 0x74f810 (name inferred)
MeReal MEAPI VincModelGetPadding(VincModelID m)
{
    return m->padding;
}

// Win 0x74f800 (name inferred)
void MEAPI VincModelSetPadding(VincModelID m, MeReal padding)
{
    m->padding = padding;
}

// Win 0x74f820 (name inferred)
void MEAPI VincModelSetSweep(VincModelID m, int on)
{
    m->sweep = on;
}

// Win 0x74f840 (name inferred)
int MEAPI VincModelGetSweep(VincModelID m)
{
    return m->sweep;
}

// Mac 0x4c18d2, Win 0x74f890 (name inferred)
void MEAPI VincModelUpdateTransform(VincModelID m)
{
    if (m->updateTransformFn != 0)
        m->updateTransformFn(m);
}

// Mac 0x4c18e9, Win 0x74faa0 (name inferred)
MeReal* MEAPI VincModelGetTransformPtr(VincModelID m)
{
    return m->transform;
}

// Mac 0x4c18f4, Win 0x74fab0 (name inferred)
void MEAPI VincModelSetTransformPtrRaw(VincModelID m, MeReal* tm)
{
    m->transform = tm;
}

// Mac 0x4c1902, Win 0x74fac0 ("VincModelSetTransform" in VincErrorTable id 7). With a relative
// transform the pointer is the body transform.
void MEAPI VincModelSetTransformPtr(VincModelID m, MeReal* tm)
{
    if (m->relTransform == 0)
        m->transform = tm;
    else
        m->bodyTransform = tm;
}

// Win 0x74fae0 (name inferred)
MeReal* MEAPI VincModelGetEffectiveTransformPtr(VincModelID m)
{
    if (m->relTransform != 0)
        return m->bodyTransform;
    return m->transform;
}

// Mac 0x4c191b, Win 0x74fb00 (name inferred)
void MEAPI VincModelSetLinearVelocityPtr(VincModelID m, MeReal* v)
{
    m->linearVelocity = v;
}

// Mac 0x4c1929, Win 0x74fb10 (name inferred)
MeReal* MEAPI VincModelGetLinearVelocityPtr(VincModelID m)
{
    return m->linearVelocity;
}

// Mac 0x4c1934, Win 0x74fb20 (name inferred)
void MEAPI VincModelSetAngularVelocityPtr(VincModelID m, MeReal* w)
{
    m->angularVelocity = w;
}

// Mac 0x4c1942 (name inferred): the default setVelocityPtrsFn
void MEAPI VincModelSetVelocityPtrs(VincModelID m, MeReal* v, MeReal* w)
{
    VincModelSetLinearVelocityPtr(m, v);
    VincModelSetAngularVelocityPtr(m, w);
}

// Mac 0x4c1970, Win 0x74fb30 (name inferred)
MeReal* MEAPI VincModelGetAngularVelocityPtr(VincModelID m)
{
    return m->angularVelocity;
}

// Mac 0x4c197b, Win 0x74fb40 (name inferred)
void MEAPI VincModelSetUserData(VincModelID m, void* data)
{
    m->userData = data;
}

// Mac 0x4c1989, Win 0x74fb50 (name inferred)
void* MEAPI VincModelGetUserData(VincModelID m)
{
    return m->userData;
}

// Mac 0x4c1994 (name inferred)
void MEAPI VincModelSetGeometry(VincModelID m, VincGeometryID g)
{
    m->geometry = g;
}

// Mac 0x4c19a1 (name inferred)
VincGeometryID MEAPI VincModelGetGeometry(VincModelID m)
{
    return m->geometry;
}

// Mac 0x4c19ab (name inferred)
void MEAPI VincModelSetRelativeTransformPtrs(VincModelID m, MeReal* relTM, MeReal* bodyTM, MeReal* tm)
{
    m->relTransform = relTM;
    m->bodyTransform = bodyTM;
    m->transform = tm;
    if (relTM == 0)
        m->updateTransformFn = 0;
    else
        m->updateTransformFn = VincModelComposeRelativeTransform;
}

// Mac 0x4c19e3, Win 0x74fc30 (name inferred)
void MEAPI VincModelGetRelativeTransformPtrs(VincModelID m, MeReal** relTM, MeReal** bodyTM)
{
    *relTM = m->relTransform;
    *bodyTM = m->bodyTransform;
}

// Mac 0x4c19fb, Win 0x74fc70 (name inferred)
void MEAPI VincModelSetM7C(VincModelID m, void* p)
{
    m->m7c = p;
}

// Mac 0x4c1a09, Win 0x74fc90 (name inferred)
void* MEAPI VincModelGetM7C(VincModelID m)
{
    return m->m7c;
}

// Mac 0x4c1a14, Win 0x74fca0 (name inferred)
void MEAPI VincModelSetM80(VincModelID m, void* p)
{
    m->m80 = p;
}

// Mac 0x4c1a25, Win 0x74fcc0 (name inferred)
void* MEAPI VincModelGetM80(VincModelID m)
{
    return m->m80;
}

// Mac 0x4c1a33, Win 0x74fcf0 (name inferred)
void MEAPI VincModelSetDensity(VincModelID m, MeReal density)
{
    m->density = density;
}

// Mac 0x4c1a45, Win 0x74fd00 (name inferred)
MeReal MEAPI VincModelGetDensity(VincModelID m)
{
    return m->density;
}

// Win 0x74fcd0 (name inferred)
void MEAPI VincModelSetUpdateTransformFn(VincModelID m, VincModelUpdateTransformFn fn)
{
    m->updateTransformFn = fn;
}

// Win 0x74fce0 (name inferred)
VincModelUpdateTransformFn MEAPI VincModelGetUpdateTransformFn(VincModelID m)
{
    return m->updateTransformFn;
}

// Mac 0x4c1a50, Win 0x74fc50 (name inferred)
void MEAPI VincModelComposeRelativeTransform(VincModelID m)
{
    MeMatrix4TMCompound((MeReal(*)[4])m->transform, (MeReal(*)[4])m->relTransform,
                        (MeReal(*)[4])m->bodyTransform);
}

// Mac 0x4c1a74, Win 0x74f8f0 (name inferred)
void MEAPI VincModelUpdate(VincModelID m, MeVector3 minCorner, MeVector3 maxCorner, MeReal dt)
{
    if (m->updateTransformFn != 0)
        m->updateTransformFn(m);
    MeReal* tm = m->transform;
    VincGeometryID g = m->geometry;
    if (g == 0)
        return;
    VincGeometryUpdateAABB(g, (MeReal(*)[4])tm, minCorner, maxCorner, m->aabb);
    if (m->linearVelocity != 0 && m->angularVelocity != 0 && m->sweep != 0 && dt != 0.0) {
        MeMatrix4 next;
        MeVector3 nextMin, nextMax;
        MeReal nextAABB[6];
        MeMatrix4TMUpdateFromVelocitiesCOM(next, 1e-4, dt, m->linearVelocity, m->angularVelocity,
                                           (MeReal(*)[4])tm, 0, m->negCOM);
        VincGeometryUpdateAABB(g, next, nextMin, nextMax, nextAABB);
        for (int i = 0; i < 3; ++i) {
            if (nextMin[i] < minCorner[i])
                minCorner[i] = nextMin[i];
            if (maxCorner[i] < nextMax[i])
                maxCorner[i] = nextMax[i];
        }
        // an aggregate's update moved its elements to the swept pose: update them again at tm
        if (m->geometry->type == VincAggregateGetTypeId())
            VincGeometryUpdateAABB(g, (MeReal(*)[4])tm, nextMin, nextMax, m->aabb);
    }
}

// Win 0x74f8b0 (and its ICF twin 0x74fa60) (name inferred)
void MEAPI VincModelUpdateAABB(VincModelID m, MeVector3 minCorner, MeVector3 maxCorner)
{
    if (m->updateTransformFn != 0)
        m->updateTransformFn(m);
    if (m->geometry != 0)
        VincGeometryUpdateAABB(m->geometry, (MeReal(*)[4])m->transform, minCorner, maxCorner, m->aabb);
}

// Mac 0x4c1c27, Win 0x74f850 (name inferred)
void MEAPI VincModelGetBSphere(VincModelID m, MeVector3 center, MeReal* radius)
{
    MeReal* tm = m->transform;
    if (m->geometry != 0) {
        MeVector3 local;
        VincGeometryGetBSphere(m->geometry, local, radius);
        VincTMTransform(tm, local, center);
    }
}

// Mac 0x4c1d03, Win 0x74f780 (name inferred)
void MEAPI VincModelSetState(VincModelID m, int state)
{
    if (m->state != state) {
        if (m->space != 0 && (m->state == 2 || state == 2))
            VincSpaceModelSetState2(m, state == 2);
        m->state = state;
    }
}

// Mac 0x4c1d52 (name inferred)
VincModelID MEAPI VincModelCreateFromPool(VincGeometryID g, VinPool* pool)
{
    VincModelID m = (VincModelID)VinPoolGetStruct(pool);
    if (m != 0) {
        m->geometry = g;
        m->transform = 0;
        m->parent = 0;
        m->padding = VincTheFramework->defaultPadding;
        m->updateTransformFn = 0;
        m->setVelocityPtrsFn = VincModelSetVelocityPtrs;
        m->relTransform = 0;
        m->bodyTransform = 0;
        m->m6c = 0;
        m->m70 = 0;
        m->density = 1.0;
        m->m7c = 0;
        m->m80 = 0;
        m->space = 0;
        m->spaceHandle = -1;
        m->userData = 0;
        m->m50 = 0;
        m->linearVelocity = s_zeroVelocity;
        m->angularVelocity = s_zeroVelocity;
        m->sweep = 0;
        m->ma8 = 0;
        m->negCOM[2] = 0.0;
        m->negCOM[1] = 0.0;
        m->negCOM[0] = 0.0;
        m->state = 0;
        m->ma4 = 0;
    }
    return m;
}

// Mac 0x4c1e58 (name inferred)
VincModelID MEAPI VincModelCreate(VincGeometryID g)
{
    return VincModelCreateFromPool(g, &VincTheFramework->modelPool);
}

// Mac 0x4c1e85, Win 0x74f750 ("VincModelDestroy" in VincErrorTable id 38)
void MEAPI VincModelDestroy(VincModelID m)
{
    if (VincModelIsAggregate(m) == 0)
        VinPoolPutStruct(&VincTheFramework->modelPool, m);
    else
        VincAggregateModelDestroy(m);
}

// Win 0x74f7d0 (name inferred)
void MEAPI VincModelDestroyToPool(VincModelID m, VinPool* pool)
{
    if (VincModelIsAggregate(m) == 0)
        VinPoolPutStruct(pool, m);
    else
        VincAggregateModelDestroyToPool(m, pool);
}

// Mac 0x4c1ecf (name inferred)
void MEAPI VincModelPoolDestroy(void)
{
    VinPoolDestroy(&VincTheFramework->modelPool);
}

// Mac 0x4c1ef5, Win 0x74f6e0 (name inferred)
int MEAPI VincModelPoolInit(int modelCount)
{
    VinPoolInitSimple(&VincTheFramework->modelPool, modelCount, sizeof(VincModel), 0);
    return 1;
}

// Mac 0x4c1f37 (name inferred). Without a relative transform the point is stored negated; with
// one it is taken into the body frame first and then negated.
void MEAPI VincModelSetCenterOfRotation(VincModelID m, const MeVector3 p)
{
    MeReal* rel = m->relTransform;
    if (rel == 0) {
        m->negCOM[0] = -p[0];
        m->negCOM[1] = -p[1];
        m->negCOM[2] = -p[2];
    } else {
        MeReal d0 = p[0] - rel[12];
        MeReal d1 = p[1] - rel[13];
        MeReal d2 = p[2] - rel[14];
        m->negCOM[0] = d0 * rel[0] + d1 * rel[1] + d2 * rel[2];
        m->negCOM[1] = d0 * rel[4] + d1 * rel[5] + d2 * rel[6];
        m->negCOM[2] = d0 * rel[8] + d1 * rel[9] + d2 * rel[10];
        m->negCOM[0] = -m->negCOM[0];
        m->negCOM[1] = -m->negCOM[1];
        m->negCOM[2] = -m->negCOM[2];
    }
}

// Mac 0x4c2042, Win 0x74fd10 (name inferred)
MeI16 MEAPI VincModelGetMassProperties(VincModelID m, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume,
                               MeReal* mass)
{
    MeI16 status;
    if (VincModelIsAggregate(m) == 0) {
        status = VincGeometryGetMassProperties(m->geometry, relTM, inertia, volume);
        MeReal s = *volume * m->density;
        *mass = s;
        inertia[0][0] *= s;
        inertia[0][1] *= s;
        inertia[0][2] *= s;
        inertia[1][0] *= s;
        inertia[1][1] *= s;
        inertia[1][2] *= s;
        inertia[2][0] *= s;
        inertia[2][1] *= s;
        inertia[2][2] *= s;
    } else {
        MeReal aggMass = 0.0;
        status = VincAggregateGetMassProperties(m->geometry, relTM, inertia, volume, &aggMass);
        *mass = m->density * aggMass;
        MeReal d = m->density;
        inertia[0][0] *= d;
        inertia[0][1] *= d;
        inertia[0][2] *= d;
        inertia[1][0] *= d;
        inertia[1][1] *= d;
        inertia[1][2] *= d;
        inertia[2][0] *= d;
        inertia[2][1] *= d;
        inertia[2][2] *= d;
    }
    MeReal* rel = m->relTransform;
    if (rel != 0) {
        // inertia in the body frame: I' = R^T I R (R = rotation of relTM, rows = axes)
        MeMatrix3 I;
        for (int i = 0; i < 3; ++i)
            for (int j = 0; j < 3; ++j)
                I[i][j] = inertia[i][j];
        for (int i = 0; i < 3; ++i) {
            for (int j = 0; j < 3; ++j) {
                inertia[i][j] = 0.0;
                for (int k = 0; k < 3; ++k)
                    for (int l = 0; l < 3; ++l)
                        inertia[i][j] = rel[k * 4 + i] * I[k][l] * rel[l * 4 + j] + inertia[i][j];
            }
        }
        MeReal s;
        s = (inertia[1][0] + inertia[0][1]) * 0.5;
        inertia[0][1] = s;
        inertia[1][0] = s;
        s = (inertia[2][0] + inertia[0][2]) * 0.5;
        inertia[0][2] = s;
        inertia[2][0] = s;
        s = (inertia[1][2] + inertia[2][1]) * 0.5;
        inertia[2][1] = s;
        inertia[1][2] = s;
        // the centre of mass frame into the body frame
        MeReal c0 = relTM[3][0], c1 = relTM[3][1], c2 = relTM[3][2];
        relTM[3][0] = c0 * rel[0] + c1 * rel[4] + c2 * rel[8] + rel[12];
        relTM[3][1] = c0 * rel[1] + c1 * rel[5] + c2 * rel[9] + rel[13];
        relTM[3][2] = c0 * rel[2] + c1 * rel[6] + c2 * rel[10] + rel[14];
    }
    return status;
}

} // extern "C"
