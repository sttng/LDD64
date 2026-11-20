// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Capsule pair test (restored;
// Mac 557c 0x4c6299-0x4c6af1).
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

namespace {

// Mac 0x4c6322 (name inferred). ext = {r, r, r + half height}: the capsule's box.
void CapsuleContacts(VincIntersectResult* r, const MeReal* ext, VincGeometryID mesh, VincOBBNode* node,
                     const MeReal* tmMesh, const MeReal* tmCap, const MeReal* rel, bool faceNormals)
{
    if (!(r->contactCount < r->contactMaxCount))
        return;
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, true);
        int before = r->contactCount;
        // NOTE: the Mac call passes a 7th argument (double 1.0) after the tolerance; 558a takes 6
        // and never reads it (cdecl, so passing it is harmless).
        // NOTE: the 4th argument is the capsule half height in 558a, but the original passes
        // ext[2] = r + half height (Mac 0x4c663a), so triangles are tested against a capsule that
        // is r too long at each end. Kept.
        typedef int (*CapsuleContacts7)(VincIntersectResult*, const VincTriangle*, MeReal, MeReal,
                                        const MeReal*, MeReal, MeReal);
        int hit = ((CapsuleContacts7)VincTriangleCapsuleContacts)(r, &t, ext[0], ext[2], tmCap, 1e-14, 1.0);
        if (hit != 0 && faceNormals) {
            for (int i = before; i < r->contactCount; ++i) {
                r->contacts[i].normal[0] = t.normal[0];
                r->contacts[i].normal[1] = t.normal[1];
                r->contacts[i].normal[2] = t.normal[2];
            }
        }
        return;
    }
    MeReal obb[16];
    NodeBoxTM(node, rel, obb);
    if (VincOBBDisjoint(obb, ext, node->halfExtents) == 0) {
        CapsuleContacts(r, ext, mesh, node->child[0], tmMesh, tmCap, rel, faceNormals);
        CapsuleContacts(r, ext, mesh, node->child[1], tmMesh, tmCap, rel, faceNormals);
    }
}

} // namespace

extern "C" {

// Mac 0x4c6af1, Win 0x764d40 (name inferred)
int MEAPI VincTriangleMeshCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1020];   // Mac: on the stack (77520 bytes)
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincGeometryID cap = VincModelGetGeometry(pair->model2);
    MeReal r = VincCapsuleGetRadius(cap);
    MeReal ext[3];
    ext[0] = r;
    ext[1] = r;
    ext[2] = r + VincCapsuleGetHalfHeight(cap);
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmCap = VincModelGetTransformPtr(pair->model2);
    MeReal rel[16];
    RelativeTM(tmMesh, tmCap, rel);
    ResultReset(result);
    CapsuleContacts(result, ext, mesh, tree->nodes, tmMesh, tmCap, rel, false);
    if (result->contactCount == 0) {
        saved.Restore(result);   // NOTE: the original leaves its stack buffer in the result
        return 0;
    }
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= request->contactMaxCount)
        maxc = result->contactMaxCount;
    if (maxc == 0) {
        ResultReset(result);
        saved.Restore(result);   // NOTE: as above
        return 1;
    }
    saved.Restore(result);
    result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    FlipNormals(result);
    result->touch = 0 < result->contactCount;
    return result->touch;
}

// Mac 0x4c62ac (name inferred)
int MEAPI VincTriangleMeshCapsuleRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshCapsuleIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 cap = VincCapsuleGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshGetTypeId(), cap, &i, 0);
    return 1;
}

} // extern "C"
