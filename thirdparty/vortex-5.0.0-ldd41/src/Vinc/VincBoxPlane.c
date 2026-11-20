/* Vortex 5.0.0 build 800 "vortexldd41" - box / plane pair test (restored).
 * Mac 0x4dff90-0x4e0e0a (557e), Win 0x81f910-0x820070. Names (name inferred). The plane is z <= 0
 * of model2's transform (identity when model2 has none). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* Mac 0x4dff90 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4dffa3, Win 0x81f910 (cdecl): the 12 box edges (rel = box in the plane frame, half
 * extents r) clipped against z <= 0, end points appended at *cursor: the 8 edges along the box's
 * x and y axes add only clipped end points, the 4 along z both (V3.8's clip helpers) */
static void BoxPlanePoints(MeReal** cursor, const MeReal* rel, const MeReal* r)
{
    MeVector3 a, b, c, p;
    MeReal ia, ib, ic;
    a[0] = rel[0] * r[0];
    a[1] = rel[1] * r[0];
    a[2] = rel[2] * r[0];
    b[0] = rel[4] * r[1];
    b[1] = rel[5] * r[1];
    b[2] = rel[6] * r[1];
    c[0] = r[2] * rel[8];
    c[1] = rel[9] * r[2];
    c[2] = rel[10] * r[2];
    ia = 1.0 / a[2];
    ib = 1.0 / b[2];
    ic = 1.0 / c[2];
#define CORNER(s1, v1, s2, v2) \
    p[0] = (rel[12] s1 v1[0]) s2 v2[0]; \
    p[1] = (rel[13] s1 v1[1]) s2 v2[1]; \
    p[2] = (rel[14] s1 v1[2]) s2 v2[2]
    CORNER(+, b, +, c); VincClipSegmentAddInnerPoints(cursor, p, a, &ia);
    CORNER(+, b, -, c); VincClipSegmentAddInnerPoints(cursor, p, a, &ia);
    CORNER(-, b, -, c); VincClipSegmentAddInnerPoints(cursor, p, a, &ia);
    CORNER(-, b, +, c); VincClipSegmentAddInnerPoints(cursor, p, a, &ia);
    CORNER(+, c, +, a); VincClipSegmentAddInnerPoints(cursor, p, b, &ib);
    CORNER(+, c, -, a); VincClipSegmentAddInnerPoints(cursor, p, b, &ib);
    CORNER(-, c, -, a); VincClipSegmentAddInnerPoints(cursor, p, b, &ib);
    CORNER(-, c, +, a); VincClipSegmentAddInnerPoints(cursor, p, b, &ib);
    CORNER(+, a, +, b); VincClipSegmentAddPoints(cursor, p, c, &ic);
    CORNER(+, a, -, b); VincClipSegmentAddPoints(cursor, p, c, &ic);
    CORNER(-, a, -, b); VincClipSegmentAddPoints(cursor, p, c, &ic);
    CORNER(-, a, +, b); VincClipSegmentAddPoints(cursor, p, c, &ic);
#undef CORNER
}

/* Mac 0x4e056a, Win 0x81fc40 (cdecl): *sep = height of the box's lowest point above z = 0 (rel =
 * box in the plane frame); dims 0x200 | number of box axes (nearly) parallel to the plane; the
 * clipped edge points (plane frame) are appended at *cursor when *sep <= *pad. Returns 1 when
 * points were added. */
static int BoxPlaneContact(MeReal* sep, MeReal** cursor, MeI16* dims, const MeReal* pad, const MeReal* r,
                           const MeReal* rel)
{
    MeReal a0 = fabs(rel[2]), a1 = fabs(rel[6]), a2 = fabs(rel[10]);
    MeReal s = rel[14] - ((r[1] * a1 + r[0] * a0) + r[2] * a2);
    MeReal* start = *cursor;
    int skip = *pad < s;
    *sep = s;
    *dims = (MeI16)(((a0 < 0.0001) + (a1 < 0.0001) + (a2 < 0.0001)) | 0x200);
    if (!skip)
        BoxPlanePoints(cursor, rel, r);
    return *cursor != start;
}

/* Mac 0x4e06b3, Win 0x81fd00: time until the box's deepest point along the plane normal reaches
 * the plane, from the relative linear velocity (0 when it is already below) */
int MEAPI VincBoxPlaneSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID box, plane;
    MeReal *tm2, *v2, *v1, dist, t;
    MeVector3 rv, n, p;
    result->pair = pair;
    box = VincModelGetGeometry(pair->model1);
    plane = VincModelGetGeometry(pair->model2);
    tm2 = VincModelGetTransformPtr(pair->model2);
    v2 = VincModelGetLinearVelocityPtr(pair->model2);
    v1 = VincModelGetLinearVelocityPtr(pair->model1);
    rv[0] = v1[0] - v2[0];
    rv[1] = v1[1] - v2[1];
    rv[2] = v1[2] - v2[2];
    VincPlaneGetNormal(plane, (MeReal(*)[4])VincModelGetTransformPtr(pair->model2), n);
    n[0] = n[0] * -1.0;
    n[1] = n[1] * -1.0;
    n[2] = n[2] * -1.0;
    VincBoxMaximumPoint(box, (MeReal(*)[4])VincModelGetTransformPtr(pair->model1), n, p);
    dist = VincPlaneGetDistance(plane, (MeReal(*)[4])tm2, p);
    if (dist < 0.0) {
        result->time = 0.0;
        return 1;
    }
    t = dist / ((rv[0] * n[0] + rv[1] * n[1]) + rv[2] * n[2]);
    result->time = t;
    if (maxTime < t)
        return 0;
    if (t < 0.0)
        result->time = 0.0;
    return 1;
}

/* Mac 0x4e0890, Win 0x81fe40 (model1 = box, model2 = plane): up to 24 contacts at the clipped box
 * edge points below the plane, normal = the plane's z axis. NOTE: returns the simplified contact
 * count > 0 without setting result->touch (both builds). */
int MEAPI VincBoxPlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* r = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    MeReal rel[16], points[24 * 3], sep;
    MeReal *cursor = points, *p, *rtm = tm1;
    VincContact contacts[24];
    VincRequest* rq;
    MeI16 dims;
    int cnt;
    if (tm2 != 0) {
        VincTMMakeRelative(tm1, tm2, rel);
        rtm = rel;
    }
    ResultClear(result);
    if (!BoxPlaneContact(&sep, &cursor, &dims, &pad, r, rtm))
        return 0;
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0) {
        ResultClear(result);
        return 1;
    }
    if (tm2 == 0) {
        result->normal[0] = 0.0;
        result->normal[1] = 0.0;
        result->normal[2] = 1.0;
    } else {
        result->normal[0] = tm2[8];
        result->normal[1] = tm2[9];
        result->normal[2] = tm2[10];
    }
    ResultClear(result);
    for (p = points; p != cursor; p += 3) {
        MeReal depth;
        if (result->contactCount >= 24)
            break;
        depth = p[2];
        if (depth < sep * 0.01) {
            VincContact* c = &contacts[result->contactCount];
            if (tm2 == 0) {
                c->position[0] = p[0];
                c->position[1] = p[1];
                c->position[2] = p[2];
            } else {
                VincTMTransform(tm2, p, c->position);
            }
            c->separation = depth;
            c->dims = dims;
            c->normal[0] = result->normal[0];
            c->normal[1] = result->normal[1];
            c->normal[2] = result->normal[2];
            ++result->contactCount;
        }
    }
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    return result->contactCount > 0;
}

/* Mac 0x4e0639, Win 0x820020 */
int MEAPI VincBoxPlaneRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincBoxPlaneIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincBoxPlaneSafeTime;
    VincGeometryPairRegisterInteractions(VincBoxGetTypeId(), VincPlaneGetTypeId(), &i, 0);
    return 1;
}
