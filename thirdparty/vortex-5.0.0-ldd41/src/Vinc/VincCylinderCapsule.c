/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / capsule pair test (restored).
 * Mac 0x4e7f02-0x4eb907 (557f), Win 0x7d83c0 (safe time), 0x7d8450 (separating-axis core,
 * VincCylinderCylinderCore.h), 0x7d9d30 (contacts), 0x7da1a0 (intersect), 0x7da460
 * (registration). Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"
#include "VincCylinderCylinder.h"
#include "VincCylinderCylinderCore.h"

#include <math.h>
#include <string.h>

/* Mac 0x4e7f02 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4e7f15, Win 0x7d83c0: the cylinder as the ellipsoid (r, r, r + hh), the capsule as
 * (r, r, r + hh) */
int MEAPI VincCylinderCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r1 = VincCylinderGetRadius(g1);
    MeReal hh1 = VincCylinderGetHalfHeight(g1);
    MeReal r2 = VincCapsuleGetRadius(g2);
    MeReal hh2 = VincCapsuleGetHalfHeight(g2);
    MeVector3 e1, e2;
    e1[2] = r1 + hh1;
    e2[2] = r2 + hh2;
    e2[0] = r2;
    e2[1] = r2;
    e1[0] = r1;
    e1[1] = r1;
    return VincEllipsoidSafeTime(pair, maxTime, e1, e2, result);
}

/* Mac 0x4eabd6, Win 0x7d9d30: the capsule (radius *r2, segment of half length *hh2 along rel's
 * row 2, centre rel row 3) against the cylinder (*r1, *hh1) in the cylinder's frame. Fills
 * seps / normals / points (first-cylinder frame) and the element ids (-1 / 1: end spheres, 0:
 * segment), *dims; returns the number of contacts.
 * 1. each end sphere against the cylinder (VincCylinderSphereContact), kept when its point is
 *    not beyond the segment end (within *pad);
 * 2. with fewer than two: the segment against the rim circle line of the cylinder (the closest
 *    point of the segment to the cylinder's axis), or the separating-axis core with the capsule
 *    as a cylinder for the rest.
 * NOTE: when both end spheres touch, 1 is returned (the second contact is written but not
 * counted), as in the original. */
static int CapsuleContacts(MeReal* seps, MeReal* normals, MeReal* points, MeI16* dims, const MeReal* pad,
                           const MeReal* r1, const MeReal* hh1, const MeReal* r2, const MeReal* hh2,
                           const MeReal* rel, int* elements)
{
    const MeReal* a = rel + 8;
    const MeReal* c = rel + 12;
    MeReal e0[3], e1[3], m;
    MeReal* p;
    int n;
    e0[0] = c[0] - *hh2 * a[0];
    e0[1] = c[1] - *hh2 * a[1];
    e0[2] = c[2] - *hh2 * a[2];
    e1[0] = *hh2 * a[0] + c[0];
    e1[1] = *hh2 * a[1] + c[1];
    e1[2] = *hh2 * a[2] + c[2];
    n = 0;
    if (VincCylinderSphereContact(seps, normals, points, dims, &m, pad, r1, hh1, r2, e0)
        && (points[0] - e0[0]) * a[0] + (points[1] - e0[1]) * a[1] + (points[2] - e0[2]) * a[2] <= *pad) {
        elements[0] = -1;
        n = 1;
    }
    p = points + n * 3;
    if (VincCylinderSphereContact(seps + n, normals + n * 3, p, dims, &m, pad, r1, hh1, r2, e1)
        && (p[0] - e1[0]) * a[0] + (p[1] - e1[1]) * a[1] + (p[2] - e1[2]) * a[2] >= -*pad) {
        elements[n] = 1;
        ++n;
    }
    if (n == 2)
        return 1;
    {
        MeReal dz = (a[2] + a[2]) * *hh2;
        MeReal dy = (a[1] + a[1]) * *hh2;
        MeReal dx = (a[0] + a[0]) * *hh2;
        MeReal h = *hh1, h2 = h + h, cx, cy, cz, den, t, s, z0, x, y, len, inv, d;
        MeReal* nn;
        cx = dy * h2 - dz * 0.0;
        cy = dz * 0.0 - dx * h2;
        cz = dx * 0.0 - dy * 0.0;
        den = cx * cx + cy * cy + cz * cz;
        if (!(0.0 < den))
            return n;
        z0 = h + e0[2];
        t = (cx * (z0 * 0.0 - h2 * e0[1]) + (h2 * e0[0] - z0 * 0.0) * cy + (e0[1] * 0.0 - e0[0] * 0.0) * cz) / den;
        p = points + n * 3;
        x = e0[0] + dx * t;
        p[0] = x;
        p[1] = dy * t + e0[1];
        p[2] = e0[2] + dz * t;
        if (t >= 0.0 && t <= 1.0
            && (s = (cx * (dy * z0 - dz * e0[1]) + (dz * e0[0] - dx * z0) * cy + (dx * e0[1] - dy * e0[0]) * cz) / den,
                s >= 0.0 && s <= 1.0)) {
            /* the segment passes the axis line within both ranges: radial contact */
            nn = normals + n * 3;
            nn[0] = x;
            y = p[1];
            nn[1] = y;
            nn[2] = 0.0;
            len = sqrt(x * nn[0] + y * nn[1] + 0.0);
            if (len == 0.0) {
                seps[n] = len - (*r1 + *r2);
            } else {
                inv = 1.0 / len;
                nn[0] = x * inv;
                nn[1] = y * inv;
                nn[2] = inv * 0.0;
                d = len - (*r1 + *r2);
                seps[n] = d;
                if (0.0 < len && d < 0.0) {
                    p[0] = nn[0] * *r1;
                    p[1] = nn[1] * *r1;
                    nn[0] = -nn[0];
                    nn[1] = -nn[1];
                    elements[n] = 0;
                    ++n;
                }
            }
        } else {
            MeReal depth;
            MeReal* cursor = p;
            nn = normals + n * 3;
            if (VincCylinderCylinderSAT(seps + n, nn, &depth, &cursor, dims, pad, r1, hh1, r2, hh2, rel)) {
                MeReal nx = nn[0], ny = nn[1], nz = nn[2];
                for (; p < cursor; p += 3) {
                    elements[n] = 0;
                    nn = normals + n * 3;
                    nn[0] = nx;
                    nn[1] = ny;
                    nn[2] = nz;
                    seps[n] = nx * p[0] + ny * p[1] + nz * p[2] - depth;
                    ++n;
                }
            }
        }
    }
    return n;
}

/* Mac 0x4eb2b2, Win 0x7da1a0 (model1 = cylinder, model2 = capsule).
 * NOTE: the original's stack arrays hold 16 contacts (Mac; Win: 16 separations, about 19
 * contacts) while CapsuleContacts can deliver up to 25 (an end sphere + 24 clipped edge points):
 * such a case overruns them in the original (UB); sized 32 here. */
int MEAPI VincCylinderCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincContact contacts[32];
    MeReal seps[32], normals[32 * 3], points[32 * 3], rel[16], pad, r1, hh1, r2, hh2;
    int elements[32];
    MeReal *tm1, *tm2;
    VincGeometryID g1, g2;
    MeI16 dims;
    int n, k, maxCount;
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    g1 = VincModelGetGeometry(pair->model1);
    g2 = VincModelGetGeometry(pair->model2);
    r1 = VincCylinderGetRadius(g1);
    hh1 = VincCylinderGetHalfHeight(g1);
    r2 = VincCapsuleGetRadius(g2);
    hh2 = VincCapsuleGetHalfHeight(g2);
    VincTMMakeRelative(tm2, tm1, rel);
    dims = 0;
    n = CapsuleContacts(seps, normals, points, &dims, &pad, &r1, &hh1, &r2, &hh2, rel, elements);
    if (n == 0) {
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
    ResultClear(result);
    for (k = 0; k < n; ++k) {
        VincContact* c = &contacts[k];
        const MeReal* nn = normals + k * 3;
        const MeReal* p = points + k * 3;
        c->normal[0] = nn[0] * tm1[0] + nn[1] * tm1[4] + nn[2] * tm1[8];
        c->normal[1] = nn[0] * tm1[1] + nn[1] * tm1[5] + nn[2] * tm1[9];
        c->normal[2] = nn[0] * tm1[2] + nn[1] * tm1[6] + nn[2] * tm1[10];
        c->separation = seps[k];
        c->element2 = elements[k];
        c->element1 = 0;
        c->dims = dims;
        c->position[0] = p[0] * tm1[0] + p[1] * tm1[4] + p[2] * tm1[8] + tm1[12];
        c->position[1] = p[0] * tm1[1] + p[1] * tm1[5] + p[2] * tm1[9] + tm1[13];
        c->position[2] = p[0] * tm1[2] + p[1] * tm1[6] + p[2] * tm1[10] + tm1[14];
        ++result->contactCount;
    }
    result->normal[0] = contacts[0].normal[0];
    result->normal[1] = contacts[0].normal[1];
    result->normal[2] = contacts[0].normal[2];
    n = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    result->contactCount = n;
    return 0 < n;
}

/* Mac 0x4e8002, Win 0x7da460 */
int MEAPI VincCylinderCapsuleRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCylinderCapsuleIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincCylinderCapsuleSafeTime;
    VincGeometryPairRegisterInteractions(VincCylinderGetTypeId(), VincCapsuleGetTypeId(), &i, 0);
    return 1;
}
