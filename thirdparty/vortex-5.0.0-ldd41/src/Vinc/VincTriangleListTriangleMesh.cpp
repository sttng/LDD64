// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs TriangleList pair test (restored,
// Mac 557d 0x4cf8ad, 0x4cfda4, 0x4d0677; Win 0x766050 / 0x766750). The list's
// generator collects the triangles near the mesh's box; each is taken into the mesh frame and run
// down the mesh's OBB tree (VincTriangleMeshTriangleNode). The contacts go straight into the
// caller's array.
#include "VincTriangleMeshPairs.h"
#include "VincTriangleMeshFinish.h"

#include <float.h>
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

// ResultReset: the original's static result reset of this file, Mac 0x4cf8ad (Win: inlined)

extern "C" {

// Mac 0x4d0677, Win 0x766050 (name inferred)
int MEAPI VincTriangleMeshTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincTriangleList* tl = (VincTriangleList*)VincModelGetGeometry(pair->model2);
    VincTriangleMesh* m = (VincTriangleMesh*)VincModelGetGeometry(pair->model1);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmList = VincModelGetTransformPtr(pair->model2);
    ResultReset(result);
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    tl->triangleCount = 0;
    // the mesh's box in the list frame -> the list's triangles there
    MeReal rel[16];
    rel[3] = rel[7] = rel[11] = rel[15] = DBL_MAX;
    RelativeTM(tmMesh, tmList, rel);
    MeVector3 lo, hi;
    MeReal aabb6[6];
    VincGeometryUpdateAABB(mesh, (MeReal(*)[4])rel, lo, hi, aabb6);
    if (tl->generator(pair, lo, hi, 0) == 0)
        return 0;
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    MeReal inv[16];
    RelativeTM(tmList, tmMesh, inv);
    int count = tl->triangleCount;
    if (tl->triangleMaxCount <= count)
        count = tl->triangleMaxCount;
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= maxc)
        maxc = result->contactMaxCount;
    VincOBBTree* tree = m->tree;
    MeReal v[3][3];
    VincTriangleRef t;
    t.index = -1;
    t.edgeData0[0] = -2.0;
    t.edgeData1[1] = 0.0;
    t.edgeData1[0] = 0.0;
    t.edgeData1[2] = 0.0;
    t.vertices[0] = v[0];
    t.vertices[1] = v[1];
    t.vertices[2] = v[2];
    VincContactMergeParams params;
    params.maxCount = 12;
    params.distanceSq = m->m6c * m->m6c;
    params.cosTolerance = m->m74;
    t.edgeData0[1] = t.edgeData0[0];
    t.edgeData0[2] = t.edgeData0[0];
    for (int k = 0; k < count; ++k) {
        const VincTriangleListTri* s = &tl->triangles[k];   // stride 0x2c on 32-bit (PORT64: the element size)
        if (s->m10[0]) {
            // NOTE: per-edge floats; the values stay for the next triangles. edgeData1[0] / [1] are
            // read from the 6th / 5th pointer and edgeData1[2] stays 0 (as in the original).
            const float* const* e = (const float* const*)s->m10;
            t.edgeData0[0] = (MeReal)*e[0];
            t.edgeData0[1] = (MeReal)*e[1];
            t.edgeData0[2] = (MeReal)*e[2];
            t.edgeData1[1] = (MeReal)*e[4];
            t.edgeData1[0] = (MeReal)*e[5];
        }
        t.index = s->userId;
        VincTMTransform(inv, s->vertices[0], v[0]);
        VincTMTransform(inv, s->vertices[1], v[1]);
        VincTMTransform(inv, s->vertices[2], v[2]);
        VincTriangleUpdateEdges(&t);
        VincTMRotate(inv, s->normal, t.normal);
        VincTriangleMeshTriangleNode(result, &t, mesh, tree->nodes, pad, &params);
        if (maxc == 0 && 0 < result->contactCount) {
            ResultReset(result);
            return 1;
        }
    }
    return FinishMeshContacts(result, tmMesh);
}

// Mac 0x4cfda4, Win 0x766750 (name inferred)
int MEAPI VincTriangleMeshTriangleListRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshTriangleListIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 list = VincTriangleListGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshInteractionsGetTypeId(), list, &i, 0);
    return 1;
}

} // extern "C"
