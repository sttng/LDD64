/* Vortex 5.0.0 build 800 "vortexldd41" - sphere / triangle list pair test (restored).
 * Mac 0x4f4777-0x4f4800 (557g), Win 0x7dc730-0x7dcd20. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"

#include <stdlib.h>
#include <string.h>

/* Mac 0x7797bc / Win 0xac987c: size of the raw contact buffer of the triangle-list tests (never
 * changed) (name inferred) */
int VincTriangleListMaxContacts = 400;

/* Mac 0x4f4777 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* Mac 0x4f4800, Win 0x7dc730 (model1 = sphere, model2 = triangle list). The triangles near the
 * sphere come from the list's sphere generator; each is moved into a frame centred on the sphere
 * and handed to VincSphereTriangleContact; the raw contacts are reduced by VincContactSimplify. */
int MEAPI VincSphereTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincTriangle tri;
    MeVector3 v0, v1, v2, cl, off, n, p;
    VincGeometryID g1;
    VincTriangleList* g2;
    MeReal sr, pad, sep;
    MeReal *tm1, *c1, *tm2;
    VincContact* tmp;
    VincContact* c;
    VincRequest* rq;
    MeI16 dims;
    int cnt, k, touch;
    tri.index = -1;
    tri.edgeData0[0] = -2.0;
    tri.edgeData1[0] = 0.0;
    tri.edgeData1[1] = 0.0;
    tri.edgeData1[2] = 0.0;
    tri.edgeData0[1] = tri.edgeData0[0];
    tri.edgeData0[2] = tri.edgeData0[0];
    g1 = VincModelGetGeometry(pair->model1);
    g2 = (VincTriangleList*)VincModelGetGeometry(pair->model2);
    sr = VincSphereGetRadius(g1);
    tm1 = VincModelGetTransformPtr(pair->model1);
    c1 = tm1 + 12;
    tm2 = VincModelGetTransformPtr(pair->model2);
    pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    ResultClear(result);
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    g2->triangleCount = 0;
    VincTMInverseTransform(tm2, c1, cl);
    g2->sphereGenerator(pair, cl, sr + pad, 0);
    tri.vertices[0] = v0;
    tri.vertices[1] = v1;
    tri.vertices[2] = v2;
    /* the original uses alloca (MSVC: __alloca_probe_16) */
    tmp = (VincContact*)malloc((size_t)VincTriangleListMaxContacts * sizeof(VincContact));
    off[0] = tm2[12] - c1[0];
    off[1] = tm2[13] - c1[1];
    off[2] = tm2[14] - c1[2];
    rq = VincModelPairGetRequestPtr(pair);
    cnt = g2->triangleCount;
    if (g2->triangleMaxCount <= cnt)
        cnt = g2->triangleMaxCount;
    c = tmp;
    for (k = 0; k < cnt; ++k) {
        const VincTriangleListTri* t = &g2->triangles[k];
        const MeReal* q;
        q = t->vertices[0];
        v0[0] = q[0] * tm2[0] + q[1] * tm2[4] + q[2] * tm2[8] + off[0];
        v0[1] = q[0] * tm2[1] + q[1] * tm2[5] + q[2] * tm2[9] + off[1];
        v0[2] = q[0] * tm2[2] + q[1] * tm2[6] + q[2] * tm2[10] + off[2];
        q = t->vertices[1];
        v1[0] = q[0] * tm2[0] + q[1] * tm2[4] + q[2] * tm2[8] + off[0];
        v1[1] = q[0] * tm2[1] + q[1] * tm2[5] + q[2] * tm2[9] + off[1];
        v1[2] = q[0] * tm2[2] + q[1] * tm2[6] + q[2] * tm2[10] + off[2];
        q = t->vertices[2];
        v2[0] = q[0] * tm2[0] + q[1] * tm2[4] + q[2] * tm2[8] + off[0];
        v2[1] = q[0] * tm2[1] + q[1] * tm2[5] + q[2] * tm2[9] + off[1];
        v2[2] = q[0] * tm2[2] + q[1] * tm2[6] + q[2] * tm2[10] + off[2];
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
        if (t->m10[0]) { /* NOTE: per-edge floats; the values stay for the next triangles */
            const float* const* e = (const float* const*)t->m10;
            tri.edgeData0[0] = (MeReal)*e[0];
            tri.edgeData0[1] = (MeReal)*e[1];
            tri.edgeData0[2] = (MeReal)*e[2];
            tri.edgeData1[0] = (MeReal)*e[3];
            tri.edgeData1[1] = (MeReal)*e[4];
            tri.edgeData1[2] = (MeReal)*e[5];
        }
        touch = VincSphereTriangleContact(&sep, n, p, &dims, &pad, &sr, &tri);
        if (!touch)
            continue;
        if (rq->contactMaxCount == 0) {
            ResultClear(result);
            free(tmp);
            return 1;
        }
        if (!(result->contactCount < VincTriangleListMaxContacts))
            continue;
        p[0] = p[0] + c1[0];
        p[1] = p[1] + c1[1];
        p[2] = p[2] + c1[2];
        c->position[0] = p[0];
        c->position[1] = p[1];
        c->position[2] = p[2];
        c->dims = dims;
        if ((dims >> 8) == 2) {
            c->normal[0] = tri.normal[0];
            c->normal[1] = tri.normal[1];
            c->normal[2] = tri.normal[2];
            if (0.0 <= tri.normal[0] * n[0] + tri.normal[1] * n[1] + tri.normal[2] * n[2]) {
                c->separation = sep;
            } else {
                MeReal s = sr * -2.0 - sep; /* the sphere centre is behind the face */
                c->separation = s;
                if (s < -sr)
                    continue;
            }
        } else {
            c->normal[0] = n[0];
            c->normal[1] = n[1];
            c->normal[2] = n[2];
            c->separation = sep;
        }
        result->normal[0] = result->normal[0] + n[0];
        result->normal[1] = result->normal[1] + n[1];
        result->normal[2] = result->normal[2] + n[2];
        c->element1 = -1;
        c->element2 = t->userId;
        ++result->contactCount;
        ++c;
    }
    k = 0;
    if (result->contactCount != 0) {
        k = VincContactSimplify(result->normal, tmp, result->contactCount, result);
        result->contactCount = k;
    }
    free(tmp);
    return k > 0;
}

/* Mac 0x4f478a, Win 0x7dcd20 */
int MEAPI VincSphereTriangleListRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincSphereTriangleListIntersect;
    VincGeometryPairRegisterInteractions(VincSphereGetTypeId(), VincTriangleListGetTypeId(), &i, 0);
    return 1;
}
