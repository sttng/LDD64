/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / triangle list pair test (restored).
 * Mac 0x4f1ea5-0x4f1f2e (557g), Win 0x7db910-0x7dbff0. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <stdlib.h>
#include <string.h>

/* Mac 0x7797b8 / Win 0xac982c: size of the raw contact buffer (never changed) (name inferred) */
int VincCylinderTriangleListMaxContacts = 400;

/* Mac 0x4f1ea5 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f1f2e, Win 0x7db910 (model1 = cylinder, model2 = triangle list). The triangles in the
 * cylinder's box come from the list's box generator; each is moved into the cylinder's frame and
 * handed to VincCylinderTriangleContacts; the raw contacts are reduced by VincContactSimplify. */
int MEAPI VincCylinderTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincTriangle tri;
    MeVector3 v0, v1, v2, minB, maxB, n, nW;
    MeReal aabb[6], rel1[16], rel2[16];
    MeReal points[64 * 3], seps[64];
    MeReal out1, out2;
    VincGeometryID g1;
    VincTriangleList* g2;
    MeReal pad, r, hh, tol;
    MeReal *tm1, *tm2;
    VincContact *tmp, *c;
    VincRequest* rq;
    MeI16 dims;
    int cnt, k, flag;
    tri.index = -1;
    tri.edgeData0[0] = -2.0;
    tri.edgeData1[0] = 0.0;
    tri.edgeData1[1] = 0.0;
    tri.edgeData1[2] = 0.0;
    tri.edgeData0[1] = tri.edgeData0[0];
    tri.edgeData0[2] = tri.edgeData0[0];
    g1 = VincModelGetGeometry(pair->model1);
    g2 = (VincTriangleList*)VincModelGetGeometry(pair->model2);
    pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    r = VincCylinderGetRadius(g1);
    hh = VincCylinderGetHalfHeight(g1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    ResultClear(result);
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    g2->triangleCount = 0;
    /* the cylinder's box in the list's frame */
    VincTMMakeRelative(VincModelGetTransformPtr(pair->model1), tm2, rel1);
    rel1[3] = rel1[7] = rel1[11] = 0.0; /* (not set by the original; unused) */
    rel1[15] = 1.0;
    VincGeometryUpdateAABB(g1, (double (*)[4])rel1, minB, maxB, aabb);
    g2->generator(pair, minB, maxB, 0);
    if (g2->triangleCount == 0)
        return 0;
    /* the original uses alloca (MSVC: __alloca_probe_16) */
    tmp = (VincContact*)malloc((size_t)VincCylinderTriangleListMaxContacts * sizeof(VincContact));
    tri.vertices[0] = v0;
    tri.vertices[1] = v1;
    tri.vertices[2] = v2;
    rq = VincModelPairGetRequestPtr(pair);
    VincTMMakeRelative(tm2, tm1, rel2); /* the list in the cylinder's frame */
    cnt = g2->triangleCount;
    if (g2->triangleMaxCount <= cnt)
        cnt = g2->triangleMaxCount;
    flag = ((VincCylinder*)g1)->flag;
    c = tmp;
    if (0 < cnt) {
        tol = pad / 1000.0;
        for (k = 0; k < cnt; ++k) {
            const VincTriangleListTri* t = &g2->triangles[k];
            MeReal* pc = points;
            MeReal* sc = seps;
            MeReal cosTol;
            VincTMTransform(rel2, t->vertices[0], v0);
            VincTMTransform(rel2, t->vertices[1], v1);
            VincTMTransform(rel2, t->vertices[2], v2);
            if (t->m10[0]) { /* NOTE: per-edge floats; the values stay for the next triangles */
                const float* const* e = (const float* const*)t->m10;
                tri.edgeData0[0] = (MeReal)*e[0];
                tri.edgeData0[1] = (MeReal)*e[1];
                tri.edgeData0[2] = (MeReal)*e[2];
                tri.edgeData1[0] = (MeReal)*e[3];
                tri.edgeData1[1] = (MeReal)*e[4];
                tri.edgeData1[2] = (MeReal)*e[5];
            }
            VincTMRotate(rel2, t->normal, tri.normal);
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
            cosTol = VincCylinderGetCosTolerance(g1);
            if (!VincCylinderTriangleContacts(&out1, n, &out2, &pc, &dims, &tol, &r, &hh, &tri, cosTol, flag, &sc))
                continue;
            if (rq->contactMaxCount == 0) {
                ResultClear(result);
                free(tmp);
                return 1;
            }
            VincTMRotate(tm1, n, nW);
            sc = seps;
            if (pc != points && result->contactCount < VincCylinderTriangleListMaxContacts) {
                const MeReal* p = points;
                for (;;) {
                    VincTMTransform(tm1, p, c->position);
                    c->dims = dims;
                    c->separation = *sc++;
                    c->element1 = 0;
                    c->element2 = t->userId;
                    c->normal[0] = nW[0];
                    c->normal[1] = nW[1];
                    c->normal[2] = nW[2];
                    ++c;
                    ++result->contactCount;
                    p += 3;
                    if (p == pc || !(result->contactCount < VincCylinderTriangleListMaxContacts))
                        break;
                }
            }
            result->normal[0] = result->normal[0] + nW[0];
            result->normal[1] = result->normal[1] + nW[1];
            result->normal[2] = result->normal[2] + nW[2];
        }
    }
    k = 0;
    if (result->contactCount != 0) {
        k = VincContactSimplify(result->normal, tmp, result->contactCount, result);
        result->contactCount = k;
    }
    free(tmp);
    return k > 0;
}

/* Mac 0x4f1eb8, Win 0x7dbff0 */
int MEAPI VincCylinderTriangleListRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincCylinderTriangleListIntersect;
    VincGeometryPairRegisterInteractions(VincCylinderGetTypeId(), VincTriangleListGetTypeId(), &i, 0);
    return 1;
}
