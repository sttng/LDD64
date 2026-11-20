/* Vortex 5.0.0 build 800 "vortexldd41" - capsule / sphere pair test (restored).
 * Mac 0x4e4943-0x4e5374 (557e), Win 0x7dac70-0x7db170. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4e4943 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4e4956, Win 0x7dae60: the capsule (as a box r, r, r + hh) and the sphere (cube sr) swept */
int MEAPI VincCapsuleSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCapsuleGetRadius(g1);
    MeReal hh = VincCapsuleGetHalfHeight(g1);
    MeReal sr = VincSphereGetRadius(g2);
    MeVector3 e1, e2;
    e1[0] = r;
    e1[1] = r;
    e1[2] = r + hh;
    e2[0] = sr;
    e2[1] = sr;
    e2[2] = sr;
    return VincEllipsoidSafeTime(pair, maxTime, e1, e2, result);
}

/* Mac 0x4e4a99, Win 0x7dac70 (cdecl; also used by the capsule / capsule test): a sphere (centre c
 * in the capsule frame, radius *sr) against the capsule (radius *r, half height *hh along z): the
 * shaft when |c z| < hh (*element = 0), else the nearer end sphere (*element = -1 bottom / +1 top).
 * n points from c towards the axis / end centre, point = c + sr n, dims 0x301. Returns 1 on overlap
 * (separation <= 0), 0 when c is within 0.001 sr of the axis / end centre. pad: unused (NOTE: no
 * padding, both builds). */
int VincCapsuleSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                             const MeReal* r, const MeReal* hh, const MeReal* sr, const MeReal* c, int* element)
{
    MeReal R = *r + *sr;
    MeReal ac2 = fabs(c[2]), m;
    (void)pad;
    if (*hh + R < ac2)
        return 0;
    m = fabs(c[1]);
    if (!(fabs(c[0]) < m))
        m = fabs(c[0]);
    if (R < m)
        return 0;
    if (ac2 < *hh) {
        MeReal len, s;
        n[0] = -c[0];
        n[1] = -c[1];
        n[2] = 0.0;
        len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            n[0] = n[0] * inv;
            n[1] = inv * n[1];
            n[2] = inv * n[2];
        }
        if (!(*sr * 0.001 < len))
            return 0;
        s = len - R;
        *sep = s;
        if (0.0 < s)
            return 0;
        point[0] = *sr * n[0] + c[0];
        point[1] = *sr * n[1] + c[1];
        point[2] = *sr * n[2] + c[2];
        *dims = 0x301;
        *element = 0;
        return 1;
    } else {
        MeReal len, inv;
        n[0] = c[0];
        n[1] = c[1];
        n[2] = (0.0 < c[2]) ? c[2] - *hh : *hh + c[2];
        len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        *sep = len;
        if (R < len)
            return 0;
        if (len < *sr * 0.001)
            return 0;
        inv = 1.0 / -len;
        n[0] = n[0] * inv;
        n[1] = n[1] * inv;
        n[2] = inv * n[2];
        *sep = *sep - R;
        point[0] = *sr * n[0] + c[0];
        point[1] = *sr * n[1] + c[1];
        point[2] = *sr * n[2] + c[2];
        *dims = 0x301;
        *element = (0.0 < c[2]) ? 1 : -1;
        return 1;
    }
}

/* Mac 0x4e4d52, Win 0x7daee0 (model1 = capsule, model2 = sphere): one contact (element1 = the
 * end / shaft flag); group as for the sphere tests. NOTE: the group normal stays in the capsule
 * frame (both builds). */
int MEAPI VincCapsuleSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCapsuleGetRadius(g1);
    MeReal hh = VincCapsuleGetHalfHeight(g1);
    MeReal sr = VincSphereGetRadius(g2);
    MeVector3 c, n, p;
    MeReal sep;
    MeI16 dims;
    int element, touch, cnt;
    VincRequest* rq;
    VincTMInverseTransform(tm1, tm2 + 12, c);
    touch = VincCapsuleSphereContact(&sep, n, p, &dims, &pad, &r, &hh, &sr, c, &element);
    ResultClear(result);
    if (!touch)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0)
        return 1;
    if (result->contactMaxCount < 1) {
        ResultClear(result);
        return 1;
    } else {
        VincContact* ct = result->contacts;
        VincTMRotate(tm1, n, result->normal);
        ct->dims = dims;
        ct->element1 = element;
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
                    a = sqrt(sr * sr - (q + sr) * (sr + q)); /* NOTE: NaN, as in the original */
                }
                result->groups[0].m04 = a * 3.141592653589793 * a;
            }
        }
        return 1;
    }
}

/* Mac 0x4e4a1f, Win 0x7db120 */
int MEAPI VincCapsuleSphereRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCapsuleSphereIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincCapsuleSphereSafeTime;
    VincGeometryPairRegisterInteractions(VincCapsuleGetTypeId(), VincSphereGetTypeId(), &i, 0);
    return 1;
}
