/* Vortex 5.0.0 build 800 "vortexldd41" - line segment against a cone (restored).
 * Mac 0x4e5fdd-0x4e674d (557f), Win 0x7e1f00 / 0x7e2350. Names (name inferred).
 * Cone frame: apex z = top, base disc at z = -bottom. The segment p0 + t (p1 - p0) is cut with the
 * double cone x^2 + y^2 = k (top - z)^2, k = (radius / height)^2 (roots on the cone's side kept),
 * and with the base plane z = -bottom inside the disc. */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>

/* the hit at parameter t: world position and distance |p1 - p0| t */
static int ConeLineSegmentHit(const MeReal* tm, MeReal x0, MeReal y0, MeReal z0, MeReal dx, MeReal dy,
                              MeReal dz, MeReal dxy2, MeReal t, VincLineSegResult* result)
{
    MeReal x = x0 + dx * t;
    MeReal y = y0 + dy * t;
    MeReal z = z0 + dz * t;
    result->position[0] = x * tm[0] + y * tm[4] + z * tm[8] + tm[12];
    result->position[1] = x * tm[1] + y * tm[5] + z * tm[9] + tm[13];
    result->position[2] = x * tm[2] + y * tm[6] + z * tm[10] + tm[14];
    result->distance = sqrt(dz * dz + dxy2) * t;
    return 1;
}

/* Mac 0x4e6008, Win 0x7e1f00. Returns 1 on a hit (position, distance; the normal is not written). */
int MEAPI VincConeLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    VincGeometryID g = VincModelGetGeometry(model);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeReal bottom = VincConeGetBottom(g);
    MeReal top = VincConeGetTop(g);
    MeReal h = VincConeGetHeight(g);
    MeReal r = VincConeGetRadius(g);
    MeVector3 a, b;
    MeReal x0, y0, z0, dx, dy, dz, k, dzk, dxy2, qa, qb, qc, disc, s, t1, t2, z1, z2, t, tc, m;
    int in1, in2;
    if (h < 1e-14)
        return 0;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    x0 = a[0];
    y0 = a[1];
    z0 = a[2];
    k = r / h;
    k = k * k;
    dz = b[2] - z0;
    dy = b[1] - y0;
    dx = b[0] - x0;
    dxy2 = dx * dx + dy * dy;
    dzk = dz * k;
    qa = dxy2 - dz * dzk;
    qb = x0 * dx + y0 * dy + dzk * (top - z0);
    qb = qb + qb;
    qc = x0 * x0 + y0 * y0 - top * k * top + (top + top) * k * z0 - z0 * k * z0;
    disc = qb * qb + qa * -4.0 * qc;
    if (disc < 0.0 || !(fabs(qa) > 1e-14)) {
        t = DBL_MAX;
        goto base;
    }
    s = sqrt(disc);
    t1 = (s - qb) / (qa + qa);
    t2 = (-qb - s) / (qa + qa);
    z1 = (t1 >= 0.0 && t1 <= 1.0) ? z0 + dz * t1 : DBL_MAX;
    z2 = (t2 >= 0.0 && t2 <= 1.0) ? z0 + dz * t2 : DBL_MAX;
    in1 = top >= z1 && z1 >= -bottom;
    in2 = top >= z2 && z2 >= -bottom;
    if (in1 && in2) {
        t = t2 > t1 ? t1 : t2;
        if (t < DBL_MAX)
            return ConeLineSegmentHit(tm, x0, y0, z0, dx, dy, dz, dxy2, t, result);
    } else if (in1) {
        t = t1;
    } else if (in2) {
        t = t2;
    } else {
        t = DBL_MAX;
    }
    /* NOTE: the original tests |dy| (the segment's y extent in the cone frame) here */
    if (fabs(dy) < 1e-14) {
        if (!in1 && !in2)
            return 0;
        if (!(t < DBL_MAX))
            return 0;
        return ConeLineSegmentHit(tm, x0, y0, z0, dx, dy, dz, dxy2, t, result);
    }
base:
    tc = (-bottom - z0) / dz;
    if (tc >= 0.0) {
        m = tc < t ? tc : t;
        t = tc > 1.0 ? t : m;
    }
    if (t < DBL_MAX) {
        MeReal x = x0 + dx * t;
        MeReal y = y0 + dy * t;
        if (r * r > x * x + y * y)
            return ConeLineSegmentHit(tm, x0, y0, z0, dx, dy, dz, dxy2, t, result);
    }
    return 0;
}

/* Mac 0x4e5fdd, Win 0x7e2350 */
void MEAPI VincConeRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincConeGetTypeId(), VincConeLineSegment);
}
