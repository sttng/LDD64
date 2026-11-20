/* Vortex 5.0.0 build 800 "vortexldd41" - line segment query against the cylinder (restored).
 * Mac 0x4efd81-0x4f0783 (557g), Win 0x7dd470-0x7dda40. Names (name inferred).
 * Control flow from the Mac build (Ghidra's reading of the Win x87 compares is off at t == 0);
 * the association of every sum checked against the Win build (diff_vinc_pairs). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>

/* the hit at parameter t of the local segment a + t d: position, distance |d| t, and the normal of
 * the side (|z| < halfHeight) or of the end cap */
static void Hit(const MeReal* tm, MeReal hh, const MeReal* a, const MeReal* d, MeReal dd, MeReal t, int sideOnly,
                VincLineSegResult* result)
{
    MeVector3 p;
    p[0] = a[0] + d[0] * t;
    p[1] = a[1] + d[1] * t;
    p[2] = a[2] + d[2] * t;
    VincTMTransform(tm, p, result->position);
    result->distance = sqrt(d[2] * d[2] + dd) * t;
    if (sideOnly || fabs(p[2]) < hh) {
        p[2] = 0.0;
        VincTMRotate(tm, p, result->normal);
        return;
    }
    result->normal[0] = tm[8];
    result->normal[1] = tm[9];
    result->normal[2] = tm[10];
    if (p[2] < 0.0) {
        result->normal[0] = -result->normal[0];
        result->normal[1] = -result->normal[1];
        result->normal[2] = -result->normal[2];
    }
}

/* Mac 0x4efdac, Win 0x7dd470: segment p0-p1 against the cylinder (radius r, half height hh,
 * axis = z of tm). Also used by the capsule's line segment query (Mac 0x4e3b4e). */
int VincLineSegmentCylinder(const MeReal* tm, MeReal r, MeReal hh, const MeReal* p0, const MeReal* p1,
                            VincLineSegResult* result)
{
    MeVector3 a, b, d;
    MeReal dd, B, r2, disc, tSide = DBL_MAX;
    int v1 = 0, v2 = 0;
    MeReal tc1, tc2, tc;
    int c1, c2, haveCap;
    if (hh < 1e-14)
        return 0;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    d[2] = b[2] - a[2];
    d[1] = b[1] - a[1];
    d[0] = b[0] - a[0];
    dd = d[0] * d[0] + d[1] * d[1];
    B = a[0] * d[0] + a[1] * d[1];
    B = B + B;
    r2 = r * r;
    disc = B * B - dd * 4.0 * ((a[0] * a[0] + a[1] * a[1]) - r2);
    if (!(disc < 0.0) && !(fabs(dd) <= 1e-14)) {
        MeReal s = sqrt(disc);
        MeReal t1 = (s - B) / (dd + dd);
        MeReal t2 = (-B - s) / (dd + dd);
        MeReal z1 = DBL_MAX, z2 = DBL_MAX;
        if (0.0 <= t1 && t1 <= 1.0)
            z1 = d[2] * t1 + a[2];
        if (0.0 <= t2 && t2 <= 1.0)
            z2 = d[2] * t2 + a[2];
        v1 = z1 <= hh && -hh <= z1;
        v2 = z2 <= hh && -hh <= z2;
        if (v1) {
            if (v2) {
                MeReal t = t1;
                if (t2 <= t1)
                    t = t2;
                tSide = t;
                if (t < DBL_MAX) {
                    Hit(tm, hh, a, d, dd, t, 1, result);
                    return 1;
                }
            } else {
                tSide = t1;
            }
        } else if (v2) {
            tSide = t2;
        }
    }
    if (fabs(d[1]) < 1e-14 && (v1 || v2) && tSide < DBL_MAX) {
        Hit(tm, hh, a, d, dd, tSide, 0, result);
        return 1;
    }
    tc1 = (-hh - a[2]) / d[2];
    tc2 = (hh - a[2]) / d[2];
    c1 = 0;
    if (0.0 <= tc1 && tc1 <= 1.0) {
        MeReal x = a[0] + d[0] * tc1, y = d[1] * tc1 + a[1];
        c1 = x * x + y * y <= r2;
    }
    c2 = 0;
    if (0.0 <= tc2 && tc2 <= 1.0) {
        MeReal x = a[0] + d[0] * tc2, y = d[1] * tc2 + a[1];
        c2 = x * x + y * y <= r2;
    }
    haveCap = 1;
    if (c1)
        tc = (c2 && tc2 <= tc1) ? tc2 : tc1;
    else if (c2)
        tc = tc2;
    else
        haveCap = 0;
    if (!(haveCap && tc < DBL_MAX && tc < tSide)) {
        if (!(tSide < DBL_MAX))
            return 0;
        tc = tSide;
    }
    Hit(tm, hh, a, d, dd, tc, 0, result);
    return 1;
}

/* Mac 0x4f0783, Win 0x7dd9e0 */
int MEAPI VincCylinderLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    VincGeometryID g = VincModelGetGeometry(model);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeReal r = VincCylinderGetRadius(g);
    MeReal hh = VincCylinderGetHalfHeight(g);
    return VincLineSegmentCylinder(tm, r, hh, p0, p1, result);
}

/* Mac 0x4efd81, Win 0x7dda40 */
void MEAPI VincCylinderRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincCylinderGetTypeId(), VincCylinderLineSegment);
}
