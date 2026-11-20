/* Vortex 5.0.0 build 800 "vortexldd41" - line segment queries against the sphere, box and plane
 * (restored). Mac 0x4f2e82-0x4f36f9 (557g), Win 0x7acd40-0x7ad470.
 * Registered per type with VincGeometryRegisterLineSegmentFn; VincLineSegIntersect (V3.2)
 * dispatches on the model's geometry type. Result: the first hit point from p0 towards p1, the
 * surface normal there and the distance from p0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <math.h>

enum { kRight = 0, kLeft = 1, kMiddle = 2 };

/* Mac 0x4f2e82, Win 0x7acd40: ray / axis-aligned box (Woo's "fast ray-box intersection",
 * Graphics Gems I). dir must be normalised. Returns 0 (miss), 1 (hit: coord, *t, face[0] = axis,
 * face[1] = 1 on the min side) or 2 (origin inside: coord = origin, *t = 0). */
int VincHitBoundingBox(const MeReal* minB, const MeReal* maxB, const MeReal* origin, const MeReal* dir,
                            MeReal* coord, MeReal* t, MeI16* face)
{
    int inside = 1;
    int quadrant[3];
    MeReal candidate[3], maxT[3];
    int i, which;
    for (i = 0; i < 3; ++i) {
        if (minB[i] <= origin[i]) {
            if (origin[i] <= maxB[i]) {
                quadrant[i] = kMiddle;
            } else {
                quadrant[i] = kRight;
                candidate[i] = maxB[i];
                inside = 0;
            }
        } else {
            quadrant[i] = kLeft;
            candidate[i] = minB[i];
            inside = 0;
        }
    }
    if (inside) {
        for (i = 0; i < 3; ++i)
            coord[i] = origin[i];
        *t = 0.0;
        return 2;
    }
    for (i = 0; i < 3; ++i) {
        if (quadrant[i] == kMiddle || dir[i] == 0.0)
            maxT[i] = -1.0;
        else
            maxT[i] = (candidate[i] - origin[i]) / dir[i];
    }
    which = maxT[0] < maxT[1];
    if (maxT[which] < maxT[2])
        which = 2;
    if (!(0.0 <= maxT[which]))
        return 0;
    for (i = 0; i < 3; ++i) {
        if (which == i) {
            coord[i] = candidate[i];
        } else {
            coord[i] = maxT[which] * dir[i] + origin[i];
            if (coord[i] < minB[i] || maxB[i] < coord[i])
                return 0;
        }
    }
    face[0] = (MeI16)which;
    face[1] = (MeI16)(quadrant[which] == kLeft);
    *t = maxT[which];
    return 1;
}

/* Mac 0x4f30ea, Win 0x7acf80: segment p0-p1 against the sphere (centre tm[12..14], radius r). The
 * entry point when p0 is outside, the exit point when p0 is inside. Also used by the capsule's
 * line segment query (Mac 0x4e3b4e). */
int VincLineSegmentSphere(const MeReal* tm, MeReal r, const MeReal* p0, const MeReal* p1,
                             VincLineSegResult* result)
{
    MeReal cx = tm[12], cy = tm[13], cz = tm[14];
    MeReal dz = p1[2] - p0[2], dy = p1[1] - p0[1], dx = p1[0] - p0[0];
    MeReal len = sqrt(dx * dx + dy * dy + dz * dz);
    MeReal r2, inv, ux, uy, uz, wx, wy, wz, proj, ww, h, s, t;
    if (!(1e-14 <= len))
        return 0;
    r2 = r * r;
    inv = 1.0 / len;
    ux = dx * inv;
    uy = dy * inv;
    uz = dz * inv;
    wz = cz - p0[2];
    wy = cy - p0[1];
    wx = cx - p0[0];
    proj = ux * wx + uy * wy + uz * wz;
    ww = wx * wx + wy * wy + wz * wz;
    if (!(0.0 <= proj || proj != proj || ww <= r2))
        return 0;
    h = ww - proj * proj;
    if (!(h <= r2))
        return 0;
    s = sqrt(r2 - h);
    if (ww <= r2)
        t = proj + s;
    else
        t = proj - s;
    if (!(t <= len))
        return 0;
    result->position[0] = ux * t + p0[0];
    result->position[1] = uy * t + p0[1];
    result->position[2] = uz * t + p0[2];
    inv = 1.0 / r;
    result->normal[0] = (result->position[0] - cx) * inv;
    result->normal[1] = (result->position[1] - cy) * inv;
    result->normal[2] = (result->position[2] - cz) * inv;
    result->distance = t;
    return 1;
}

/* Mac 0x4f3308, Win 0x7ad110 */
int MEAPI VincSphereLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    MeReal r = VincSphereGetRadius(VincModelGetGeometry(model));
    return VincLineSegmentSphere(VincModelGetTransformPtr(model), r, p0, p1, result);
}

/* Mac 0x4f335b, Win 0x7ad160: segment crossing the plane z = 0 (either direction); the distance is
 * negative when p0 is below the plane */
int MEAPI VincPlaneLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    MeReal* tm;
    MeVector3 a, b, local;
    MeReal dx, dy, dz, t, len;
    VincModelGetGeometry(model);
    tm = VincModelGetTransformPtr(model);
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    if ((0.0 < a[2] && 0.0 < b[2]) || (a[2] < 0.0 && b[2] < 0.0))
        return 0;
    dx = b[0] - a[0];
    dy = b[1] - a[1];
    dz = b[2] - a[2];
    if (fabs(dz) < 1e-14)
        return 0;
    t = -(a[2] / dz);
    local[0] = dx * t + a[0];
    local[1] = a[1] + t * dy;
    local[2] = 0.0;
    VincTMTransform(tm, local, result->position);
    result->normal[0] = tm[8];
    result->normal[1] = tm[9];
    result->normal[2] = tm[10];
    len = sqrt(dx * dx + dy * dy + dz * dz);
    result->distance = len * t;
    if (a[2] < 0.0)
        result->distance = -(len * t);
    return 1;
}

/* Mac 0x4f36f9, Win 0x7ad2f0: the box face hit first (no normal when p0 is inside the box) */
int MEAPI VincBoxLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    VincGeometryID g = VincModelGetGeometry(model);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeVector3 a, b, minB, maxB, u, coord;
    MeReal* radii;
    MeReal len, inv, t;
    MeI16 face[2];
    int hit;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    radii = VincBoxGetRadii(g);
    minB[0] = -radii[0];
    maxB[0] = radii[0];
    minB[1] = -radii[1];
    maxB[1] = radii[1];
    minB[2] = -radii[2];
    maxB[2] = radii[2];
    u[0] = b[0] - a[0];
    u[1] = b[1] - a[1];
    u[2] = b[2] - a[2];
    len = sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    if (!(1e-14 <= len))
        return 0;
    inv = 1.0 / len;
    u[0] = u[0] * inv;
    u[1] = u[1] * inv;
    u[2] = inv * u[2];
    hit = VincHitBoundingBox(minB, maxB, a, u, coord, &t, face);
    if (hit == 0 || !(t <= len))
        return 0;
    VincTMTransform(tm, coord, result->position);
    if (hit == 1) {
        const MeReal* axis = tm + face[0] * 4;
        result->normal[0] = axis[0];
        result->normal[1] = axis[1];
        result->normal[2] = axis[2];
        if (face[1]) {
            result->normal[0] = -result->normal[0];
            result->normal[1] = -result->normal[1];
            result->normal[2] = -result->normal[2];
        }
    }
    result->distance = t;
    return 1;
}

/* Mac 0x4f3069, Win 0x7ad2d0 */
void MEAPI VincPlaneRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincPlaneGetTypeId(), VincPlaneLineSegment);
}

/* Mac 0x4f3094, Win 0x7ad2b0 */
void MEAPI VincSphereRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincSphereGetTypeId(), VincSphereLineSegment);
}

/* Mac 0x4f30bf, Win 0x7ad470 */
void MEAPI VincBoxRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincBoxGetTypeId(), VincBoxLineSegment);
}
