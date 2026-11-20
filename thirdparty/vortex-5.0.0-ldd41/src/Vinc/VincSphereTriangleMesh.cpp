// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Sphere pair test (restored;
// Mac 557d 0x4cb774-0x4cbeb3, Win 0x7d4a80-0x7d4ca0). Walks the mesh's OBB tree with the sphere
// centre in the node frames and collects the triangle-sphere contacts of 558a into a
// 1000-contact buffer, then simplifies them.
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

// ResultReset: the original's static result reset of this file, Mac 0x4cb774 (Win: inlined)

namespace {

// Mac 0x4cb787, Win 0x7d4a80 (name inferred): the sphere (centre c in the box frame) can reach the
// box (half extents e) on every axis
bool SphereNearBox(const MeReal* e, const MeReal* c, MeReal radius)
{
    for (int k = 0; k < 3; ++k)
        if (radius + e[k] < fabs(c[k]))
            return false;
    return true;
}

// Mac 0x4cb851, Win 0x7d4ac0 (name inferred)
void SphereContacts(VincIntersectResult* r, VincGeometryID mesh, VincOBBNode* node, const MeReal* tmMesh,
                    const MeReal* tmSphere, MeReal radius)
{
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, true);
        MeVector3 center = {tmSphere[12], tmSphere[13], tmSphere[14]};
        if (VincTriangleSphereContacts(r, &t, radius, center, 1e-14)) {
            VincContact* c = &r->contacts[r->contactCount - 1];
            c->normal[0] = -c->normal[0];
            c->normal[1] = -c->normal[1];
            c->normal[2] = -c->normal[2];
        }
        return;
    }
    MeVector3 center, local;
    VincTMInverseTransform(tmMesh, tmSphere + 12, center);
    MeReal d0 = center[0] - node->center[0], d1 = center[1] - node->center[1], d2 = center[2] - node->center[2];
    local[0] = d0 * node->axis[0][0] + d1 * node->axis[0][1] + d2 * node->axis[0][2];
    local[1] = d0 * node->axis[1][0] + d1 * node->axis[1][1] + d2 * node->axis[1][2];
    local[2] = d0 * node->axis[2][0] + d1 * node->axis[2][1] + d2 * node->axis[2][2];
    if (SphereNearBox(node->halfExtents, local, radius)) {
        SphereContacts(r, mesh, node->child[0], tmMesh, tmSphere, radius);
        SphereContacts(r, mesh, node->child[1], tmMesh, tmSphere, radius);
    }
}

} // namespace

extern "C" {

// Mac 0x4cbeb3, Win 0x7d4ca0 (name inferred). The contact normals point from the sphere to the
// mesh (flipped per contact); the result normal is the simplified sum.
int MEAPI VincTriangleMeshSphereIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1000];   // the original: on the stack
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    MeReal radius = VincSphereGetRadius(VincModelGetGeometry(pair->model2));
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmSphere = VincModelGetTransformPtr(pair->model2);
    ResultReset(result);
    SphereContacts(result, mesh, tree->nodes, tmMesh, tmSphere, radius);
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
    result->touch = 0 < result->contactCount;
    return result->touch;
}

// Mac 0x4cb7db (name inferred)
int MEAPI VincTriangleMeshSphereRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshSphereIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 sphere = VincSphereGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshInteractionsGetTypeId(), sphere, &i, 0);
    return 1;
}

} // extern "C"
