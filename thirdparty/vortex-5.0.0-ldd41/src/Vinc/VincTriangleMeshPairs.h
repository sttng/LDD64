/* Vortex 5.0.0 build 800 "vortexldd41" - inline helpers shared by the four triangle mesh pair
 * tests (VincBoxTriangleMesh.cpp, VincCapsuleTriangleMesh.cpp, VincConvexTriangleMesh.cpp,
 * VincCylinderTriangleMesh.cpp). The original repeats this code in each file
 * (e.g. the result reset is a static function in each: Mac 0x4c4ff3, 0x4c6299, 0x4c6f3d, 0x4c7e2c). */
#ifndef VINC_VINCTRIANGLEMESHPAIRS_H
#define VINC_VINCTRIANGLEMESHPAIRS_H

#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincMath.h"
#include "VincInternal.h"

#include <math.h>

namespace VincTriMeshPairs {

inline void ResultReset(VincIntersectResult* r)
{
    r->contactGroupCount = 0;
    r->contactCount = 0;
}

/* Win 0x75c060 (out of line there; name inferred): pose of a in b's frame (rows 0..2 rotation, 12..14 translation; 3, 7, 11, 15 unused) */
inline void RelativeTM(const MeReal* a, const MeReal* b, MeReal* rel)
{
    for (int i = 0; i < 3; ++i) {
        rel[i * 4 + 0] = a[i * 4 + 0] * b[0] + a[i * 4 + 1] * b[1] + a[i * 4 + 2] * b[2];
        rel[i * 4 + 1] = a[i * 4 + 0] * b[4] + a[i * 4 + 1] * b[5] + a[i * 4 + 2] * b[6];
        rel[i * 4 + 2] = a[i * 4 + 0] * b[8] + a[i * 4 + 1] * b[9] + a[i * 4 + 2] * b[10];
    }
    MeReal dz = a[14] - b[14];
    MeReal dy = a[13] - b[13];
    MeReal dx = a[12] - b[12];
    rel[12] = dx * b[0] + dy * b[1] + dz * b[2];
    rel[13] = dx * b[4] + dy * b[5] + dz * b[6];
    rel[14] = dx * b[8] + dy * b[9] + dz * b[10];
}

/* the node's box in the other geometry's frame */
inline void NodeBoxTM(const VincOBBNode* node, const MeReal* rel, MeReal* obb)
{
    VincTMRotate(rel, node->axis[0], obb + 0);
    VincTMRotate(rel, node->axis[1], obb + 4);
    VincTMRotate(rel, node->axis[2], obb + 8);
    VincTMTransform(rel, node->center, obb + 12);
}

inline bool IsLeaf(const VincOBBNode* node)
{
    return node->child[0] == 0 && node->child[1] == 0;
}

/* unit normal (v1 - v0) x (v2 - v1) and the edges */
inline void TriangleSetup(VincTriangleRef* t)
{
    MeReal* a = t->vertices[0];
    MeReal* b = t->vertices[1];
    MeReal* c = t->vertices[2];
    for (int k = 0; k < 3; ++k) {
        t->edges[0][k] = b[k] - a[k];
        t->edges[1][k] = c[k] - b[k];
        t->edges[2][k] = a[k] - c[k];
    }
    t->normal[2] = t->edges[0][0] * t->edges[1][1] - t->edges[0][1] * t->edges[1][0];
    t->normal[0] = t->edges[0][1] * t->edges[1][2] - t->edges[0][2] * t->edges[1][1];
    t->normal[1] = t->edges[0][2] * t->edges[1][0] - t->edges[1][2] * t->edges[0][0];
    MeReal len = sqrt(t->normal[0] * t->normal[0] + t->normal[1] * t->normal[1]
                      + t->normal[2] * t->normal[2]);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        t->normal[0] = t->normal[0] * inv;
        t->normal[1] = t->normal[1] * inv;
        t->normal[2] = inv * t->normal[2];
    }
}

/* triangle i of the mesh; vertices transformed by tm (NULL: mesh frame) into v[3][3] */
inline void LeafTriangle(VincGeometryID mesh, int i, const MeReal* tm, MeReal (*v)[3], VincTriangleRef* t,
                         bool cornerData)
{
    MeReal *a, *b, *c;
    VincTriangleMeshGetTrianglePtrs(mesh, i, &a, &b, &c);
    if (tm != 0) {
        VincTMTransform(tm, a, v[0]);
        VincTMTransform(tm, b, v[1]);
        VincTMTransform(tm, c, v[2]);
        t->vertices[0] = v[0];
        t->vertices[1] = v[1];
        t->vertices[2] = v[2];
    } else {
        t->vertices[0] = a;
        t->vertices[1] = b;
        t->vertices[2] = c;
    }
    t->edgeData0[0] = t->edgeData0[1] = t->edgeData0[2] = -2.0;
    t->edgeData1[0] = t->edgeData1[1] = t->edgeData1[2] = 0.0;
    t->index = i;
    if (cornerData)
        VincTriangleMeshGetCornerData(mesh, i, t->edgeData0, t->edgeData1);
    TriangleSetup(t);
}

/* contact normals and the result normal from model1 (mesh) towards model2 -> flipped */
inline void FlipNormals(VincIntersectResult* r)
{
    for (int i = 0; i < r->contactCount; ++i) {
        r->contacts[i].normal[0] = -r->contacts[i].normal[0];
        r->contacts[i].normal[1] = -r->contacts[i].normal[1];
        r->contacts[i].normal[2] = -r->contacts[i].normal[2];
    }
    r->normal[0] = -r->normal[0];
    r->normal[1] = -r->normal[1];
    r->normal[2] = -r->normal[2];
}

/* contact buffer swapped in for the tree walk (the original uses a 1000-1020 entry stack array) */
struct SwappedContacts {
    VincContact* contacts;
    int maxCount;
    void Swap(VincIntersectResult* r, VincContact* buffer, int n)
    {
        contacts = r->contacts;
        maxCount = r->contactMaxCount;
        r->contacts = buffer;
        r->contactMaxCount = n;
    }
    void Restore(VincIntersectResult* r)
    {
        r->contacts = contacts;
        r->contactMaxCount = maxCount;
    }
};

} // namespace VincTriMeshPairs

#endif
