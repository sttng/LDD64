// Vortex 5.0.0 build 800 "vortexldd41" - convex hulls through Qhull 2010.1: cnvComputeHull and
// the hull helpers (restored). Mac: the end of 557a (0x4b14d8-0x4b22f2) and the
// start of 557b (cnvComputeHull 0x4b22f2; the rest of that TU is VincConvexMesh.cpp); Win
// 0x7aecc0-0x7b0040 (cnvComputeHull's helpers separate, inlined on Mac) and 0x7b6af0-0x7b6cd0.
// cnvComputeHull is LDD's only qhull caller (thirdparty/qhull-2010.1-vortex/NOTES.md, "API surface
// used by Vortex"): it walks qhull's facet / vertex lists into records, chains each facet's edges
// clockwise around its outward normal and copies the result into a CnvHull. Names inferred except
// cnvComputeHull and VincConvexMeshCreateHull (named in cnvComputeHull's messages).
#include "Vinc/VincConvexMesh.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"

#include <math.h>
#include <stdio.h>
#include <string.h>

extern "C" {
#include "libqhull.h"
#include "mem.h"
#include "qset.h"
}
#include "Me/MeCrtMath.h"

namespace {

// ---- the walk records (file statics in the original: Mac 0x78023c / 0x780240, Win 0xaf8f80 /
// 0xaf8f7c) ----
struct VertexRec;
struct FacetRec;
struct EdgeRec {               // 0x10: the edge between two facets
    VertexRec* a;              // +0x00 (lower address first when created)
    VertexRec* b;              // +0x04
    FacetRec* f0;              // +0x08
    FacetRec* f1;              // +0x0c
};
struct VertexRec {             // Mac 0x2c, Win 0x30
    vertexT* v;                // +0x00
    int facetCount;            // +0x04
    facetT** facets;           // +0x08 (num_facets slots)
    int edgeCount;             // +0x0c
    EdgeRec* edges;            // +0x10 (num_vertices slots)
    MeVector3 normal;          // +0x14 +0x18 mean of the facet normals
};
struct FacetRec {              // 0x14
    facetT* f;                 // +0x00
    int edgeCount;             // +0x04
    EdgeRec* edges;            // +0x08 (num_facets slots)
    int vertexCount;           // +0x0c
    vertexT** vertices;        // +0x10 (num_vertices slots)
};

VertexRec* s_vertexRecs = 0;   // Mac 0x78023c, Win 0xaf8f80
FacetRec* s_facetRecs = 0;     // Mac 0x780240, Win 0xaf8f7c

// the shared point hull of cnvGetPointConvex (Mac 0x780200-0x780238, Win 0xaf8f88-0xaf8fc0)
CnvVertex s_pointVertex;       // Mac 0x780200
CnvHull s_pointHull;           // Mac 0x780228
CnvHull* s_pointConvex = 0;    // Mac 0x780238

inline void* Alloc(size_t n) { return MeMemoryAPI.create(n); }
inline void Free(void* p) { MeMemoryAPI.destroy(p); }

// Mac 0x4b1785 (regparm): out = a x b
void Cross(MeReal* out, const MeReal* a, const MeReal* b)
{
    out[0] = a[1] * b[2] - a[2] * b[1];
    out[1] = a[2] * b[0] - a[0] * b[2];
    out[2] = a[0] * b[1] - a[1] * b[0];
}

// Mac 0x4b17de (regparm)
MeReal Dot(const MeReal* a, const MeReal* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// Mac 0x4b1807 (regparm): out = a - b
void Sub(MeReal* out, const MeReal* a, const MeReal* b)
{
    out[0] = a[0] - b[0];
    out[1] = a[1] - b[1];
    out[2] = a[2] - b[2];
}

// Mac 0x4b1838 (regparm)
void Copy(MeReal* out, const MeReal* a)
{
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
}

// Mac 0x4b1859 (regparm; Win: inlined): the record of a facet (linear search, always present)
FacetRec* FindFacetRec(facetT* f)
{
    FacetRec* r = s_facetRecs;
    while (r->f != f)
        ++r;
    return r;
}

// (Mac: inlined; Win: inlined): the record of a vertex
VertexRec* FindVertexRec(vertexT* v)
{
    VertexRec* r = s_vertexRecs;
    while (r->v != v)
        ++r;
    return r;
}

// Mac 0x4b1877, Win 0x7aecc0 (regparm): the hull vertex made from a vertex record
CnvVertex* HullVertex(CnvVertex* vertices, const VertexRec* r)
{
    return vertices + (r - s_vertexRecs);
}

// Mac 0x4b19a8, Win 0x7aecf0: appends e unless the list holds it already; 1 when appended
int AddEdge(EdgeRec* edges, int* count, const EdgeRec* e)
{
    for (int i = 0; i < *count; ++i)
        if (memcmp(&edges[i], e, sizeof(EdgeRec)) == 0)
            return 0;
    edges[*count] = *e;
    ++*count;
    return 1;
}

// Mac 0x4b1a38 (regparm; Win 0x7b29b0 is the same helper in VincConvexMesh): swaps n bytes
void MemSwap(void* a, void* b, size_t n)
{
    char tmp[128];
    memcpy(tmp, a, n);
    memcpy(a, b, n);
    memcpy(b, tmp, n);
}

// Mac 0x4b1ad6 (recursive), Win 0x7af230 (a loop): makes edges[0..n) a chain starting at `from`:
// each edge's a is the previous edge's b. Stops at the first gap.
void ChainEdges(EdgeRec* edges, int n, VertexRec* from)
{
    if (n <= 0)
        return;
    int i;
    for (i = 0; i < n; ++i) {
        if (edges[i].a == from)
            break;
        if (edges[i].b == from) {
            MemSwap(&edges[i].a, &edges[i].b, sizeof(VertexRec*));
            break;
        }
    }
    if (i == n)
        return;
    if (i != 0)
        MemSwap(&edges[i], &edges[0], sizeof(EdgeRec));
    if (n > 1)
        ChainEdges(edges + 1, n - 1, edges[0].b);
}

// Win 0x7af110 (Mac: inlined): turns the chain of a facet clockwise around its normal (the chain
// is reversed when it runs counter-clockwise)
void OrientChain(EdgeRec* edges, int n, const MeReal* normal)
{
    MeReal u[3], w[3], c[3];
    const MeReal* p0 = edges[0].a->v->point;
    Sub(u, edges[0].b->v->point, p0);
    Sub(w, edges[1].b->v->point, p0);
    Cross(c, u, w);
    if (0.0 < Dot(c, normal)) {
        int i;
        for (i = 0; i != n; ++i)
            MemSwap(&edges[i].a, &edges[i].b, sizeof(VertexRec*));
        for (i = 0; i != n / 2; ++i)
            MemSwap(&edges[i], &edges[n - 1 - i], sizeof(EdgeRec));
    }
}

// Mac 0x4b1b65, Win 0x7af770 (regparm): the angle from a to b around axis, in [0, 2 pi]
// (+ pi as a float constant in both builds). NOTE: on Win pi is added to the CRT atan2's
// unrounded 64-bit-mantissa result (MeAtan2L / MeLongAdd, Me/MeCrtMath.h).
MeReal Angle(const MeReal* a, const MeReal* b, const MeReal* axis)
{
    MeReal c[3];
    Cross(c, a, b);
    MeReal cosine = Dot(a, b);
    MeReal sine = sqrt(c[0] * c[0] + c[1] * c[1] + c[2] * c[2]);
    if (Dot(c, axis) < 0.0)
        sine = -sine;
    return MeLongAdd(MeAtan2L(sine, cosine), (MeReal)3.14159265358979f);
}

// Win 0x7aefb0 (Mac: inlined): one record per qhull vertex with its facets
void MakeVertexRecs(void)
{
    int nv = qh num_vertices, nf = qh num_facets;
    s_vertexRecs = (VertexRec*)Alloc(nv * sizeof(VertexRec));
    for (int i = 0; i < qh num_vertices; ++i) {
        s_vertexRecs[i].edgeCount = 0;
        s_vertexRecs[i].edges = (EdgeRec*)Alloc(nv * sizeof(EdgeRec));
        s_vertexRecs[i].facetCount = 0;
        s_vertexRecs[i].facets = (facetT**)Alloc(nf * sizeof(facetT*));
    }
    VertexRec* r = s_vertexRecs;
    for (vertexT* v = qh vertex_list; v != 0 && v->next != 0; v = v->next, ++r) {
        r->v = v;
        if (v->neighbors != 0)          // only when qhull merged (qh VERTEXneighbors)
            for (void** p = &v->neighbors->e[0].p; *p != 0; ++p)
                r->facets[r->facetCount++] = (facetT*)*p;
    }
}

// Win 0x7aeec0 (Mac: inlined): one record per qhull facet with its vertices
void MakeFacetRecs(void)
{
    int nf = qh num_facets, nv = qh num_vertices;
    s_facetRecs = (FacetRec*)Alloc(nf * sizeof(FacetRec));
    for (int i = 0; i < qh num_facets; ++i) {
        s_facetRecs[i].edgeCount = 0;
        s_facetRecs[i].edges = (EdgeRec*)Alloc(nf * sizeof(EdgeRec));
        s_facetRecs[i].vertexCount = 0;
        s_facetRecs[i].vertices = (vertexT**)Alloc(nv * sizeof(vertexT*));
    }
    FacetRec* r = s_facetRecs;
    for (facetT* f = qh facet_list; f != 0 && f->next != 0; f = f->next, ++r) {
        r->f = f;
        if (f->vertices != 0)
            for (void** p = &f->vertices->e[0].p; *p != 0; ++p)
                r->vertices[r->vertexCount++] = (vertexT*)*p;
    }
}

// Win 0x7af0a0 (Mac: inlined): the vertex records of the first two vertices facets a and b share
// (in a's vertex order)
void CommonVertices(const FacetRec* a, const FacetRec* b, VertexRec** out)
{
    int k = 0;
    for (int i = 0; i < a->vertexCount; ++i)
        for (int j = 0; j < b->vertexCount; ++j)
            if (a->vertices[i] == b->vertices[j]) {
                out[k++] = FindVertexRec(a->vertices[i]);
                if (k == 2)
                    return;
            }
}

// Win 0x7aed80 (Mac: inlined): the edge between facets fa and fb through va, vb, added to all four
void AddFacetEdge(VertexRec* va, VertexRec* vb, FacetRec* fa, FacetRec* fb)
{
    EdgeRec e;
    e.a = va < vb ? va : vb;
    e.b = va < vb ? vb : va;
    e.f0 = fa < fb ? fa : fb;
    e.f1 = fa < fb ? fb : fa;
    AddEdge(va->edges, &va->edgeCount, &e);
    AddEdge(vb->edges, &vb->edgeCount, &e);
    AddEdge(fa->edges, &fa->edgeCount, &e);
    AddEdge(fb->edges, &fb->edgeCount, &e);
}

// Win 0x7af3c0 (Mac: inlined): the edges of one facet with each of its neighbours
void MakeFacetEdges(facetT* f)
{
    FacetRec* r = FindFacetRec(f);
    if (f->neighbors == 0)
        return;
    for (void** p = &f->neighbors->e[0].p; *p != 0; ++p) {
        FacetRec* n = FindFacetRec((facetT*)*p);
        // NOTE: the original leaves the second entry as uninitialised stack when the facets share
        // fewer than two vertices (not seen with qhull's hulls); zeroed here
        VertexRec* common[2] = {0, 0};
        CommonVertices(r, n, common);
        AddFacetEdge(common[0], common[1], r, n);
    }
}

// Win 0x7af2e0 (Mac: inlined): chain + orient every facet's edges; every vertex's edges start
// at that vertex
void OrderEdges(void)
{
    for (int i = 0; i < qh num_facets; ++i) {
        FacetRec* r = &s_facetRecs[i];
        ChainEdges(r->edges + 1, r->edgeCount - 1, r->edges[0].b);
        OrientChain(r->edges, r->edgeCount, r->f->normal);
    }
    for (int i = 0; i < qh num_vertices; ++i) {
        VertexRec* r = &s_vertexRecs[i];
        for (int k = 0; k < r->edgeCount; ++k)
            if (r->edges[k].a != r)
                MemSwap(&r->edges[k].a, &r->edges[k].b, sizeof(VertexRec*));
    }
}

// Win 0x7af430 (Mac: inlined): vertex normals = mean of the facet normals
void VertexNormals(void)
{
    for (int i = 0; i < qh num_vertices; ++i) {
        VertexRec* r = &s_vertexRecs[i];
        r->normal[0] = 0.0;
        r->normal[1] = 0.0;
        r->normal[2] = 0.0;
        for (int k = 0; k < r->facetCount; ++k) {
            r->normal[0] = r->normal[0] + r->facets[k]->normal[0];
            r->normal[1] = r->normal[1] + r->facets[k]->normal[1];
            r->normal[2] = r->normal[2] + r->facets[k]->normal[2];
        }
        r->normal[0] = r->normal[0] / (MeReal)r->facetCount;
        r->normal[1] = r->normal[1] / (MeReal)r->facetCount;
        r->normal[2] = r->normal[2] / (MeReal)r->facetCount;
    }
}

// Win 0x7af4c0 (Mac: inlined): the hull vertices (position, neighbours, faces)
void CopyVertices(CnvHull* hull)
{
    for (int i = 0; i < qh num_vertices; ++i) {
        VertexRec* r = &s_vertexRecs[i];
        CnvVertex* v = &hull->vertices[i];
        Copy(v->pos, r->v->point);
        v->neighbourCount = r->edgeCount;
        v->neighbours = (CnvVertex**)Alloc(r->edgeCount * sizeof(CnvVertex*));
        for (int k = 0; k < r->edgeCount; ++k)
            v->neighbours[k] = HullVertex(hull->vertices, r->edges[k].b);
        v->faceCount = r->facetCount;
        v->faces = (CnvFace**)Alloc(r->facetCount * sizeof(CnvFace*));
        for (int k = 0; k < r->facetCount; ++k)
            v->faces[k] = hull->faces + (FindFacetRec(r->facets[k]) - s_facetRecs);
    }
}

// Win 0x7af610 (Mac: inlined): the hull faces (normal, corners)
void CopyFaces(CnvHull* hull, int normalMode)
{
    for (int i = 0; i < qh num_facets; ++i) {
        FacetRec* r = &s_facetRecs[i];
        CnvFace* f = &hull->faces[i];
        Copy(f->normal, r->f->normal);
        f->count = r->edgeCount;
        f->edges = (CnvEdge*)Alloc(r->edgeCount * sizeof(CnvEdge));
        for (int k = 0; k < r->edgeCount; ++k) {
            CnvEdge* e = &f->edges[k];
            VertexRec* a = r->edges[k].a;
            e->vertex = HullVertex(hull->vertices, a);
            Copy(e->pos, a->v->point);
            if (normalMode == 0)
                Copy(e->normal, a->normal);
            else if (normalMode == 1)
                Copy(e->normal, r->f->normal);
        }
    }
}

// Win 0x7aee00 (Mac: inlined)
void FreeRecs(void)
{
    for (int i = 0; i < qh num_vertices; ++i) {
        Free(s_vertexRecs[i].edges);
        Free(s_vertexRecs[i].facets);
    }
    Free(s_vertexRecs);
    s_vertexRecs = 0;
    for (int i = 0; i < qh num_facets; ++i) {
        Free(s_facetRecs[i].edges);
        Free(s_facetRecs[i].vertices);
    }
    Free(s_facetRecs);
    s_facetRecs = 0;
}

} // namespace

extern "C" {

// Mac 0x4b14d8, Win 0x7b6af0: a two-vertex hull p0 - p1 (each vertex the other's neighbour)
CnvHull* MEAPI cnvSegmentSet(CnvHull* segment, const MeReal* p0, const MeReal* p1)
{
    segment->faces = 0;
    segment->faceCount = 0;
    Copy(segment->vertices[0].pos, p0);
    Copy(segment->vertices[1].pos, p1);
    segment->vertices[0].neighbours[0] = &segment->vertices[1];
    segment->vertices[1].neighbours[0] = &segment->vertices[0];
    segment->vertices[0].faces = 0;
    segment->vertices[0].faceCount = 0;
    segment->vertices[1].faces = 0;
    segment->vertices[1].faceCount = 0;
    return segment;
}

// Mac 0x4b157c, Win 0x7b6b70
CnvHull* MEAPI cnvSegmentCreate(const MeReal* p0, const MeReal* p1)
{
    CnvHull* h = (CnvHull*)Alloc(sizeof(CnvHull));
    h->vertexCount = 2;
    h->vertices = (CnvVertex*)Alloc(2 * sizeof(CnvVertex));
    h->vertices[0].neighbourCount = 1;
    h->vertices[0].neighbours = (CnvVertex**)Alloc(sizeof(CnvVertex*));
    h->vertices[1].neighbourCount = 1;
    h->vertices[1].neighbours = (CnvVertex**)Alloc(sizeof(CnvVertex*));
    return cnvSegmentSet(h, p0, p1);
}

// Mac 0x4b1613, Win 0x7b6bd0
CnvHull* MEAPI cnvCreateSegment(MeReal halfLength)
{
    MeReal p0[3] = {0.0, 0.0, halfLength};
    MeReal p1[3] = {0.0, 0.0, -halfLength};
    return cnvSegmentCreate(p0, p1);
}

// Mac 0x4b1667, Win 0x7b6c10
CnvHull* MEAPI cnvSetSegment(CnvHull* segment, MeReal halfLength)
{
    MeReal p0[3] = {0.0, 0.0, halfLength};
    MeReal p1[3] = {0.0, 0.0, -halfLength};
    return cnvSegmentSet(segment, p0, p1);
}

// Mac 0x4b16c2, Win 0x7b6c50
void MEAPI cnvHullDestroy(CnvHull* hull)
{
    for (int i = 0; i < hull->vertexCount; ++i) {
        Free(hull->vertices[i].faces);
        Free(hull->vertices[i].neighbours);
    }
    Free(hull->vertices);
    for (int i = 0; i < hull->faceCount; ++i)
        Free(hull->faces[i].edges);
    Free(hull->faces);
    Free(hull);
}

// Mac 0x4b1897, Win 0x7afd40 (stdcall with one argument on Win: both callers pass 0)
CnvHull* MEAPI cnvGetPointConvex(int /*unused*/)
{
    if (s_pointConvex == 0) {
        s_pointConvex = &s_pointHull;
        s_pointHull.faces = 0;
        s_pointHull.faceCount = 0;
        s_pointHull.vertices = &s_pointVertex;
        s_pointHull.vertexCount = 1;
        s_pointVertex.pos[0] = 0.0;
        s_pointVertex.pos[1] = 0.0;
        s_pointVertex.pos[2] = 0.0;
        s_pointVertex.neighbours = 0;
        s_pointVertex.neighbourCount = 0;
        s_pointVertex.faces = 0;
        s_pointVertex.faceCount = 0;
    }
    return s_pointConvex;
}

// Mac 0x4b1913, Win 0x7b0040
void MEAPI cnvDestroyTriangle(CnvHull* hull)
{
    Free(hull->faces[0].edges);
    Free(hull->vertices[2].faces);
    Free(hull->vertices[1].faces);
    Free(hull->vertices[0].faces);
    Free(hull->vertices[2].neighbours);
    Free(hull->vertices[1].neighbours);
    Free(hull->vertices[0].neighbours);
    Free(hull->vertices);
    Free(hull->faces);
    Free(hull);
}

// Mac 0x4b1c36, Win 0x7af810
void MEAPI cnvUpdate(CnvHull* hull)
{
    int n = hull->vertexCount;
    MeReal cx, cy, cz;
    if (n < 4) {
        cz = 0.0;
        cy = 0.0;
        cx = 0.0;
    } else {
        cx = 0.0;
        cy = 0.0;
        cz = 0.0;
        for (int i = 0; i != n; ++i) {
            cx = cx + hull->vertices[i].pos[0];
            cy = cy + hull->vertices[i].pos[1];
            cz = cz + hull->vertices[i].pos[2];
        }
        MeReal inv = 1.0 / (MeReal)n;
        cx = cx * inv;
        cy = cy * inv;
        cz = cz * inv;
    }
    for (int i = 0; i < n; ++i) {
        CnvVertex* v = &hull->vertices[i];
        CnvVertex** nb = v->neighbours;
        int count = v->neighbourCount;
        MeReal* dirs = (MeReal*)Alloc(count * 3 * sizeof(MeReal));
        MeReal** order = (MeReal**)Alloc(count * sizeof(MeReal*));
        for (int k = 0; k != count; ++k) {
            Sub(dirs + 3 * k, nb[k]->pos, v->pos);
            order[k] = dirs + 3 * k;
        }
        // the axis: from the centroid to the vertex (or the vertex position, or x)
        MeReal axis[3];
        axis[0] = v->pos[0] - cx;
        axis[1] = v->pos[1] - cy;
        axis[2] = v->pos[2] - cz;
        MeReal len = sqrt(axis[2] * axis[2] + (axis[1] * axis[1] + (axis[0] * axis[0] + 0.0)));
        if (len < 1e-05) {
            MeReal plen = sqrt(v->pos[2] * v->pos[2] + (v->pos[1] * v->pos[1] + (v->pos[0] * v->pos[0] + 0.0)));
            if (1e-05 < plen) {
                MeReal inv = 1.0 / plen;
                axis[0] = v->pos[0] * inv;
                axis[1] = v->pos[1] * inv;
                axis[2] = v->pos[2] * inv;
            } else {
                axis[0] = 1.0;
                axis[1] = 0.0;
                axis[2] = 0.0;
            }
        } else {
            MeReal inv = 1.0 / len;
            axis[0] = axis[0] * inv;
            axis[1] = axis[1] * inv;
            axis[2] = axis[2] * inv;
        }
        // selection sort: each next neighbour is the one at the smallest angle from the previous
        for (int k = 1; k < count; ++k) {
            int best = k;
            MeReal bestAngle = Angle(order[k - 1], order[k], axis);
            for (int j = k + 1; j < count; ++j) {
                MeReal a = Angle(order[k - 1], order[j], axis);
                if (a < bestAngle) {
                    best = j;
                    bestAngle = a;
                }
            }
            if (best != k) {
                CnvVertex* t = nb[k];
                MeReal* o = order[k];
                nb[k] = nb[best];
                order[k] = order[best];
                nb[best] = t;
                order[best] = o;
            }
        }
        Free(dirs);
        Free(order);
    }
}

// Mac 0x4b202f, Win 0x7afdb0
CnvHull* MEAPI cnvCreateTriangle(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* normal)
{
    CnvHull* h = (CnvHull*)Alloc(sizeof(CnvHull));
    h->faceCount = 1;
    h->faces = (CnvFace*)Alloc(sizeof(CnvFace));
    h->vertexCount = 3;
    CnvVertex* v = (CnvVertex*)Alloc(3 * sizeof(CnvVertex));
    h->vertices = v;
    Copy(v[0].pos, v0);
    Copy(v[1].pos, v1);
    Copy(v[2].pos, v2);
    v[0].neighbourCount = 2;
    v[0].neighbours = (CnvVertex**)Alloc(2 * sizeof(CnvVertex*));
    v[0].neighbours[0] = &v[1];
    v[0].neighbours[1] = &v[2];
    v[1].neighbourCount = 2;
    v[1].neighbours = (CnvVertex**)Alloc(2 * sizeof(CnvVertex*));
    v[1].neighbours[0] = &v[2];
    v[1].neighbours[1] = &v[0];
    v[2].neighbourCount = 2;
    v[2].neighbours = (CnvVertex**)Alloc(2 * sizeof(CnvVertex*));
    v[2].neighbours[0] = &v[0];
    v[2].neighbours[1] = &v[1];
    cnvUpdate(h);
    for (int k = 0; k < 3; ++k) {
        v[k].faceCount = 1;
        v[k].faces = (CnvFace**)Alloc(sizeof(CnvFace*));
        v[k].faces[0] = h->faces;
    }
    CnvFace* f = h->faces;
    f->count = 3;
    f->edges = (CnvEdge*)Alloc(3 * sizeof(CnvEdge));
    for (int k = 0; k < 3; ++k) {
        f->edges[k].vertex = &v[k];
        Copy(f->edges[k].pos, v[k].pos);
        Copy(f->edges[k].normal, normal);
    }
    Copy(f->normal, normal);
    return h;
}

// Mac 0x4b22f2, Win 0x7afb30
CnvHull* MEAPI cnvComputeHull(int n, const MeReal* points, int normalMode, const char* options)
{
    FILE* errfile = stderr;
    if (options == 0)
        options = "Qs";
    char* command = (char*)Alloc(strlen(options) + strlen("qhull ") + 1);
    strcpy(command, "qhull ");
    strcat(command, options);
    CnvHull* hull = 0;
    int exitcode = qh_new_qhull(3, n, (coordT*)points, False, command, (FILE*)0, errfile);
    if (exitcode == 0) {
        hull = (CnvHull*)Alloc(sizeof(CnvHull));
        MakeVertexRecs();
        MakeFacetRecs();
        for (facetT* f = qh facet_list; f != 0 && f->next != 0; f = f->next)
            MakeFacetEdges(f);
        OrderEdges();
        VertexNormals();
        hull->vertexCount = qh num_vertices;
        hull->vertices = (CnvVertex*)Alloc(qh num_vertices * sizeof(CnvVertex));
        hull->faceCount = qh num_facets;
        hull->faces = (CnvFace*)Alloc(qh num_facets * sizeof(CnvFace));
        CopyVertices(hull);
        CopyFaces(hull, normalMode);
        cnvUpdate(hull);
        FreeRecs();
        qh_freeqhull(qh_ALL);
        int totlong, curlong, totshort, curshort, maxlong, totbuffer;
        qh_memtotal(&totlong, &curlong, &totshort, &curshort, &maxlong, &totbuffer);
        // NOTE: bytes and pieces are swapped in both messages (as in the original)
        if (curlong != 0 || totlong != 0)
            MeWarning(0, "qhull internal warning (VincConvexMeshCreateHull, #1): did not free %d bytes of MeI32 memory (%d pieces)\n",
                      curlong, totlong);
        if (curshort != 0 || totshort != 0)
            MeWarning(0, "qhull internal warning (VincConvexMeshCreateHull, #1): did not free %d bytes of MeI16 memory (%d pieces)\n",
                      curshort, totshort);
    } else {
        MeWarning(0, "cnvComputeHull: qhull terminated with errorcode %d\n", exitcode);
        qh_freeqhull(qh_ALL);
    }
    Free(command);
    return hull;
}

} // extern "C"
