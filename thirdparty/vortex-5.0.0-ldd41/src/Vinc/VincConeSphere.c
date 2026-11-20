/* Vortex 5.0.0 build 800 "vortexldd41" - cone / sphere pair test (restored).
 * Mac 0x4e71c9-0x4e7f02 (557f), Win 0x7e24a0 / 0x7e24e0 / 0x7e2530 / 0x7e2ae0. Names (name
 * inferred). One contact, found in the cone's (r, z) half plane: apex region, base face, side
 * line (closest point on the generator) or base rim. A cone of height 0 gives NaN intermediates;
 * Win's x87 compares (fcom + "test ah, 41h", no parity check) then branch unlike the IEEE compares
 * here (and on Mac). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4e71c9, Win 0x7e24a0: parameter of the projection of (px, py) on the line (x1, y1) ->
 * (x2, y2) */
static MeReal ProjectOnLine(MeReal x1, MeReal y1, MeReal x2, MeReal y2, MeReal px, MeReal py)
{
    MeReal dx = x2 - x1;
    MeReal dy = y2 - y1;
    return ((y1 - py) * (y1 - y2) - dx * (x1 - px)) / (dx * dx + dy * dy);
}

/* Mac 0x4e7233, Win 0x7e24e0: the cone's radius at height z (0 outside [-bottom, top]) */
static int ConeRadiusAt(const VincCone* cone, MeReal z, MeReal* radius)
{
    MeReal top = cone->top;
    if (-cone->bottom > z || z > top)
        return 0;
    *radius = cone->radius / (cone->bottom + top) * (top - z);
    return 1;
}

static void ConeTransform(const MeReal* tm, MeReal x, MeReal y, MeReal z, MeReal* p)
{
    p[0] = x * tm[0] + y * tm[4] + z * tm[8] + tm[12];
    p[1] = x * tm[1] + y * tm[5] + z * tm[9] + tm[13];
    p[2] = x * tm[2] + y * tm[6] + z * tm[10] + tm[14];
}

static void ConeRotate(const MeReal* tm, MeReal x, MeReal y, MeReal z, MeReal* p)
{
    p[0] = x * tm[0] + y * tm[4] + z * tm[8];
    p[1] = x * tm[1] + y * tm[5] + z * tm[9];
    p[2] = x * tm[2] + y * tm[6] + z * tm[10];
}

/* Mac 0x4e7311, Win 0x7e2530 (model1 = cone, model2 = sphere). The normal points from the sphere
 * to the cone. */
int MEAPI VincConeSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1;
    MeReal* tm2;
    VincCone* cone;
    VincSphere* sphere;
    VincContact* c;
    MeReal dx, dy, dz, x, y, z, sr, top, bottom, R, rho2, rho, slope, inv, dist;
    int maxCount;
    result->contactGroupCount = 0;
    result->contactCount = 0;
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    cone = (VincCone*)VincModelGetGeometry(pair->model1);
    sphere = (VincSphere*)VincModelGetGeometry(pair->model2);
    dz = tm2[14] - tm1[14];
    dy = tm2[13] - tm1[13];
    dx = tm2[12] - tm1[12];
    z = dx * tm1[8] + dy * tm1[9] + dz * tm1[10];
    sr = sphere->radius;
    top = cone->top;
    bottom = cone->bottom;
    R = cone->radius;
    if (!(z <= sr + top) || !(-(sr + bottom) <= z))
        return 0;
    x = dx * tm1[0] + dy * tm1[1] + dz * tm1[2];
    y = tm1[4] * dx + tm1[5] * dy + tm1[6] * dz;
    rho2 = x * x + y * y;
    rho = sqrt(rho2);
    if (!(rho <= R + sr))
        return 0;
    slope = R / (top + bottom);
    maxCount = VincModelPairGetRequestPtr(pair)->contactMaxCount;
    if (result->contactMaxCount <= maxCount)
        maxCount = result->contactMaxCount;
    if (rho * slope + top < z) {
        /* apex region */
        MeReal nx, ny = 0.0, nz;
        nz = top - z;
        y = 0.0 - y;
        x = 0.0 - x;
        dist = sqrt(x * x + y * y + nz * nz);
        if (!(dist <= sr))
            return 0;
        if (maxCount == 0)
            return 1;
        result->contactCount = 1;
        c = result->contacts;
        ConeTransform(tm1, 0.0, 0.0, top, c->position);
        if (!(dist < 1e-14)) {
            inv = 1.0 / dist;
            nx = x * inv;
            ny = y * inv;
            nz = nz * inv;
        } else {
            nz = -1.0;
            nx = 0.0;
        }
        ConeRotate(tm1, nx, ny, nz, c->normal);
        c->separation = dist - sr;
        return 1;
    } else {
        MeReal negBottom = -bottom;
        if (z < negBottom && rho <= R) {
            /* below the base face */
            if (maxCount == 0)
                return 1;
            result->contactCount = 1;
            c = result->contacts;
            c->normal[0] = tm1[0] * 0.0 + tm1[4] * 0.0 + tm1[8];
            c->normal[1] = tm1[1] * 0.0 + tm1[5] * 0.0 + tm1[9];
            c->normal[2] = tm1[2] * 0.0 + tm1[6] * 0.0 + tm1[10];
            ConeTransform(tm1, x, y, negBottom, c->position);
            c->separation = -(z + bottom + sr);
            return 1;
        }
        if (rho < R || (rho - R) * slope - bottom <= z) {
            /* side: the closest point of the generator line (R, -bottom) -> (0, top) */
            MeReal rz, rc, t, zc, ux, uy, uz, px, py, ddx, ddy, ddz, len;
            int inside = ConeRadiusAt(cone, z, &rz);
            t = ProjectOnLine(R, negBottom, 0.0, top, rho, z);
            if (!(t <= 1.0) || !(0.0 <= t))
                return 0;
            zc = (top + bottom) * t - bottom;
            ConeRadiusAt(cone, zc, &rc);
            uy = 0.0;
            ux = 1.0;
            if (1e-14 <= rho) {
                uy = y;
                ux = x;
            }
            len = sqrt(ux * ux + uy * uy + 0.0);
            uz = 0.0;
            if (len != 0.0) {
                len = 1.0 / len;
                ux = ux * len;
                uy = uy * len;
                uz = len * 0.0;
            }
            py = rc * uy;
            px = rc * ux;
            ddz = zc - z;
            ddy = py - y;
            ddx = px - x;
            dist = sqrt(ddx * ddx + ddy * ddy + ddz * ddz);
            if (!((inside && rz > rho) || dist <= sr))
                return 0;
            if (1e-14 <= dist) {
                inv = 1.0 / dist;
                ux = ddx * inv;
                uy = ddy * inv;
                uz = ddz * inv;
            }
            if (!inside || rz <= rho) {
                dist = dist - sr;
            } else { /* the centre is inside the cone */
                dist = -(sr + dist);
                uz = -uz;
                uy = -uy;
                ux = -ux;
            }
            if (maxCount == 0)
                return 1;
            result->contactCount = 1;
            c = result->contacts;
            ConeTransform(tm1, px, py, zc, c->position);
            ConeRotate(tm1, ux, uy, uz, c->normal);
            c->separation = dist;
            return 1;
        } else {
            /* the base rim */
            MeReal ux = x, uy = y, px, py, len;
            len = sqrt(rho2 + 0.0);
            if (len != 0.0) {
                len = 1.0 / len;
                ux = x * len;
                uy = len * y;
            }
            py = R * uy;
            px = ux * R;
            dz = negBottom - z;
            dy = py - y;
            dx = px - x;
            dist = sqrt(dx * dx + dy * dy + dz * dz);
            if (!(dist <= sr))
                return 0;
            if (maxCount == 0)
                return 1;
            c = result->contacts;
            ConeTransform(tm1, px, py, negBottom, c->position);
            if (!(dist < 1e-14)) {
                inv = 1.0 / dist;
                dx = dx * inv;
                dy = dy * inv;
                dz = dz * inv;
            } else {
                dz = -1.0;
                dy = 0.0;
                dx = 0.0;
            }
            ConeRotate(tm1, dx, dy, dz, c->normal);
            c->separation = dist - sr;
            result->contactCount = 1;
            return 1;
        }
    }
}

/* Mac 0x4e729b, Win 0x7e2ae0 */
int MEAPI VincConeSphereRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincConeSphereIntersect;
    VincGeometryPairRegisterInteractions(VincConeGetTypeId(), VincSphereGetTypeId(), &i, 0);
    return 1;
}
