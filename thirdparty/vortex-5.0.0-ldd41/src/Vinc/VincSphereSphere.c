/* Vortex 5.0.0 build 800 "vortexldd41" - sphere / sphere pair test (restored).
 * Mac 0x4f410b-0x4f441e (557g), Win 0x820080-0x8204b0. Reached in LDD (ball game, height field
 * scenes). Names (name inferred) after the Mcd pair-test naming. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincIntersect.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4f410b (regparm EAX = result; Win: inlined): no contacts, no groups */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f4198, Win 0x820310: earliest time at which the spheres, moving with their linear
 * velocities, touch; 0 when they overlap now. Returns 1 when that time is <= maxTime. */
int MEAPI VincSphereSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    MeReal *v1, *v2, *tm1, *tm2;
    MeReal x1, y1, z1, x2, y2, z2, r1, r2, dx, dy, dz, s;
    result->pair = pair;
    result->time = maxTime;
    v1 = VincModelGetLinearVelocityPtr(pair->model1);
    v2 = VincModelGetLinearVelocityPtr(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    x1 = tm1[12]; y1 = tm1[13]; z1 = tm1[14];
    x2 = tm2[12]; y2 = tm2[13]; z2 = tm2[14];
    r1 = VincSphereGetRadius(VincModelGetGeometry(pair->model1));
    r2 = VincSphereGetRadius(VincModelGetGeometry(pair->model2));
    dz = z2 - z1;
    dy = y2 - y1;
    dx = x2 - x1;
    s = (dx * dx + dy * dy + dz * dz) - (r1 + r2) * (r1 + r2);
    if (s < 0.0) {
        result->time = 0.0;
        return 1;
    } else {
        MeReal wz = v2[2] - v1[2], wy = v2[1] - v1[1], wx = v2[0] - v1[0];
        MeReal ww = wx * wx + wy * wy + wz * wz;
        MeReal b, disc, t;
        if (ww < 1e-14)
            return 0;
        b = dx * wx + dy * wy + dz * wz;
        if (0.0 <= b)
            return 0;
        disc = b * b - s * ww;
        if (disc < 0.0)
            return 0;
        t = (-b - sqrt(disc)) / ww;
        result->time = t;
        return t <= maxTime;
    }
}

/* Mac 0x4f441e, Win 0x820080: one contact at the weighted centre point, normal from sphere 2 to
 * sphere 1; group area pi a^2 of the overlap disc when the request asks for it (m24) */
int MEAPI VincSphereSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r1 = VincSphereGetRadius(g1);
    MeReal r2 = VincSphereGetRadius(g2);
    MeReal nz = tm1[14] - tm2[14];
    MeReal ny = tm1[13] - tm2[13];
    MeReal nx = tm1[12] - tm2[12];
    MeReal dist = nx * nx + ny * ny + nz * nz;
    MeReal sep;
    VincRequest* rq;
    int n;
    if (dist != 0.0) {
        MeReal inv;
        dist = sqrt(dist);
        inv = 1.0 / dist;
        nx = nx * inv;
        ny = ny * inv;
        nz = nz * inv;
    } else {
        nz = 1.0;
        ny = 0.0;
        nx = 0.0;
    }
    sep = dist - (r1 + r2);
    ResultClear(result);
    if (!(sep <= pad))
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
        MeReal inv, w1, w2;
        c->dims = 0x303;
        c->normal[0] = nx;
        c->normal[1] = ny;
        c->normal[2] = nz;
        c->separation = sep;
        inv = 1.0 / (r1 + r2);
        w1 = r2 * inv;
        w2 = inv * r1;
        c->position[0] = w2 * tm2[12] + w1 * tm1[12];
        c->position[1] = w2 * tm2[13] + w1 * tm1[13];
        c->position[2] = w2 * tm2[14] + w1 * tm1[14];
        result->normal[0] = nx;
        result->normal[1] = ny;
        result->normal[2] = nz;
        if (rq->contactGroupMaxCount > 0) {
            result->contactGroupCount = 1;
            result->groups[0].count = 1;
            result->groups[0].normal[0] = nx;
            result->groups[0].normal[1] = ny;
            result->groups[0].normal[2] = nz;
            if (rq->m24 == 0) {
                result->groups[0].m04 = 1.0;
            } else {
                MeReal rmin = r1, p = pad, a;
                if (r2 <= r1)
                    rmin = r2;
                if (p < -sep)
                    p = -sep;
                a = sqrt(rmin * rmin - (p + rmin) * (rmin + p));
                result->groups[0].m04 = a * 3.141592653589793 * a;
            }
        }
        result->contactCount = 1;
        return 1;
    }
}

/* Mac 0x4f411e, Win 0x8204b0 */
int MEAPI VincSphereSphereRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i); /* hello, goodbye and the distance slots stay 0 */
    i.intersectFn = VincSphereSphereIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincSphereSphereSafeTime;
    VincGeometryPairRegisterInteractions(VincSphereGetTypeId(), VincSphereGetTypeId(), &i, 0);
    return 1;
}
