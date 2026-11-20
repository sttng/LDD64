/* Vortex 5.0.0 build 800 "vortexldd41" - box / sphere pair test (restored).
 * Mac 0x4e0e0a-0x4e17b6 (557e), Win 0x81e240-0x81e7e0. Names (name inferred) after the Mcd
 * pair-test naming; the pair-table cell (Box, Sphere) of the original holds the Win addresses. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4e0e0a (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4e0e1d, Win 0x81e500: the box and the sphere (as a cube of half extent r) swept */
int MEAPI VincBoxSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    MeReal sr = VincSphereGetRadius(VincModelGetGeometry(pair->model2));
    MeReal* radii = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    MeVector3 e2;
    e2[0] = sr;
    e2[1] = sr;
    e2[2] = sr;
    return VincEllipsoidSafeTime(pair, maxTime, radii, e2, result);
}

/* Mac 0x4e0ead, Win 0x81e240 (cdecl; also used by the box / capsule test): the sphere (centre c
 * in the box frame, radius *sr) against the box (half extents radii). Centre farther than *pad
 * from the box: normal = nearest box point - c normalised (dims 0x300 | 3 - clamped axes: 0x300
 * vertex, 0x301 edge, 0x302 face); else the nearest face, normal = minus its outward normal (dims
 * 0x302). *sep = separation, point = c + sr n. Returns 1 when the box is within *pad + *sr of c. */
int VincBoxSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                         const MeReal* radii, const MeReal* sr, const MeReal* c)
{
    MeReal d2 = 0.0, best, s = 0.0;
    int clamped = 0, i, axis;
    n[2] = 0.0;
    n[1] = 0.0;
    n[0] = 0.0;
    for (i = 0; i < 3; ++i) {
        MeReal ci = c[i], ri = radii[i];
        if (!(ci <= ri)) {
            MeReal d = ri - ci;
            d2 = d * d + d2;
            n[i] = d;
            ++clamped;
        } else if (ci < -ri) {
            MeReal d = -ri - ci;
            d2 = d * d + d2;
            n[i] = d;
            ++clamped;
        }
    }
    if (!(d2 <= (*pad + *sr) * (*pad + *sr)))
        return 0;
    if (d2 <= *pad * *pad) {
        /* the centre is (nearly) inside: push out through the nearest face */
        best = DBL_MAX;
        axis = 0;
        for (i = 0; i < 3; ++i) {
            MeReal d = fabs(radii[i] - c[i]);
            if (d < best) {
                axis = i;
                best = d;
                s = -1.0;
            }
            d = fabs(radii[i] + c[i]);
            if (d < best) {
                axis = i;
                best = d;
                s = 1.0;
            }
        }
        n[axis] = s; /* NOTE: s is uninitialised in the original when no distance is < DBL_MAX */
        *sep = (fabs(c[axis]) - radii[axis]) - *sr;
        point[0] = n[0] * *sr + c[0];
        point[1] = c[1] + n[1] * *sr;
        point[2] = c[2] + *sr * n[2];
        *dims = 0x302;
        return 1;
    } else {
        MeReal len, inv, nx, ny, nz;
        *dims = (MeI16)((3 - clamped) | 0x300);
        len = sqrt(d2);
        inv = 1.0 / len;
        nx = n[0];
        n[0] = nx * inv;
        ny = n[1];
        n[1] = ny * inv;
        nz = n[2];
        n[2] = inv * nz;
        point[0] = nx * inv * *sr + c[0];
        point[1] = c[1] + ny * inv * *sr;
        point[2] = c[2] + inv * nz * *sr;
        *sep = len - *sr;
        return 1;
    }
}

/* Mac 0x4e11bb, Win 0x81e560 (model1 = box, model2 = sphere). NOTE: the group normal stays in the
 * box frame (both builds). */
int MEAPI VincBoxSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* radii = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    MeReal sr = VincSphereGetRadius(VincModelGetGeometry(pair->model2));
    MeVector3 c, n, p;
    MeReal sep;
    MeI16 dims;
    VincRequest* rq;
    int touch, cnt;
    VincTMInverseTransform(tm1, tm2 + 12, c);
    touch = VincBoxSphereContact(&sep, n, p, &dims, &pad, radii, &sr, c);
    ResultClear(result);
    if (!touch)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0)
        return 1;
    if (cnt < 1) {
        ResultClear(result);
        return 0;
    } else {
        VincContact* ct = result->contacts;
        VincTMRotate(tm1, n, result->normal);
        ct->dims = dims;
        ct->separation = sep;
        VincTMTransform(tm1, p, ct->position);
        ct->normal[0] = result->normal[0];
        ct->normal[1] = result->normal[1];
        ct->normal[2] = result->normal[2];
        result->contactCount = 1;
        if (rq->contactGroupMaxCount > 0) {
            result->contactGroupCount = 1;
            result->groups[0].count = 1;
            result->groups[0].normal[0] = n[0];
            result->groups[0].normal[1] = n[1];
            result->groups[0].normal[2] = n[2];
            if (rq->m24 == 0) {
                result->groups[0].m04 = 1.0;
            } else {
                MeReal a = sr;
                if (sep < -sr) {
                    MeReal q = pad;
                    if (q < -sep)
                        q = -sep;
                    a = sqrt(sr * sr - (q + sr) * (sr + q)); /* NOTE: NaN for a deep sphere, as in the original */
                }
                result->groups[0].m04 = a * 3.141592653589793 * a;
            }
        }
        return 1;
    }
}

/* Mac 0x4e1141, Win 0x81e790 */
int MEAPI VincBoxSphereRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincBoxSphereIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincBoxSphereSafeTime;
    VincGeometryPairRegisterInteractions(VincBoxGetTypeId(), VincSphereGetTypeId(), &i, 0);
    return 1;
}
