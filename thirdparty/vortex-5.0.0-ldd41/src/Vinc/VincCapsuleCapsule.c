/* Vortex 5.0.0 build 800 "vortexldd41" - capsule / capsule pair test (restored).
 * Mac 0x4e272f-0x4e3b23 (557e), Win 0x7d7960-0x7d8380. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4e272f (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4e2742, Win 0x7d7a60: both capsules as boxes (r, r, r + hh) swept */
int MEAPI VincCapsuleCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r1 = VincCapsuleGetRadius(g1);
    MeReal hh1 = VincCapsuleGetHalfHeight(g1);
    MeReal r2 = VincCapsuleGetRadius(g2);
    MeReal hh2 = VincCapsuleGetHalfHeight(g2);
    MeVector3 e1, e2;
    e1[0] = r1;
    e1[1] = r1;
    e1[2] = r1 + hh1;
    e2[0] = r2;
    e2[1] = r2;
    e2[2] = r2 + hh2;
    return VincEllipsoidSafeTime(pair, maxTime, e1, e2, result);
}

/* Mac 0x4e28a9, Win 0x7d7960 (cdecl): a sphere (centre c in the capsule frame, radius *sr) against
 * the capsule's shaft only (radius *r, half height *hh): n from c towards the axis (minus c's
 * radial direction), point = c + sr n, dims 0x301; 1 when overlapping (separation <= 0).
 * pad: unused. */
static int SphereShaftContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                              const MeReal* r, const MeReal* hh, const MeReal* sr, const MeReal* c)
{
    MeReal ac2 = fabs(c[2]);
    MeReal R = *r + *sr;
    MeReal m, len, s;
    (void)pad;
    if (!((*r + *hh) + *sr >= ac2))
        return 0;
    m = fabs(c[0]);
    if (m < fabs(c[1]))
        m = fabs(c[1]);
    if (!(m < R || m == R))
        return 0;
    if (!(ac2 < *hh))
        return 0;
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
    if (!(0.0 >= s))
        return 0;
    point[0] = *sr * n[0] + c[0];
    point[1] = *sr * n[1] + c[1];
    point[2] = *sr * n[2] + c[2];
    *dims = 0x301;
    return 1;
}

/* Mac 0x4e2a36, Win 0x7d7af0 (cdecl): capsule 2 (rel = capsule 2 in capsule 1's frame) against
 * capsule 1 (radius *r1, half height *hh1 along z): (1) the closest points of the two axes when
 * they are interior to both segments (one contact, normal across the axes, element1 = element2 =
 * 0); otherwise (2) capsule 2's end spheres against capsule 1 (VincCapsuleSphereContact: element2
 * -1 / +1, element1 = the end / shaft flag) and, unless both hit, (3) capsule 1's end spheres
 * against capsule 2's shaft (element1 -1 / +1, element2 0; normal / point moved to capsule 1's
 * frame). Writes up to 4 contacts into the arrays (sep[k], n[3k], point[3k], dims[k], e1[k],
 * e2[k]); returns their number. */
static int CapsuleCapsuleContacts(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                                  const MeReal* r1, const MeReal* hh1, const MeReal* r2, const MeReal* hh2,
                                  const MeReal* rel, int* e1, int* e2)
{
    MeReal b0, b1, b2, a2, c0, c1, c2, cc;
    MeReal P[3], P2[3], E[3];
    int k, e;
    /* axis 2 = P + t b (t in [0, 1]), axis 1 = Q + s a, Q = (0, 0, -hh1), a = (0, 0, 2 hh1) */
    b0 = (rel[8] * 2.0) * *hh2;
    b1 = (rel[9] * 2.0) * *hh2;
    b2 = (rel[10] * 2.0) * *hh2;
    a2 = *hh1 * 2.0;
    P[0] = rel[12] - *hh2 * rel[8];
    P[1] = rel[13] - rel[9] * *hh2;
    P[2] = rel[14] - rel[10] * *hh2;
    P2[0] = rel[8] * *hh2 + rel[12];
    P2[1] = rel[9] * *hh2 + rel[13];
    P2[2] = rel[14] + rel[10] * *hh2;
    c0 = a2 * b1 - b2 * 0.0;
    c1 = b2 * 0.0 - a2 * b0;
    c2 = b0 * 0.0 - b1 * 0.0;
    cc = (c0 * c0 + c1 * c1) + c2 * c2;
    if (0.0 < cc) {
        MeReal d0 = P[0] - 0.0, d1 = P[1] - 0.0, d2 = P[2] - -*hh1;
        MeReal u0 = d2 * 0.0 - d1 * a2;
        MeReal u1 = a2 * d0 - d2 * 0.0;
        MeReal u2 = d1 * 0.0 - d0 * 0.0;
        MeReal t1 = ((u0 * c0 + u1 * c1) + u2 * c2) / cc;
        MeReal v0 = d2 * b1 - d1 * b2;
        MeReal v1 = b2 * d0 - d2 * b0;
        MeReal v2 = d1 * b0 - d0 * b1;
        MeReal t2;
        point[0] = b0 * t1 + P[0];
        point[1] = b1 * t1 + P[1];
        point[2] = b2 * t1 + P[2];
        if (0.0 <= t1 && t1 <= 1.0) {
            t2 = ((v0 * c0 + v1 * c1) + v2 * c2) / cc;
            if (0.0 <= t2 && t2 <= 1.0) {
                MeReal len, s;
                n[0] = point[0];
                n[1] = point[1];
                n[2] = 0.0;
                len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
                if (len != 0.0) {
                    MeReal inv = 1.0 / len;
                    n[0] = n[0] * inv;
                    n[1] = inv * n[1];
                    n[2] = inv * n[2];
                }
                if (*pad < len) {
                    s = len - (*r1 + *r2);
                    *sep = s;
                    if (s < 0.0) {
                        point[0] = *r1 * n[0];
                        point[1] = *r1 * n[1];
                        n[0] = n[0] * -1.0;
                        n[1] = -1.0 * n[1];
                        *e2 = 0;
                        *e1 = 0;
                        return 1;
                    }
                }
            }
        }
    }
    /* capsule 2's end spheres */
    k = 0;
    if (VincCapsuleSphereContact(sep, n, point, dims, pad, r1, hh1, r2, P, &e)) {
        e1[0] = e;
        e2[0] = -1;
        k = 1;
    }
    if (VincCapsuleSphereContact(sep + k, n + 3 * k, point + 3 * k, dims + k, pad, r1, hh1, r2, P2, &e)) {
        e1[k] = e;
        e2[k] = 1;
        ++k;
    }
    if (k == 2)
        return 2;
    /* capsule 1's end spheres against capsule 2's shaft, in capsule 2's frame */
    {
        MeReal x = -rel[12], y = -rel[13], z = -rel[14];
        MeReal T0 = (rel[1] * y + rel[0] * x) + rel[2] * z;
        MeReal T1 = (rel[5] * y + x * rel[4]) + rel[6] * z;
        MeReal T2 = (x * rel[8] + rel[9] * y) + rel[10] * z;
        int end;
        for (end = 0; end < 2; ++end) {
            MeReal* nk = n + 3 * k;
            MeReal* pk = point + 3 * k;
            if (end == 0) {
                E[0] = T0 - *hh1 * rel[2];
                E[1] = T1 - *hh1 * rel[6];
                E[2] = T2 - *hh1 * rel[10];
            } else {
                E[0] = *hh1 * rel[2] + T0;
                E[1] = *hh1 * rel[6] + T1;
                E[2] = T2 + *hh1 * rel[10];
            }
            if (SphereShaftContact(sep + k, nk, pk, dims + k, pad, r2, hh2, r1, E)) {
                MeReal v[3];
                v[0] = -nk[0];
                v[1] = -nk[1];
                v[2] = -nk[2];
                VincTMRotate(rel, v, nk);
                v[0] = pk[0];
                v[1] = pk[1];
                v[2] = pk[2];
                VincTMTransform(rel, v, pk);
                e1[k] = end ? 1 : -1;
                e2[k] = 0;
                ++k;
            }
        }
    }
    return k;
}

/* Mac 0x4e348c, Win 0x7d8040 (model1, model2 = capsules) */
int MEAPI VincCapsuleCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r1 = VincCapsuleGetRadius(g1);
    MeReal hh1 = VincCapsuleGetHalfHeight(g1);
    MeReal r2 = VincCapsuleGetRadius(g2);
    MeReal hh2 = VincCapsuleGetHalfHeight(g2);
    MeReal rel[16], sep[4], n[12], point[12];
    MeI16 dims[4];
    int e1[4], e2[4], count, cnt, k;
    VincContact contacts[4];
    VincRequest* rq;
    dims[0] = dims[1] = dims[2] = dims[3] = 0; /* NOTE: the axis / axis contact never sets its dims
                                                  (uninitialised in both builds) */
    VincTMMakeRelative(tm2, tm1, rel);
    count = CapsuleCapsuleContacts(sep, n, point, dims, &pad, &r1, &hh1, &r2, &hh2, rel, e1, e2);
    if (count == 0) {
        ResultClear(result);
        return 0;
    }
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount <= cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0) {
        ResultClear(result);
        return 1;
    }
    ResultClear(result);
    for (k = 0; k < count; ++k) {
        VincContact* c = &contacts[k];
        VincTMRotate(tm1, n + 3 * k, c->normal);
        c->separation = sep[k];
        c->element1 = e1[k];
        c->element2 = e2[k];
        c->dims = dims[k];
        VincTMTransform(tm1, point + 3 * k, c->position);
        ++result->contactCount;
    }
    result->normal[0] = contacts[0].normal[0];
    result->normal[1] = contacts[0].normal[1];
    result->normal[2] = contacts[0].normal[2];
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    return result->contactCount > 0;
}

/* Mac 0x4e282f, Win 0x7d8330 */
int MEAPI VincCapsuleCapsuleRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCapsuleCapsuleIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincCapsuleCapsuleSafeTime;
    VincGeometryPairRegisterInteractions(VincCapsuleGetTypeId(), VincCapsuleGetTypeId(), &i, 0);
    return 1;
}
