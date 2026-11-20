// Vortex 5.0.0 build 800 "vortexldd41" - pair tests and helpers of the Convex geometry (type 9)
// against the primitives, the height field ("ConvexMeshTriList") and triangle lists
// ("ConvexTriList"), the convex line segment query, segment distance helpers used by the hull code,
// the per-pair data pool and the convex hull accessors (restored; Mac 557c
// 0x4ba87b-0x4bf81e; Win: 0x75c6c0, 0x75c6d0, 0x763600, 0x763d10 paired, the rest is unpaired).
// The hull, the convex-convex core (Mac 0x4b6426) and the hull builders are in VincConvexMesh.cpp /
// VincConvexHull.cpp (declared in Vinc/VincConvexMesh.h).
#include "Vinc/VincIntersect.h"
#include "Vinc/VincMath.h"
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincHeightField.h"
#include "Vinc/VincConvexInteractions.h"
#include "Me/MeMemory.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "VincInternal.h"

extern "C" {
CnvHull* VincConvexTriangleHull = 0;   // Mac 0x779530: one triangle as a hull (shared)
CnvHull* VincConvexBoxHull = 0;        // Mac 0x779534: unit box hull, rescaled per test (shared)
}

namespace {

CnvHull* s_hull538 = 0;                // Mac 0x779538: freed by the pool teardown, never set here

// ---- per-pair data pool (Mac 0x77953c) ----
const int kPairDataSize = sizeof(CnvPairCache); // pair cache object (Mac 0x10c, Win 0x118 bytes)

struct PairDataBlockEntry {
    void* objects;
    int count;
};
struct PairDataBlockList {             // Mac 0x328 bytes
    PairDataBlockEntry entries[100];   // +0x000
    unsigned int count;                // +0x320
    PairDataBlockList* next;           // +0x324
};
struct PairDataPool {                  // Mac 0x14 bytes
    int growBy;                        // +0x00 (100)
    int capacity;                      // +0x04
    void** begin;                      // +0x08 free stack
    void** top;                        // +0x0c
    PairDataBlockList* blocks;         // +0x10
};
PairDataPool* s_pairDataPool = 0;      // Mac 0x77953c

// Mac 0x4bf7d2 (name inferred)
void FreeArray(void** p, int /*count*/)
{
    if (p != 0)
        MeMemoryAPI.destroy(p);
}

// Mac 0x4bf7f8 (name inferred)
void FreeObjects(void* p, int /*count*/)
{
    if (p != 0)
        MeMemoryAPI.destroy(p);
}

// Mac 0x4bf81e (name inferred; the original is the recursion unrolled eight levels deep)
void FreeBlockChain(PairDataBlockList* b)
{
    while (b->count != 0) {
        --b->count;
        FreeObjects(b->entries[b->count].objects, b->entries[b->count].count);
    }
    if (b->next != 0) {
        FreeBlockChain(b->next);
        MeMemoryAPI.destroy(b->next);
        b->next = 0;
    }
}

void ReleaseBlocks(PairDataPool* pool)
{
    PairDataBlockList* b = pool->blocks;
    if (b != 0) {
        while (b->count != 0) {
            --b->count;
            FreeObjects(b->entries[b->count].objects, b->entries[b->count].count);
        }
        if (b->next != 0) {
            FreeBlockChain(b->next);
            MeMemoryAPI.destroy(b->next);
            b->next = 0;
        }
        MeMemoryAPI.destroy(pool->blocks);
        pool->blocks = 0;
    }
}

PairDataBlockList* NewBlockList(PairDataBlockList* next)
{
    PairDataBlockList* b = (PairDataBlockList*)MeMemoryAPI.create(sizeof(PairDataBlockList));
    if (b != 0) {
        b->count = 0;
        b->next = next;
    }
    return b;
}

// Mac 0x4bf21d (name inferred): terminate action of the convex interactions
void MEAPI ConvexInteractionsTerm(void)
{
    PairDataPool* pool = s_pairDataPool;
    if (pool != 0) {
        ReleaseBlocks(pool);
        pool->blocks = NewBlockList(0);
        FreeArray(pool->begin, pool->capacity);
        pool->begin = 0;
        pool->top = 0;
        pool->capacity = 0;
        ReleaseBlocks(pool);
        MeMemoryAPI.destroy(s_pairDataPool);
        s_pairDataPool = 0;
    }
    if (VincConvexBoxHull != 0)
        cnvHullDestroy(VincConvexBoxHull);
    if (s_hull538 != 0)
        cnvHullDestroy(s_hull538);
    s_hull538 = 0;
    VincConvexBoxHull = 0;
}

// ---- small helpers ----

// a * b^T for the rotations and (Ta - Tb) * b^T: the pose of a in b's frame (rows 0..2, 12..14)
void RelativeTM(const MeReal* a, const MeReal* b, MeReal* rel)
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

// unit normal of v0 v1 v2 and the edges, in the order of the original (Mac 0x4bb6c4 ...)
void TriangleSetup(VincTriangleRef* t)
{
    MeReal* a = t->vertices[0];
    MeReal* b = t->vertices[1];
    MeReal* c = t->vertices[2];
    t->edges[0][2] = b[2] - a[2];
    t->edges[0][1] = b[1] - a[1];
    t->edges[0][0] = b[0] - a[0];
    t->edges[1][2] = c[2] - b[2];
    t->edges[1][1] = c[1] - b[1];
    t->edges[1][0] = c[0] - b[0];
    t->edges[2][2] = a[2] - c[2];
    t->edges[2][1] = a[1] - c[1];
    t->edges[2][0] = a[0] - c[0];
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

inline MeReal Dot0(const MeReal* a, const MeReal* b)
{
    MeReal s = 0.0;
    s = s + a[0] * b[0];
    s = s + a[1] * b[1];
    s = s + a[2] * b[2];
    return s;
}

// Mac 0x4bad99 (name inferred): segment p q against the plane n.x = d
int SegmentPlane(MeReal* out, const MeReal* n, const MeReal* d, const MeReal* p, const MeReal* q)
{
    MeReal dp = (p[0] * n[0] + p[1] * n[1] + p[2] * n[2]) - *d;
    MeReal dq = (n[0] * q[0] + n[1] * q[1] + n[2] * q[2]) - *d;
    if (dp > 0.0 && dq > 0.0)
        return 0;
    if (dp < 0.0 && dq < 0.0)
        return 0;
    MeReal ez = q[2] - p[2], ey = q[1] - p[1], ex = q[0] - p[0];
    MeReal den = ex * n[0] + n[1] * ey + n[2] * ez;
    if (fabs(den) < 1e-14)
        return 0;
    MeReal t = -dp / den;
    out[0] = t * ex + p[0];
    out[1] = ey * t + p[1];
    out[2] = ez * t + p[2];
    return 1;
}

} // namespace

extern "C" {

// ---- line segment ----

// Mac 0x4ba8a6 (name inferred): segment p0-p1 against the hull's face planes (Cyrus-Beck)
int MEAPI VincConvexMeshLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1,
                                    VincLineSegResult* r)
{
    VincConvexGeometry* g = (VincConvexGeometry*)VincModelGetGeometry(model);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeReal a[3], b[3];
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    MeReal dx = b[0] - a[0], dy = b[1] - a[1], dz = b[2] - a[2];
    MeReal len = sqrt(dx * dx + dy * dy + dz * dz);
    if (len == 0.0)
        return 0;
    MeReal inv = 1.0 / len;
    if (len < 1e-7)
        return 0;
    CnvHull* hull = g->hull;
    if (hull->faceCount <= 0)
        return 0;
    MeReal tEnter = -1e+20, tExit = len;
    int enterFace = 0, exitFace = 0;
    for (int i = 0; i != hull->faceCount; ++i) {
        CnvFace* f = &hull->faces[i];
        MeReal* v = f->edges[0].pos;
        if (tExit < tEnter)
            return 0;
        MeReal dn = dx * inv * f->normal[0] + dy * inv * f->normal[1] + dz * inv * f->normal[2];
        MeReal dist = (a[0] * f->normal[0] + a[1] * f->normal[1] + a[2] * f->normal[2])
                      - (f->normal[0] * v[0] + v[1] * f->normal[1] + v[2] * f->normal[2]);
        if (1e-14 <= fabs(dn)) {
            MeReal t = -dist / dn;
            if (0.0 <= dn) {
                if (t < 0.0)
                    return 0;
                if (t < tExit) {
                    exitFace = i;
                    tExit = t;
                }
            } else {
                if (tExit < t)
                    return 0;
                if (tEnter < t) {
                    enterFace = i;
                    tEnter = t;
                }
            }
        } else if (0.0 < dist) {
            return 0;
        }
    }
    MeReal* n = hull->faces[enterFace].normal;
    if (tEnter < 0.0) {
        tEnter = tExit;
        if (len <= tExit)
            return 0;
        n = hull->faces[exitFace].normal;
    }
    MeReal f = tEnter / len;
    r->position[0] = (p1[0] - p0[0]) * f + p0[0];
    r->position[1] = p0[1] + (p1[1] - p0[1]) * f;
    r->position[2] = p0[2] + (p1[2] - p0[2]) * f;
    r->distance = tEnter;
    VincTMRotate(tm, n, r->normal);
    r->model = model;
    return 1;
}

// Mac 0x4ba87b (name inferred)
void MEAPI VincConvexMeshRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincConvexMeshGetTypeId(), VincConvexMeshLineSegment);
}

// ---- convex vs plane ----

// Mac 0x4baf7a, Win 0x7b7090 (name inferred)
int MEAPI VincConvexMeshPlaneIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tmC = VincModelGetTransformPtr(pair->model1);
    MeReal* tmP = VincModelGetTransformPtr(pair->model2);
    CnvHull* hull = ((VincConvexGeometry*)VincModelGetGeometry(pair->model1))->hull;
    // the plane (normal -z of the plane) in the convex frame: plane[0..2] . x = plane[3]
    MeReal plane[4];
    MeReal nz = -tmP[10], ny = -tmP[9], nx = -tmP[8];
    plane[0] = nx * tmC[0] + ny * tmC[1] + nz * tmC[2];
    plane[1] = nx * tmC[4] + ny * tmC[5] + nz * tmC[6];
    plane[2] = nx * tmC[8] + ny * tmC[9] + nz * tmC[10];
    MeReal dz = tmP[14] - tmC[14], dy = tmP[13] - tmC[13], dx = tmP[12] - tmC[12];
    plane[3] = (dx * tmC[0] + dy * tmC[1] + dz * tmC[2]) * plane[0]
               + (dx * tmC[4] + dy * tmC[5] + dz * tmC[6]) * plane[1]
               + plane[2] * (dx * tmC[8] + dy * tmC[9] + dz * tmC[10]);
    static MeReal points[500][3];      // Mac: stack (1500 doubles)
    // NOTE: only position / normal / separation are written; VincContactSimplify reads the
    // uninitialised dims / elements (stale stack in the original: the kept contacts may differ)
    VincContact contacts[500];         // Mac: stack (38000 bytes)
    MeReal* buf2000[500];              // cnvPlaneSupport's points / dots / count / normal (unused here)
    MeReal buf4000[500];
    int out16;
    MeReal out24[3];
    int count = 0;
    int maxCount = 500, flag = 0;
    MeReal depth;
    bool haveSection = false;
    if (hull->faceCount < 1) {
        depth = cnvPlaneSupport(buf2000, buf4000, &maxCount, plane, &out16, hull, 0.001, out24, &flag);
        depth = (plane[3] - depth) + (MeReal)0.001f;
        result->touch = 0;
        result->contactGroupCount = 0;
        result->contactCount = 0;
    } else {
        // the polygon where the plane cuts the hull's edges.
        // NOTE: no bound on the 500 points (as in the original)
        for (int f = 0; f < hull->faceCount; ++f) {
            CnvFace* face = &hull->faces[f];
            for (int k = 0; k < face->count;) {
                CnvEdge* e = &face->edges[k];
                ++k;
                int ok = SegmentPlane(points[count], plane, &plane[3], e->vertex->pos,
                                      face->edges[k % face->count].vertex->pos);
                count = count + 1 - (ok == 0);
            }
        }
        depth = cnvPlaneSupport(buf2000, buf4000, &maxCount, plane, &out16, hull, 0.001, out24, &flag);
        depth = (plane[3] - depth) + (MeReal)0.001f;
        result->touch = 0;
        result->contactGroupCount = 0;
        result->contactCount = 0;
        haveSection = count != 0;
    }
    if (!haveSection) {
        if (0.0 < depth)
            return 0;
        count = 0;
        for (int i = 0; i < hull->vertexCount; ++i, ++count) {
            points[count][0] = hull->vertices[i].pos[0];
            points[count][1] = hull->vertices[i].pos[1];
            points[count][2] = hull->vertices[i].pos[2];
        }
    }
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    int maxc = request->contactMaxCount;
    if (result->contactMaxCount <= request->contactMaxCount)
        maxc = result->contactMaxCount;
    if (maxc != 0) {
        if (0 < count && result->contactCount < 500) {
            int i = 0;
            VincContact* c = contacts;
            do {
                c->separation = depth;
                c->normal[0] = tmP[8];
                c->normal[1] = tmP[9];
                c->normal[2] = tmP[10];
                VincTMTransform(tmC, points[i], c->position);
                ++result->contactCount;
                ++c;
                ++i;
            } while (i != count && result->contactCount < 500);
        }
        result->normal[0] = tmP[8];
        result->normal[1] = tmP[9];
        result->normal[2] = tmP[10];
        result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
        result->touch = 1;
    }
    return 1;
}

// Mac 0x4baf0f (name inferred)
int MEAPI VincConvexMeshPlaneRegisterInteraction(void)
{
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = VincConvexMeshPlaneIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;           // NOTE: left uninitialised by the original
    i.distanceBoundedFn = 0;
    MeI16 plane = VincPlaneGetTypeId();
    VincGeometryPairRegisterInteractions(VincConvexGetTypeId(), plane, &i, 0);
    return 1;
}

// ---- convex vs one triangle ----

// Mac 0x4bc585 (name inferred): the triangle becomes the shared one-triangle hull; contacts with
// normals from the triangle towards the convex; returns hit (and *count unless maxCount == 0)
int MEAPI VincConvexTriangleContacts(VincGeometryID convex, const MeReal* relTM, VincTriangleRef* tri,
                                     int maxCount, VincContact* contacts, int* count)
{
    CnvHull* h = VincConvexTriangleHull;
    MeReal identity[16] = {1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 0.0, 1.0};
    VincConvexDesc a, b;
    b.convex = ((VincConvexGeometry*)convex)->hull;
    b.tm = (MeReal*)relTM;
    b.margin = ((VincConvexGeometry*)convex)->margin;
    for (int k = 0; k < 3; ++k) {
        h->vertices[k].pos[0] = tri->vertices[k][0];
        h->vertices[k].pos[1] = tri->vertices[k][1];
        h->vertices[k].pos[2] = tri->vertices[k][2];
    }
    CnvEdge* e = h->faces[0].edges;
    for (int k = 0; k < 3; ++k) {
        e[k].pos[0] = h->vertices[k].pos[0];
        e[k].pos[1] = h->vertices[k].pos[1];
        e[k].pos[2] = h->vertices[k].pos[2];
        e[k].normal[0] = tri->normal[0];
        e[k].normal[1] = tri->normal[1];
        e[k].normal[2] = tri->normal[2];
    }
    h->faces[0].normal[0] = tri->normal[0];
    h->faces[0].normal[1] = tri->normal[1];
    h->faces[0].normal[2] = tri->normal[2];
    cnvUpdate(h);
    a.convex = VincConvexTriangleHull;
    a.tm = identity;
    a.margin = 0.0;
    VincIntersectResult r;
    memset(&r, 0, sizeof(r));
    r.contacts = contacts;
    r.contactMaxCount = maxCount;
    r.contactCount = 0;
    CnvPairCache pairData;             // Mac: 0x2c bytes of stack (the non-cylinder part), zeroed
    memset(&pairData, 0, sizeof(pairData));
    MeReal axis[3] = {0.0, 0.0, 0.0};
    MeReal pad = 0.0;
    int hit = VincConvexConvexIntersect(&r, &a, &b, 0, 0, &pad, &pairData, axis, maxCount);
    if (hit == 0)
        return 0;
    if (maxCount == 0)
        return 1;
    for (int i = 0; i < r.contactCount; ++i) {
        contacts[i].dims = 0;
        contacts[i].normal[0] = -contacts[i].normal[0];
        contacts[i].normal[1] = -contacts[i].normal[1];
        contacts[i].normal[2] = -contacts[i].normal[2];
    }
    *count = r.contactCount;
    return 0 < r.contactCount;
}

// Mac 0x4bc92c (name inferred)
int MEAPI VincConvexModelTriangleContacts(VincModelID model, const MeReal* relTM, VincTriangleRef* tri,
                                          int maxCount, VincContact* contacts, int* count)
{
    return VincConvexTriangleContacts(VincModelGetGeometry(model), relTM, tri, maxCount, contacts, count);
}

// ---- convex vs height field ("ConvexMeshTriList") ----

// Mac 0x4bb6c4, Win 0x763600 (name inferred)
int MEAPI VincConvexMeshHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tmC = VincModelGetTransformPtr(pair->model1);
    MeReal* tmH = VincModelGetTransformPtr(pair->model2);
    VincGeometryID convex = VincModelGetGeometry(pair->model1);
    VincGeometryID hf = VincModelGetGeometry(pair->model2);
    MeReal rel[16];
    RelativeTM(tmC, tmH, rel);
    MeReal xy[4];
    VincConvexMeshGetXYAABB(convex, (MeReal(*)[4])rel, xy);
    MeVector3 center, c;
    MeReal radius;
    VincConvexMeshGetBSphere(convex, center, &radius);
    VincTMTransform(rel, center, c);
    int maxc = VincModelPairGetRequestPtr(pair)->contactMaxCount;
    result->contactGroupCount = 0;
    result->contactCount = 0;
    int x0, x1, y0, y1;
    if (VincHeightFieldGetCellRange(hf, xy, &x0, &x1, &y0, &y1) == 0)
        return 0;
    MeVector3 A, B, C;
    VincTriangleRef tri;
    tri.index = -1;
    tri.edgeData0[0] = tri.edgeData0[1] = tri.edgeData0[2] = -2.0;
    tri.edgeData1[0] = tri.edgeData1[1] = tri.edgeData1[2] = 0.0;
    tri.vertices[0] = A;
    tri.vertices[1] = B;
    tri.vertices[2] = C;
    MeReal sum[3] = {0.0, 0.0, 0.0};
    if (!(y0 < y1))
        return 0;
    static VincContact contacts[400];  // Mac: stack (30400 bytes)
    int n = 0;
    VincContact* cursor = contacts;
    int xCount = ((const VincHeightField*)hf)->xCount;   // x vertex count (+0x08 on 32-bit; PORT64: the field)
    int x = x0, y = y0;
    do {
        int remaining;
        while (x < x1 && (remaining = 400 - n, 0 < remaining)) {
            VincHeightFieldGetVertex(hf, A, x, y);
            int x2 = x + 1;
            VincHeightFieldGetVertex(hf, B, x2, y);
            int y2 = y + 1;
            VincHeightFieldGetVertex(hf, C, x, y2);
            TriangleSetup(&tri);
            tri.index = (x + (xCount - 1) * y) * 2;
            MeReal ex = c[0] - B[0], ey = c[1] - B[1], ez = c[2] - B[2];
            MeReal d = ex * tri.normal[0] + ey * tri.normal[1] + ez * tri.normal[2];
            int cnt;
            if (-radius < d && d < radius
                && VincConvexModelTriangleContacts(pair->model1, rel, &tri, remaining, cursor, &cnt) != 0) {
                if (maxc == 0)
                    return 1;
                n += cnt;
                remaining -= cnt;
                for (int k = 0; k < cnt; ++k, ++cursor) {
                    cursor->element2 = tri.index;
                    sum[0] = sum[0] + cursor->normal[0];
                    sum[1] = sum[1] + cursor->normal[1];
                    sum[2] = sum[2] + cursor->normal[2];
                }
                if (remaining < 1)
                    break;
            }
            VincHeightFieldGetVertex(hf, A, x2, y);
            VincHeightFieldGetVertex(hf, B, x2, y2);
            VincHeightFieldGetVertex(hf, C, x, y2);
            tri.index = (x + (xCount - 1) * y) * 2 + 1;
            TriangleSetup(&tri);
            // NOTE: the distance uses the first triangle's (x + 1, y) vertex, which is A here
            d = ex * tri.normal[0] + ey * tri.normal[1] + ez * tri.normal[2];
            x = x2;
            if (-radius < d && d < radius
                && VincConvexModelTriangleContacts(pair->model1, rel, &tri, remaining, cursor, &cnt) != 0) {
                if (maxc == 0)
                    return 1;
                n += cnt;
                for (int k = 0; k < cnt; ++k, ++cursor) {
                    sum[0] = sum[0] + cursor->normal[0];
                    sum[1] = sum[1] + cursor->normal[1];
                    sum[2] = sum[2] + cursor->normal[2];
                    cursor->element2 = tri.index;
                }
            }
        }
        ++y;
        x = x0;
    } while (y < y1);
    if (n < 1)
        return 0;
    VincContactSimplifyNamed(sum, contacts, n, result, "ConvexMeshTriList");
    for (int i = 0; i < result->contactCount; ++i) {
        VincContact* k = &result->contacts[i];
        MeVector3 p;
        VincTMTransform(tmH, k->position, p);
        k->position[0] = p[0];
        k->position[1] = p[1];
        k->position[2] = p[2];
        VincTMRotate(tmH, k->normal, p);
        k->normal[0] = p[0];
        k->normal[1] = p[1];
        k->normal[2] = p[2];
    }
    MeVector3 nn;
    VincTMRotate(tmH, result->normal, nn);
    result->normal[0] = nn[0];
    result->normal[1] = nn[1];
    result->normal[2] = nn[2];
    return 1;
}

namespace {
// Mac 0x4bb691 (name inferred)
void MEAPI ConvexTriangleHullTerm(void)
{
    if (VincConvexTriangleHull != 0)
        cnvDestroyTriangle(VincConvexTriangleHull);
    VincConvexTriangleHull = 0;
}

// the one-triangle hull (Mac 0x644a40: (0,0,0) (0,0,1) (0,1,0))
void CreateTriangleHull(void)
{
    MeReal v[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 1.0, 0.0, 1.0, 0.0};
    MeReal normal[3] = {1.0, 0.0, 0.0};
    if (VincConvexTriangleHull == 0)
        VincConvexTriangleHull = cnvCreateTriangle(v, v + 3, v + 6, normal);
}
} // namespace

// Mac 0x4bb5a5, Win 0x763d10 (name inferred)
int MEAPI VincConvexMeshHeightFieldRegisterInteraction(void)
{
    CreateTriangleHull();
    VincRegisterTerminateAction(ConvexTriangleHullTerm);
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = VincConvexMeshHeightFieldIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;           // NOTE: left uninitialised by the original
    i.distanceBoundedFn = 0;
    MeI16 hf = VincHeightFieldGetTypeId();
    VincGeometryPairRegisterInteractions(VincConvexGetTypeId(), hf, &i, 0);
    return 1;
}

// ---- convex vs triangle list ("ConvexTriList") ----

// Mac 0x4bc96d, Win 0x766ed0 (name inferred)
int MEAPI VincConvexMeshTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tmC = VincModelGetTransformPtr(pair->model1);
    MeReal* tmT = VincModelGetTransformPtr(pair->model2);
    VincGeometryID convex = VincModelGetGeometry(pair->model1);
    VincTriangleList* tl = (VincTriangleList*)VincModelGetGeometry(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal rel[16];
    RelativeTM(tmC, tmT, rel);
    MeReal xy[4];
    VincConvexMeshGetXYAABB(convex, (MeReal(*)[4])rel, xy);   // computed, not used
    MeVector3 center, c;
    MeReal radius;
    VincConvexMeshGetBSphere(convex, center, &radius);
    VincTMTransform(rel, center, c);
    tl->triangleCount = 0;
    result->contactGroupCount = 0;
    result->contactCount = 0;
    MeVector3 A, B, C;
    VincTriangleRef tri;
    tri.index = -1;
    tri.edgeData0[0] = tri.edgeData0[1] = tri.edgeData0[2] = -2.0;
    tri.edgeData1[0] = tri.edgeData1[1] = tri.edgeData1[2] = 0.0;
    tri.vertices[0] = A;
    tri.vertices[1] = B;
    tri.vertices[2] = C;
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    MeReal rel2[16];
    RelativeTM(VincModelGetTransformPtr(pair->model1), tmT, rel2);
    MeVector3 mn, mx;
    MeReal aabb[6];
    VincGeometryUpdateAABB(convex, (MeReal(*)[4])rel2, mn, mx, aabb);
    tl->generator(pair, mn, mx, 0);
    int triCount = tl->triangleCount;
    MeReal sum[3] = {0.0, 0.0, 0.0};
    VincRequest* request = VincModelPairGetRequestPtr(pair);
    MeReal minDist = 0.0;
    if (request->contactMaxCount == 0)
        minDist = -radius;
    if (triCount <= 0)
        return 0;
    // one extra slot: the original sums the normal of the slot after each new contact (below)
    static VincContact contacts[401];  // Mac: stack (30400 bytes)
    memset(contacts, 0, sizeof(contacts));
    int n = 0;
    VincContact* cursor = contacts;
    int i = 0;
    do {
        VincTriangleListTri* t = (VincTriangleListTri*)((char*)tl->triangles + i * (int)sizeof(VincTriangleListTri));   // 0x2c on 32-bit (PORT64)
        MeReal* v0 = t->vertices[0];
        MeReal* nrm = t->normal;
        MeReal d = (c[0] - v0[0]) * nrm[0] + (c[1] - v0[1]) * nrm[1] + (c[2] - v0[2]) * nrm[2];
        if (minDist <= d && d <= radius) {
            for (int k = 0; k < 3; ++k) {
                tri.vertices[k][0] = t->vertices[k][0];
                tri.vertices[k][1] = t->vertices[k][1];
                tri.vertices[k][2] = t->vertices[k][2];
            }
            tri.normal[0] = nrm[0];
            tri.normal[1] = nrm[1];
            tri.normal[2] = nrm[2];
            for (int k = 0; k < 3; ++k) {
                MeReal* p = tri.vertices[k];
                MeReal* q = tri.vertices[(k + 1) % 3];
                tri.edges[k][2] = q[2] - p[2];
                tri.edges[k][1] = q[1] - p[1];
                tri.edges[k][0] = q[0] - p[0];
            }
            int cap = 400 - n;
            if (request->contactMaxCount < cap)
                cap = request->contactMaxCount;
            int cnt;
            if (VincConvexModelTriangleContacts(pair->model1, rel, &tri, cap, cursor, &cnt) != 0) {
                if (request->contactMaxCount == 0)
                    return 1;
                n += cnt;
                for (int k = 0; k < cnt; ++k) {
                    cursor->element1 = -1;
                    cursor->element2 = t->userId;
                    ++cursor;
                    // NOTE: original quirk: sums the normal of the slot AFTER the new contact
                    // (not written yet: stale stack data in the original, zero here)
                    sum[0] = sum[0] + cursor->normal[0];
                    sum[1] = sum[1] + cursor->normal[1];
                    sum[2] = sum[2] + cursor->normal[2];
                }
            }
        }
        ++i;
    } while (i < triCount && n < 400);
    if (n < 1)
        return 0;
    VincContactSimplifyNamed(sum, contacts, n, result, "ConvexTriList");
    int k = 0;
    for (; k < result->contactCount; ++k) {
        VincContact* q = &result->contacts[k];
        MeVector3 p;
        VincTMTransform(tmT, q->position, p);
        q->position[0] = p[0];
        q->position[1] = p[1];
        q->position[2] = p[2];
        VincTMRotate(tmT, q->normal, p);
        q->normal[0] = p[0];
        q->normal[1] = p[1];
        q->normal[2] = p[2];
    }
    // NOTE: original bug: rotates the normal of contacts[contactCount] (one past the last contact)
    // instead of result->normal; guarded against writing past the caller's array.
    if (k < result->contactMaxCount) {
        VincContact* q = &result->contacts[k];
        MeVector3 p;
        VincTMRotate(tmT, q->normal, p);
        q->normal[0] = p[0];
        q->normal[1] = p[1];
        q->normal[2] = p[2];
    }
    return 1;
}

namespace {
// Mac 0x4bc556 (name inferred)
void MEAPI ConvexTriangleHullTerm2(void)
{
    if (VincConvexTriangleHull != 0)
        cnvDestroyTriangle(VincConvexTriangleHull);
    VincConvexTriangleHull = 0;
}
} // namespace

// Mac 0x4bc46d (name inferred)
int MEAPI VincConvexMeshTriangleListRegisterInteraction(void)
{
    CreateTriangleHull();   // Mac 0x644ac0: the same triangle
    VincRegisterTerminateAction(ConvexTriangleHullTerm2);
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = VincConvexMeshTriangleListIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    MeI16 tl = VincTriangleListGetTypeId();
    VincGeometryPairRegisterInteractions(VincConvexGetTypeId(), tl, &i, 0);
    return 1;
}

// ---- per-pair data (hello / goodbye of the convex pair tests via thunks) ----

// Mac 0x4bd6c2 (name inferred)
int MEAPI VincConvexPairDataRelease(VincModelID /*m1*/, VincModelID /*m2*/, void* data)
{
    if (data != 0) {
        PairDataPool* pool = s_pairDataPool;
        *pool->top = data;
        ++pool->top;
    }
    return 1;
}

// Mac 0x4bd6ee (name inferred)
int MEAPI VincConvexPairDataAcquire(VincModelID /*m1*/, VincModelID /*m2*/, void** data)
{
    PairDataPool* pool = s_pairDataPool;
    void* obj = 0;
    if (pool->top == pool->begin) {
        int n = pool->growBy;
        if (0 < n) {
            char* objs = (char*)MeMemoryAPI.create(n * kPairDataSize);
            if (objs != 0) {
                PairDataBlockList* b = pool->blocks;
                if (99 < b->count)
                    pool->blocks = NewBlockList(b);
                b = pool->blocks;
                b->entries[b->count].objects = objs;
                b->entries[b->count].count = n;
                ++b->count;
                int old = pool->capacity;
                pool->capacity = n + old;
                void** arr = (void**)MeMemoryAPI.create((n + old) * sizeof(void*));
                for (void** p = arr + pool->capacity; arr < p;)
                    *--p = 0;
                if (arr != 0) {
                    FreeArray(pool->begin, pool->capacity - n);
                    pool->begin = arr;
                    pool->top = arr;
                    for (char* p = objs; p < objs + n * kPairDataSize; p += kPairDataSize)
                        *pool->top++ = p;
                    obj = *--pool->top;
                }
            }
        }
    } else {
        obj = *--pool->top;
    }
    // NOTE: the original stores through the NULL object when the allocation failed; guarded
    if (obj != 0)
        *(int*)obj = 0;
    *data = obj;
    return 1;
}

// ---- convex vs primitives ----

// Mac 0x4bd9f3 (name inferred): the box hull scaled to radii (the unit hull is made once)
CnvHull* MEAPI VincConvexBoxHullUpdate(CnvHull* hull, const MeReal* radii)
{
    static MeReal s_unitBox[8][3];     // Mac 0x780280
    if (hull == 0) {
        MeReal v[8][3] = {{1.0, 1.0, 1.0},  {-1.0, 1.0, 1.0},  {1.0, -1.0, 1.0},  {-1.0, -1.0, 1.0},
                          {1.0, 1.0, -1.0}, {-1.0, 1.0, -1.0}, {1.0, -1.0, -1.0}, {-1.0, -1.0, -1.0}};
        hull = (CnvHull*)cnvComputeHull(8, &v[0][0], 1, 0);
        for (int k = 0; k < 8; ++k) {
            s_unitBox[k][0] = hull->vertices[k].pos[0];
            s_unitBox[k][1] = hull->vertices[k].pos[1];
            s_unitBox[k][2] = hull->vertices[k].pos[2];
        }
    }
    for (int k = 0; k < 8; ++k) {
        hull->vertices[k].pos[0] = radii[0] * s_unitBox[k][0];
        hull->vertices[k].pos[1] = radii[1] * s_unitBox[k][1];
        hull->vertices[k].pos[2] = radii[2] * s_unitBox[k][2];
    }
    return hull;
}

// Mac 0x4bdb96, Win 0x764150 (name inferred): convex vs convex (registered by 0x4b3945 through the
// non-lazy pointer Mac 0x70273c)
int MEAPI VincConvexConvexPairIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincConvexGeometry* g1 = (VincConvexGeometry*)VincModelGetGeometry(pair->model1);
    VincConvexGeometry* g2 = (VincConvexGeometry*)VincModelGetGeometry(pair->model2);
    VincConvexDesc a, b;
    a.margin = g1->margin;
    b.margin = g2->margin;
    a.convex = g1->hull;
    b.convex = g2->hull;
    a.tm = VincModelGetTransformPtr(pair->model1);
    b.tm = VincModelGetTransformPtr(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    return VincConvexConvexIntersectCached(result, &a, &b, &pad, (CnvPairCache*)pair->pairData,
                                           pair->request->contactMaxCount);
}

// Mac 0x4bdc61, Win 0x764780 (name inferred): the cylinder is passed as the "convex" itself
int MEAPI VincCylinderConvexIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincConvexDesc a, b;
    a.convex = VincModelGetGeometry(pair->model1);
    a.margin = 0.0;
    VincConvexGeometry* g2 = (VincConvexGeometry*)VincModelGetGeometry(pair->model2);
    b.margin = g2->margin;
    b.convex = g2->hull;
    a.tm = VincModelGetTransformPtr(pair->model1);
    b.tm = VincModelGetTransformPtr(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    CnvPairCache* pd = (CnvPairCache*)pair->pairData;   // cylA: Mac +0x2c / Win +0x30, axis +0xf4 / +0x100
    return VincConvexConvexIntersect(result, &a, &b, &pd->cylA, 0, &pad, pd, pd->axis,
                                     pair->request->contactMaxCount);
}

// Mac 0x4bdd44, Win 0x7645b0 (name inferred)
int MEAPI VincBoxConvexIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincConvexBoxHull = VincConvexBoxHullUpdate(VincConvexBoxHull,
                                                VincBoxGetRadii(VincModelGetGeometry(pair->model1)));
    VincConvexDesc a, b;
    a.convex = VincConvexBoxHull;
    a.margin = 0.0;
    VincConvexGeometry* g2 = (VincConvexGeometry*)VincModelGetGeometry(pair->model2);
    b.margin = g2->margin;
    b.convex = g2->hull;
    a.tm = VincModelGetTransformPtr(pair->model1);
    b.tm = VincModelGetTransformPtr(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    return VincConvexConvexIntersectCached(result, &a, &b, &pad, (CnvPairCache*)pair->pairData,
                                           pair->request->contactMaxCount);
}

// Mac 0x4bde34, Win 0x764330 (name inferred): the sphere is a point with margin = radius
int MEAPI VincSphereConvexIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal radius = VincSphereGetRadius(VincModelGetGeometry(pair->model1));
    VincConvexDesc a, b;
    a.convex = cnvGetPointConvex(0);
    a.margin = radius;
    VincConvexGeometry* g2 = (VincConvexGeometry*)VincModelGetGeometry(pair->model2);
    b.margin = g2->margin;
    b.convex = g2->hull;
    a.tm = VincModelGetTransformPtr(pair->model1);
    b.tm = VincModelGetTransformPtr(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    return VincConvexConvexIntersectCached(result, &a, &b, &pad, (CnvPairCache*)pair->pairData,
                                           pair->request->contactMaxCount);
}

// Mac 0x4bdf0f, Win 0x7643e0 (name inferred): the capsule is a segment with margin = radius; element1 of each
// contact: -1 at the side, else 1 / 0 for the end the contact is near
int MEAPI VincCapsuleConvexIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    static CnvHull* s_segment = cnvCreateSegment(1.0);   // Mac 0x780268 (guarded static)
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    MeReal radius = VincCapsuleGetRadius(g1);
    MeReal half = VincCapsuleGetHalfHeight(g1);
    VincConvexDesc a, b;
    a.convex = cnvSetSegment(s_segment, half);
    a.margin = radius;
    VincConvexGeometry* g2 = (VincConvexGeometry*)VincModelGetGeometry(pair->model2);
    b.margin = g2->margin;
    b.convex = g2->hull;
    MeReal* tm = VincModelGetTransformPtr(pair->model1);
    a.tm = tm;
    b.tm = VincModelGetTransformPtr(pair->model2);
    result->contactMaxCount = pair->request->contactMaxCount;
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    if (VincConvexConvexIntersectCached(result, &a, &b, &pad, (CnvPairCache*)pair->pairData,
                                        pair->request->contactMaxCount) == 0)
        return 0;
    MeReal az = half * tm[10], ay = half * tm[9], ax = half * tm[8];
    MeReal r = radius + pad;
    r = r * r;
    for (int i = 0; i < result->contactCount; ++i) {
        VincContact* c = &result->contacts[i];
        MeReal dz = (tm[14] - az) - c->position[2];
        MeReal dy = (tm[13] - ay) - c->position[1];
        MeReal dx = (tm[12] - ax) - c->position[0];
        if (r < dx * dx + dy * dy + dz * dz) {
            dz = (az + tm[14]) - c->position[2];
            dy = (ay + tm[13]) - c->position[1];
            dx = (ax + tm[12]) - c->position[0];
            c->element1 = dx * dx + dy * dy + dz * dz <= r;
        } else {
            c->element1 = -1;
        }
    }
    return 1;
}

namespace {
int RegisterWithConvex(MeI16 type, VincIntersectFn fn)
{
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = fn;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;           // NOTE: left uninitialised by the original
    i.distanceBoundedFn = 0;
    int convex = VincConvexGetTypeId();
    VincGeometryPairRegisterInteractions(type, convex, &i, 0);
    return 1;
}
} // namespace

// Mac 0x4bd847 (name inferred)
int MEAPI VincBoxConvexRegisterInteraction(void)
{
    return RegisterWithConvex(VincBoxGetTypeId(), VincBoxConvexIntersect);
}

// Mac 0x4bd8b2 (name inferred)
int MEAPI VincCapsuleConvexRegisterInteraction(void)
{
    return RegisterWithConvex(VincCapsuleGetTypeId(), VincCapsuleConvexIntersect);
}

// Mac 0x4bd91d (name inferred)
int MEAPI VincCylinderConvexRegisterInteraction(void)
{
    return RegisterWithConvex(VincCylinderGetTypeId(), VincCylinderConvexIntersect);
}

// Mac 0x4bd988 (name inferred)
int MEAPI VincSphereConvexRegisterInteraction(void)
{
    return RegisterWithConvex(VincSphereGetTypeId(), VincSphereConvexIntersect);
}

// ---- segment distance helpers (called by the convex hull code: Mac 0x4b3db2, 0x4b45ab) ----

// Mac 0x4be1be (name inferred): distance of p to segment a-b; closest point in out
MeReal MEAPI VincPointSegmentDistance(const MeReal* p, const MeReal* a, const MeReal* b, MeReal* out)
{
    MeReal d[3] = {b[0] - a[0], b[1] - a[1], b[2] - a[2]};
    MeReal w[3] = {p[0] - a[0], p[1] - a[1], p[2] - a[2]};
    MeReal t = Dot0(w, d) / Dot0(d, d);
    if (0.0 <= t && t <= 1.0) {
        for (int k = 0; k < 3; ++k)
            out[k] = (b[k] - a[k]) * t + a[k];
        MeReal e[3] = {w[0] - t * d[0], w[1] - t * d[1], w[2] - t * d[2]};
        return sqrt(Dot0(e, e));
    }
    if (0.0 <= t || t != t) {
        out[0] = b[0];
        out[1] = b[1];
        out[2] = b[2];
        MeReal e[3] = {p[0] - b[0], p[1] - b[1], p[2] - b[2]};
        return sqrt(Dot0(e, e));
    }
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
    return sqrt(Dot0(w, w));
}

// Mac 0x4be470 (name inferred): parameters s (on a0-a1) and t (on b0-b1) of the closest points of
// the two lines; nearly parallel (|.|^2 det < 0.1): s = 0.5, *parallel = 1. Returns the distance.
MeReal MEAPI VincSegmentSegmentParameters(const MeReal* a0, const MeReal* a1, const MeReal* b0,
                                          const MeReal* b1, MeReal* s, MeReal* t, char* parallel)
{
    MeReal d1[3], d2[3], r[3];
    for (int k = 0; k < 3; ++k) {
        d1[k] = a1[k] - a0[k];
        d2[k] = b1[k] - b0[k];
        r[k] = b0[k] - a0[k];
    }
    MeReal d1d2 = 0.0, d1r = 0.0, d2r = 0.0, d1d1 = 0.0, d2d2 = 0.0;
    for (int k = 0; k < 3; ++k) {
        d1d2 = d1d2 + d1[k] * d2[k];
        d1r = d1r + d1[k] * r[k];
        d2r = d2r + d2[k] * r[k];
        d1d1 = d1d1 + d1[k] * d1[k];
        d2d2 = d2[k] * d2[k] + d2d2;
    }
    MeReal det = d2d2 * d1d1 - d1d2 * d1d2;
    if (0.1 <= det || det != det) {
        *s = (d2d2 * d1r - d1d2 * d2r) / det;
        *parallel = 0;
    } else {
        *s = 0.5;
        *parallel = 1;
    }
    MeReal u = (d1d2 * *s - d2r) / d2d2;
    *t = u;
    MeReal sum = 0.0;
    for (int k = 0; k < 3; ++k) {
        MeReal e = ((b0[k] - a0[k]) + (b1[k] - b0[k]) * u) - (a1[k] - a0[k]) * *s;
        sum = sum + e * e;
    }
    return sqrt(sum);
}

// Mac 0x4be710 (name inferred): closest points pa on a0-a1, pb on b0-b1; *count = 1 point pair,
// 2 for an overlapping parallel pair (then pa / pb bound the overlap), 0 for parallel without
// overlap. Returns the distance.
MeReal MEAPI VincSegmentSegmentClosestPoints(const MeReal* a0, const MeReal* a1, const MeReal* b0,
                                             const MeReal* b1, MeReal* pa, MeReal* pb, char* parallel,
                                             int* count)
{
    MeReal s, t;
    *count = 1;
    MeReal d = VincSegmentSegmentParameters(a0, a1, b0, b1, &s, &t, parallel);
    if (*parallel == 0) {
        *count = 1;
        if (0.0 <= s && s <= 1.0) {
            if (t < 0.0 || 1.0 < t) {
                if (0.0 <= t || t != t) {
                    d = VincPointSegmentDistance(b1, a0, a1, pa);
                    pb[0] = b1[0];
                    pb[1] = b1[1];
                    pb[2] = b1[2];
                } else {
                    d = VincPointSegmentDistance(b0, a0, a1, pa);
                    pb[0] = b0[0];
                    pb[1] = b0[1];
                    pb[2] = b0[2];
                }
            } else {
                for (int k = 0; k < 3; ++k) {
                    pa[k] = (a1[k] - a0[k]) * s + a0[k];
                    pb[k] = (b1[k] - b0[k]) * t + b0[k];
                }
            }
            return d;
        }
        const MeReal* end = (0.0 <= s || s != s) ? a1 : a0;   // s > 1: a1, s < 0: a0
        d = VincPointSegmentDistance(end, b0, b1, pb);
        if (0.0 <= t && t <= 1.0) {
            pa[0] = end[0];
            pa[1] = end[1];
            pa[2] = end[2];
        }
        MeReal q[3];
        const MeReal* other = (0.0 <= t) ? b1 : b0;
        MeReal d2 = VincPointSegmentDistance(other, a0, a1, q);
        if (d <= d2) {
            pa[0] = end[0];
            pa[1] = end[1];
            pa[2] = end[2];
        } else {
            pa[0] = q[0];
            pa[1] = q[1];
            pa[2] = q[2];
            pb[0] = other[0];
            pb[1] = other[1];
            pb[2] = other[2];
            d = d2;
        }
        return d;
    }
    *count = 2;
    MeReal dz = a1[2] - a0[2], dy = a1[1] - a0[1], dx = a1[0] - a0[0];
    MeReal base = a0[0] * dx + a0[1] * dy + a0[2] * dz;
    MeReal u0 = dx * b0[0] + dy * b0[1] + dz * b0[2];
    MeReal u1 = dx * b1[0] + dy * b1[1] + dz * b1[2];
    MeReal len = (dx * a1[0] + dy * a1[1] + dz * a1[2]) - base;
    MeReal lo = 0.0, hi = 1.0;
    if (1e-05 <= fabs(len) || len != len) {
        u0 = (u0 - base) / len;
        u1 = (u1 - base) / len;
    } else {
        hi = 0.0;
        u0 = u0 - base;
        u1 = u1 - base;
    }
    const MeReal* first = b1;
    const MeReal* second = b0;
    MeReal uf = u1, us = u0;
    if (!(u1 <= u0)) {
        first = b0;
        second = b1;
        uf = u0;
        us = u1;
    }
    if (0.0 <= uf || uf != uf) {
        pa[0] = first[0];
        pa[1] = first[1];
        pa[2] = first[2];
        lo = uf;
    } else {
        pa[0] = a0[0];
        pa[1] = a0[1];
        pa[2] = a0[2];
    }
    if (us <= hi) {
        pb[0] = second[0];
        pb[1] = second[1];
        pb[2] = second[2];
        hi = us;
    } else {
        pb[0] = a1[0];
        pb[1] = a1[1];
        pb[2] = a1[2];
    }
    if (lo <= hi) {
        if (hi < lo + 1e-05)
            *count = 1;
    } else {
        *count = 0;
    }
    return d;
}

// ---- registration ----

// Mac 0x4bf077 (name inferred): the pair data pool, then the convex interactions
int MEAPI VincConvexMeshRegisterInteractions(void)
{
    if (s_pairDataPool == 0) {
        PairDataPool* pool = (PairDataPool*)MeMemoryAPI.create(sizeof(PairDataPool));
        s_pairDataPool = pool;
        if (pool != 0) {
            pool->growBy = 100;
            pool->capacity = 0;
            pool->begin = 0;
            pool->top = 0;
            pool->blocks = NewBlockList(0);
            pool->growBy = 100;
        }
        if (s_pairDataPool == 0)
            return 0;
    }
    VincRegisterTerminateAction(ConvexInteractionsTerm);
    VincConvexMeshRegisterSelfInteraction();
    VincBoxConvexRegisterInteraction();
    VincSphereConvexRegisterInteraction();
    VincCylinderConvexRegisterInteraction();
    VincConvexMeshPlaneRegisterInteraction();
    VincConvexMeshRegisterLineSegment();
    return 1;
}

// Mac 0x4bf149, Win 0x75c6d0 (name inferred)
int MEAPI VincConvexGetTypeId(void)
{
    return VincConvexMeshGetTypeId();
}

// Mac 0x4bf157, Win 0x75c6c0 (name inferred)
void MEAPI VincConvexRegisterType(void)
{
    VincConvexMeshRegisterType();
}

// ---- hull accessors (used by Vx, Mac 0x4423c0 ...) ----

// Mac 0x4bf164 (name inferred)
void MEAPI VincConvexGetFaceNormal(VincGeometryID g, int face, MeVector3 n)
{
    CnvFace* f = &((VincConvexGeometry*)g)->hull->faces[face];
    n[0] = f->normal[0];
    n[1] = f->normal[1];
    n[2] = f->normal[2];
}

// Mac 0x4bf19b (name inferred)
MeReal* MEAPI VincConvexGetFaceVertexPtr(VincGeometryID g, int face, int k)
{
    return ((VincConvexGeometry*)g)->hull->faces[face].edges[k].pos;
}

// Mac 0x4bf1ba (name inferred)
void MEAPI VincConvexGetFaceVertex(VincGeometryID g, int face, int k, MeVector3 v)
{
    MeReal* p = ((VincConvexGeometry*)g)->hull->faces[face].edges[k].pos;
    v[0] = p[0];
    v[1] = p[1];
    v[2] = p[2];
}

// Mac 0x4bf1f8 (name inferred)
int MEAPI VincConvexGetFaceVertexCount(VincGeometryID g, int face)
{
    return ((VincConvexGeometry*)g)->hull->faces[face].count;
}

// Mac 0x4bf20f (name inferred)
int MEAPI VincConvexGetFaceCount(VincGeometryID g)
{
    return ((VincConvexGeometry*)g)->hull->faceCount;
}

} // extern "C"
