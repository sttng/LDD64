/* Vortex 5.0.0 build 800 "vortexldd41" - capsule / triangle list pair test (restored).
 * Mac 0x4e5374-0x4e5da3 (557f), Win 0x7d7400 / 0x7d78d0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincMath.h"

#include <math.h>
#include <string.h>

/* VincTriangleCapsuleContacts takes a 7th argument on both builds (Mac: two zero words, Win: a
 * double 0.0) that it never reads; Vinc/VincTriangleContacts.h declares the six it uses. */
typedef int (*VincTriangleCapsuleContacts7Fn)(VincIntersectResult*, const VincTriangle*, MeReal, MeReal,
                                              const MeReal*, MeReal, MeReal);

/* Mac 0x4e5374 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* p = tm * q */
static void TransformPoint(const MeReal* tm, const MeReal* q, MeReal* p)
{
    p[0] = q[0] * tm[0] + q[1] * tm[4] + q[2] * tm[8] + tm[12];
    p[1] = q[0] * tm[1] + q[1] * tm[5] + q[2] * tm[9] + tm[13];
    p[2] = q[0] * tm[2] + q[1] * tm[6] + q[2] * tm[10] + tm[14];
}

/* Mac 0x4e53fd, Win 0x7d7400 (model1 = capsule, model2 = triangle list). The triangles in the
 * capsule's bounding box (in the list's frame) come from the list's box generator; each one, moved
 * to world space, goes to VincTriangleCapsuleContacts, which appends the contacts to the result
 * directly (stopping 4 short of the result's capacity). Too many contacts are reduced with
 * VincContactSimplifyResult; the summed normal is normalised. */
int MEAPI VincCapsuleTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincTriangle tri;
    MeVector3 v0, v1, v2, minCorner, maxCorner;
    MeReal rel[16], aabb[6];
    VincGeometryID g1;
    VincTriangleList* g2;
    MeReal pad1, pad2, r, hh, len;
    MeReal *tm1, *tm2, *tmA;
    int n, maxCount, k;
    tri.index = -1;
    tri.edgeData0[0] = -2.0;
    tri.edgeData1[0] = 0.0;
    tri.edgeData1[1] = 0.0;
    tri.edgeData1[2] = 0.0;
    tri.edgeData0[1] = tri.edgeData0[0];
    tri.edgeData0[2] = tri.edgeData0[0];
    g1 = VincModelGetGeometry(pair->model1);
    g2 = (VincTriangleList*)VincModelGetGeometry(pair->model2);
    pad1 = VincModelGetPadding(pair->model1);
    pad2 = VincModelGetPadding(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    r = VincCapsuleGetRadius(g1);
    hh = VincCapsuleGetHalfHeight(g1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    VincCapsuleGetBoundingRadius(g1); /* result unused */
    ResultClear(result);
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    g2->triangleCount = 0;
    tmA = VincModelGetTransformPtr(pair->model1);
    VincTMMakeRelative(tmA, tm2, rel);
    VincGeometryUpdateAABB(g1, (MeReal(*)[4])rel, minCorner, maxCorner, aabb);
    g2->generator(pair, minCorner, maxCorner, 0);
    if (g2->triangleCount == 0)
        return 0;
    tri.vertices[0] = v0;
    tri.vertices[1] = v1;
    tri.vertices[2] = v2;
    n = g2->triangleCount;
    if (g2->triangleMaxCount <= n)
        n = g2->triangleMaxCount;
    maxCount = VincModelPairGetRequestPtr(pair)->contactMaxCount;
    if (result->contactMaxCount <= maxCount)
        maxCount = result->contactMaxCount;
    if (0 < n && result->contactCount < result->contactMaxCount - 4) {
        k = 0;
        do {
            const VincTriangleListTri* t = &g2->triangles[k];
            TransformPoint(tm2, t->vertices[0], v0);
            TransformPoint(tm2, t->vertices[1], v1);
            TransformPoint(tm2, t->vertices[2], v2);
            if (t->m10[0]) { /* NOTE: per-edge floats; the values stay for the next triangles */
                const float* const* e = (const float* const*)t->m10;
                tri.edgeData0[0] = (MeReal)*e[0];
                tri.edgeData0[1] = (MeReal)*e[1];
                tri.edgeData0[2] = (MeReal)*e[2];
                tri.edgeData1[0] = (MeReal)*e[3];
                tri.edgeData1[1] = (MeReal)*e[4];
                tri.edgeData1[2] = (MeReal)*e[5];
            }
            VincTMRotate(tm2, t->normal, tri.normal);
            tri.edges[0][2] = v1[2] - v0[2];
            tri.edges[0][1] = v1[1] - v0[1];
            tri.edges[0][0] = v1[0] - v0[0];
            tri.edges[1][2] = v2[2] - v1[2];
            tri.edges[1][1] = v2[1] - v1[1];
            tri.edges[1][0] = v2[0] - v1[0];
            tri.edges[2][2] = v0[2] - v2[2];
            tri.edges[2][1] = v0[1] - v2[1];
            tri.edges[2][0] = v0[0] - v2[0];
            tri.index = t->userId;
            ((VincTriangleCapsuleContacts7Fn)(void (*)(void))VincTriangleCapsuleContacts)(result, &tri, r, hh, tm1,
                                                                                        pad1 + pad2, 0.0);
            if (maxCount == 0 && 0 < result->contactCount) {
                ResultClear(result);
                return 1;
            }
            ++k;
        } while (k != n && result->contactCount < result->contactMaxCount - 4);
    }
    if (result->contactCount == 0)
        return 0;
    if (maxCount < result->contactCount)
        VincContactSimplifyResult(result);
    len = sqrt(result->normal[0] * result->normal[0] + result->normal[1] * result->normal[1]
               + result->normal[2] * result->normal[2]);
    if (len == 0.0)
        return 1;
    len = 1.0 / len;
    result->normal[0] = result->normal[0] * len;
    result->normal[1] = result->normal[1] * len;
    result->normal[2] = len * result->normal[2];
    return 1;
}

/* Mac 0x4e5387, Win 0x7d78d0 */
int MEAPI VincCapsuleTriangleListRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCapsuleTriangleListIntersect;
    VincGeometryPairRegisterInteractions(VincCapsuleGetTypeId(), VincTriangleListGetTypeId(), &i, 0);
    return 1;
}
