/* Vortex 5.0.0 build 800 "vortexldd41" - convex hulls ("cnv": built through Qhull 2010.1), the
 * Convex geometry (type 9, registered as "Convex") and the convex-convex core shared by every
 * Convex pair test: GJK distance with Johnson's sub-algorithm, an expanding-polytope penetration
 * depth and the contact generator (restored; src/Vinc/VincConvexMesh.cpp).
 *
 * Mac: 557b 0x4b22f2-0x4ba87b plus the cnv helpers at the end of 557a (0x4b14d8-0x4b22f2).
 * Win: 0x7aecc0-0x7b03b0 (hull), 0x7b03b0-0x7b2a60 (contacts), 0x7b5f10-0x7b6cd0 (geometry,
 * segment), 0x80b010-0x80d860 (GJK / penetration depth). Win-only (dead-stripped on the Mac):
 * VincConvexMeshCreate / CreateSegment / Init / GetType.
 *
 * Only "cnvComputeHull" and "VincConvexMeshCreateHull" appear in strings (the hull messages);
 * every other name is inferred (the MathEngine "cnv" convex library that Vinc continues).
 *
 * Layouts are natural (Mac i386: doubles 4-aligned; MSVC: 8-aligned): both columns checked.
 * Calling conventions follow the Win build: the hull functions and the geometry type functions are
 * __stdcall (MEAPI), the support / GJK / convex-convex core is cdecl.
 */
#ifndef VINC_VINCCONVEXMESH_H
#define VINC_VINCCONVEXMESH_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- the hull (cnvComputeHull) ---- */
struct CnvFace;
typedef struct CnvVertex {          /*  Mac   Win   (0x28 on both) */
    MeVector3 pos;                  /* +0x00 +0x00 */
    int faceCount;                  /* +0x18 +0x18 faces around the vertex (qhull order) */
    struct CnvFace** faces;         /* +0x1c +0x1c */
    int neighbourCount;             /* +0x20 +0x20 vertices joined by an edge, sorted around the
                                                   vertex by cnvUpdate */
    struct CnvVertex** neighbours;  /* +0x24 +0x24 */
} CnvVertex;

typedef struct CnvEdge {            /* one corner of a face (Mac 0x34, Win 0x38) */
    CnvVertex* vertex;              /* +0x00 +0x00 */
    MeVector3 pos;                  /* +0x04 +0x08 copy of the vertex position */
    MeVector3 normal;               /* +0x1c +0x20 mode 0: vertex normal (mean of its facet
                                                   normals); mode 1: the face normal; other modes:
                                                   not written */
} CnvEdge;

typedef struct CnvFace {            /* Mac 0x24, Win 0x28 */
    int count;                      /* +0x00 corners, clockwise seen along -normal */
    CnvEdge* edges;                 /* +0x04 */
    MeVector3 normal;               /* +0x08 outward unit normal (qhull) */
    int m20;                        /* +0x20 never written */
} CnvFace;

typedef struct CnvHull {            /* 0x10 on both */
    int vertexCount;                /* +0x00 */
    CnvVertex* vertices;            /* +0x04 */
    int faceCount;                  /* +0x08 0: a point or a segment (cnvGetPointConvex,
                                                   cnvCreateSegment) */
    CnvFace* faces;                 /* +0x0c */
} CnvHull;

/* ---- the Convex geometry (type 9) ---- */
typedef struct VincConvexGeometry { /* 0x48 on both (VincConvexMeshCreate) */
    VincGeometry base;              /* +0x00 type id */
    CnvHull* hull;                  /* +0x04 owned: destroyed with the geometry */
    MeReal margin;                  /* +0x08 radius added around the hull */
    MeReal radius;                  /* +0x10 bounding sphere around `center` (incl. margin) */
    CnvVertex* hint[6];             /* +0x18 support hints of the AABB directions -x +x -y +y -z +z */
    MeVector3 center;               /* +0x30 mean of the hull vertices */
} VincConvexGeometry;

/* ---- arguments of the convex-convex core ---- */
typedef struct VincConvexDesc {     /* (name inferred) Mac/Win 0x10 */
    void* convex;                   /* +0x00 CnvHull*, or a cylinder geometry (see `cylinder`
                                                   arguments below) */
    MeReal* tm;                     /* +0x04 model transform (NULL: identity) */
    MeReal margin;                  /* +0x08 */
} VincConvexDesc;

/* support points of a cylinder (the cylinder is not a hull: its support points are computed into
 * this cache; used[i] marks points the simplex refers to) */
typedef struct CnvCylinderCache {   /* Mac 0x64, Win 0x68 */
    char used[4];                   /* +0x00 */
    MeVector3 points[4];            /* +0x04 +0x08 */
} CnvCylinderCache;

/* the per-pair data of the Convex pair tests (VincModelPair::pairData; V3.2's pool) */
typedef struct CnvPairCache {       /* Mac 0x10c, Win 0x118 */
    int count;                      /* +0x00 simplex size; 0: not initialised */
    MeReal* a[4];                   /* +0x04 support points of A (hull vertex / cylA point) */
    MeReal* b[4];                   /* +0x14 support points of B */
    MeReal* hintA;                  /* +0x24 last support point of A (hill-climbing start) */
    MeReal* hintB;                  /* +0x28 */
    CnvCylinderCache cylA;          /* +0x2c +0x30 */
    CnvCylinderCache cylB;          /* +0x90 +0x98 */
    MeVector3 axis;                 /* +0xf4 +0x100 last separating direction */
} CnvPairCache;

/* ---- hulls ---- */
/* Mac 0x4b22f2, Win 0x7afb30: the convex hull of n points (x, y, z) through
 * qh_new_qhull(3, n, points, False, "qhull " + (options ? options : "Qs"), NULL, stderr).
 * normalMode 0: corner normals = vertex normals, 1: = face normal, else not written.
 * NULL (and "cnvComputeHull: qhull terminated with errorcode %d") on a qhull error. */
CnvHull* MEAPI cnvComputeHull(int n, const MeReal* points, int normalMode, const char* options);
/* Mac 0x4b16c2, Win 0x7b6c50 */
void MEAPI cnvHullDestroy(CnvHull* hull);
/* Mac 0x4b1c36, Win 0x7af810: sorts every vertex's neighbours by angle around the vertex */
void MEAPI cnvUpdate(CnvHull* hull);
/* Mac 0x4b202f, Win 0x7afdb0: a one-face hull (v0, v1, v2) with face/corner normal `normal` */
CnvHull* MEAPI cnvCreateTriangle(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* normal);
/* Mac 0x4b1913, Win 0x7b0040: frees a cnvCreateTriangle hull */
void MEAPI cnvDestroyTriangle(CnvHull* hull);
/* Mac 0x4b1897, Win 0x7afd40: the shared one-vertex hull at the origin (spheres); the argument is
 * not used (0 at both call sites) */
CnvHull* MEAPI cnvGetPointConvex(int unused);
/* Mac 0x4b1613, Win 0x7b6bd0: the segment (0, 0, h) - (0, 0, -h) as a two-vertex hull */
CnvHull* MEAPI cnvCreateSegment(MeReal halfLength);
/* Mac 0x4b1667, Win 0x7b6c10: resets a cnvCreateSegment hull to half length h; returns it */
CnvHull* MEAPI cnvSetSegment(CnvHull* segment, MeReal halfLength);
/* Mac 0x4b157c, Win 0x7b6b70 / Mac 0x4b14d8, Win 0x7b6af0 */
CnvHull* MEAPI cnvSegmentCreate(const MeReal* p0, const MeReal* p1);
CnvHull* MEAPI cnvSegmentSet(CnvHull* segment, const MeReal* p0, const MeReal* p1);

/* Mac 0x4b3292, Win 0x7b03b0: the support feature of a hull in direction dir (unit):
 * a face whose normal is within eps of dir (*isFace = 1, its corners in points[], up to *maxCount,
 * *normal = the face normal), else an edge or a vertex (points[0..1]); a hull without faces:
 * every vertex within eps of the maximum. dots: scratch (one per corner / vertex). Returns the
 * support value (the mean of the two ends for an edge / face). */
MeReal cnvPlaneSupport(MeReal** points, MeReal* dots, int* maxCount, const MeReal* dir, int* count,
                             CnvHull* hull, MeReal eps, MeReal* normal, int* isFace);

/* ---- support and GJK (Mac 0x4b75b4-0x4b888a, Win 0x80b010-0x80d860) ---- */
/* Mac 0x4b761f, Win 0x80bc30: hill climbing from `hint` (NULL: vertex 0) over the neighbour graph;
 * *dot = support value */
CnvVertex* cnvHullSupport(CnvHull* hull, CnvVertex* hint, MeReal* dot, const MeReal* dir);
/* Win-only dead code (no callers; dead-stripped on the Mac): Win 0x80bbd0 support by a linear scan,
 * Win 0x80bdd0 / 0x80bdf0 neighbour lookup / the next (step 1) or previous neighbour around a vertex */
CnvVertex* cnvHullSupportScan(CnvHull* hull, const MeReal* dir, MeReal* dot);
CnvVertex** cnvFindNeighbour(CnvVertex* v, CnvVertex* neighbour);
CnvVertex* cnvNextNeighbour(CnvHull* hull, CnvVertex* v, CnvVertex* neighbour, int step);
/* Mac 0x4b777d, Win 0x80bd60: hull support (cylinder == NULL) or a cylinder support point written
 * into the first unused slot of `cylinder` */
MeReal* cnvSupport(void* convex, CnvCylinderCache* cylinder, MeReal* hint, MeReal* dot, const MeReal* dir);
/* Mac 0x4b77da, Win 0x80bd00: hull support, or the cylinder support point written to `out` */
MeReal* cnvSupportPoint(void* convex, int isCylinder, MeReal* out, MeReal* dot, const MeReal* dir);
/* Mac 0x4b7813, Win 0x80b040: a one-point simplex along the line between the two origins */
void cnvPairCacheInit(CnvPairCache* cache, CnvCylinderCache* cylA, CnvCylinderCache* cylB, MeReal* axis,
                            const VincConvexDesc* a, const VincConvexDesc* b);
/* the world simplex points + their weights (Mac 0x4ba361 locals) */
typedef struct CnvSimplex {         /* Mac 0x28, Win 0x30 */
    MeReal* pointsA;                /* +0x00 4 x 3 (new[]) */
    MeReal* pointsB;                /* +0x04 */
    MeReal lambda[4];               /* +0x08 */
} CnvSimplex;
/* Mac 0x4b7adc, Win 0x80be50: GJK (at most 30 steps). Returns 0: full simplex (penetration),
 * 1: distance below 1e-8, 2: closer than `margins`, 3: separated, 4: no further progress / step
 * limit. closestA / closestB: the closest points, *distance2 their squared distance */
int cnvGJK(void* convexA, void* convexB, MeReal* tmA, MeReal* tmB, MeReal* closestA, MeReal* closestB,
                 CnvPairCache* cache, CnvSimplex* simplex, CnvCylinderCache* cylA, CnvCylinderCache* cylB,
                 MeReal margins, MeReal* distance2);
/* Mac 0x4b888a, Win 0x80c3b0: penetration depth by expanding the simplex into a polytope
 * (<= 100 steps); normal: the separating direction (B to A). Returns the depth (< 0) */
MeReal cnvPenetrationDepth(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache,
                                 CnvCylinderCache* cylA, CnvCylinderCache* cylB, CnvSimplex* simplex,
                                 MeReal* normal);
/* Mac 0x4ba361, Win 0x80d6e0: signed distance between the hulls (without the margins: callers
 * subtract them) and the unit normal from B to A */
MeReal cnvDistance(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache,
                         CnvCylinderCache* cylA, CnvCylinderCache* cylB, MeReal* normal);
/* Win 0xafc838 (Mac: a constant 0): 0 = cnvPenetrationDepth, 1 = one support step along the cached
 * axis (Win 0x80d510) */
extern int cnvPenetrationMethod;

/* ---- convex vs convex (Mac 0x4b6426 / 0x4b6755, Win 0x7b2730 / 0x7b28c0) ---- */
/* cylA / cylB: non-NULL when that side's `convex` is a cylinder geometry. Writes the contacts
 * into r (r->contactMaxCount) and r->normal; returns touch */
int VincConvexConvexIntersect(VincIntersectResult* r, const VincConvexDesc* a, const VincConvexDesc* b,
                                    CnvCylinderCache* cylA, CnvCylinderCache* cylB, const MeReal* padding,
                                    CnvPairCache* cache, MeReal* axis, int maxCount);
/* the same for two hulls (no cylinder); axis = cache->axis */
int VincConvexConvexIntersectCached(VincIntersectResult* r, const VincConvexDesc* a, const VincConvexDesc* b,
                                          const MeReal* padding, CnvPairCache* cache, int maxCount);
/* hello / goodbye of the Convex pair tests (Mac 0x4b3922 / 0x4b38ff, Win 0x7b13e0 / 0x7b1400):
 * acquire / release pair->pairData from V3.2's pool */
int MEAPI VincConvexPairHello(VincModelPair* pair);
int MEAPI VincConvexPairGoodbye(VincModelPair* pair);
/* Mac 0x4b3945, Win 0x7b1420: Convex-Convex (VincConvexConvexPairIntersect) */
int MEAPI VincConvexMeshRegisterSelfInteraction(void);

/* ---- the geometry type ---- */
MeI16 MEAPI VincConvexMeshGetTypeId(void);          /* Mac 0x4b67a8, Win 0x7b5f10 */
int MEAPI VincConvexMeshGetType(void);              /* Win 0x7b66f0 (int) */
void MEAPI VincConvexMeshRegisterType(void);        /* Mac 0x4b6826, Win 0x7b6a20 */
/* Win 0x7b6a60: a Convex geometry owning `hull` */
VincGeometryID MEAPI VincConvexMeshCreate(CnvHull* hull, MeReal margin);
/* Win 0x7b6aa0: a segment hull (half length) with margin `radius` (a capsule) */
VincGeometryID MEAPI VincConvexMeshCreateSegment(MeReal radius, MeReal halfLength);
/* Win 0x7b6700: centre, radius and AABB hints; returns 1 */
int MEAPI VincConvexMeshInit(VincConvexGeometry* g, CnvHull* hull, MeReal margin);
/* the registered type functions */
void MEAPI VincConvexMeshDestroy(VincGeometryID g);                                         /* Mac 0x4b67ee, Win 0x7b61b0 */
void MEAPI VincConvexMeshUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                                    MeReal* aabb6);                                         /* Mac 0x4b7578, Win 0x7b6450 */
void MEAPI VincConvexMeshGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4);           /* Mac 0x4b6d86, Win 0x7b6540 */
void MEAPI VincConvexMeshGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius);    /* Mac 0x4b67ba, Win 0x7b6480 */
void MEAPI VincConvexMeshMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4b7034, Win 0x7b64b0 */
int MEAPI VincConvexMeshGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia,
                                          MeReal* volume);                                  /* Mac 0x4b6892, Win 0x7b6760 */

#ifdef __cplusplus
}
#endif

#endif
