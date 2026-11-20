// Vortex 5.0.0 build 800 "vortexldd41" - one triangle against a triangle mesh (restored,
// Mac 557d 0x4cf8c0-0x4d028f, 0x4d15ce; Win 0x765980-0x765e70, 0x7668a0). The
// height field and triangle list pair tests (VincHeightFieldTriangleMesh.cpp,
// VincTriangleListTriangleMesh.cpp) feed their triangles, in the mesh frame, through the mesh's OBB
// tree; each mesh triangle that the triangle / box test lets through is intersected with it
// (VincTriTriIntersectPoints) and the contacts are merged into the result.
#include "VincTriangleMeshPairs.h"
#include "Vinc/VincTriangleMeshContacts.h"

namespace {

// Mac 0x4cf8c0, Win 0x765980 (name inferred): separating axis test of a triangle (in the box
// frame) and a box (half extents e), widened by pad; 1: separated
bool TriangleBoxSeparated(const MeReal* pad, const MeReal* e, const VincTriangleRef* t)
{
    MeReal p = *pad;
    MeReal pp = p;
    if (p < 0.0)
        pp = 0.0;
    const MeReal* v0 = t->vertices[0];
    // the triangle's plane
    if (p < fabs(t->normal[0] * v0[0] + t->normal[1] * v0[1] + t->normal[2] * v0[2])
                - (fabs(t->normal[0]) * e[0] + fabs(t->normal[1]) * e[1] + fabs(t->normal[2]) * e[2]))
        return true;
    // the box faces
    for (int k = 0; k < 3; ++k) {
        MeReal a = t->vertices[0][k], b = t->vertices[1][k], c = t->vertices[2][k];
        MeReal mn, mx;
        if (a <= b) {
            mn = a;
            mx = b <= a ? a : b;
        } else {
            mn = b;
            mx = a;
        }
        if (mn <= c) {
            if (mx < c)
                mx = c;
        } else {
            mn = c;
        }
        if (p < fabs((mx + mn) * 0.5) - ((mx - mn) * 0.5 + e[k]))
            return true;
    }
    // edge x box axis
    MeReal ae[3][3], len2[3];
    for (int i = 0; i < 3; ++i) {
        ae[i][0] = fabs(t->edges[i][0]);
        ae[i][1] = fabs(t->edges[i][1]);
        ae[i][2] = fabs(t->edges[i][2]);
        len2[i] = t->edges[i][0] * t->edges[i][0] + t->edges[i][1] * t->edges[i][1] + t->edges[i][2] * t->edges[i][2];
    }
    for (int i = 0; i < 3; ++i) {
        const MeReal* ei = t->edges[i];
        const MeReal* en = t->edges[(i + 1) % 3];
        const MeReal* vi = t->vertices[i];
        for (int j = 0; j < 3; ++j) {
            int u = (j + 1) % 3, w = (j + 2) % 3;
            MeReal d = len2[i] - ae[i][j] * ae[i][j];
            if (pp * pp < d) {
                MeReal h = (en[u] * ei[w] - en[w] * ei[u]) * 0.5;
                MeReal inv = 1.0 / sqrt(d);
                if (pp < fabs((vi[u] * ei[w] - ei[u] * vi[w]) + h) * inv
                             - (e[u] * ae[i][w] + e[w] * ae[i][u] + fabs(h)) * inv)
                    return true;
            }
        }
    }
    return false;
}

// Mac 0x4cfe1a, Win 0x765d70 (name inferred): v is admissible for t's edges (those with edge data:
// edgeData0 != -2): for each such edge (unit) e, e . (n x v) < edgeData1 + 0.4
bool TriangleEdgesAdmit(const VincTriangleRef* t, const MeReal* v)
{
    const MeReal* n = t->normal;
    for (int i = 0; i < 3; ++i) {
        if (t->edgeData0[i] == -2.0)
            continue;
        MeReal x = t->edges[i][0], y = t->edges[i][1], z = t->edges[i][2];
        MeReal len = sqrt(x * x + y * y + z * z);
        if (len != 0.0) {
            len = 1.0 / len;
            x = x * len;
            y = y * len;
            z = len * z;
        }
        if (t->edgeData1[i] + 0.4 <= x * (n[1] * v[2] - n[2] * v[1]) + y * (n[2] * v[0] - v[2] * n[0])
                                         + z * (v[1] * n[0] - n[1] * v[0]))
            return false;
    }
    return true;
}

// Win 0x7667a0 (Mac: inlined) (name inferred): ((n x e0) . e1) / 2, n the given normal
inline MeReal TriangleArea(const VincTriangleRef* t, const MeReal* n)
{
    const MeReal* e0 = t->edges[0];
    const MeReal* e1 = t->edges[1];
    return ((n[0] * e0[1] - n[1] * e0[0]) * e1[2] + (e0[0] * n[2] - n[0] * e0[2]) * e1[1]
            + (n[1] * e0[2] - e0[1] * n[2]) * e1[0]) * 0.5;
}

// Mac 0x4cff82, Win 0x765cb0 (name inferred): src in the node's box frame (vertices into out's
// vertex arrays, edges, rotated normal, edge data; the index is not copied)
void TriangleToNode(const VincOBBNode* node, const VincTriangleRef* src, VincTriangleRef* out)
{
    for (int k = 0; k < 3; ++k) {
        const MeReal* p = src->vertices[k];
        MeReal d0 = p[0] - node->center[0], d1 = p[1] - node->center[1], d2 = p[2] - node->center[2];
        MeReal* q = out->vertices[k];
        q[0] = d0 * node->axis[0][0] + d1 * node->axis[0][1] + d2 * node->axis[0][2];
        q[1] = d0 * node->axis[1][0] + d1 * node->axis[1][1] + d2 * node->axis[1][2];
        q[2] = d0 * node->axis[2][0] + d1 * node->axis[2][1] + d2 * node->axis[2][2];
    }
    VincTriangleUpdateEdges(out);
    const MeReal* n = src->normal;
    out->normal[0] = node->axis[0][0] * n[0] + node->axis[0][1] * n[1] + node->axis[0][2] * n[2];
    out->normal[1] = node->axis[1][0] * n[0] + node->axis[1][1] * n[1] + node->axis[1][2] * n[2];
    out->normal[2] = node->axis[2][0] * n[0] + node->axis[2][1] * n[1] + node->axis[2][2] * n[2];
    for (int k = 0; k < 3; ++k) {
        out->edgeData0[k] = src->edgeData0[k];
        out->edgeData1[k] = src->edgeData1[k];
    }
}

} // namespace

extern "C" {

// Mac 0x4d15ce, Win 0x7668a0 (name inferred): contacts of the mesh triangle a with the triangle b
// (both in the mesh frame) appended to result->contacts (merged, params->maxCount <= 30); 0 when
// they do not intersect or no contact direction is admissible.
int VincTriangleMeshTriangleContacts(VincIntersectResult* r, const VincTriangleRef* a, const VincTriangleRef* b,
                                     MeReal pad, const VincContactMergeParams* params)
{
    (void)pad;
    MeReal pts[16][3];   // the original: 6 points
    int count;
    int kind = VincTriTriIntersectPoints(a->vertices[0], a->vertices[1], a->vertices[2], b->vertices[0],
                                         b->vertices[1], b->vertices[2], &pts[0][0], 6, &count);
    if (kind == 0)
        return 0;
    int limit = params->maxCount;
    if (29 < limit)
        limit = 30;
    if (count != 2) {
        // coplanar overlap (or a degenerate segment): the points with b's normal, no depth
        for (int i = 0; i < count; ++i) {
            if (r->contactCount < r->contactMaxCount) {
                VincContact* c = &r->contacts[r->contactCount];
                c->position[0] = pts[i][0];
                c->position[1] = pts[i][1];
                c->position[2] = pts[i][2];
                c->normal[0] = b->normal[0];
                c->normal[1] = b->normal[1];
                c->normal[2] = b->normal[2];
                c->separation = 0.0;
                c->element1 = a->index;
                c->element2 = b->index;
                VincContactsMergeAdd(r, params, limit);
            }
        }
        return 1;
    }
    // the segment: choose the separating direction (a face normal) the edge data admits
    MeVector3 dir, test;
    bool ok;
    if (kind == 2) {
        dir[0] = b->normal[0]; dir[1] = b->normal[1]; dir[2] = b->normal[2];
        test[0] = -dir[0]; test[1] = -dir[1]; test[2] = -dir[2];
        ok = TriangleEdgesAdmit(a, test);
    } else if (kind == 3) {
        dir[0] = -a->normal[0]; dir[1] = -a->normal[1]; dir[2] = -a->normal[2];
        ok = TriangleEdgesAdmit(b, dir);
    } else if (TriangleArea(b, b->normal) < TriangleArea(a, a->normal)) {
        dir[0] = -a->normal[0]; dir[1] = -a->normal[1]; dir[2] = -a->normal[2];
        ok = TriangleEdgesAdmit(b, dir);
        if (!ok) {
            dir[0] = b->normal[0]; dir[1] = b->normal[1]; dir[2] = b->normal[2];
            test[0] = -dir[0]; test[1] = -dir[1]; test[2] = -dir[2];
            ok = TriangleEdgesAdmit(a, test);
        }
    } else {
        dir[0] = b->normal[0]; dir[1] = b->normal[1]; dir[2] = b->normal[2];
        test[0] = -dir[0]; test[1] = -dir[1]; test[2] = -dir[2];
        ok = TriangleEdgesAdmit(a, test);
        if (!ok) {
            dir[0] = -a->normal[0]; dir[1] = -a->normal[1]; dir[2] = -a->normal[2];
            ok = TriangleEdgesAdmit(b, dir);
        }
    }
    if (!ok)
        return 0;
    VincTriTriSeparation sep;
    VincTriTriSeparationInit(&sep, a->vertices[0], a->vertices[1], a->vertices[2], b->vertices[0], b->vertices[1],
                             b->vertices[2], (MeReal*)a->normal, (MeReal*)b->normal, dir);
    VincTriTriSeparationCompute(&sep);
    for (int i = 0; i < 2; ++i) {
        if (r->contactCount < r->contactMaxCount) {
            VincContact* c = &r->contacts[r->contactCount];
            c->position[0] = pts[i][0];
            c->position[1] = pts[i][1];
            c->position[2] = pts[i][2];
            c->normal[0] = dir[0];
            c->normal[1] = dir[1];
            c->normal[2] = dir[2];
            c->separation = 0.0 < sep.depth ? 0.0 : sep.depth;
            c->element1 = a->index;
            c->element2 = b->index;
            VincContactsMergeAdd(r, params, limit);
        }
    }
    return 1;
}

// Mac 0x4d028f, Win 0x765e70 (name inferred): the triangle tri (mesh frame) down the mesh's OBB tree
void VincTriangleMeshTriangleNode(VincIntersectResult* r, const VincTriangleRef* tri, VincGeometryID mesh,
                                  const VincOBBNode* node, MeReal pad, const VincContactMergeParams* params)
{
    MeReal v[3][3];
    VincTriangleRef local;
    local.index = -1;
    local.edgeData0[0] = local.edgeData0[1] = local.edgeData0[2] = -2.0;
    local.edgeData1[0] = local.edgeData1[1] = local.edgeData1[2] = 0.0;
    local.vertices[0] = v[0];
    local.vertices[1] = v[1];
    local.vertices[2] = v[2];
    TriangleToNode(node, tri, &local);
    if (TriangleBoxSeparated(&pad, node->halfExtents, &local))
        return;
    if (!VincTriMeshPairs::IsLeaf(node)) {
        VincTriangleMeshTriangleNode(r, tri, mesh, node->child[0], pad, params);
        VincTriangleMeshTriangleNode(r, tri, mesh, node->child[1], pad, params);
        return;
    }
    // the mesh triangle (mesh frame)
    local.index = node->triangle;
    VincTriangleMeshGetCornerData(mesh, local.index, local.edgeData0, local.edgeData1);
    VincTriangleMeshGetTrianglePtrs(mesh, node->triangle, &local.vertices[0], &local.vertices[1], &local.vertices[2]);
    VincTriMeshPairs::TriangleSetup(&local);
    int saved = r->contactCount;
    if (!VincTriangleMeshTriangleContacts(r, &local, tri, pad, params))
        r->contactCount = saved;
}

} // extern "C"
