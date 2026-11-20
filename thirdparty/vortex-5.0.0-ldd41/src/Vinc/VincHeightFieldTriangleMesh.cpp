// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs HeightField pair test (restored,
// Mac 557d 0x4ca32e-0x4ca3b6, Win 0x764fe0 / 0x765890). The height field cells
// under the mesh's box are split into two triangles each; every triangle is taken into the mesh
// frame and run down the mesh's OBB tree (VincTriangleMeshTriangleNode). The contacts go straight
// into the caller's array; element2 = the height field triangle (2 per cell).
#include "VincTriangleMeshPairs.h"
#include "VincTriangleMeshFinish.h"

#include <float.h>
#include "Vinc/VincTriangleMeshContacts.h"

using namespace VincTriMeshPairs;

// ResultReset: the original's static result reset of this file, Mac 0x4ca32e (Win: inlined)

namespace {

// the height field's vertex (x, y) in the mesh frame
inline void HFVertex(VincGeometryID hf, int x, int y, const MeReal* rel, MeReal* out)
{
    MeVector3 p;
    VincHeightFieldGetVertex(hf, p, x, y);
    VincTMTransform(rel, p, out);
}

// the number of vertices along x (height field +0x08) (name inferred)
inline int HFRowLength(VincGeometryID hf)
{
    return ((const int*)hf)[2];
}

} // namespace

extern "C" {

// Mac 0x4ca3b6, Win 0x764fe0 (name inferred)
int MEAPI VincTriangleMeshHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincGeometryID mesh = VincModelGetGeometry(pair->model1);
    VincGeometryID hf = VincModelGetGeometry(pair->model2);
    VincTriangleMesh* m = (VincTriangleMesh*)VincModelGetGeometry(pair->model1);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* tmMesh = VincModelGetTransformPtr(pair->model1);
    MeReal* tmHF = VincModelGetTransformPtr(pair->model2);
    ResultReset(result);
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    // the mesh's box in the height field frame -> the cells below it
    MeReal rel[16];
    rel[3] = rel[7] = rel[11] = rel[15] = DBL_MAX;
    RelativeTM(tmMesh, tmHF, rel);
    MeVector3 lo, hi;
    MeReal aabb6[6];
    VincGeometryUpdateAABB(mesh, (MeReal(*)[4])rel, lo, hi, aabb6);
    MeReal xy[4] = {lo[0], hi[0], lo[1], hi[1]};
    int x0, x1, y0, y1;
    if (!VincHeightFieldGetCellRange(hf, xy, &x0, &x1, &y0, &y1)) {
        ResultReset(result);
        return 0;
    }
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    MeReal inv[16];
    inv[3] = inv[7] = inv[11] = inv[15] = DBL_MAX;
    RelativeTM(tmHF, tmMesh, inv);
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= maxc)
        maxc = result->contactMaxCount;
    VincOBBTree* tree = m->tree;
    MeReal v[3][3];
    VincTriangleRef t;
    t.index = -1;
    t.edgeData0[0] = t.edgeData0[1] = t.edgeData0[2] = 1.0;
    t.edgeData1[0] = t.edgeData1[1] = t.edgeData1[2] = 0.0;
    t.vertices[0] = v[0];
    t.vertices[1] = v[1];
    t.vertices[2] = v[2];
    VincContactMergeParams params;
    params.maxCount = 12;
    params.distanceSq = m->m6c * m->m6c;
    params.cosTolerance = m->m74;
    for (int y = y0; y < y1; ++y) {
        for (int x = x0; x < x1; ++x) {
            if (result->contactMaxCount <= result->contactCount) {
                y = y1;
                break;
            }
            // first triangle: (x, y) (x + 1, y) (x, y + 1)
            HFVertex(hf, x, y, inv, v[0]);
            HFVertex(hf, x + 1, y, inv, v[1]);
            HFVertex(hf, x, y + 1, inv, v[2]);
            VincTriangleUpdateEdges(&t);
            VincTriangleUpdateNormal(&t);
            int before = result->contactCount;
            VincTriangleMeshTriangleNode(result, &t, mesh, tree->nodes, pad, &params);
            if (0 < result->contactCount) {
                if (maxc == 0) {
                    ResultReset(result);
                    return 1;
                }
                for (int i = before; i < result->contactCount; ++i)
                    result->contacts[i].element2 = (x + (HFRowLength(hf) - 1) * y) * 2;
                if (result->contactMaxCount <= result->contactCount)
                    break;
            }
            // second triangle: (x + 1, y) (x + 1, y + 1) (x, y + 1)
            v[0][0] = v[1][0];
            v[0][1] = v[1][1];
            v[0][2] = v[1][2];
            HFVertex(hf, x + 1, y + 1, inv, v[1]);
            VincTriangleUpdateEdges(&t);
            VincTriangleUpdateNormal(&t);
            before = result->contactCount;
            VincTriangleMeshTriangleNode(result, &t, mesh, tree->nodes, pad, &params);
            if (0 < result->contactCount) {
                if (maxc == 0) {
                    ResultReset(result);
                    return 1;
                }
                for (int i = before; i < result->contactCount; ++i)
                    result->contacts[i].element2 = (x + (HFRowLength(hf) - 1) * y) * 2 + 1;
                if (result->contactMaxCount <= result->contactCount)
                    break;
            }
        }
    }
    return FinishMeshContacts(result, tmMesh);
}

// Mac 0x4ca341, Win 0x765890 (name inferred)
int MEAPI VincTriangleMeshHeightFieldRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshHeightFieldIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 hf = VincHeightFieldGetTypeId();
    VincGeometryPairRegisterInteractions(VincTriangleMeshInteractionsGetTypeId(), hf, &i, 0);
    return 1;
}

} // extern "C"
