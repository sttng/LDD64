// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Convex pair test (restored;
// Mac 557c 0x4c6f3d-0x4c7e2c, Win 0x75b980 / 0x75bec0 paired). The tree walk runs in the
// mesh frame: each node box is tested as a (shared, rescaled) box hull against the convex with V3.1's
// convex-convex test; leaves go through the one-triangle hull (VincConvexTriangleContacts).
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincConvexInteractions.h"

#include <string.h>

using namespace VincTriMeshPairs;

namespace {

// Mac 0x4c6f50 (name inferred). rel: mesh in the convex frame, inv: convex in the mesh frame.
void ConvexContacts(VincIntersectResult* r, VincGeometryID convex, VincGeometryID mesh, VincOBBNode* node,
                    const MeReal* rel, const MeReal* inv, MeReal padding)
{
    if (IsLeaf(node)) {
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, 0, 0, &t, true);
        int count = 0;
        if (VincConvexTriangleContacts(convex, inv, &t, r->contactMaxCount - r->contactCount,
                                       r->contacts + r->contactCount, &count) != 0) {
            r->contactCount += count;
            r->touch = 1;
        }
        return;
    }
    MeReal obb[16];
    NodeBoxTM(node, rel, obb);
    MeReal pad = padding;
    VincConvexBoxHull = VincConvexBoxHullUpdate(VincConvexBoxHull, node->halfExtents);
    MeReal identity[16] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    VincConvexDesc a, b;
    a.convex = VincConvexBoxHull;
    a.tm = obb;
    a.margin = 0.0;
    b.convex = ((VincConvexGeometry*)convex)->hull;
    b.tm = identity;
    b.margin = ((VincConvexGeometry*)convex)->margin;
    VincIntersectResult probe;
    memcpy(&probe, r, sizeof(probe));
    probe.contactMaxCount = 0;
    if (VincConvexConvexIntersectCached(&probe, &a, &b, &pad, (CnvPairCache*)r->pair->pairData, 0) != 0) {
        ConvexContacts(r, convex, mesh, node->child[0], rel, inv, padding);
        if (r->contactMaxCount != 0 || r->touch == 0)
            ConvexContacts(r, convex, mesh, node->child[1], rel, inv, padding);
    }
}

} // namespace

extern "C" {

// Mac 0x4c771f, Win 0x75bc00 (name inferred)
int MEAPI VincTriangleMeshConvexIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1000];   // Mac: on the stack (76000 bytes)
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    result->touch = 0;
    MeReal pad1 = VincModelGetPadding(pair->model1);
    MeReal pad2 = VincModelGetPadding(pair->model2);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincGeometryID convex = VincModelGetGeometry(pair->model2);
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmCvx = VincModelGetTransformPtr(pair->model2);
    MeReal rel[16];
    RelativeTM(tmMesh, tmCvx, rel);
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= request->contactMaxCount)
        maxc = result->contactMaxCount;
    if (maxc == 0)
        result->contactMaxCount = 0;
    ResultReset(result);
    // the inverse of rel: transposed rotation, translation -t R^T
    MeReal inv[16];
    inv[0] = rel[0];
    inv[1] = rel[4];
    inv[2] = rel[8];
    inv[4] = rel[1];
    inv[5] = rel[5];
    inv[6] = rel[9];
    inv[8] = rel[2];
    inv[9] = rel[6];
    inv[10] = rel[10];
    inv[3] = inv[7] = inv[11] = inv[15] = 0.0;
    MeReal tz = -rel[14], ty = -rel[13], tx = -rel[12];
    inv[12] = tx * rel[0] + ty * rel[1] + tz * rel[2];
    inv[13] = tx * rel[4] + ty * rel[5] + tz * rel[6];
    inv[14] = tx * rel[8] + ty * rel[9] + tz * rel[10];
    ConvexContacts(result, convex, mesh, tree->nodes, rel, inv, pad1 + pad2);
    if (maxc == 0) {
        ResultReset(result);
        saved.Restore(result);   // NOTE: the original leaves its stack buffer in the result
        return result->touch;
    }
    if (result->contactCount == 0) {
        saved.Restore(result);   // NOTE: as above
        return 0;
    }
    saved.Restore(result);
    result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    // contacts from the mesh frame to world, normals flipped (towards the mesh)
    for (int i = 0; i < result->contactCount; ++i) {
        VincContact* c = &result->contacts[i];
        MeReal n[3] = {-c->normal[0], -c->normal[1], -c->normal[2]};
        VincTMRotate(tmMesh, n, c->normal);
        MeReal p[3] = {c->position[0], c->position[1], c->position[2]};
        VincTMTransform(tmMesh, p, c->position);
    }
    MeReal n[3] = {-result->normal[0], -result->normal[1], -result->normal[2]};
    VincTMRotate(tmMesh, n, result->normal);
    result->touch = 0 < result->contactCount;
    return result->touch;
}

namespace {
// Mac 0x4c76ec, Win 0x75b980 (name inferred)
void MEAPI ConvexTriangleHullTerm3(void)
{
    if (VincConvexTriangleHull != 0)
        cnvDestroyTriangle(VincConvexTriangleHull);
    VincConvexTriangleHull = 0;
}
} // namespace

// Mac 0x4c75ff, Win 0x75bec0 (name inferred)
int MEAPI VincTriangleMeshConvexRegisterInteraction(void)
{
    MeReal v[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0};   // Mac 0x644dc0
    MeReal normal[3] = {1.0, 0.0, 0.0};
    if (VincConvexTriangleHull == 0)
        VincConvexTriangleHull = cnvCreateTriangle(v, v + 3, v + 6, normal);
    VincRegisterTerminateAction(ConvexTriangleHullTerm3);
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = VincTriangleMeshConvexIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;           // NOTE: left uninitialised by the original
    i.distanceBoundedFn = 0;
    int convexType = VincConvexGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshGetTypeId(), convexType, &i, 0);
    return 1;
}

} // extern "C"
