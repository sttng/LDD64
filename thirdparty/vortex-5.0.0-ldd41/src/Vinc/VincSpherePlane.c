/* Vortex 5.0.0 build 800 "vortexldd41" - sphere / plane pair test (restored).
 * Mac 0x4f3b62-0x4f3f34 (557g), Win 0x81f540-0x81f8b0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4f3b62 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f3bef, Win 0x81f660 (model1 = sphere, model2 = plane) */
int MEAPI VincSpherePlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    MeReal r, dx, dy, dz, nx, ny, nz, sep;
    VincRequest* rq;
    int n;
    VincModelGetGeometry(pair->model2);
    r = VincSphereGetRadius(g1);
    dx = tm1[12];
    dy = tm1[13];
    dz = tm1[14];
    if (!tm2) {
        nz = 1.0;
        ny = 0.0;
        nx = 0.0;
    } else {
        dx = dx - tm2[12];
        dy = dy - tm2[13];
        dz = dz - tm2[14];
        nx = tm2[8];
        ny = tm2[9];
        nz = tm2[10];
    }
    ResultClear(result);
    sep = (nx * dx + ny * dy + nz * dz) - r;
    if (pad < sep)
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    n = rq->contactMaxCount;
    if (result->contactMaxCount <= n)
        n = result->contactMaxCount;
    if (n == 0)
        return 1;
    if (result->contactMaxCount < 1) {
        ResultClear(result);
        return 0;
    } else {
        VincContact* c = result->contacts;
        c->dims = 0x203;
        c->normal[0] = nx;
        c->normal[1] = ny;
        c->normal[2] = nz;
        c->separation = sep;
        c->position[0] = tm1[12] - r * nx;
        c->position[1] = tm1[13] - r * ny;
        c->position[2] = tm1[14] - r * nz;
        result->normal[0] = nx;
        result->normal[1] = ny;
        result->normal[2] = nz;
        result->contactCount = 1;
        if (rq->contactGroupMaxCount > 0) {
            result->contactGroupCount = 1;
            result->groups[0].count = 1;
            result->groups[0].normal[0] = nx;
            result->groups[0].normal[1] = ny;
            result->groups[0].normal[2] = nz;
            if (rq->m24 == 0) {
                result->groups[0].m04 = 1.0;
            } else {
                MeReal a = r;
                if (sep < -r) {
                    /* NOTE: r^2 - (p + r)^2 < 0 here (p >= -sep > r): NaN area, as in the
                     * original (both builds) */
                    MeReal p = pad;
                    if (p < -sep)
                        p = -sep;
                    a = sqrt(r * r - (p + r) * (r + p));
                }
                result->groups[0].m04 = a * 3.141592653589793 * a;
            }
        }
        return 1;
    }
}

/* Mac 0x4f3f34, Win 0x81f540 */
int MEAPI VincSpherePlaneSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    MeReal *v1, *v2, *tm1;
    MeReal r, dist;
    MeVector3 n;
    result->pair = pair;
    result->time = maxTime;
    v1 = VincModelGetLinearVelocityPtr(pair->model1);
    v2 = VincModelGetLinearVelocityPtr(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    VincModelGetTransformPtr(pair->model2);
    r = VincSphereGetRadius(VincModelGetGeometry(pair->model1));
    VincPlaneGetNormal(VincModelGetGeometry(pair->model2), (double (*)[4])VincModelGetTransformPtr(pair->model2), n);
    dist = VincPlaneGetDistance(VincModelGetGeometry(pair->model2),
                                (double (*)[4])VincModelGetTransformPtr(pair->model2), tm1 + 12);
    if (r < dist) {
        MeReal vn = (v1[0] - v2[0]) * n[0] + (v1[1] - v2[1]) * n[1] + (v1[2] - v2[2]) * n[2];
        MeReal t;
        if (!(0.0 < vn))
            return 0;
        t = (dist - r) / vn;
        result->time = t;
        return t <= maxTime;
    }
    result->time = 0.0;
    return 1;
}

/* Mac 0x4f3b75, Win 0x81f8b0 */
int MEAPI VincSpherePlaneRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i); /* hello, goodbye and the distance slots stay 0 */
    i.intersectFn = VincSpherePlaneIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincSpherePlaneSafeTime;
    VincGeometryPairRegisterInteractions(VincSphereGetTypeId(), VincPlaneGetTypeId(), &i, 0);
    return 1;
}
