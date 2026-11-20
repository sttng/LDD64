/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / cylinder pair test (restored).
 * Mac 0x4eb907-0x4efd81 (557f), Win 0x7dda70-0x7de480 (helpers), 0x7ddc40 (safe time),
 * 0x7de480 (separating-axis core), 0x7dfd00 (intersect), 0x7dff70 (registration). Names (name
 * inferred). See Vinc/VincCylinderCylinder.h and VincCylinderCylinderCore.h. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"
#include "VincCylinderCylinder.h"
#include "VincCylinderCylinderCore.h"

#include <math.h>
#include <string.h>

/* Mac 0x4eb907 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4eb91a, Win 0x7ddc40: both cylinders as ellipsoids (r, r, r + hh) */
int MEAPI VincCylinderCylinderSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r1 = VincCylinderGetRadius(g1);
    MeReal hh1 = VincCylinderGetHalfHeight(g1);
    MeReal r2 = VincCylinderGetRadius(g2);
    MeReal hh2 = VincCylinderGetHalfHeight(g2);
    MeVector3 e1, e2;
    e1[2] = r1 + hh1;
    e2[2] = r2 + hh2;
    e2[0] = r2;
    e2[1] = r2;
    e1[0] = r1;
    e1[1] = r1;
    return VincEllipsoidSafeTime(pair, maxTime, e1, e2, result);
}

/* Mac 0x4eba07, Win 0x7ddb30. The products with sin(angles[1]) use MeSinMul (Me/MeCrtMath.h):
 * the Win build multiplies that sine unrounded. */
void VincCylinderRimGradient(MeReal* gradient, const MeReal* angles)
{
    const MeReal* g = s_cylinderRim;
    MeReal c0 = cos(angles[0]);
    MeReal s0 = sin(angles[0]);
    MeReal c1 = cos(angles[1]);
    MeReal k = g[6] * g[7];
    MeReal c0c1 = c0 * c1, c0s1 = MeSinMul(angles[1], c0), s0c1 = s0 * c1, s0s1 = MeSinMul(angles[1], s0);
    gradient[0] = -g[6] * (c0 * g[1] - s0 * g[0]) - (g[4] * c0c1 + c0s1 * g[5] - s0c1 * g[2] - s0s1 * g[3]) * k;
    gradient[1] = g[7] * (c1 * g[9] - MeSinMul(angles[1], g[8])) - k * (c0c1 * g[3] + g[5] * s0c1 - c0s1 * g[2] - g[4] * s0s1);
}

/* Mac 0x4ebbac, Win 0x7dda70 (products with sin(angles[1]) as in VincCylinderRimGradient) */
MeReal VincCylinderRimObjective(const MeReal* angles)
{
    const MeReal* g = s_cylinderRim;
    MeReal c0 = cos(angles[0]);
    MeReal s0 = sin(angles[0]);
    MeReal c1 = cos(angles[1]);
    return (MeSinMul(angles[1], g[9]) + c1 * g[8]) * g[7] - (c0 * g[0] + s0 * g[1]) * g[6]
           - g[7] * g[6] * (c0 * c1 * g[2] + MeSinMul(angles[1], c0) * g[3] + s0 * c1 * g[4] + MeSinMul(angles[1], s0) * g[5]);
}

/* Mac 0x4ebd4f, Win 0x7dde80 */
void VincCylinderClipEdges(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* hh1,
                           const MeReal* u1, const MeReal* v1, char clipFirst, const MeReal* r2,
                           const MeReal* hh2, const MeReal* u2, const MeReal* v2, char clipSecond)
{
    static const MeReal kLo = -1.0, kHi = 1.0;
    MeReal rt[9], t[3], p[3], cap[3], side[3], other[3];
    MeReal mx, my, mz;
    /* rt = rel^T (rows), t = the first cylinder's centre in the second's frame */
    rt[0] = rel[0];
    rt[1] = rel[4];
    rt[2] = rel[8];
    rt[3] = rel[1];
    rt[4] = rel[5];
    rt[5] = rel[9];
    rt[6] = rel[2];
    rt[7] = rel[6];
    rt[8] = rel[10];
    mz = -rel[14];
    my = -rel[13];
    mx = -rel[12];
    t[0] = mx * rt[0] + my * rt[3] + mz * rt[6];
    t[1] = mx * rt[1] + my * rt[4] + mz * rt[7];
    t[2] = mx * rt[2] + my * rt[5] + mz * rt[8];
    if (clipSecond) {
        MeReal rr = *r2, h;
        cap[2] = rr * u2[2];
        cap[1] = rr * u2[1];
        cap[0] = rr * u2[0];
        other[2] = rr * v2[2];
        other[1] = rr * v2[1];
        other[0] = rr * v2[0];
        h = *hh2;
        side[2] = h * rel[10];
        side[1] = h * rel[9];
        side[0] = h * rel[8];
        /* the two cap diameters */
        p[2] = rel[14] + side[2];
        p[1] = rel[13] + side[1];
        p[0] = rel[12] + side[0];
        VincClipSegmentCylinderAddInnerPoints(cursor, p, cap, r1, *hh1, &kLo, &kHi);
        p[2] = rel[14] - side[2];
        p[1] = rel[13] - side[1];
        p[0] = rel[12] - side[0];
        VincClipSegmentCylinderAddInnerPoints(cursor, p, cap, r1, *hh1, &kLo, &kHi);
        /* the four side lines */
        p[2] = rel[14] + cap[2];
        p[1] = rel[13] + cap[1];
        p[0] = rel[12] + cap[0];
        VincClipSegmentCylinderAddPoints(cursor, p, side, r1, *hh1, &kLo, &kHi);
        p[2] = rel[14] - cap[2];
        p[1] = rel[13] - cap[1];
        p[0] = rel[12] - cap[0];
        VincClipSegmentCylinderAddPoints(cursor, p, side, r1, *hh1, &kLo, &kHi);
        p[2] = other[2] + rel[14];
        p[1] = other[1] + rel[13];
        p[0] = other[0] + rel[12];
        VincClipSegmentCylinderAddPoints(cursor, p, side, r1, *hh1, &kLo, &kHi);
        p[2] = rel[14] - other[2];
        p[1] = rel[13] - other[1];
        p[0] = rel[12] - other[0];
        VincClipSegmentCylinderAddPoints(cursor, p, side, r1, *hh1, &kLo, &kHi);
    }
    if (clipFirst) {
        MeReal buf[36];
        MeReal* end = buf;
        MeReal* q;
        MeReal h;
        cap[0] = (u1[0] * rt[0] + u1[1] * rt[3] + u1[2] * rt[6]) * *r1;
        cap[1] = (u1[0] * rt[1] + u1[1] * rt[4] + u1[2] * rt[7]) * *r1;
        cap[2] = (u1[0] * rt[2] + u1[1] * rt[5] + u1[2] * rt[8]) * *r1;
        other[0] = (v1[0] * rt[0] + v1[1] * rt[3] + v1[2] * rt[6]) * *r1;
        other[1] = (v1[0] * rt[1] + v1[1] * rt[4] + v1[2] * rt[7]) * *r1;
        other[2] = (v1[0] * rt[2] + v1[1] * rt[5] + v1[2] * rt[8]) * *r1;
        h = *hh1;
        side[2] = h * rt[8];
        side[1] = h * rt[7];
        side[0] = h * rt[6];
        p[2] = t[2] + side[2];
        p[1] = t[1] + side[1];
        p[0] = t[0] + side[0];
        VincClipSegmentCylinderAddInnerPoints(&end, p, cap, r2, *hh2, &kLo, &kHi);
        p[2] = t[2] - side[2];
        p[1] = t[1] - side[1];
        p[0] = t[0] - side[0];
        VincClipSegmentCylinderAddInnerPoints(&end, p, cap, r2, *hh2, &kLo, &kHi);
        p[2] = t[2] + cap[2];
        p[1] = t[1] + cap[1];
        p[0] = t[0] + cap[0];
        VincClipSegmentCylinderAddPoints(&end, p, side, r2, *hh2, &kLo, &kHi);
        p[2] = t[2] - cap[2];
        p[1] = t[1] - cap[1];
        p[0] = t[0] - cap[0];
        VincClipSegmentCylinderAddPoints(&end, p, side, r2, *hh2, &kLo, &kHi);
        p[2] = other[2] + t[2];
        p[1] = other[1] + t[1];
        p[0] = other[0] + t[0];
        VincClipSegmentCylinderAddPoints(&end, p, side, r2, *hh2, &kLo, &kHi);
        p[2] = t[2] - other[2];
        p[1] = t[1] - other[1];
        p[0] = t[0] - other[0];
        VincClipSegmentCylinderAddPoints(&end, p, side, r2, *hh2, &kLo, &kHi);
        for (q = buf; q != end; q += 3) {
            MeReal* o = *cursor;
            *cursor = o + 3;
            o[0] = q[0] * rel[0] + q[1] * rel[4] + q[2] * rel[8] + rel[12];
            o[1] = q[0] * rel[1] + q[1] * rel[5] + q[2] * rel[9] + rel[13];
            o[2] = q[0] * rel[2] + q[1] * rel[6] + q[2] * rel[10] + rel[14];
        }
    }
}

/* Mac 0x4ec9cf, Win 0x7ddcd0 */
void VincCylinderFrame(MeReal* u, MeReal* v, const MeReal* a, const MeReal* b, const MeReal* c)
{
    MeReal x, y, z, len;
    x = a[1] * b[2] - a[2] * b[1];
    v[0] = x;
    y = a[2] * b[0] - b[2] * a[0];
    v[1] = y;
    z = b[1] * a[0] - a[1] * b[0];
    v[2] = z;
    if (x * x + y * y + z * z < 1e-15) {
        x = a[1] * c[2] - a[2] * c[1];
        v[0] = x;
        y = a[2] * c[0] - c[2] * a[0];
        v[1] = y;
        z = c[1] * a[0] - a[1] * c[0];
        v[2] = z;
        if (x * x + y * y + z * z < 1e-15) {
            /* perpendicular to a: zero at the smallest component */
            MeReal w[3];
            int i = a[0] > a[1] ? 1 : 0;
            int j, k;
            if (a[i] > a[2])
                i = 2;
            j = (i + 1 + (i >> 1)) & 3;
            k = (j + 1 + (j >> 1)) & 3;
            w[i] = 0.0;
            w[j] = a[k];
            w[k] = -a[j];
            v[0] = w[0];
            v[1] = w[1];
            v[2] = w[2];
        }
    }
    x = v[0];
    y = v[1];
    z = v[2];
    len = sqrt(x * x + y * y + z * z);
    if (len != 0.0) {
        len = 1.0 / len;
        v[0] = x * len;
        v[1] = y * len;
        v[2] = len * z;
    }
    u[0] = v[1] * a[2] - v[2] * a[1];
    u[1] = v[2] * a[0] - a[2] * v[0];
    u[2] = a[1] * v[0] - v[1] * a[0];
}

/* Mac 0x4ef78d, Win 0x7dfd00. The contact points of the core (first cylinder's frame) whose
 * separation along the normal is below 1 % of the separation become contacts (normal = the
 * result's normal, dims of the core; the element fields are not written), reduced by
 * VincContactSimplify. */
int MEAPI VincCylinderCylinderIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincContact contacts[48];
    MeReal points[48 * 3];
    MeReal* cursor = points;
    MeReal rel[16], n[3], sep, depth, pad, r1, hh1, r2, hh2;
    MeReal *tm1, *tm2;
    VincGeometryID g1, g2;
    MeI16 dims;
    int maxCount, count;
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    g1 = VincModelGetGeometry(pair->model1);
    g2 = VincModelGetGeometry(pair->model2);
    r1 = VincCylinderGetRadius(g1);
    hh1 = VincCylinderGetHalfHeight(g1);
    r2 = VincCylinderGetRadius(g2);
    hh2 = VincCylinderGetHalfHeight(g2);
    VincTMMakeRelative(tm2, tm1, rel);
    if (!VincCylinderCylinderSAT(&sep, n, &depth, &cursor, &dims, &pad, &r1, &hh1, &r2, &hh2, rel)) {
        ResultClear(result);
        return 0;
    }
    maxCount = VincModelPairGetRequestPtr(pair)->contactMaxCount;
    if (result->contactMaxCount <= maxCount)
        maxCount = result->contactMaxCount;
    if (maxCount == 0) {
        ResultClear(result);
        return 1;
    }
    result->normal[0] = n[0] * tm1[0] + n[1] * tm1[4] + n[2] * tm1[8];
    result->normal[1] = n[0] * tm1[1] + n[1] * tm1[5] + n[2] * tm1[9];
    result->normal[2] = n[0] * tm1[2] + n[1] * tm1[6] + n[2] * tm1[10];
    ResultClear(result);
    if (points != cursor && result->contactCount < result->contactMaxCount) {
        const MeReal* p = points;
        VincContact* c = contacts;
        for (;;) {
            MeReal d = p[0] * n[0] + p[1] * n[1] + p[2] * n[2] - depth;
            if (d < sep * 0.01) {
                c->position[0] = p[0] * tm1[0] + p[1] * tm1[4] + p[2] * tm1[8] + tm1[12];
                c->position[1] = p[0] * tm1[1] + p[1] * tm1[5] + p[2] * tm1[9] + tm1[13];
                c->position[2] = p[0] * tm1[2] + p[1] * tm1[6] + p[2] * tm1[10] + tm1[14];
                c->dims = dims;
                c->separation = d;
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
    count = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    result->contactCount = count;
    return 0 < count;
}

/* (not in the binaries; tests) this file's copy of the separating-axis core, Win 0x7de480 */
int VincCylinderCylinderSATForTests(MeReal* sep, MeReal* normal, MeReal* depth, MeReal** cursor, MeI16* dims,
                                    const MeReal* pad, const MeReal* r1, const MeReal* hh1, const MeReal* r2,
                                    const MeReal* hh2, const MeReal* rel)
{
    return VincCylinderCylinderSAT(sep, normal, depth, cursor, dims, pad, r1, hh1, r2, hh2, rel);
}

/* (not in the binaries; tests) this file's rim data (Win 0xafa198) */
const MeReal* VincCylinderRimDataForTests(void)
{
    return s_cylinderRim;
}

/* Mac 0x4ebcd5, Win 0x7dff70 */
int MEAPI VincCylinderCylinderRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCylinderCylinderIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincCylinderCylinderSafeTime;
    VincGeometryPairRegisterInteractions(VincCylinderGetTypeId(), VincCylinderGetTypeId(), &i, 0);
    return 1;
}
