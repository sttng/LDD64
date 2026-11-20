/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / plane pair test (restored).
 * Mac 0x4f07e8-0x4f0da2 (557g), Win 0x7e0510-0x7e0ac0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4f07e8 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f07fb, Win 0x7e0510: cylinder (radius *r, half height *hh, transform rel = the cylinder in
 * the plane's frame) against the plane z = 0. *depth = height of the lowest rim point; when it is
 * <= *pad, appends the contact points to *cursor: the four generator lines (+-u, +-v) and the two
 * cap diameters clipped by z <= 0, or the deepest point when none survives. */
int VincCylinderPlaneContactPoints(MeReal* depth, MeReal** cursor, const MeReal* pad, const MeReal* r,
                                   const MeReal* hh, const MeReal* rel)
{
    const MeReal* a = rel + 8;   /* the cylinder axis in the plane frame */
    const MeReal* c = rel + 12;  /* its centre */
    MeReal u0 = a[1], u1 = -a[0], w = 0.0;
    MeReal v0, v1, v2, d, hA0, hA1, hA2, invHA2, invV2;
    MeVector3 p, hA, v;
    MeReal* start;
    MeReal l2 = u0 * u0 + u1 * u1 + 0.0 * 0.0;
    if (1e-15 <= l2 || l2 != l2) {
        MeReal s = sqrt(l2);
        if (s != 0.0) {
            MeReal inv = 1.0 / s;
            u0 = inv * u0;
            u1 = u1 * inv;
            w = inv * 0.0;
        }
        v0 = a[2] * u1 - a[1] * w;
        v1 = a[0] * w - a[2] * u0;
        v2 = a[1] * u0 - a[0] * u1;
    } else {
        u1 = 1.0;
        u0 = 0.0;
        w = 0.0;
        v2 = 0.0;
        v1 = 0.0;
        v0 = 1.0;
    }
    d = c[2] - (fabs(a[2]) * *hh + fabs(v2) * *r);
    *depth = d;
    if (!(d <= *pad))
        return 0;
    v[0] = *r * v0;
    v[1] = *r * v1;
    v[2] = *r * v2;
    hA0 = a[0] * *hh;
    hA1 = a[1] * *hh;
    hA2 = a[2] * *hh;
    hA[0] = hA0;
    hA[1] = hA1;
    hA[2] = hA2;
    invV2 = 1.0 / v[2];
    invHA2 = 1.0 / hA2;
    start = *cursor;
    p[0] = *r * u0 + c[0];
    p[1] = *r * u1 + c[1];
    p[2] = *r * w + c[2];
    VincClipSegmentAddPoints(cursor, p, hA, &invHA2);
    p[0] = c[0] + v[0];
    p[1] = c[1] + v[1];
    p[2] = c[2] + v[2];
    VincClipSegmentAddPoints(cursor, p, hA, &invHA2);
    p[0] = c[0] - u0 * *r;
    p[1] = c[1] - u1 * *r;
    p[2] = c[2] - w * *r;
    VincClipSegmentAddPoints(cursor, p, hA, &invHA2);
    p[0] = c[0] - v[0];
    p[1] = c[1] - v[1];
    p[2] = c[2] - v[2];
    VincClipSegmentAddPoints(cursor, p, hA, &invHA2);
    p[0] = c[0] + hA[0];
    p[1] = c[1] + hA[1];
    p[2] = c[2] + hA[2];
    VincClipSegmentAddInnerPoints(cursor, p, v, &invV2);
    p[0] = c[0] - hA[0];
    p[1] = c[1] - hA[1];
    p[2] = c[2] - hA[2];
    VincClipSegmentAddInnerPoints(cursor, p, v, &invV2);
    if (*cursor == start) {
        MeReal a2 = a[2] * a[2];
        MeReal* q = *cursor;
        *cursor = q + 3;
        q[0] = c[0];
        q[1] = c[1];
        q[2] = c[2];
        if (0.9999999999 < a2) { /* axis vertical: centre of the lower cap */
            if (a[2] < 0.0) {
                q[0] = hA[0] + q[0];
                q[1] = q[1] + hA[1];
                q[2] = hA[2] + q[2];
            } else {
                q[0] = q[0] - hA[0];
                q[1] = q[1] - hA[1];
                q[2] = q[2] - hA[2];
            }
        } else if (a2 < 1e-10) { /* axis horizontal: lowest line */
            q[2] = q[2] - *r;
        } else {                 /* the lowest rim point */
            q[0] = q[0] - v[0];
            q[1] = q[1] - v[1];
            q[2] = q[2] - v[2];
            if (a[2] < 0.0)
                VincVec3AddTo(q, hA);
            else
                VincVec3SubFrom(q, hA);
        }
    }
    return 1;
}

/* Mac 0x4f0e18, Win 0x7e0870 (model1 = cylinder, model2 = plane; NULL plane transform = identity) */
int MEAPI VincCylinderPlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    MeReal r, hh, depth;
    MeReal rel[16];
    MeReal points[12 * 3];
    MeReal* cursor;
    VincContact tmp[12];
    VincRequest* rq;
    int n, ok;
    MeI16 dims;
    VincModelGetGeometry(pair->model2);
    memset(tmp, 0, sizeof tmp); /* NOTE: uninitialised in the original: the fields after dims
                                   (element / model ids) reach the result as stack garbage */
    r = VincCylinderGetRadius(g1);
    hh = VincCylinderGetHalfHeight(g1);
    rel[0] = DBL_MAX; /* NOTE: only these four are set without a plane transform; the rest of rel is
                         read uninitialised by the original (rel is zeroed here) */
    rel[1] = rel[2] = rel[3] = 0.0;
    rel[4] = DBL_MAX;
    rel[5] = rel[6] = rel[7] = 0.0;
    rel[8] = DBL_MAX;
    rel[9] = rel[10] = rel[11] = 0.0;
    rel[12] = DBL_MAX;
    rel[13] = rel[14] = rel[15] = 0.0;
    if (tm2)
        VincTMMakeRelative(tm1, tm2, rel);
    cursor = points;
    ok = VincCylinderPlaneContactPoints(&depth, &cursor, &pad, &r, &hh, rel);
    ResultClear(result);
    if (!ok)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    n = rq->contactMaxCount;
    if (result->contactMaxCount <= n)
        n = result->contactMaxCount;
    if (n == 0)
        return 1;
    if (!tm2) {
        result->normal[0] = 0.0;
        result->normal[1] = 0.0;
        result->normal[2] = 1.0;
    } else {
        result->normal[0] = tm2[8];
        result->normal[1] = tm2[9];
        result->normal[2] = tm2[10];
    }
    dims = 2;
    if (rel[10] * rel[10] <= 0.999999)
        dims = (rel[10] * rel[10] < 1e-6) ? 3 : 1;
    ResultClear(result);
    if (cursor != points && result->contactCount < result->contactMaxCount) {
        const MeReal* p = points;
        VincContact* c = tmp;
        for (;;) {
            MeReal z = p[2];
            if (z < depth * 0.01) {
                if (!tm2) {
                    c->position[0] = p[0];
                    c->position[1] = p[1];
                    c->position[2] = p[2];
                } else {
                    VincTMTransform(tm2, p, c->position);
                }
                c->dims = (MeI16)(dims | 0x200);
                c->separation = z;
                c->normal[0] = result->normal[0];
                c->normal[1] = result->normal[1];
                c->normal[2] = result->normal[2];
                ++result->contactCount;
                ++c;
            }
            p += 3;
            if (p == cursor || !(result->contactCount < result->contactMaxCount))
                break;
        }
    }
    if (result->contactCount == 0)
        return 0;
    n = VincContactSimplify(result->normal, tmp, result->contactCount, result);
    result->contactCount = n;
    return 0 < n;
}

/* Mac 0x4f0da2, Win 0x7e0ac0 */
int MEAPI VincCylinderPlaneRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCylinderPlaneIntersect;
    VincGeometryPairRegisterInteractions(VincCylinderGetTypeId(), VincPlaneGetTypeId(), &i, 0);
    return 1;
}
