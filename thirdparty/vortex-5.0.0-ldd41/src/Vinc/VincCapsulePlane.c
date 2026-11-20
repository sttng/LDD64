/* Vortex 5.0.0 build 800 "vortexldd41" - capsule / plane pair test (restored).
 * Mac 0x4e3e62-0x4e4943 (557e), Win 0x7db180-0x7db6a0. Names (name inferred). The plane is z <= 0
 * of model2's transform. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4e3e62 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4e3e75, Win 0x7db180 (cdecl): the capsule (radius *r, half height *hh; rel = capsule in
 * the plane frame, axis z = rel row 2) against z <= 0. *sep = height of its lowest point; when
 * <= *pad, candidate points (plane frame) are appended at *cursor with a flag each: the end-sphere
 * generators (segment end -+ (0, 0, r) clipped) +1 top / -1 bottom, and, unless both ends added
 * points, the two shaft lines in the plane through the axis (flag 0). Returns 1 when *sep <= *pad
 * (even without points). */
static int CapsulePlaneContact(MeReal* sep, MeReal** cursor, const MeReal* pad, const MeReal* r,
                               const MeReal* hh, const MeReal* rel, int* flags)
{
    const MeReal* z = rel + 8;
    MeReal w0 = z[1], w1 = -z[0], w2 = 0.0, len2, u0, u1, u2, s;
    len2 = (w0 * w0 + w1 * w1) + w2 * w2;
    if (1e-15 <= len2 || len2 != len2) {
        MeReal len = sqrt(len2);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            w0 = inv * w0;
            w1 = w1 * inv;
            w2 = 0.0 * inv;
        }
        u0 = z[2] * w1 - z[1] * w2;
        u1 = w2 * z[0] - z[2] * w0;
        u2 = z[1] * w0 - w1 * z[0];
    } else {
        u0 = 1.0;
        u1 = 0.0;
        u2 = 0.0;
    }
    s = rel[14] - (fabs(z[2]) * *hh + *r);
    *sep = s;
    if (!(s <= *pad))
        return 0;
    {
        MeReal Z[3], v[3], p[3], invZ, invR;
        MeReal* start = *cursor;
        MeReal* q;
        int top = 0, bottom = 0;
        Z[0] = *hh * z[0];
        Z[1] = z[1] * *hh;
        Z[2] = z[2] * *hh;
        v[0] = 0.0;
        v[1] = 0.0;
        v[2] = *r;
        invZ = 1.0 / Z[2];
        invR = 1.0 / *r;
        p[0] = Z[0] + rel[12];
        p[1] = Z[1] + rel[13];
        p[2] = Z[2] + rel[14];
        VincClipSegmentAddPoints(cursor, p, v, &invR);
        for (q = start; q < *cursor; q += 3) {
            *flags++ = 1;
            top = 1;
        }
        start = *cursor;
        p[0] = rel[12] - Z[0];
        p[1] = rel[13] - Z[1];
        p[2] = rel[14] - Z[2];
        VincClipSegmentAddPoints(cursor, p, v, &invR);
        for (q = start; q < *cursor; q += 3) {
            *flags++ = -1;
            bottom = 1;
        }
        start = *cursor;
        if (!top || !bottom) {
            p[0] = *r * u0 + rel[12];
            p[1] = *r * u1 + rel[13];
            p[2] = *r * u2 + rel[14];
            VincClipSegmentAddPoints(cursor, p, Z, &invZ);
            p[0] = rel[12] - *r * u0;
            p[1] = rel[13] - *r * u1;
            p[2] = rel[14] - *r * u2;
            VincClipSegmentAddPoints(cursor, p, Z, &invZ);
            for (q = start; q < *cursor; q += 3)
                *flags++ = 0;
        }
    }
    return 1;
}

/* Mac 0x4e4315, Win 0x7db3f0 (model1 = capsule, model2 = plane): contacts at the candidate points
 * below 1 % of the separation, at most result->contactMaxCount; element1 = the point's flag,
 * dims 0x200 | (2 axis normal to the plane, 3 parallel, 1 else). NOTE (both builds): without a
 * plane transform the relative transform is uninitialised (here: DBL_MAX / 0, as the original's
 * initialised part). */
int MEAPI VincCapsulePlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    MeReal r, hh, rel[16], points[16 * 3], sep;
    MeReal *cursor = points, *p;
    VincContact contacts[16];
    int flags[16], cnt, i;
    VincRequest* rq;
    MeI16 dims;
    MeReal c2;
    (void)VincModelGetGeometry(pair->model2);
    r = VincCapsuleGetRadius(g1);
    hh = VincCapsuleGetHalfHeight(g1);
    memset(rel, 0, sizeof rel);
    rel[0] = rel[4] = rel[8] = rel[12] = DBL_MAX;
    if (tm2 != 0)
        VincTMMakeRelative(tm1, tm2, rel);
    i = CapsulePlaneContact(&sep, &cursor, &pad, &r, &hh, rel, flags);
    ResultClear(result);
    if (!i)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0)
        return 1;
    if (tm2 == 0) {
        result->normal[0] = 0.0;
        result->normal[1] = 0.0;
        result->normal[2] = 1.0;
    } else {
        result->normal[0] = tm2[8];
        result->normal[1] = tm2[9];
        result->normal[2] = tm2[10];
    }
    c2 = rel[10] * rel[10];
    if (c2 <= 0.999999)
        dims = (MeI16)((1e-06 <= c2) ? 1 : 3);
    else
        dims = 2;
    dims = (MeI16)(dims | 0x200);
    ResultClear(result);
    for (p = points, i = 0; p != cursor; p += 3, ++i) {
        MeReal depth;
        if (result->contactMaxCount <= result->contactCount)
            break;
        depth = p[2];
        if (depth < sep * 0.01) {
            VincContact* c = &contacts[result->contactCount];
            if (tm2 == 0) {
                c->position[0] = p[0];
                c->position[1] = p[1];
                c->position[2] = p[2];
            } else {
                VincTMTransform(tm2, p, c->position);
            }
            c->separation = depth;
            c->dims = dims;
            c->element1 = flags[i];
            c->element2 = 0;
            c->normal[0] = result->normal[0];
            c->normal[1] = result->normal[1];
            c->normal[2] = result->normal[2];
            ++result->contactCount;
        }
    }
    if (result->contactCount == 0)
        return 0;
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    return result->contactCount > 0;
}

/* Mac 0x4e429f, Win 0x7db660 */
int MEAPI VincCapsulePlaneRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCapsulePlaneIntersect;
    VincGeometryPairRegisterInteractions(VincCapsuleGetTypeId(), VincPlaneGetTypeId(), &i, 0);
    return 1;
}
