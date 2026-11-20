/* Vortex 5.0.0 build 800 "vortexldd41" - box / capsule pair test (restored).
 * Mac 0x4dcf2d-0x4ddbf5 (557e), Win 0x7da540-0x7dac60. Names (name inferred). The capsule is
 * tested as its two end spheres (box / sphere contact) and its shaft (the box / cylinder test). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4dcf2d (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4dcf40, Win 0x7da6e0: the box and the capsule (as a box r, r, r + hh) swept */
int MEAPI VincBoxCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCapsuleGetRadius(g2);
    MeReal hh = VincCapsuleGetHalfHeight(g2);
    MeReal* radii = VincBoxGetRadii(g1);
    MeVector3 e2;
    e2[0] = r;
    e2[1] = r;
    e2[2] = r + hh;
    return VincEllipsoidSafeTime(pair, maxTime, radii, e2, result);
}

/* Mac 0x4dcff1, Win 0x7da680 (cdecl): the shaft = the box / cylinder separation; the element flag
 * of every candidate point it adds is cleared (flags[i] for the i-th new point) */
static int BoxCapsuleShaft(MeReal* sep, MeReal* n, MeReal* d, MeReal** cursor, const MeReal* pad, const MeReal* r1,
                           const MeReal* r, const MeReal* hh, const MeReal* rel, int* flags)
{
    MeReal* start = *cursor;
    MeReal* p;
    if (!VincBoxCylinderSeparation(sep, n, d, cursor, pad, r1, r, hh, rel))
        return 0;
    for (p = start; p < *cursor; p += 3)
        *flags++ = 0;
    return 1;
}

/* Mac 0x4dd073, Win 0x7da540 (cdecl): the end spheres (box frame: T -+ hh z) against the box,
 * written as box-frame contacts; a contact counts only on the sphere's outer half (its point not
 * beyond the end towards the shaft by more than *pad): element1 = 0, element2 = -1 bottom / +1 top.
 * Returns the number of contacts (0..2). */
static int BoxCapsuleEnds(VincContact* c, const MeReal* pad, const MeReal* r1, const MeReal* r, const MeReal* hh,
                          const MeReal* rel)
{
    MeReal bottom[3], top[3];
    int count = 0;
    bottom[0] = rel[12] - rel[8] * *hh;
    bottom[1] = rel[13] - rel[9] * *hh;
    bottom[2] = rel[14] - rel[10] * *hh;
    top[0] = *hh * rel[8] + rel[12];
    top[1] = rel[9] * *hh + rel[13];
    top[2] = rel[10] * *hh + rel[14];
    if (VincBoxSphereContact(&c->separation, c->normal, c->position, &c->dims, pad, r1, r, bottom)
        && (c->position[0] - bottom[0]) * rel[8] + (c->position[1] - bottom[1]) * rel[9]
                   + (c->position[2] - bottom[2]) * rel[10] <= *pad) {
        c->element1 = 0;
        c->element2 = -1;
        count = 1;
        ++c;
    }
    if (VincBoxSphereContact(&c->separation, c->normal, c->position, &c->dims, pad, r1, r, top)
        && -*pad <= (c->position[0] - top[0]) * rel[8] + (c->position[1] - top[1]) * rel[9]
                        + (c->position[2] - top[2]) * rel[10]) {
        c->element1 = 0;
        c->element2 = 1;
        ++count;
    }
    return count;
}

/* Mac 0x4dd2c4, Win 0x7da750 (model1 = box, model2 = capsule). NOTE (both builds): with two end
 * contacts the shaft is not tested and the dims of the end contacts come from the (then
 * uninitialised) shaft normal (zero here); without a shaft result the result normal is the first
 * end contact's normal in the box frame. */
int MEAPI VincBoxCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* r1 = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCapsuleGetRadius(g2);
    MeReal hh = VincCapsuleGetHalfHeight(g2);
    MeReal rel[16], points[96 * 3], n[3], sep = 0.0, d = 0.0;
    MeReal *cursor = points, *p;
    VincContact ends[2], contacts[48];
    int flags[96];
    VincRequest* rq;
    MeI16 dims;
    int nEnds, shaft = 0, cnt, i;
    n[0] = n[1] = n[2] = 0.0;
    VincTMMakeRelative(tm2, tm1, rel);
    nEnds = BoxCapsuleEnds(ends, &pad, r1, &r, &hh, rel);
    if (nEnds < 2)
        shaft = BoxCapsuleShaft(&sep, n, &d, &cursor, &pad, r1, &r, &hh, rel, flags);
    if (!shaft && nEnds == 0) {
        ResultClear(result);
        return 0;
    }
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0) {
        ResultClear(result);
        return 1;
    }
    dims = VincBoxCylinderDims(n, rel + 8);
    ResultClear(result);
    for (i = 0; i < nEnds; ++i) {
        VincContact* c = &contacts[result->contactCount];
        memcpy(c, &ends[i], sizeof *c);
        VincTMTransform(tm1, ends[i].position, c->position);
        VincTMRotate(tm1, ends[i].normal, c->normal);
        c->dims = dims;
        ++result->contactCount;
    }
    if (!shaft) {
        result->normal[0] = ends[0].normal[0];
        result->normal[1] = ends[0].normal[1];
        result->normal[2] = ends[0].normal[2];
    } else {
        VincTMRotate(tm1, n, result->normal);
    }
    for (p = points, i = 0; p != cursor; p += 3, ++i) {
        MeReal depth;
        if (result->contactCount >= 48)
            break;
        depth = (p[0] * n[0] + p[1] * n[1] + p[2] * n[2]) - d;
        if (depth < sep * 0.01) {
            VincContact* c = &contacts[result->contactCount];
            VincTMTransform(tm1, p, c->position);
            c->dims = dims;
            c->separation = depth;
            c->element1 = 0;
            c->element2 = flags[i];
            c->normal[0] = result->normal[0];
            c->normal[1] = result->normal[1];
            c->normal[2] = result->normal[2];
            ++result->contactCount;
        }
    }
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    return result->contactCount > 0;
}

/* Mac 0x4dd24a, Win 0x7dac10 */
int MEAPI VincBoxCapsuleRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincBoxCapsuleIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincBoxCapsuleSafeTime;
    VincGeometryPairRegisterInteractions(VincBoxGetTypeId(), VincCapsuleGetTypeId(), &i, 0);
    return 1;
}
