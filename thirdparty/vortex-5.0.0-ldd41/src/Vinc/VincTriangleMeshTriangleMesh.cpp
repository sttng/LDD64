// Vortex 5.0.0 build 800 "vortexldd41" - TriangleMesh vs TriangleMesh pair test (restored,
// Mac 557d 0x4cc003-0x4ce0b8, 0x4ce134, 0x4cedbc; Win 0x767520-0x769d10 / 0x768c50).
// Both OBB trees are walked together in world space (compact 3x4 transforms); leaf pairs are
// intersected (VincTriTriIntersectPoints) and their contacts merged into the caller's array. Small
// meshes (fewer than 120 triangles together and one with fewer than 20) take a simpler leaf test and
// a post pass that replaces every normal and depth by one estimate (line segments through both meshes).
// Win: the entry point runs the "mcd" license gate first; restored as the licensed
// behaviour (VxLicense).
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincTriangleMeshContacts.h"

#include <float.h>
#include <string.h>

using namespace VincTriMeshPairs;

// ResultReset: the original's static result reset of this file, Mac 0x4cc003 (Win: inlined)

namespace {

// compact 3x4 transform (rotation rows 0..2, position) of a TM
inline void CompactTM(const MeReal* tm, MeReal* c)
{
    c[0] = tm[0]; c[1] = tm[1]; c[2] = tm[2];
    c[3] = tm[4]; c[4] = tm[5]; c[5] = tm[6];
    c[6] = tm[8]; c[7] = tm[9]; c[8] = tm[10];
    c[9] = tm[12]; c[10] = tm[13]; c[11] = tm[14];
}

// Win 0x769520 (Mac: inlined): p transformed by the compact tm
inline void CompactTransform(const MeReal* c, const MeReal* p, MeReal* out)
{
    out[0] = p[0] * c[0] + p[1] * c[3] + p[2] * c[6] + c[9];
    out[1] = p[0] * c[1] + p[1] * c[4] + p[2] * c[7] + c[10];
    out[2] = p[0] * c[2] + p[1] * c[5] + p[2] * c[8] + c[11];
}

// Win 0x769580 (Mac: inlined)
inline void CompactRotate(const MeReal* c, const MeReal* v, MeReal* out)
{
    out[0] = v[0] * c[0] + v[1] * c[3] + v[2] * c[6];
    out[1] = v[0] * c[1] + v[1] * c[4] + v[2] * c[7];
    out[2] = v[0] * c[2] + v[1] * c[5] + v[2] * c[8];
}

// the node's box data from its half extents on (half extents, axes, centre)
inline const MeReal* Box(const VincOBBNode* n)
{
    return n->halfExtents;
}

// triangle i of mesh in world space (v[3][3], contiguous)
inline void WorldTriangle(VincGeometryID mesh, int i, const MeReal* tm, MeReal (*v)[3])
{
    MeReal *a, *b, *c;
    VincTriangleMeshGetTrianglePtrs(mesh, i, &a, &b, &c);
    CompactTransform(tm, a, v[0]);
    CompactTransform(tm, b, v[1]);
    CompactTransform(tm, c, v[2]);
}

inline void WorldNormal(VincGeometryID mesh, int i, const MeReal* tm, MeReal* n)
{
    MeReal* p;
    VincTriangleMeshGetNormalPtr(mesh, i, &p);
    CompactRotate(tm, p, n);
}

inline VincGeometryID TreeMesh(const VincOBBTree* t)
{
    return &t->mesh->base;
}

// Mac 0x4cc08c, Win 0x7677f0 (name inferred): d admissible for triangle i of mesh
// (world vertices v, world normal n): for its edges with corner data (edgeData0 != -2),
// e . (n x d) < edgeData1 + 0.4
bool MeshTriangleEdgesAdmit(VincGeometryID mesh, int i, MeReal (*v)[3], const MeReal* n, const MeReal* d)
{
    MeVector3 data0, data1;
    VincTriangleMeshGetCornerData(mesh, i, data0, data1);
    for (int k = 0; k < 3; ++k) {
        if (data0[k] == -2.0)
            continue;
        const MeReal* p = v[k];
        const MeReal* q = v[(k + 1) % 3];
        MeReal x = q[0] - p[0], y = q[1] - p[1], z = q[2] - p[2];
        MeReal len = sqrt(x * x + y * y + z * z);
        if (len != 0.0) {
            len = 1.0 / len;
            x = x * len;
            y = y * len;
            z = len * z;
        }
        if (data1[k] + 0.4 <= x * (n[1] * d[2] - n[2] * d[1]) + y * (n[2] * d[0] - d[2] * n[0])
                                  + z * (d[1] * n[0] - n[1] * d[0]))
            return false;
    }
    return true;
}

inline void SetDir(MeReal* dir, const MeReal* v, MeReal sign)
{
    if (sign < 0.0) {
        dir[0] = -v[0];
        dir[1] = -v[1];
        dir[2] = -v[2];
    } else {
        dir[0] = v[0];
        dir[1] = v[1];
        dir[2] = v[2];
    }
}

inline void WriteContact(VincIntersectResult* r, const MeReal* p, const MeReal* n, MeReal sep)
{
    VincContact* c = &r->contacts[r->contactCount];
    c->position[0] = p[0];
    c->position[1] = p[1];
    c->position[2] = p[2];
    c->normal[0] = n[0];
    c->normal[1] = n[1];
    c->normal[2] = n[2];
    c->separation = sep;
}

// Mac 0x4ce134, Win 0x769d10 (name inferred): one leaf pair (the full contact test)
int LeafContacts(VincIntersectResult* r, const VincContactMergeParams* params, const VincOBBNode* n1,
                 const VincOBBTree* t1, const MeReal* tm1, const VincOBBNode* n2, const VincOBBTree* t2,
                 const MeReal* tm2)
{
    if (VincOBBFlatFlatDisjointTM(Box(n1), tm1, Box(n2), tm2) != 0)
        return 0;
    VincGeometryID m1 = TreeMesh(t1), m2 = TreeMesh(t2);
    int i1 = n1->triangle, i2 = n2->triangle;
    MeReal v1[3][3], v2[3][3], pts[16][3];   // the original: 6 points
    WorldTriangle(m1, i1, tm1, v1);
    WorldTriangle(m2, i2, tm2, v2);
    int count;
    int kind = VincTriTriIntersectPoints(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], &pts[0][0], 6, &count);
    if (kind == 0)
        return 0;
    int limit = params->maxCount;
    if (29 < limit)
        limit = 30;
    if (count != 2) {
        // NOTE: the second triangle's normal is rotated with the FIRST mesh's transform (as in the
        // original)
        MeVector3 n;
        WorldNormal(m2, i2, tm1, n);
        for (int i = 0; i < count; ++i) {
            if (r->contactCount < r->contactMaxCount) {
                WriteContact(r, pts[i], n, 0.0);
                r->contacts[r->contactCount].element1 = i1;
                r->contacts[r->contactCount].element2 = i2;
                VincContactsMergeAdd(r, params, limit);
            }
        }
        return 1;
    }
    MeVector3 na, nb, dir, test;
    WorldNormal(m1, i1, tm1, na);
    WorldNormal(m2, i2, tm2, nb);
    bool ok;
    if (kind == 2) {
        SetDir(dir, nb, 1.0);
        SetDir(test, nb, -1.0);
        ok = MeshTriangleEdgesAdmit(m1, i1, v1, na, test);
    } else if (kind == 3) {
        SetDir(dir, na, -1.0);
        ok = MeshTriangleEdgesAdmit(m2, i2, v2, nb, dir);
    } else {
        MeReal area1 = VincTriangleMeshGetTriangleArea(m1, i1);
        MeReal area2 = VincTriangleMeshGetTriangleArea(m2, i2);
        if (na[0] * nb[0] + na[1] * nb[1] + na[2] * nb[2] < 0.0) {
            dir[2] = area2 * nb[2] - area1 * na[2];
            dir[1] = area2 * nb[1] - area1 * na[1];
            dir[0] = area2 * nb[0] - area1 * na[0];
        } else {
            dir[2] = area1 * na[2] + area2 * nb[2];
            dir[1] = area1 * na[1] + area2 * nb[1];
            dir[0] = area1 * na[0] + area2 * nb[0];
        }
        MeReal len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
        if (1e-14 < len) {
            MeReal inv = 1.0 / len;
            dir[0] = dir[0] * inv;
            dir[1] = dir[1] * inv;
            dir[2] = inv * dir[2];
        }
        if (!(len < 1e-14)) {
            if (!MeshTriangleEdgesAdmit(m2, i2, v2, nb, dir))
                return 0;
            SetDir(test, dir, -1.0);
            ok = MeshTriangleEdgesAdmit(m1, i1, v1, na, test);
        } else {
            SetDir(dir, nb, 1.0);
            SetDir(test, nb, -1.0);
            ok = MeshTriangleEdgesAdmit(m1, i1, v1, na, test);
        }
    }
    if (!ok)
        return 0;
    VincTriTriSeparation sep;
    VincTriTriSeparationInit(&sep, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], na, nb, dir);
    VincTriTriSeparationCompute(&sep);
    for (int i = 0; i < 2; ++i) {
        if (r->contactCount < r->contactMaxCount) {
            WriteContact(r, pts[i], dir, 0.0 < sep.depth ? 0.0 : sep.depth);
            r->contacts[r->contactCount].element1 = i1;
            r->contacts[r->contactCount].element2 = i2;
            VincContactsMergeAdd(r, params, limit);
        }
    }
    return 1;
}

// Mac 0x4cedbc, Win 0x769910 (name inferred): one leaf pair of small meshes: no edge data test, at
// most 30 raw contacts (merge limit 4); the element ids are not set (the post pass rewrites
// normals and depths)
int LeafContactsSmall(VincIntersectResult* r, const VincContactMergeParams* params, const VincOBBNode* n1,
                      const VincOBBTree* t1, const MeReal* tm1, const VincOBBNode* n2, const VincOBBTree* t2,
                      const MeReal* tm2)
{
    if (VincOBBFlatFlatDisjointTM(Box(n1), tm1, Box(n2), tm2) != 0)
        return 0;
    VincGeometryID m1 = TreeMesh(t1), m2 = TreeMesh(t2);
    int i1 = n1->triangle, i2 = n2->triangle;
    MeReal v1[3][3], v2[3][3], pts[16][3];
    WorldTriangle(m1, i1, tm1, v1);
    WorldTriangle(m2, i2, tm2, v2);
    int count;
    int kind = VincTriTriIntersectPoints(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], &pts[0][0], 6, &count);
    if (kind == 0)
        return 0;
    MeVector3 na, nb, dir;
    WorldNormal(m1, i1, tm1, na);
    WorldNormal(m2, i2, tm2, nb);
    SetDir(dir, nb, 1.0);
    if (kind != 2) {
        if (kind == 3) {
            SetDir(dir, na, -1.0);
        } else {
            MeReal area1 = VincTriangleMeshGetTriangleArea(m1, i1);
            MeReal area2 = VincTriangleMeshGetTriangleArea(m2, i2);
            if (na[0] * nb[0] + na[1] * nb[1] + na[2] * nb[2] < 0.0) {
                dir[2] = area2 * nb[2] - area1 * na[2];
                dir[1] = area2 * nb[1] - area1 * na[1];
                dir[0] = area2 * nb[0] - area1 * na[0];
            } else {
                dir[2] = area1 * na[2] + area2 * nb[2];
                dir[1] = area1 * na[1] + area2 * nb[1];
                dir[0] = area1 * na[0] + area2 * nb[0];
            }
            MeReal len = sqrt(dir[0] * dir[0] + dir[1] * dir[1] + dir[2] * dir[2]);
            if (1e-14 < len) {
                MeReal inv = 1.0 / len;
                dir[0] = dir[0] * inv;
                dir[1] = dir[1] * inv;
                dir[2] = inv * dir[2];
            }
            if (len < 1e-14)
                SetDir(dir, nb, 1.0);
        }
    }
    VincTriTriSeparation sep;
    VincTriTriSeparationInit(&sep, v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], na, nb, dir);
    VincTriTriSeparationCompute(&sep);
    int limit = params->maxCount;
    if (3 < limit)
        limit = 4;
    if (count == 2) {
        for (int i = 0; i < 2; ++i) {
            if (r->contactCount < r->contactMaxCount && r->contactCount < 30) {
                WriteContact(r, pts[i], dir, 0.0 < sep.depth ? 0.0 : sep.depth);
                VincContactsMergeAdd(r, params, limit);
            }
        }
    } else {
        // NOTE: the second triangle's normal with the FIRST mesh's transform (as in the original)
        MeVector3 n;
        WorldNormal(m2, i2, tm1, n);
        for (int i = 0; i < count; ++i) {
            if (r->contactCount < r->contactMaxCount && r->contactCount < 30) {
                WriteContact(r, pts[i], n, 0.0);
                VincContactsMergeAdd(r, params, limit);
            }
        }
    }
    return 1;
}

// Mac 0x4cd057, Win 0x767c60 (name inferred): both trees together
void ContactWalk(VincIntersectResult* r, const VincContactMergeParams* params, const VincOBBNode* n1,
                 const VincOBBTree* t1, const MeReal* tm1, const VincOBBNode* n2, const VincOBBTree* t2,
                 const MeReal* tm2, bool small)
{
    bool leaf1 = IsLeaf(n1), leaf2 = IsLeaf(n2);
    if (leaf1 && leaf2) {
        VincTriangleMeshGetTriangleCount(TreeMesh(t1));
        VincTriangleMeshGetTriangleCount(TreeMesh(t2));
        if (small)
            LeafContactsSmall(r, params, n1, t1, tm1, n2, t2, tm2);
        else
            LeafContacts(r, params, n1, t1, tm1, n2, t2, tm2);
        return;
    }
    if (!leaf1 && !leaf2) {
        if (VincOBBDisjointTM(Box(n1), tm1, Box(n2), tm2) != 0)
            return;
        if (n2->halfExtents[0] + n2->halfExtents[1] + n2->halfExtents[2]
            < n1->halfExtents[0] + n1->halfExtents[1] + n1->halfExtents[2]) {
            ContactWalk(r, params, n1->child[0], t1, tm1, n2, t2, tm2, small);
            ContactWalk(r, params, n1->child[1], t1, tm1, n2, t2, tm2, small);
        } else {
            ContactWalk(r, params, n1, t1, tm1, n2->child[0], t2, tm2, small);
            ContactWalk(r, params, n1, t1, tm1, n2->child[1], t2, tm2, small);
        }
        return;
    }
    if (leaf1) {
        if (VincOBBFlatDisjointTM(Box(n1), tm1, Box(n2), tm2) != 0)
            return;
        ContactWalk(r, params, n1, t1, tm1, n2->child[0], t2, tm2, small);
        ContactWalk(r, params, n1, t1, tm1, n2->child[1], t2, tm2, small);
        return;
    }
    if (VincOBBFlatDisjointTM(Box(n2), tm2, Box(n1), tm1) != 0)
        return;
    ContactWalk(r, params, n1->child[0], t1, tm1, n2, t2, tm2, small);
    ContactWalk(r, params, n1->child[1], t1, tm1, n2, t2, tm2, small);
}

// Mac 0x4cd56c, Win 0x767910 (name inferred): touch only, stops at the first intersecting pair
void TouchWalk(const VincOBBNode* n1, const VincOBBTree* t1, const MeReal* tm1, const VincOBBNode* n2,
               const VincOBBTree* t2, const MeReal* tm2, int* count)
{
    bool leaf1 = IsLeaf(n1), leaf2 = IsLeaf(n2);
    if (leaf1 && leaf2) {
        if (VincOBBDisjointTM(Box(n1), tm1, Box(n2), tm2) != 0)
            return;
        MeReal v1[3][3], v2[3][3];
        WorldTriangle(TreeMesh(t1), n1->triangle, tm1, v1);
        WorldTriangle(TreeMesh(t2), n2->triangle, tm2, v2);
        if (VincTriTriOverlap(v1[0], v1[1], v1[2], v2[0], v2[1], v2[2]))
            *count = *count + 1;
        return;
    }
    if (leaf1) {
        int d = VincOBBFlatDisjointTM(Box(n1), tm1, Box(n2), tm2);
        if (*count != 0 || d != 0)
            return;
        TouchWalk(n1, t1, tm1, n2->child[0], t2, tm2, count);
        if (*count != 0)
            return;
        TouchWalk(n1, t1, tm1, n2->child[1], t2, tm2, count);
        return;
    }
    if (leaf2) {
        int d = VincOBBFlatDisjointTM(Box(n2), tm2, Box(n1), tm1);
        if (*count != 0 || d != 0)
            return;
        TouchWalk(n1->child[0], t1, tm1, n2, t2, tm2, count);
        if (*count != 0)
            return;
        TouchWalk(n1->child[1], t1, tm1, n2, t2, tm2, count);
        return;
    }
    int d = VincOBBDisjointTM(Box(n1), tm1, Box(n2), tm2);
    if (*count != 0 || d != 0)
        return;
    TouchWalk(n1->child[0], t1, tm1, n2, t2, tm2, count);
    if (*count != 0)
        return;
    TouchWalk(n1->child[1], t1, tm1, n2, t2, tm2, count);
}

// Mac 0x4cc669, Win 0x767ee0 (name inferred): small meshes: one normal (the fan normals of the
// contact polygon, oriented away from the second mesh) and one depth (two line segments through
// the contact centre) for all contacts; fewer than 3 contacts: the normals are only normalised
int SmallPostPass(VincIntersectResult* r, const VincOBBTree* t1, const MeReal* tm1, const MeReal* tm2)
{
    int n = r->contactCount;
    if (n == 0)
        return 0;
    VincContact* c = r->contacts;
    if (n < 3) {
        for (int i = 0; i < n; ++i) {
            MeReal x = c[i].normal[0];
            MeReal len = sqrt(x * x + c[i].normal[1] * c[i].normal[1] + c[i].normal[2] * c[i].normal[2]);
            if (len != 0.0) {
                len = 1.0 / len;
                c[i].normal[0] = x * len;
                c[i].normal[1] = c[i].normal[1] * len;
                c[i].normal[2] = c[i].normal[2] * len;
            }
        }
        return 1;
    }
    MeReal fan[32][3];   // the original: a stack array (more than 32 contacts are not possible here)
    int best = -1;
    MeReal bestLen = 1e-14;
    for (int i = 0; i < n - 1; ++i) {
        const MeReal* p = c[i].position;
        const MeReal* q = c[i + 1].position;
        const MeReal* s = c[(i + 2) % n].position;
        MeReal ez = q[2] - p[2], ey = q[1] - p[1], ex = q[0] - p[0];
        MeReal fz = s[2] - p[2], fy = s[1] - p[1], fx = s[0] - p[0];
        MeReal z = ey * fx - ex * fy;
        MeReal x = ez * fy - ey * fz;
        fan[i][0] = x;
        MeReal y = ex * fz - ez * fx;
        fan[i][1] = y;
        fan[i][2] = z;
        MeReal l2 = x * x + y * y + z * z;
        if (bestLen < l2) {
            best = i;
            bestLen = l2;
        }
    }
    if (best == -1) {
        ResultReset(r);
        return 0;
    }
    const MeReal* center = t1->mesh->center;
    MeReal sx = 0.0, sy = 0.0, sz = 0.0;
    for (int i = 0; i < n - 1; ++i) {
        MeReal x = fan[i][0], y = fan[i][1], z = fan[i][2];
        if (fan[best][0] * x + fan[best][1] * y + fan[best][2] * z <= 0.0) {
            sx = sx - x;
            sy = sy - y;
            sz = sz - z;
        } else {
            sx = x + sx;
            sy = y + sy;
            sz = z + sz;
        }
    }
    MeReal len = sqrt(sx * sx + sy * sy + sz * sz);
    if (len != 0.0) {
        len = 1.0 / len;
        sx = sx * len;
        sy = sy * len;
        sz = len * sz;
    }
    // NOTE: both "centres" are the FIRST mesh's centre, once with each transform (as in the original)
    MeVector3 c1, c2;
    CompactTransform(tm1, center, c1);
    CompactTransform(tm2, center, c2);
    MeReal side = (c1[0] - c2[0]) * sx + sy * (c1[1] - c2[1]) + (c1[2] - c2[2]) * sz;
    if (side < 0.0) {
        sz = -sz;
        sy = -sy;
        sx = -sx;
    }
    MeReal mx = 0.0, my = 0.0, mz = 0.0;
    for (int i = 0; i < n; ++i) {
        mx = mx + c[i].position[0];
        my = my + c[i].position[1];
        mz = mz + c[i].position[2];
    }
    MeReal inv = 1.0 / (MeReal)n;
    mx = mx * inv;
    my = my * inv;
    mz = mz * inv;
    VincLineSegResult ls;
    MeVector3 p0, p1;
    p0[0] = mx - sx * 0.1;
    p0[1] = my - sy * 0.1;
    p0[2] = mz - sz * 0.1;
    p1[0] = mx + sx * 0.9;
    p1[1] = my + sy * 0.9;
    p1[2] = mz + sz * 0.9;
    MeReal depth2 = VincTriangleMeshLineSegment(r->pair->model2, p0, p1, &ls) ? ls.distance : 0.0;
    p0[0] = mx + sx * 0.1;
    p0[1] = my + sy * 0.1;
    p0[2] = mz + sz * 0.1;
    p1[0] = mx - sx * 0.9;
    p1[1] = my - sy * 0.9;
    p1[2] = mz - sz * 0.9;
    MeReal depth1 = VincTriangleMeshLineSegment(r->pair->model1, p0, p1, &ls) ? ls.distance : 0.0;
    MeReal v = -((depth1 + depth2) - (MeReal)0.2f);
    MeReal sep = 0.0;
    if (v <= 0.0)
        sep = v;
    for (int i = 0; i < n; ++i) {
        c[i].normal[0] = sx;
        c[i].normal[1] = sy;
        c[i].normal[2] = sz;
        c[i].separation = sep;
    }
    return 1;
}

// Mac 0x4cdcb4, Win 0x768960 (name inferred)
int MeshMeshIntersect(VincIntersectResult* r, int maxMerge, VincModelID m1, VincModelID m2)
{
    VincGeometryID g1 = VincModelGetGeometry(m1);
    VincGeometryID g2 = VincModelGetGeometry(m2);
    VincTriangleMesh* mesh1 = (VincTriangleMesh*)g1;
    VincTriangleMesh* mesh2 = (VincTriangleMesh*)g2;
    const VincOBBTree* t1 = mesh1->tree;
    const VincOBBTree* t2 = mesh2->tree;
    MeReal tm1[12], tm2[12];
    CompactTM(VincModelGetTransformPtr(m1), tm1);
    CompactTM(VincModelGetTransformPtr(m2), tm2);
    if (r->contactMaxCount == 0 || maxMerge == 0) {
        // Mac 0x4cdc66 (Win: inlined)
        int count = 0;
        TouchWalk(t1->nodes, t1, tm1, t2->nodes, t2, tm2, &count);
        int touch = 0 < count;
        ResultReset(r);
        r->touch = touch;
        return touch;
    }
    int n1 = VincTriangleMeshGetTriangleCount(g1);
    int n2 = VincTriangleMeshGetTriangleCount(g2);
    bool fewer = n2 + n1 < 120;
    VincContactMergeParams params;
    params.maxCount = maxMerge;
    MeReal d = mesh2->m6c;
    if (mesh1->m6c < d)
        d = mesh1->m6c;
    params.distanceSq = d * d;
    params.cosTolerance = mesh2->m74;
    if (mesh2->m74 <= mesh1->m74)
        params.cosTolerance = mesh1->m74;
    ResultReset(r);
    bool small = fewer && (n1 < 20 || n2 < 20);
    // Mac 0x4cd4cf (Win 0x7687e0)
    ContactWalk(r, &params, t1->nodes, t1, tm1, t2->nodes, t2, tm2, small);
    if (small)
        SmallPostPass(r, t1, tm1, tm2);
    if (!small) {
        for (int i = 0; i < r->contactCount; ++i) {
            VincContact* c = &r->contacts[i];
            MeReal x = c->normal[0];
            MeReal len = sqrt(x * x + c->normal[1] * c->normal[1] + c->normal[2] * c->normal[2]);
            bool drop = len == 0.0;
            if (!drop) {
                MeReal inv = 1.0 / len;
                c->normal[0] = x * inv;
                c->normal[1] = c->normal[1] * inv;
                c->normal[2] = c->normal[2] * inv;
                drop = len < 1e-07;
            }
            if (drop) {
                r->contactCount = r->contactCount - 1;
                memcpy(c, &r->contacts[r->contactCount], sizeof(VincContact));
                --i;
            }
        }
    }
    int touch = 0;
    if (r->contactCount != 0) {
        r->normal[0] = r->contacts[0].normal[0];
        r->normal[1] = r->contacts[0].normal[1];
        r->normal[2] = r->contacts[0].normal[2];
        touch = 0 < r->contactCount;
    }
    r->touch = touch;
    return 0 < r->contactCount;
}

} // namespace

extern "C" {

// Mac 0x4ce0b8, Win 0x768c50 (name inferred)
int MEAPI VincTriangleMeshTriangleMeshIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    (void)pad;   // passed on but not used
    int touch = MeshMeshIntersect(result, 12, pair->model1, pair->model2);
    VincContactSimplifyResult(result);
    return touch;
}

// Mac 0x4cc016, Win 0x769450 (name inferred)
int MEAPI VincTriangleMeshTriangleMeshRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = VincTriangleMeshTriangleMeshIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 mesh = VincTriangleMeshInteractionsGetTypeId();
    VincGeometryPairRegisterInteractions(mesh, VincTriangleMeshInteractionsGetTypeId(), &i, 0);
    return 1;
}

} // extern "C"
