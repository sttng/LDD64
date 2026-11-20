/* Vortex 5.0.0 build 800 "vortexldd41" - the TriangleMesh pair tests of sub-TU 557d (restored):
 * TriangleMesh vs plane / sphere / height field / triangle list / triangle mesh,
 * the mesh line segment query, the triangle-triangle contact generator they share (Mac 0x4cc248 ...
 * 0x4da2f7) and the 3x3 Jacobi eigen solver of the same file. All names inferred.
 *
 * Registration (VincGeometryPairRegisterInteractions(mesh, other, ...): model1 is the mesh):
 *   pair                     intersect (Mac / Win)    register (Mac / Win)
 *   mesh - plane             0x4c9ed4 / 0x7d4f70      0x4c97b4 / 0x7d5140
 *   mesh - sphere            0x4cbeb3 / 0x7d4ca0      0x4cb7db / 0x7d4db0
 *   mesh - height field      0x4ca3b6 / 0x764fe0      0x4ca341 / 0x765890
 *   mesh - mesh              0x4ce0b8 / 0x768c50      0x4cc016 / 0x769450
 *   mesh - triangle list     0x4d0677 / 0x766050      0x4cfda4 / 0x766750
 *   line segment             0x4c958f / 0x775710      0x4c8db9 / 0x7757e0
 * Internal helpers shared between the files are plain cdecl on Win (no MEAPI). */
#ifndef VINC_VINCTRIANGLEMESHCONTACTS_H
#define VINC_VINCTRIANGLEMESHCONTACTS_H

#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincContactSimplify.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- pair tests (model1 = the mesh) ---- */
int MEAPI VincTriangleMeshPlaneIntersect(VincModelPair* p, VincIntersectResult* r);        /* Mac 0x4c9ed4, Win 0x7d4f70 */
int MEAPI VincTriangleMeshSphereIntersect(VincModelPair* p, VincIntersectResult* r);       /* Mac 0x4cbeb3, Win 0x7d4ca0 */
int MEAPI VincTriangleMeshHeightFieldIntersect(VincModelPair* p, VincIntersectResult* r);  /* Mac 0x4ca3b6, Win 0x764fe0 */
int MEAPI VincTriangleMeshTriangleMeshIntersect(VincModelPair* p, VincIntersectResult* r); /* Mac 0x4ce0b8, Win 0x768c50 */
int MEAPI VincTriangleMeshTriangleListIntersect(VincModelPair* p, VincIntersectResult* r); /* Mac 0x4d0677, Win 0x766050 */
int MEAPI VincTriangleMeshLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1,
                                      VincLineSegResult* result);                          /* Mac 0x4c958f, Win 0x775710 */

int MEAPI VincTriangleMeshPlaneRegisterInteraction(void);        /* Mac 0x4c97b4, Win 0x7d5140 */
int MEAPI VincTriangleMeshSphereRegisterInteraction(void);       /* Mac 0x4cb7db, Win 0x7d4db0 */
int MEAPI VincTriangleMeshHeightFieldRegisterInteraction(void);  /* Mac 0x4ca341, Win 0x765890 */
int MEAPI VincTriangleMeshTriangleMeshRegisterInteraction(void); /* Mac 0x4cc016, Win 0x769450 */
int MEAPI VincTriangleMeshTriangleListRegisterInteraction(void); /* Mac 0x4cfda4, Win 0x766750 */
int MEAPI VincTriangleMeshRegisterLineSegment(void);             /* Mac 0x4c8db9, Win 0x7757e0 */

/* Mac 0x4d1f97, Win 0x75cc20 (a thunk to VincTriangleMeshGetTypeId 0x7b2e70): the type id as the
 * registrations above (and V3.2's mesh pairs) read it */
MeI16 MEAPI VincTriangleMeshInteractionsGetTypeId(void);
/* Mac 0x4d1fa5, Win 0x75cae0: register the type and its line segment query (VxFrame) */
void MEAPI VincTriangleMeshInit(void);
/* Mac 0x4d2082, Win 0x766ba0: line segment + mesh vs plane / sphere / box / cylinder (VxFrame) */
int MEAPI VincTriangleMeshRegisterInteractions(void);
/* Mac 0x4d1fb7: the root OBB of the mesh as a TM (axes rows, centre row 3) and its half extents */
void MEAPI VincTriangleMeshGetRootOBB(VincGeometryID mesh, MeMatrix4 tm, MeVector3 halfExtents);

/* ---- triangle / triangle contacts (VincTriangleTriangleContacts.c) ---- */
/* contact merge parameters (Mac 0x14, Win 0x18 bytes): the mesh pairs pass {12, (mesh->m6c)^2,
 * mesh->m74} */
typedef struct VincContactMergeParams {
    int maxCount;                    /* +0x00 merge limit (<= 30 used) */
    MeReal distanceSq;               /* +0x04 / +0x08 contacts closer than this are merged */
    MeReal cosTolerance;             /* +0x0c / +0x10 normals at least this parallel: keep the deeper one */
} VincContactMergeParams;
/* Mac 0x4cc248, Win 0x767520 */
int VincContactsMergeAdd(VincIntersectResult* r, const VincContactMergeParams* params, int limit);
/* Mac 0x4d6c11 */
int VincTriTriSolveLine(int i, int j, int k, MeReal t, const MeReal* n1, const MeReal* n2, MeReal d1, MeReal d2,
                        MeReal* out);
/* Mac 0x4d6cd9 */
int VincTriTriCoplanarPoints(MeReal d, const MeReal* n, const MeReal* v0, const MeReal* v1, const MeReal* v2,
                             const MeReal* u0, const MeReal* u1, const MeReal* u2, MeReal* pts, int maxCount,
                             int* count);
/* Mac 0x4da2f7, Win 0x7d3020 */
int VincTriTriIntersectPoints(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* u0,
                              const MeReal* u1, const MeReal* u2, MeReal* pts, int maxCount, int* count);

/* Mac 0x4d615c, Win 0x7d1480: boolean triangle / triangle test (Moeller); Mac 0x4d5531, Win 0x7d09a0:
 * its coplanar case (returns 5 / 0) */
int VincTriTriOverlap(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* u0, const MeReal* u1,
                      const MeReal* u2);
int VincTriTriCoplanarOverlap(const MeReal* n, const MeReal* v0, const MeReal* v1, const MeReal* v2,
                              const MeReal* u0, const MeReal* u1, const MeReal* u2);
/* OBB / OBB tests of the mesh / mesh walk: box = an OBB node from its half extents on (half extents,
 * axes, centre), tm = compact 3x4 transform (rotation rows, position); 0 = overlap, 1..15 the
 * separating axis, 16 = the bounding spheres are apart. Flat = a leaf box (third extent left out). */
int VincOBBDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB);          /* Mac 0x4d3204, Win 0x7d5190 */
int VincOBBFlatDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB);      /* Mac 0x4d20a8, Win 0x7d6420 */
int VincOBBFlatFlatDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB);  /* Mac 0x4d43fd, Win 0x7d5b40 */

/* ---- one triangle against the mesh (VincTriangleMeshTriangle.cpp) ---- */
/* Mac 0x4d15ce, Win 0x7668a0: the mesh triangle a against the triangle b (mesh frame) */
int VincTriangleMeshTriangleContacts(VincIntersectResult* r, const VincTriangle* a, const VincTriangle* b,
                                     MeReal pad, const VincContactMergeParams* params);
/* Mac 0x4d028f, Win 0x765e70: tri (mesh frame) down the OBB tree from node */
void VincTriangleMeshTriangleNode(VincIntersectResult* r, const VincTriangle* tri, VincGeometryID mesh,
                                  const struct VincOBBNode* node, MeReal pad, const VincContactMergeParams* params);

/* VincContactSimplifyResult (V3.7, 559a): Vinc/VincContactSimplify.h */

/* ---- 3x3 symmetric eigen decomposition (cyclic Jacobi) ---- */
/* Mac 0x4d1e3e, Win 0x7c0ea0: a (row-major 3x3, overwritten with the rotated matrix: eigenvalues on
 * the diagonal), v (rotations accumulated: columns = eigenvectors; the caller initialises it),
 * at most maxSweeps sweeps while |(a10, a20, a21)| > 0.001; returns the number of sweeps */
int VincJacobi3(MeReal* a, MeReal* v, int maxSweeps);
/* Mac 0x4d1d4b, Win 0x7c0cf0: rotation (c, s) annihilating a[p][q] (p > q) */
void VincJacobiRotation(const MeReal* a, int p, int q, MeReal* s, MeReal* c);

#ifdef __cplusplus
}
#endif

#endif
