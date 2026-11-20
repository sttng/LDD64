// Vortex 5.0.0 build 800 "vortexldd41" - hello / goodbye / intersect / safe time and the swept
// (continuous) intersection driver (restored; Mac 557c 0x4bfdb8, 0x4c0347-0x4c11f6;
// Win 0x7542b0-0x754e50).
#include "Vinc/VincIntersect.h"
#include "Me/MeMath.h"

#include <float.h>
#include <math.h>

#include "VincInternal.h"

namespace {

VincHelloCallback s_helloCallback = 0;   // Mac 0x779558, Win 0xaf201c
void* s_helloCallbackData = 0;           // Mac 0x77955c, Win 0xaf2020
MeReal s_sweepCoarseFactor = 0.5;        // Mac 0x779550: coarse steps = (int)(factor / request->m10)

// Mac 0x4c0347 (regparm; inlined on Win) (name inferred)
inline void ResultReset(VincIntersectResult* r)
{
    r->contactGroupCount = 0;
    r->contactCount = 0;
}

// Mac 0x4c0575, Win 0x7545e0 (regparm) (name inferred): after the sweep loop, give the model its
// AABB of the start pose back.
void RestoreModelAABB(VincModelID m, MeReal* tm, const MeReal* savedAABB)
{
    if (VincSpaceModelIsFrozen(m) == 0) {
        if (m->geometry->type == VincAggregateGetTypeId()) {
            MeVector3 mn, mx;
            VincAggregateUpdateAABB(m->geometry, (MeReal(*)[4])tm, mx, mn, m->aabb);
        } else {
            for (int i = 0; i < 6; ++i)
                m->aabb[i] = savedAABB[i];
        }
    }
}

// Mac 0x4c0627, Win 0x754370 (name inferred): number of sub-steps of a swept intersection, from
// the AABB half extents and the velocities.
int SweepStepCount(VincModelID m1, VincModelID m2, MeReal dt, MeReal tolerance)
{
    MeReal hx2 = (m2->aabb[1] - m2->aabb[0]) * 0.5;
    MeReal hy2 = (m2->aabb[3] - m2->aabb[2]) * 0.5;
    MeReal hz2 = (m2->aabb[5] - m2->aabb[4]) * 0.5;
    MeReal* v1 = m1->linearVelocity;
    MeReal* v2 = m2->linearVelocity;
    MeReal dx = v1[0] - v2[0];
    MeReal dy = v1[1] - v2[1];
    MeReal dz = v1[2] - v2[2];
    MeReal ext[3];
    MeReal speed[3];
    if (m1->sweep == 0) {
        ext[0] = hx2;
        ext[1] = hy2;
        ext[2] = hz2;
        MeReal* w = m2->angularVelocity;
        speed[0] = fabs(hy2 * w[2] - hz2 * w[1]) + dx;
        speed[1] = fabs(hz2 * w[0] - hx2 * w[2]) + dy;
        speed[2] = fabs(hx2 * w[1] - hy2 * w[0]) + dz;
    } else {
        MeReal hx1 = (m1->aabb[1] - m1->aabb[0]) * 0.5;
        MeReal hy1 = (m1->aabb[3] - m1->aabb[2]) * 0.5;
        MeReal hz1 = (m1->aabb[5] - m1->aabb[4]) * 0.5;
        if (m2->sweep == 0) {
            ext[0] = hx1;
            ext[1] = hy1;
            ext[2] = hz1;
            MeReal* w = m1->angularVelocity;
            speed[0] = fabs(hy1 * w[2] - hz1 * w[1]) + dx;
            speed[1] = fabs(hz1 * w[0] - hx1 * w[2]) + dy;
            speed[2] = fabs(hx1 * w[1] - hy1 * w[0]) + dz;
        } else {
            MeReal mx = hx2;
            if (hx1 < hx2)
                mx = hx1;
            MeReal my = hy2;
            if (hy1 < hy2)
                my = hy1;
            MeReal mz = hz2;
            if (hz1 < hz2)
                mz = hz1;
            ext[0] = mx * 0.5;
            ext[1] = my * 0.5;
            ext[2] = mz * 0.5;
            MeReal* w1 = m1->angularVelocity;
            MeReal* w2 = m2->angularVelocity;
            speed[0] = dx + fabs(hy1 * w1[2] - hz1 * w1[1]) + fabs(hy2 * w2[2] - hz2 * w2[1]);
            speed[1] = dy + fabs(hz1 * w1[0] - hx1 * w1[2]) + fabs(hz2 * w2[0] - hx2 * w2[2]);
            speed[2] = dz + fabs(hx1 * w1[1] - hy1 * w1[0]) + fabs(hx2 * w2[1] - hy2 * w2[0]);
        }
    }
    int n = 1;
    for (int i = 0; i < 3; ++i) {
        MeReal q = (fabs(speed[i]) * dt) / (tolerance * ext[i]);
        if ((MeReal)n < q)
            n = (int)q;
    }
    return n;
}

// Mac 0x4c0a68, Win 0x754630 (name inferred): sub-stepped intersection over dt for sweeping
// models. Coarse steps of `coarse` sub-steps without contacts (contactMaxCount = 0) until a touch,
// then back one coarse step and continue one by one; the first sub-step with contacts wins
// (result->time = its time).
int SweptIntersect(VincModelPair* pair, VincIntersectResult* result, MeReal dt)
{
    VincModelID m1 = pair->model1;
    VincModelID m2 = pair->model2;
    if (m1->sweep != 0 || m2->sweep != 0) {
        MeReal tolerance = VincModelPairGetRequestPtr(pair)->m10;
        int steps = SweepStepCount(m1, m2, dt, tolerance);
        if (steps > 1000)
            steps = 1000;
        int c = (int)((1.0 / tolerance) * s_sweepCoarseFactor);
        int coarse = 1;
        if (0 < c)
            coarse = c;
        if (steps != 1) {
            int isAggregate = m1->geometry->type == VincAggregateGetTypeId()
                              || m2->geometry->type == VincAggregateGetTypeId();
            MeReal* tm1 = VincModelGetTransformPtr(m1);
            MeReal* tm2 = VincModelGetTransformPtr(m2);
            MeVector4 q1, q2;
            MeQuaternionFromTM(q1, (MeReal(*)[4])tm1);
            MeQuaternionFromTM(q2, (MeReal(*)[4])tm2);
            int savedMaxCount = 0;
            VincRequest* request = 0;
            if (coarse > 1) {
                request = VincModelPairGetRequestPtr(pair);
                savedMaxCount = request->contactMaxCount;
            }
            VincIntersectFn fn = pair->intersectFn;
            int flag1 = VincSpaceModelIsFrozen(m1);
            int flag2 = VincSpaceModelIsFrozen(m2);
            MeReal saved1[6], saved2[6];
            MeMatrix4 tmA, tmB;
            for (int i = 0; i < steps; i += coarse) {
                MeReal t = (MeReal)i * (dt / (MeReal)steps);
                if (i < 1) {
                    if (isAggregate) {
                        for (int k = 0; k < 6; ++k) {
                            saved1[k] = m1->aabb[k];
                            saved2[k] = m2->aabb[k];
                        }
                    }
                } else {
                    if (coarse > 1)
                        request->contactMaxCount = 0;
                    MeMatrix4TMUpdateFromVelocitiesCOM(tmA, 1e-4, t, m1->linearVelocity,
                                                       m1->angularVelocity, (MeReal(*)[4])tm1, q1,
                                                       m1->negCOM);
                    VincModelSetTransformPtrRaw(m1, &tmA[0][0]);
                    MeMatrix4TMUpdateFromVelocitiesCOM(tmB, 1e-4, t, m2->linearVelocity,
                                                       m2->angularVelocity, (MeReal(*)[4])tm2, q2,
                                                       m2->negCOM);
                    VincModelSetTransformPtrRaw(m2, &tmB[0][0]);
                    if (isAggregate) {
                        MeVector3 a, b;
                        if (flag1 == 0)
                            VincGeometryUpdateAABB(m1->geometry, tmA, a, b, m1->aabb);
                        if (flag2 == 0)
                            VincGeometryUpdateAABB(m2->geometry, tmB, b, a, m2->aabb);
                    }
                }
                int touch = fn(pair, result);
                result->touch = touch;
                if (touch != 0) {
                    if (coarse < 2 || i < coarse) {
                        if (0 < result->contactCount) {
                            result->time = t;
                            break;
                        }
                        result->touch = 0;
                    } else {
                        ResultReset(result);
                        result->touch = 0;
                        i -= coarse;
                        request->contactMaxCount = savedMaxCount;
                        coarse = 1;
                    }
                }
            }
            VincModelSetTransformPtrRaw(m1, tm1);
            VincModelSetTransformPtrRaw(m2, tm2);
            // NOTE: the original stores through the request pointer even when it is NULL (only
            // possible when the request tolerance m10 > 0.5, i.e. coarse == 1): guarded here.
            if (request != 0)
                request->contactMaxCount = savedMaxCount;
            if (isAggregate) {
                RestoreModelAABB(m1, tm1, saved1);
                RestoreModelAABB(m2, tm2, saved2);
            } else {
                for (int k = 0; k < result->contactCount; ++k) {
                    result->contacts[k].model1 = result->pair->model1;
                    result->contacts[k].model2 = result->pair->model2;
                }
            }
            return result->touch;
        }
    }
    VincIntersect(pair, result);
    return result->touch;
}

// the element models of the contacts (shared tail of VincIntersect / VincIntersectWithTime)
inline void SetContactModels(VincIntersectResult* result)
{
    if (0 < result->contactCount
        && result->pair->model1->geometry->type != VincAggregateGetTypeId()
        && result->pair->model2->geometry->type != VincAggregateGetTypeId()) {
        for (int k = 0; k < result->contactCount; ++k) {
            result->contacts[k].model1 = result->pair->model1;
            result->contacts[k].model2 = result->pair->model2;
        }
    }
}

} // namespace

extern "C" {

// Mac 0x4bfdb8 (name inferred)
void MEAPI VincGoodbyeEach(VincModelPairContainer* c)
{
    for (int i = 0; i < c->goodbyeEnd; ++i)
        VincGoodbye(c->array[i]);
}

// Mac 0x4c035a, Win 0x7542b0 (name inferred)
void MEAPI VincGoodbye(VincModelPair* p)
{
    if (p->goodbyeFn != 0)
        p->goodbyeFn(p);
}

// Mac 0x4c0371, Win 0x754d30 ("VincSafeTime" in VincErrorTable id 34)
int MEAPI VincSafeTime(VincModelPair* p, MeReal maxTime, VincSafeTimeResult* r)
{
    VincSafeTimeFn fn = p->safeTimeFn;
    if (fn == 0 || p->model1->linearVelocity == 0 || p->model2->linearVelocity == 0
        || p->model1->angularVelocity == 0 || p->model2->angularVelocity == 0)
        r->time = maxTime;
    else
        fn(p, maxTime, r);
    return fn != 0;
}

// Mac 0x4c03ce, Win 0x754d90 (name inferred)
void MEAPI VincSetHelloCallback(VincHelloCallback fn, void* data)
{
    s_helloCallback = fn;
    s_helloCallbackData = data;
}

// Mac 0x4c03eb, Win 0x754260 (name inferred)
void MEAPI VincHelloWithRequest(VincModelPair* p, VincRequest* r)
{
    p->request = r;
    int swap;
    VincInteractions* i = VincPairTableGetInteractions(p, &swap);
    p->intersectFn = i->intersectFn;
    p->sweptIntersectFn = i->sweptIntersectFn;
    p->safeTimeFn = i->safeTimeFn;
    p->goodbyeFn = i->goodbyeFn;
    if (swap != 0) {
        VincModelID m = p->model1;
        p->model1 = p->model2;
        p->model2 = m;
    }
    if (i->helloFn != 0)
        i->helloFn(p);
}

// Mac 0x4c0445 (name inferred)
void MEAPI VincHello(VincModelPair* p)
{
    if (s_helloCallback != 0)
        s_helloCallback(p, s_helloCallbackData);
    int swap;
    VincInteractions* i = VincPairTableGetInteractions(p, &swap);
    p->intersectFn = i->intersectFn;
    p->sweptIntersectFn = i->sweptIntersectFn;
    p->safeTimeFn = i->safeTimeFn;
    p->goodbyeFn = i->goodbyeFn;
    if (swap != 0) {
        VincModelID m = p->model1;
        p->model1 = p->model2;
        p->model2 = m;
    }
    if (i->helloFn != 0)
        i->helloFn(p);
}

// Mac 0x4c04ba, Win 0x7542d0 (name inferred)
int MEAPI VincIntersect(VincModelPair* p, VincIntersectResult* r)
{
    r->pair = p;
    r->time = 0.0;
    if (p->intersectFn == 0) {
        ResultReset(r);
        r->touch = 0;
        return 0;
    }
    r->touch = p->intersectFn(p, r);
    SetContactModels(r);
    return 1;
}

// Mac 0x4c10cb, Win 0x754b60 (name inferred)
int MEAPI VincIntersectWithTime(VincModelPair* p, VincIntersectResult* r, MeReal dt)
{
    r->pair = p;
    r->time = 0.0;
    if (p->intersectFn == 0) {
        ResultReset(r);
        r->touch = 0;
        return 0;
    }
    if ((p->model1->sweep == 0 && p->model2->sweep == 0) || dt <= 0.0) {
        r->touch = p->intersectFn(p, r);
        SetContactModels(r);
        return 1;
    }
    if (p->sweptIntersectFn == 0) {
        SweptIntersect(p, r, dt);
        return 1;
    }
    r->touch = p->sweptIntersectFn(p, r, dt);
    return 1;
}

// Mac 0x4c11f6, Win 0x754a20 (name inferred): advance both models to the safe time, then the swept
// intersection over the rest of dt. NOTE: no NULL check of the pair's safe time function.
int MEAPI VincIntersectSafeTime(VincModelPair* p, VincIntersectResult* r, MeReal dt)
{
    ResultReset(r);
    VincSafeTimeResult st;
    int ok = 0;
    if (p->safeTimeFn(p, dt, &st) != 0) {
        VincModelID m1 = p->model1;
        VincModelID m2 = p->model2;
        MeReal* tm1 = VincModelGetTransformPtr(m1);
        MeReal* tm2 = VincModelGetTransformPtr(m2);
        MeMatrix4 tmA, tmB;
        MeMatrix4TMUpdateFromVelocitiesCOM(tmA, 1e-4, st.time, m1->linearVelocity, m1->angularVelocity,
                                           (MeReal(*)[4])tm1, 0, m1->negCOM);
        MeMatrix4TMUpdateFromVelocitiesCOM(tmB, 1e-4, st.time, m2->linearVelocity, m2->angularVelocity,
                                           (MeReal(*)[4])tm2, 0, m2->negCOM);
        VincModelSetTransformPtrRaw(m1, &tmA[0][0]);
        VincModelSetTransformPtrRaw(m2, &tmB[0][0]);
        ok = SweptIntersect(p, r, dt - st.time);
        VincModelSetTransformPtrRaw(m1, tm1);
        VincModelSetTransformPtrRaw(m2, tm2);
        r->time = r->time + st.time;
    }
    return ok;
}

// Win 0x754c50 (name inferred; dead-stripped on the Mac)
int MEAPI VincIntersectAtTime(VincModelPair* p, VincIntersectResult* r, MeReal t)
{
    VincModelID m1 = p->model1;
    VincModelID m2 = p->model2;
    MeReal* tm1 = VincModelGetTransformPtr(m1);
    MeReal* tm2 = VincModelGetTransformPtr(m2);
    MeMatrix4 tmA, tmB;
    MeMatrix4TMUpdateFromVelocitiesCOM(tmA, 1e-4, t, m1->linearVelocity, m1->angularVelocity,
                                       (MeReal(*)[4])tm1, 0, m1->negCOM);
    MeMatrix4TMUpdateFromVelocitiesCOM(tmB, 1e-4, t, m2->linearVelocity, m2->angularVelocity,
                                       (MeReal(*)[4])tm2, 0, m2->negCOM);
    VincModelSetTransformPtrRaw(m1, &tmA[0][0]);
    VincModelSetTransformPtrRaw(m2, &tmB[0][0]);
    int ok = VincIntersect(p, r);
    VincModelSetTransformPtrRaw(m1, tm1);
    VincModelSetTransformPtrRaw(m2, tm2);
    r->time = t;
    return ok;
}

// Win 0x754dd0 (name inferred)
void MEAPI VincDistanceBoundsInit(MeReal* bounds3)
{
    bounds3[0] = 0.0;
    bounds3[1] = DBL_MAX;
    bounds3[2] = 0.0;
}

// Win 0x754df0 (name inferred). NOTE: swaps the pair's models in place (like VincHello).
int MEAPI VincDistanceBounded(VincModelPair* p, const MeReal* bounds3, VincDistanceResult* r)
{
    int swap;
    VincInteractions* i = VincPairTableGetInteractions(p, &swap);
    VincDistanceBoundedFn fn = i->distanceBoundedFn;
    if (fn != 0 && swap != 0) {
        VincModelID m = p->model1;
        p->model1 = p->model2;
        p->model2 = m;
    }
    r->pair = p;
    if (fn == 0) {
        r->distance = DBL_MAX;
        r->m10 = DBL_MAX;
        return 0;
    }
    fn(p, bounds3, r);
    return 1;
}

// Win 0x754e50 (name inferred)
int MEAPI VincDistance(VincModelPair* p, VincDistanceResult* r)
{
    int swap;
    VincInteractions* i = VincPairTableGetInteractions(p, &swap);
    VincDistanceFn fn = i->distanceFn;
    if (fn == 0) {
        MeReal bounds[3];
        VincDistanceBoundsInit(bounds);
        return VincDistanceBounded(p, bounds, r);
    }
    if (swap != 0) {
        VincModelID m = p->model1;
        p->model1 = p->model2;
        p->model2 = m;
    }
    r->pair = p;
    fn(p, r);
    return 1;
}

} // extern "C"
