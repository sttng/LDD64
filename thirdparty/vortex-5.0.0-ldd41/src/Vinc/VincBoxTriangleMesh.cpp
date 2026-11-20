// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Box pair test (restored;
// Mac 557c 0x4c4ff3-0x4c5e5b). Walks the mesh's OBB tree in the box frame and collects the
// triangle-box contacts of 558a into a 1000-contact buffer, then simplifies them.
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

namespace {

VincRequest s_requestNoPair = {6, 0, 0.0, 0.0, 0, 0.0, 0, 0, 0.0};   // Mac 0x779780

// Mac 0x4c507c, Win 0x7d4500 (name inferred)
void BoxContacts(VincIntersectResult* r, const MeReal* radii, VincGeometryID mesh, VincOBBNode* node,
                 const MeReal* tmMesh, const MeReal* tmBox, const MeReal* rel)
{
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, true);
        VincTriangleBoxContacts(r, &t, radii, tmBox, 1e-14);
        return;
    }
    MeReal obb[16];
    NodeBoxTM(node, rel, obb);
    if (VincOBBDisjoint(obb, radii, node->halfExtents) == 0) {
        BoxContacts(r, radii, mesh, node->child[0], tmMesh, tmBox, rel);
        BoxContacts(r, radii, mesh, node->child[1], tmMesh, tmBox, rel);
    }
}

// Mac 0x4c5781, Win 0x7d46a0 (name inferred): touch only
bool BoxTouch(VincIntersectResult* r, const MeReal* radii, VincGeometryID mesh, VincOBBNode* node,
              const MeReal* tmMesh, const MeReal* tmBox, const MeReal* rel)
{
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, false);
        int hit = VincTriangleBoxContacts(r, &t, radii, tmBox, 1e-14);
        ResultReset(r);
        return hit != 0;
    }
    MeReal obb[16];
    NodeBoxTM(node, rel, obb);
    if (VincOBBDisjoint(obb, radii, node->halfExtents) != 0)
        return false;
    if (BoxTouch(r, radii, mesh, node->child[0], tmMesh, tmBox, rel))
        return true;
    return BoxTouch(r, radii, mesh, node->child[1], tmMesh, tmBox, rel);
}

} // namespace

extern "C" {

// Mac 0x4c5e5b, Win 0x7d4860 (name inferred)
int MEAPI VincTriangleMeshBoxIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1000];   // Mac: on the stack (76000 bytes)
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    const MeReal* radii = VincBoxGetRadii(VincModelGetGeometry(pair->model2));
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmBox = VincModelGetTransformPtr(pair->model2);
    MeReal rel[16];
    RelativeTM(tmMesh, tmBox, rel);
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    if (request == 0)
        request = &s_requestNoPair;
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= request->contactMaxCount)
        maxc = result->contactMaxCount;
    ResultReset(result);
    if (maxc == 0) {
        bool touch = BoxTouch(result, radii, mesh, tree->nodes, tmMesh, tmBox, rel);
        // NOTE: the original returns here with result->contacts still pointing at its stack buffer
        // (and contactMaxCount 1000); restored to the caller's array instead.
        saved.Restore(result);
        return touch;
    }
    BoxContacts(result, radii, mesh, tree->nodes, tmMesh, tmBox, rel);
    if (result->contactCount == 0) {
        saved.Restore(result);   // NOTE: not restored by the original (see above)
        return 0;
    }
    saved.Restore(result);
    result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    FlipNormals(result);
    result->touch = 0 < result->contactCount;
    return result->touch;
}

// Mac 0x4c5006 (name inferred)
int MEAPI VincTriangleMeshBoxRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshBoxIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 box = VincBoxGetTypeId();
    // the original reads the mesh type through wrapper Mac 0x4d1f97 (same value)
    VincGeometryPairRegisterInteractions(VincTriangleMeshGetTypeId(), box, &i, 0);
    return 1;
}

} // extern "C"
