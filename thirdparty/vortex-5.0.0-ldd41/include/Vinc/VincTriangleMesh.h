/* Vortex 5.0.0 build 800 "vortexldd41" - the TriangleMesh geometry (type 10), its OBB tree
 * helpers and the triangle-triangle separation solver (restored;
 * src/Vinc/VincTriangleMesh.cpp, VincTriangleMeshBuild.cpp, VincOBBTree.cpp, VincTriTriSeparation.cpp).
 *
 * The Mac build only keeps what the registered type functions and the pair tests reach (Apple's
 * linker dead-stripped creation and building: LDD never makes a triangle mesh). The Win build
 * keeps the whole API (Win 0x7b2e80-0x7b3f60), restored from the Win bodies.
 * Strings: "TriangleMesh", "VincTriangleMeshBuild(mesh)", "VincTriangleMeshAddTriangle()". */
#ifndef VINC_VINCTRIANGLEMESH_H
#define VINC_VINCTRIANGLEMESH_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* one corner of a triangle (0x10 bytes on both) */
typedef struct VincTriangleMeshCorner {
    MeReal* position;                /* +0x00 user vertex (not copied) */
    int m04;                         /* +0x04 (Win 0x7b3450 returns m04 / 3: vertex index * 3) */
    float m08;                       /* +0x08 per-corner value read as edgeData0 by the pair tests */
    float m0c;                       /* +0x0c per-corner value read as edgeData1 */
} VincTriangleMeshCorner;

/* OBB tree node (Mac 0x88, Win 0x90 bytes) (names inferred) */
typedef struct VincOBBNode {
    struct VincOBBNode* child[2];    /* +0x00 both NULL: leaf */
    int m08;                         /* +0x08 */
    MeVector3 halfExtents;           /* +0x0c / +0x10 */
    MeVector3 axis[3];               /* +0x24 / +0x28 box axes (rows) */
    MeVector3 center;                /* +0x6c / +0x70 */
    int triangle;                    /* +0x84 / +0x88 leaf: triangle index (-1: inner node) */
#if defined(_WIN32)
    int fitKind;                     /* Win +0x8c (the Mac padding has none: its builder is dead-stripped):
                                        0 fitted box, 1 axis-aligned, 2 axis-aligned at the depth limit */
#endif
} VincOBBNode;

/* OBB tree (0x2c bytes on both; built by the Win-only VincTriangleMeshBuild) (names inferred) */
typedef struct VincOBBTree {
    struct McdTriangleMesh* mesh;    /* +0x00 */
    VincOBBNode* nodes;              /* +0x04 2 * triangleCount - 1 nodes; nodes->child[0]: root */
    int m08;                         /* +0x08 */
    int m0c;                         /* +0x0c mesh flags & 1 */
    int m10;                         /* +0x10 mesh flags & 2 */
    int m14;                         /* +0x14 (-1) */
    void* m18;                       /* +0x18 MeMemoryAPI block */
    void* m1c;                       /* +0x1c new[] */
    void* m20;                       /* +0x20 new[] */
    int m24;                         /* +0x24 */
    void* m28;                       /* +0x28 MeMemoryAPI block */
} VincOBBTree;

typedef void (MEAPI *VincTriangleMeshDestroyCallback)(void* data);

/* TriangleMesh geometry (Mac 0x84, Win 0x88 bytes). Mac / Win offsets. */
typedef struct McdTriangleMesh {
    VincGeometry base;               /* +0x00 */
    VincOBBTree* tree;               /* +0x04 */
    int* m08;                        /* +0x08 one int per triangle (1 after build) */
    int triangleCount;               /* +0x0c */
    VincTriangleMeshCorner* corners; /* +0x10 3 per triangle */
    MeReal* normals;                 /* +0x14 3 per triangle (user array, or computed) */
    MeReal* areas;                   /* +0x18 one per triangle (after build) */
    int triangleMaxCount;            /* +0x1c capacity of corners */
    VincTriangleMeshDestroyCallback destroyCallback; /* +0x20 */
    void* destroyCallbackData;       /* +0x24 */
    void* m28;                       /* +0x28 freed by destroy */
    MeVector3 halfExtents;           /* +0x2c / +0x30 local AABB */
    MeVector3 center;                /* +0x44 / +0x48 */
    MeReal radius;                   /* +0x5c / +0x60 |halfExtents| */
    MeReal degenerateTolerance;      /* +0x64 / +0x68 (1e-14) minimum triangle "area" */
    MeReal m6c;                      /* +0x6c / +0x70 radius * 0.01f (build) */
    MeReal m74;                      /* +0x74 / +0x78 (MeReal)0.8f (build) */
    int flags;                       /* +0x7c / +0x80 create argument */
    int m80;                         /* +0x80 / +0x84 (-1) */
} VincTriangleMesh;

/* ---- TriangleMesh (VincTriangleMesh.cpp) (names inferred) ---- */
MeI16 MEAPI VincTriangleMeshGetTypeId(void);                                    /* Mac 0x4c41bc, Win 0x7b2e70 */
int MEAPI VincTriangleMeshGetTriangleCount(VincGeometryID g);                   /* Mac 0x4c41ce, Win 0x7b2f40 */
void MEAPI VincTriangleMeshGetTrianglePtrs(VincGeometryID g, int i, MeReal** v0, MeReal** v1,
                                     MeReal** v2);                        /* Mac 0x4c41d9, Win 0x7b2f80 */
void MEAPI VincTriangleMeshGetNormalPtr(VincGeometryID g, int i, MeReal** n);   /* Mac 0x4c421b, Win 0x7b3130 */
int MEAPI VincTriangleMeshIsBuilt(VincGeometryID g);                            /* Mac 0x4c4234, Win 0x7b3480 */
void MEAPI VincTriangleMeshGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal* aabb6);  /* Mac 0x4c425a, Win 0x7b3540 */
void MEAPI VincTriangleMeshGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4c4419, Win 0x7b36b0 */
void MEAPI VincTriangleMeshGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4); /* Mac 0x4c444d, Win 0x7b3660 */
void MEAPI VincTriangleMeshUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                                MeReal* aabb6);                           /* Mac 0x4c44b5, Win 0x7b3630 */
void MEAPI VincTriangleMeshDestroy(VincGeometryID g);                           /* Mac 0x4c44f1, Win 0x7b34d0 */
void MEAPI VincTriangleMeshRegisterType(void);                                  /* Mac 0x4c4581, Win 0x7b4330 */
void MEAPI VincTriangleMeshGetCornerData(VincGeometryID g, int i, MeVector3 data0, MeVector3 data1); /* Mac 0x4c45ed, Win 0x7b3150 */
void MEAPI VincTriangleMeshMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4c4695, Win 0x7b36e0 */
MeReal MEAPI VincTriangleMeshGetTriangleArea(VincGeometryID g, int i);          /* Mac 0x4c4718, Win 0x7b3de0 */
void MEAPI VincTriangleMeshGetTriangleNormal(VincGeometryID g, int i, MeVector3 n); /* Mac 0x4c4810, Win 0x7b3cd0 */
int MEAPI VincTriangleMeshGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia,
                                      MeReal* volume);                    /* Mac 0x4c4975, Win 0x7b3fe0 */
/* Win-only API (dead-stripped on the Mac) */
VincGeometryID MEAPI VincTriangleMeshCreate(int triangleMaxCount, int flags);   /* Win 0x7b2e80 */
void MEAPI VincTriangleMeshSetTriangleData(VincGeometryID g, VincTriangleMeshCorner* corners, MeReal* normals,
                                     int count);                          /* Win 0x7b2f20 */
void MEAPI VincTriangleMeshSetDegenerateTolerance(VincGeometryID g, MeReal tol); /* Win 0x7b2f50 */
void MEAPI VincTriangleMeshGetM08(VincGeometryID g, int** p);                   /* Win 0x7b2f60 */
void MEAPI VincTriangleMeshGetVertex0Ptr(VincGeometryID g, int i, MeReal** v0); /* Win 0x7b2fc0 */
void MEAPI VincTriangleMeshGetTriangle(VincGeometryID g, int i, MeVector3 v0, MeVector3 v1, MeVector3 v2); /* Win 0x7b2fe0 */
void MEAPI VincTriangleMeshComputeNormals(VincGeometryID g);                    /* Win 0x7b3050 */
VincOBBTree* MEAPI VincTriangleMeshGetTree(VincGeometryID g);                   /* Win 0x7b31b0 */
void MEAPI VincTriangleMeshSetNormal(VincGeometryID g, int i, const MeVector3 n); /* Win 0x7b31c0 */
void MEAPI VincTriangleMeshSetTriangleMaxCount(VincGeometryID g, int n);        /* Win 0x7b31f0 */
int MEAPI VincTriangleMeshAddTriangle(VincGeometryID g, MeReal* v0, MeReal* v1, MeReal* v2); /* Win 0x7b3220 */
void MEAPI VincTriangleMeshComputeBounds(VincGeometryID g);                     /* Win 0x7b3310 */
int MEAPI VincTriangleMeshGetCornerVertexIndex(VincGeometryID g, int i, int corner); /* Win 0x7b3450 */
void MEAPI VincTriangleMeshSetDestroyCallback(VincGeometryID g, VincTriangleMeshDestroyCallback fn); /* Win 0x7b34b0 */
void MEAPI VincTriangleMeshSetDestroyCallbackData(VincGeometryID g, void* data); /* Win 0x7b34c0 */
void MEAPI VincTriangleMeshComputeAreas(VincGeometryID g);                      /* Win 0x7b3c30 */
/* extreme vertices along dir (support search) over a triangle subset / all / every other */
void MEAPI VincTriangleMeshExtremeVerticesOf(VincGeometryID g, const int* tris, int n, const MeReal* dir,
                                       MeReal** maxV, MeReal** minV, MeReal* maxD, MeReal* minD); /* Win 0x7b3730 */
void MEAPI VincTriangleMeshExtremeVertices(VincGeometryID g, const MeReal* dir, MeReal** maxV, MeReal** minV,
                                     MeReal* maxD, MeReal* minD);         /* Win 0x7b3880 */
void MEAPI VincTriangleMeshExtremeVerticesSparse(VincGeometryID g, const MeReal* dir, MeReal** maxV,
                                           MeReal** minV, MeReal* maxD, MeReal* minD); /* Win 0x7b39b0 */
void MEAPI VincTriangleMeshExtremeVerticesMarked(VincGeometryID g, const int* tris, int n, const MeReal* dir,
                                           MeReal** maxV, MeReal** minV, MeReal* maxD, MeReal* minD,
                                           const int* vertexIndices3, int* marks, int mark); /* Win 0x7b3a90 */
int MEAPI VincTriangleMeshBuild(VincGeometryID g);                              /* Win 0x7b3f60 (VincTriangleMeshBuild.cpp) */
int MEAPI VincTriangleMeshBuildTree(VincGeometryID g);                          /* Win 0x7b3e80 (VincTriangleMeshBuild.cpp) */

/* ---- triangle mesh pair tests (VincBoxTriangleMesh.cpp ...; model1 = the mesh) (names inferred) ---- */
int MEAPI VincTriangleMeshBoxIntersect(VincModelPair* p, VincIntersectResult* r);        /* Mac 0x4c5e5b, Win 0x7d4860 */
int MEAPI VincTriangleMeshBoxRegisterInteraction(void);                                  /* Mac 0x4c5006 */
int MEAPI VincTriangleMeshCapsuleIntersect(VincModelPair* p, VincIntersectResult* r);    /* Mac 0x4c6af1, Win 0x764d40 */
int MEAPI VincTriangleMeshCapsuleRegisterInteraction(void);                              /* Mac 0x4c62ac */
int MEAPI VincTriangleMeshConvexIntersect(VincModelPair* p, VincIntersectResult* r);     /* Mac 0x4c771f, Win 0x75bc00 */
int MEAPI VincTriangleMeshConvexRegisterInteraction(void);                               /* Mac 0x4c75ff, Win 0x75bec0 */
int MEAPI VincTriangleMeshCylinderIntersect(VincModelPair* p, VincIntersectResult* r);   /* Mac 0x4c868a, Win 0x7d41a0 */
int MEAPI VincTriangleMeshCylinderRegisterInteraction(void);                             /* Mac 0x4c7e3f */

/* ---- OBB tree (VincOBBTree.cpp) (names inferred) ---- */
/* separating axis test of two boxes: relTM (rows 0..2 rotation, 12..14 translation of box b in
 * box a's frame), half extents a, b; 0 = overlap, else the first separating axis 1..15, 16 = the
 * bounding spheres are apart (Mac 0x4c274c) */
int VincOBBDisjoint(const MeReal* relTM, const MeReal* a, const MeReal* b);   /* Mac 0x4c274c, Win 0x7cd8d0 */
void MEAPI VincOBBTreeTerm(VincOBBTree* tree);                                  /* Mac 0x4c3076 */
void MEAPI VincOBBTreeDestroy(VincOBBTree* tree);                               /* Mac 0x4c30f0, Win 0x80e1e0 */

/* construction (Win-only, VincOBBTreeBuild.cpp) (names inferred) */
void MEAPI VincOBBTreeInit(VincOBBTree* tree, VincGeometryID mesh);              /* Win 0x80e270 */
void MEAPI VincOBBTreeSetMaxDepth(VincOBBTree* tree, int depth);                 /* Win 0x80dc60 */
int MEAPI VincOBBTreeBuild(VincOBBTree* tree);                                   /* Win 0x80e150 */
void MEAPI VincOBBTreeBuildVertexIndex(VincOBBTree* tree);                       /* Win 0x8022e0 */
MeReal MEAPI VincOBBTreeGetVolume(VincOBBTree* tree);                            /* Win 0x80e240 */

/* ---- triangle-triangle separation (VincTriTriSeparation.cpp) (names inferred) ---- */
/* two triangles a (v[0..2], normal na) and b (v[3..5], normal nb) and a direction: the shortest
 * translation along dir that separates them (Mac 0x4c312a ... 0x4c4126). Mac 0x170 bytes. */
typedef struct VincTriTriSeparation {
    MeReal* v[6];                    /* +0x00 vertex pointers: a0 a1 a2 b0 b1 b2 */
    MeReal* normal[2];               /* +0x18 face normals of a, b */
    MeVector3 dir;                   /* +0x20 */
    MeReal edgePlane[2][3][4];       /* +0x38 per triangle and edge: plane (normal, d) */
    MeReal plane[2][4];              /* +0xf8 face planes (normal, d) */
    MeVector3 point[2];              /* +0x138 result: closest points on a and b */
    MeReal depth;                    /* +0x168 DBL_MAX: none */
} VincTriTriSeparation;
void MEAPI VincTriTriSeparationInit(VincTriTriSeparation* s, MeReal* a0, MeReal* a1, MeReal* a2, MeReal* b0,
                              MeReal* b1, MeReal* b2, MeReal* na, MeReal* nb, const MeReal* dir); /* Mac 0x4c3196 */
int MEAPI VincTriTriSeparationCompute(VincTriTriSeparation* s);                 /* Mac 0x4c4126 */

#ifdef __cplusplus
}
#endif

#endif
