/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / sphere pair test (restored).
 * Mac 0x4f13e5-0x4f1815 (557g), Win 0x7dffd0-0x7e04b0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4f13e5 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f13f8, Win 0x7e01f0: the cylinder as an ellipsoid (r, r, r + hh) against the sphere */
int MEAPI VincCylinderSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCylinderGetRadius(g1);
    MeReal hh = VincCylinderGetHalfHeight(g1);
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

/* Mac 0x4f14c1, Win 0x7dffd0; also used by Mac 0x4eabd6 (557f). The sphere (centre c in the
 * cylinder frame, radius *sr) against the cylinder (*r, *hh) in the cylinder frame: the nearest
 * feature of the cap planes (dims 0x302), the side (0x303, *m = r) or the rim edge (0x301);
 * separation, normal (from the sphere centre towards the cylinder, cylinder frame), contact point
 * on the sphere. Returns 1 when within *pad. */
int VincCylinderSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, MeReal* m, const MeReal* pad,
                              const MeReal* r, const MeReal* hh, const MeReal* sr, const MeReal* c)
{
    MeReal nx = 0.0, ny = 0.0, nz = 0.0;
    MeReal cz = c[2], rs = *sr, h = *hh;
    MeReal d = (fabs(cz) - rs) - h; /* distance to the cap planes */
    MeReal pd = *pad;
    MeReal cx, cy, rho, R, dSide;
    int mode, noTouch;
    if (pd < d) {
        *sep = d;
        noTouch = 1;
        goto capNormal;
    }
    cx = c[0];
    cy = c[1];
    rho = sqrt(cx * cx + cy * cy);
    R = *r;
    dSide = (rho - R) - rs;
    mode = 0;
    noTouch = 0;
    if (dSide <= d) {
        goto edge;
    } else {
        noTouch = pd < dSide;
        mode = 1;
        if (dSide <= pd) {
            d = dSide;
            goto edge;
        }
        *sep = dSide;
        goto features;
    }
edge:
    if (-rs <= d && R < rho && h < fabs(cz)) {
        MeReal len, dEdge;
        nz = ((0.0 < cz) ? h : -h) - cz;
        ny = cy * (R / rho) - cy;
        nx = cx * (R / rho) - cx;
        len = sqrt(nx * nx + ny * ny + nz * nz);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            nx = nx * inv;
            ny = ny * inv;
            nz = nz * inv;
        }
        dEdge = len - rs;
        if (d < dEdge) {
            noTouch = pd < dEdge;
            mode = 2;
            *sep = dEdge;
            goto features;
        }
    }
    *sep = d;
    if (mode == 0)
        goto capNormal;
features:
    if (mode == 1) {
        if (*pad <= rho) { /* NOTE: compares the padding with rho (original) */
            MeReal y = c[1];
            n[0] = -(1.0 / rho) * c[0];
            n[1] = -(1.0 / rho) * y;
            n[2] = 0.0;
            *dims = 0x303;
            *m = *r;
        } else {
            noTouch = 1;
        }
    } else {
        n[0] = nx;
        n[1] = ny;
        n[2] = nz;
        *dims = 0x301;
        *m = 0.0;
    }
    goto done;
capNormal:
    n[0] = 0.0;
    n[1] = 0.0;
    n[2] = (c[2] <= 0.0) ? 1.0 : -1.0;
    *dims = 0x302;
    *m = 0.0;
done:
    point[0] = *sr * n[0] + c[0];
    point[1] = *sr * n[1] + c[1];
    point[2] = *sr * n[2] + c[2];
    return !noTouch;
}

/* Mac 0x4f188f, Win 0x7e0270 (model1 = cylinder, model2 = sphere). NOTE: the group normal is
 * left in the cylinder frame (both builds). */
int MEAPI VincCylinderSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCylinderGetRadius(g1);
    MeReal hh = VincCylinderGetHalfHeight(g1);
    MeReal sr = VincSphereGetRadius(g2);
    MeVector3 cl, n, p;
    MeReal sep, m;
    MeI16 dims;
    VincRequest* rq;
    int touch, cnt;
    VincTMInverseTransform(tm1, tm2 + 12, cl);
    n[0] = n[1] = n[2] = 0.0; /* (the original leaves n uninitialised on the no-touch path) */
    touch = VincCylinderSphereContact(&sep, n, p, &dims, &m, &pad, &r, &hh, &sr, cl);
    ResultClear(result);
    if (!touch)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount <= cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0)
        return 1;
    if (result->contactMaxCount < 1) {
        ResultClear(result);
        return 0;
    } else {
        VincContact* c = result->contacts;
        VincTMRotate(tm1, n, result->normal);
        c->dims = dims;
        c->separation = sep;
        VincTMTransform(tm1, p, c->position);
        c->normal[0] = result->normal[0];
        c->normal[1] = result->normal[1];
        c->normal[2] = result->normal[2];
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

/* Mac 0x4f1815, Win 0x7e04b0 */
int MEAPI VincCylinderSphereRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCylinderSphereIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincCylinderSphereSafeTime;
    VincGeometryPairRegisterInteractions(VincCylinderGetTypeId(), VincSphereGetTypeId(), &i, 0);
    return 1;
}
