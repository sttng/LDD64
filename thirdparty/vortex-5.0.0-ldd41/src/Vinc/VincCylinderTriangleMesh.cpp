// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs Cylinder pair test (restored;
// Mac 557c 0x4c7e2c-0x4c8af1).
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincPrimitives.h"

using namespace VincTriMeshPairs;

namespace {

// Mac 0x4c7eb5 (name inferred). ext = {r, r, half height}.
void CylinderContacts(VincIntersectResult* r, const MeReal* ext, VincGeometryID mesh, VincOBBNode* node,
                      const MeReal* tmMesh, const MeReal* tmCyl, const MeReal* rel, bool faceNormals,
                      MeReal cosTolerance, int flag)
{
    if (IsLeaf(node)) {
        MeReal v[3][3];
        VincTriangleRef t;
        LeafTriangle(mesh, node->triangle, tmMesh, v, &t, true);
        int before = r->contactCount;
        int hit = VincTriangleCylinderContacts(r, &t, ext[0], ext[2], tmCyl, 1e-14, cosTolerance, flag);
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
        CylinderContacts(r, ext, mesh, node->child[0], tmMesh, tmCyl, rel, faceNormals, cosTolerance, flag);
        CylinderContacts(r, ext, mesh, node->child[1], tmMesh, tmCyl, rel, faceNormals, cosTolerance, flag);
    }
}

} // namespace

extern "C" {

// Mac 0x4c868a, Win 0x7d41a0 (name inferred)
int MEAPI VincTriangleMeshCylinderIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static VincContact buffer[1000];   // Mac: on the stack (76000 bytes)
    SwappedContacts saved;
    saved.Swap(result, buffer, 1000);
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincCylinder* cyl = (VincCylinder*)VincModelGetGeometry(pair->model2);
    MeReal r = VincCylinderGetRadius(&cyl->base);
    MeReal ext[3];
    ext[0] = r;
    ext[1] = r;
    ext[2] = VincCylinderGetHalfHeight(&cyl->base);
    VincOBBTree* tree = ((VincTriangleMesh*)mesh)->tree;
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmCyl = VincModelGetTransformPtr(pair->model2);
    MeReal rel[16];
    RelativeTM(tmMesh, tmCyl, rel);
    ResultReset(result);
    int flag = cyl->flag;
    MeReal cosTolerance = cyl->cosTolerance;
    bool faceNormals = VincModelPairGetRequestPtr(pair)->faceNormalsFirst != 0;
    CylinderContacts(result, ext, mesh, tree->nodes, tmMesh, tmCyl, rel, faceNormals, cosTolerance, flag);
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

// Mac 0x4c7e3f (name inferred)
int MEAPI VincTriangleMeshCylinderRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshCylinderIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 cyl = VincCylinderGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshGetTypeId(), cyl, &i, 0);
    return 1;
}

} // extern "C"
