// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Plane pair test (restored;
// Mac 557d 0x4c97a1-0x4c9ed4, Win 0x7d4e10 / 0x7d4f70). Walks the mesh's OBB tree in the plane
// frame (a node is visited when its box reaches z <= 0) and collects the triangle-plane contacts of
// 558a into a 1000-contact buffer, then simplifies them.
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

// ResultReset: the original's static result reset of this file, Mac 0x4c97a1 (Win: inlined)

namespace {

// Mac 0x4c982a, Win 0x7d4e10 (name inferred). rel = the mesh frame in the plane frame.
void PlaneContacts(VincIntersectResult* r, VincGeometryID mesh, VincOBBNode* node, const MeReal* tmMesh,
                   const MeReal* tmPlane, const MeReal* rel)
{
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, false);
        VincTrianglePlaneContacts(r, &t, tmPlane, 1e-14);
        return;
    }
    MeReal obb[16];
    NodeBoxTM(node, rel, obb);
    if (obb[14] <= fabs(obb[2]) * node->halfExtents[0] + fabs(obb[6]) * node->halfExtents[1]
                       + fabs(obb[10]) * node->halfExtents[2]) {
        PlaneContacts(r, mesh, node->child[0], tmMesh, tmPlane, rel);
        PlaneContacts(r, mesh, node->child[1], tmMesh, tmPlane, rel);
    }
}

} // namespace

extern "C" {

// Mac 0x4c9ed4, Win 0x7d4f70 (name inferred)
int MEAPI VincTriangleMeshPlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1000];   // the original: on the stack
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincModelGetGeometry(pair->model2);
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmPlane = VincModelGetTransformPtr(pair->model2);
    MeReal rel[16];
    RelativeTM(tmMesh, tmPlane, rel);
    ResultReset(result);
    PlaneContacts(result, mesh, tree->nodes, tmMesh, tmPlane, rel);
    if (result->contactCount == 0) {
        // NOTE: the original returns with result->contacts still pointing at its stack buffer
        // (and contactMaxCount 1000); restored to the caller's array (as mesh pairs do).
        saved.Restore(result);
        return 0;
    }
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= maxc)
        maxc = result->contactMaxCount;
    if (maxc == 0) {
        ResultReset(result);
        saved.Restore(result);   // NOTE: not restored by the original (see above)
        return 1;
    }
    saved.Restore(result);
    result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    FlipNormals(result);
    MeReal len = sqrt(result->normal[0] * result->normal[0] + result->normal[1] * result->normal[1]
                      + result->normal[2] * result->normal[2]);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        result->normal[0] = result->normal[0] * inv;
        result->normal[1] = result->normal[1] * inv;
        result->normal[2] = inv * result->normal[2];
    }
    result->touch = 0 < result->contactCount;
    return result->touch;
}

// Mac 0x4c97b4 (Win: inlined into the registration list 0x75cae0 / VxFrame) (name inferred)
int MEAPI VincTriangleMeshPlaneRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshPlaneIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 plane = VincPlaneGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshInteractionsGetTypeId(), plane, &i, 0);
    return 1;
}

} // extern "C"
