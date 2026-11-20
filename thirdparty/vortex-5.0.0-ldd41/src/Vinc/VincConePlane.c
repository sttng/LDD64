/* Vortex 5.0.0 build 800 "vortexldd41" - cone / plane pair test (restored).
 * Mac 0x4e674d-0x4e71c9 (557f), Win 0x7e2b40 / 0x7e2cd0 / 0x7e3010. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* p = tm * (x, y, z) */
static void ConePoint(const MeReal* tm, MeReal x, MeReal y, MeReal z, MeReal* p)
{
    p[0] = x * tm[0] + y * tm[4] + z * tm[8] + tm[12];
    p[1] = x * tm[1] + y * tm[5] + z * tm[9] + tm[13];
    p[2] = x * tm[2] + y * tm[6] + z * tm[10] + tm[14];
}

/* Mac 0x4e67c3, Win 0x7e2b40: the cone's deepest point(s) along dir (world; the negated plane
 * normal). Returns 2 when the axis points against dir within eps (the base faces the plane:
 * points[0] = base centre), 0 for one point (the apex, or the rim point when the side is not
 * parallel to the plane), 1 for two points (rim point + apex: the side lies on the plane). */
int VincConeDeepestPoints(const VincCone* cone, const MeReal* tm, const MeReal* dir, MeReal* points, MeReal eps)
{
    MeReal r = cone->radius;
    MeReal d0 = dir[0], d1 = dir[1], d2 = dir[2];
    MeReal c = d0 * tm[8] + d1 * tm[9] + d2 * tm[10];
    MeReal lx, ly, len, inv;
    if (!(eps <= fabs(c + 1.0))) {
        ConePoint(tm, 0.0, 0.0, -cone->bottom, points);
        return 2;
    }
    c = c - cone->sinHalfAngle;
    if (!(c <= eps)) {
        ConePoint(tm, 0.0, 0.0, cone->top, points);
        return 0;
    }
    lx = d0 * tm[0] + tm[1] * d1 + d2 * tm[2];
    ly = tm[4] * d0 + tm[5] * d1 + tm[6] * d2;
    len = sqrt(lx * lx + ly * ly + 0.0);
    if (len != 0.0) {
        inv = 1.0 / len;
        lx = lx * inv;
        ly = inv * ly;
    }
    ly = r * ly;
    lx = lx * r;
    ConePoint(tm, lx, ly, -cone->bottom, points);
    if (eps <= fabs(c))
        return 0;
    ConePoint(tm, 0.0, 0.0, cone->top, points + 3);
    return 1;
}

/* one contact: position p, normal n (the plane's), separation sep */
static void ConePlaneContact(VincContact* c, const MeReal* p, const MeReal* n, MeReal sep)
{
    c->position[0] = p[0];
    c->position[1] = p[1];
    c->position[2] = p[2];
    c->normal[0] = n[0];
    c->normal[1] = n[1];
    c->normal[2] = n[2];
    c->separation = sep;
}

/* Mac 0x4e6c82, Win 0x7e2cd0 (model1 = cone, model2 = plane z = 0 of its transform): up to three
 * contacts (base on the plane), two (a side line on it) or one.
 * NOTE: the separation is (deepest point . plane normal) - the z translation of the plane's
 * transform, not - (plane position . normal): exact only for a plane through the origin or with
 * normal +z. Kept as in the original. */
int MEAPI VincConePlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    VincCone* cone = (VincCone*)VincModelGetGeometry(pair->model1);
    const MeReal* n = tm2 + 8;
    MeReal dir[3], pts[6], r, b, sep;
    VincRequest* rq;
    int kind, maxCount;
    result->contactGroupCount = 0;
    result->contactCount = 0;
    dir[0] = -n[0];
    dir[1] = -n[1];
    dir[2] = -n[2];
    r = cone->radius;
    b = cone->bottom;
    kind = VincConeDeepestPoints(cone, tm1, dir, pts, 0.001);
    rq = VincModelPairGetRequestPtr(pair);
    sep = pts[0] * n[0] + pts[1] * n[1] + pts[2] * n[2] - tm2[14];
    if (0.0 < sep)
        return 0;
    maxCount = rq->contactMaxCount;
    if (result->contactMaxCount <= maxCount)
        maxCount = result->contactMaxCount;
    if (kind == 2) {
        if (maxCount != 0) {
            MeReal p[3], x, y;
            b = -b;
            ConePoint(tm1, r, 0.0, b, p);
            ConePlaneContact(&result->contacts[0], p, n, sep);
            if (++result->contactCount < result->contactMaxCount) {
                y = r * 0.5 * 1.7320508075688772;
                x = -(r * 0.5);
                ConePoint(tm1, x, y, b, p);
                ConePlaneContact(&result->contacts[1], p, n, sep);
                if (++result->contactCount < result->contactMaxCount) {
                    y = -y;
                    ConePoint(tm1, x, y, b, p);
                    ConePlaneContact(&result->contacts[2], p, n, sep);
                    ++result->contactCount;
                    return 1;
                }
            }
        }
    } else if (kind == 1) {
        if (maxCount != 0) {
            ConePlaneContact(&result->contacts[0], pts, n, sep);
            if (++result->contactCount < result->contactMaxCount) {
                ConePlaneContact(&result->contacts[1], pts + 3, n, sep);
                ++result->contactCount;
                return 1;
            }
        }
    } else if (kind == 0) {
        if (maxCount != 0) {
            result->contactCount = 1;
            ConePlaneContact(&result->contacts[0], pts, n, sep);
            return 1;
        }
    } else {
        return 0;
    }
    return 1;
}

/* Mac 0x4e674d, Win 0x7e3010 */
int MEAPI VincConePlaneRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincConePlaneIntersect;
    VincGeometryPairRegisterInteractions(VincConeGetTypeId(), VincPlaneGetTypeId(), &i, 0);
    return 1;
}
