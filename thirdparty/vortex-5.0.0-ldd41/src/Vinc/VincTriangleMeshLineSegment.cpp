// Vortex 5.0.0 build 800 "vortexldd41" - line segment query against a triangle mesh (restored,
// Mac 557d 0x4c8db9-0x4c958f, Win 0x775570-0x775710). The segment is walked
// down the mesh's OBB tree (segment / box separating axis test in the node frame); the leaf
// triangles are tested in world space with VincLineSegmentTriangle (Mac 0x4c8af1).
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Vinc/VincPairTests.h"

namespace {

// Win 0x7751f0 (Mac: inlined): segment (centre c, half vector h, both in the node frame) against
// the node box (half extents e): the three face axes and the three edge cross axes.
inline bool SegmentOverlapsBox(const MeReal* e, const MeReal* a, const MeReal* b)
{
    MeReal h0 = (b[0] - a[0]) * 0.5, c0 = (a[0] + b[0]) * 0.5, ah0 = fabs(h0);
    if (!(fabs(c0) <= ah0 + e[0]))
        return false;
    MeReal h1 = (b[1] - a[1]) * 0.5, c1 = (a[1] + b[1]) * 0.5, ah1 = fabs(h1);
    if (!(fabs(c1) <= ah1 + e[1]))
        return false;
    MeReal h2 = (b[2] - a[2]) * 0.5, c2 = (a[2] + b[2]) * 0.5, ah2 = fabs(h2);
    if (!(fabs(c2) <= ah2 + e[2]))
        return false;
    if (!(fabs(c1 * h2 - h1 * c2) <= e[1] * ah2 + ah1 * e[2]))
        return false;
    if (!(fabs(c0 * h2 - h0 * c2) <= e[0] * ah2 + ah0 * e[2]))
        return false;
    return fabs(h1 * c0 - h0 * c1) <= ah1 * e[0] + ah0 * e[1];
}

// p in the node's box frame (Win 0x7667e0, ECX = node axes; Mac: inlined)
inline void NodeLocal(const VincOBBNode* node, const MeReal* p, MeReal* out)
{
    MeReal d0 = p[0] - node->center[0], d1 = p[1] - node->center[1], d2 = p[2] - node->center[2];
    out[0] = d0 * node->axis[0][0] + d1 * node->axis[0][1] + d2 * node->axis[0][2];
    out[1] = d0 * node->axis[1][0] + d1 * node->axis[1][1] + d2 * node->axis[1][2];
    out[2] = d0 * node->axis[2][0] + d1 * node->axis[2][1] + d2 * node->axis[2][2];
}

// Mac 0x4c8de9, Win 0x775570 (name inferred): p0 / p1 world, a / b the same in the mesh frame,
// d = p1 - p0, *len = |d|, *tBest = parameter of the nearest hit so far
void LineSegmentNode(VincOBBNode* node, VincModelID model, const MeReal* p0, const MeReal* p1, const MeReal* a,
                     const MeReal* b, const MeReal* d, const MeReal* len, MeReal* tBest, VincLineSegResult* result)
{
    MeVector3 la, lb;
    NodeLocal(node, a, la);
    NodeLocal(node, b, lb);
    if (!SegmentOverlapsBox(node->halfExtents, la, lb))
        return;
    if (!VincTriMeshPairs::IsLeaf(node)) {
        LineSegmentNode(node->child[0], model, p0, p1, a, b, d, len, tBest, result);
        LineSegmentNode(node->child[1], model, p0, p1, a, b, d, len, tBest, result);
        return;
    }
    int i = node->triangle;
    VincGeometryID mesh = VincModelGetGeometry(model);
    MeReal *v0, *v1, *v2, *n;
    VincTriangleMeshGetTrianglePtrs(mesh, i, &v0, &v1, &v2);
    VincTriangleMeshGetNormalPtr(mesh, i, &n);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeVector3 wn, w0, w1, w2;
    VincTMRotate(tm, n, wn);
    VincTMTransform(tm, v0, w0);
    VincTMTransform(tm, v1, w1);
    VincTMTransform(tm, v2, w2);
    if (VincLineSegmentTriangle(w0, w1, w2, wn, p0, p1, d, len, tBest, result->position, &result->distance)) {
        result->normal[0] = wn[0];
        result->normal[1] = wn[1];
        result->normal[2] = wn[2];
        result->element = i;
    }
}

} // namespace

extern "C" {

// Mac 0x4c958f, Win 0x775710 (name inferred): 1 when the segment hits a triangle (result: world
// position / normal, distance from p0, triangle index)
int MEAPI VincTriangleMeshLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1,
                                      VincLineSegResult* result)
{
    VincTriangleMesh* mesh = (VincTriangleMesh*)VincModelGetGeometry(model);
    VincOBBTree* tree = mesh->tree;
    MeReal* tm = VincModelGetTransformPtr(model);
    MeVector3 a, b, d;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    MeReal tBest = 2.0;
    d[0] = p1[0] - p0[0];
    d[1] = p1[1] - p0[1];
    d[2] = p1[2] - p0[2];
    MeReal len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    LineSegmentNode(tree->nodes, model, p0, p1, a, b, d, &len, &tBest, result);
    return tBest <= 1.0;
}

// Mac 0x4c8db9 (Win: inlined into 0x75cae0) (name inferred)
int MEAPI VincTriangleMeshRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincTriangleMeshGetTypeId(), VincTriangleMeshLineSegment);
    return 1;
}

} // extern "C"
