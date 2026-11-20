/* Vortex 5.0.0 build 800 "vortexldd41" - private declarations of the Vinc sources
 * above: helpers shared by those files, and the functions of other Vinc files they
 * call, each section naming where they are defined (VincConvexMeshInteractions.cpp,
 * VincModelPair.cpp, VincAggregateGeometry.cpp, VincMassProperties.c) or including the public
 * header that declares them. Names inferred unless noted.
 */
#ifndef VINC_VINCINTERNAL_H
#define VINC_VINCINTERNAL_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ---- triangle passed to the triangle contact generators: V3.6's VincTriangle ---- */
#include "Vinc/VincTriangleContacts.h"
typedef VincTriangle VincTriangleRef;

/* ---- convex hulls and the Convex geometry: V3.1's Vinc/VincConvexMesh.h (CnvHull, CnvVertex,
 * CnvFace, CnvEdge, VincConvexGeometry, VincConvexDesc, CnvPairCache, cnv*, VincConvexMesh*) ---- */
#ifdef __cplusplus
}
#endif
#include "Vinc/VincConvexMesh.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- convex interactions shared with the mesh pair tests (VincConvexMeshInteractions.cpp) ---- */
extern CnvHull* VincConvexTriangleHull;   /* Mac 0x779530 */
extern CnvHull* VincConvexBoxHull;        /* Mac 0x779534 */
CnvHull* MEAPI VincConvexBoxHullUpdate(CnvHull* hull, const MeReal* radii);   /* Mac 0x4bd9f3 */
int MEAPI VincConvexTriangleContacts(VincGeometryID convex, const MeReal* relTM, VincTriangleRef* tri,
                                     int maxCount, VincContact* contacts, int* count); /* Mac 0x4bc585 */

/* ---- default request (VincModelPair.cpp) ---- */
void MEAPI VincRequestInitDefault(void);                 /* Mac 0x4c24eb (name inferred) */
extern VincRequest* VincDefaultRequestPtr;         /* Mac 0x77ce74, Win 0xaf2060 */

/* ---- 558e (V3.8): aggregate ("Composite") geometry, src/Vinc/VincAggregateGeometry.cpp ---- */
MeI16 MEAPI VincAggregateGetTypeId(void);                                  /* Mac 0x54b028, Win 0x759cb0 */
int MEAPI VincAggregateAddElementGeometry(VincGeometryID agg, VincModelID element, MeReal* relTM); /* Mac 0x54b03a, Win 0x759f00 */
int MEAPI VincAggregateRemoveElementGeometry(VincGeometryID agg, int i);   /* Mac 0x54b0a4, Win 0x759f50 */
VincModelID MEAPI VincAggregateGetElement(VincGeometryID agg, int i);      /* Mac 0x54b0dd, Win 0x75a020 */
int MEAPI VincAggregateGetElementCount(VincGeometryID agg);                /* Mac 0x54b0ee, Win 0x75a040 */
int MEAPI VincAggregateGetMaxElementCount(VincGeometryID agg);             /* Mac 0x54b0f9, Win 0x75a050 */
void MEAPI VincAggregateGetElementAndTransform(VincGeometryID agg, int i, VincModelID* m, MeReal** relTM); /* Win 0x759ff0 */
VincModelID* MEAPI VincAggregateRemoveAllElements(VincGeometryID agg);     /* Win 0x75a060 */
void MEAPI VincAggregateGeometryDestroy(VincGeometryID agg);               /* Mac 0x54b104, Win 0x75a090 */
void MEAPI VincAggregateUpdateElementTransforms(VincGeometryID agg, const MeReal* tm); /* Mac 0x54c2ab, Win 0x759f90 */
void MEAPI VincAggregateGeometryUpdate(VincGeometryID agg);                /* Mac 0x54b8a8, Win 0x75ab90 */
VincGeometryID MEAPI VincAggregateGeometryCreate(int maxElements);         /* Mac 0x54b8bb, Win 0x759cc0 */
void MEAPI VincAggregateUpdateAABB(VincGeometryID agg, MeMatrix4 tm, MeVector3 minCorner,
                             MeVector3 maxCorner, MeReal* aabb6);    /* Mac 0x54c59b, Win 0x75a260 */
void MEAPI VincAggregateGetXYAABB(VincGeometryID agg, MeMatrix4 tm, MeReal* out4); /* Mac 0x54c530, Win 0x75a2d0 */
void MEAPI VincAggregateGetBSphere(VincGeometryID agg, MeVector3 center, MeReal* radius); /* Mac 0x54b13c, Win 0x75a310 */
void MEAPI VincAggregateMaximumPoint(VincGeometryID agg, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x54b9a2, Win 0x75ac10 */
MeI16 MEAPI VincAggregateGetMassProperties(VincGeometryID agg, MeMatrix4 relTM, MeMatrix3 inertia,
                                     MeReal* volume, MeReal* mass);  /* Mac 0x54c666, Win 0x75ad10 */
int MEAPI VincAggregateGetMassPropertiesFn(VincGeometryID agg, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x54d21d, Win 0x75b770 */
MeI16 MEAPI VincModelsGetMassProperties(VincModelID* models, int count, MeMatrix4 relTM, MeMatrix3 inertia,
                                        MeReal* volume, MeReal* mass);  /* Mac 0x54d24d, Win 0x75b240 */
int MEAPI VincAggregateIntersect(struct McdModelPair* pair, struct McdIntersectResult* r); /* Mac 0x54bcf4, Win 0x75a410 */
int MEAPI VincAggregateLineSegIntersectFiltered(VincModelID m, const MeReal* p0, const MeReal* p1,
                                                int (MEAPI *filter)(VincModelID, void*), void* data,
                                                struct McdLineSegResult* result); /* Mac 0x54b1bc, Win 0x75a840 */
int MEAPI VincAggregateLineSegIntersect(VincModelID m, const MeVector3 p0, const MeVector3 p1,
                                        struct McdLineSegResult* result);        /* Mac 0x54b327, Win 0x75a960 */
void MEAPI VincAggregateRegisterLineSegment(void);                         /* Mac 0x54b191 */
int MEAPI VincAggregateRegisterInteractions(void);                         /* Mac 0x54b35f, Win 0x75aba0 */
void MEAPI VincAggregateRegisterType(void);                                /* Mac 0x54b936, Win 0x75b7a0 */
/* Win-only (dead-stripped on the Mac): the distance query of an aggregate (model1) against any
 * model (the nearest element) and its registration for every type (names inferred) */
int MEAPI VincAggregateDistanceBounded(struct McdModelPair* pair, const MeReal* bounds3,
                                       struct McdDistanceResult* result);        /* Win 0x75a9a0 */
int MEAPI VincAggregateRegisterDistance(void);                             /* Win 0x75ab50 */
/* Win 0x7acac0 (pair table, dead-stripped on the Mac; defined in VincAggregateGeometry.cpp, its
 * only Vinc caller here) (name inferred): interaction slot 6 of the cells (a, b) and (b, a) */
void MEAPI VincGeometryPairRegisterDistanceBoundedFn(int typeA, int typeB, VincDistanceBoundedFn fn);
/* 558e mass helpers (src/Vinc/VincMassProperties.c) used by the triangle mesh and the convex mesh
 * (MEAPI here; cdecl in the Win original): per-triangle tetrahedron properties (13 doubles: volume,
 * centroid[3], inertia[9] about the origin + the centroid's parallel-axis term) and their
 * volume-weighted sum about the centre (out: volume, centre[3], inertia[9]) */
void MEAPI VincTetraMassProperties(MeReal* out13, const MeVector3 a, const MeVector3 b, const MeVector3 c); /* Mac 0x549786, Win 0x7ae770 */
void MEAPI VincSumMassProperties(const MeReal* tetra13, int count, MeReal* out13);  /* Mac 0x54939f, Win 0x7ae040 */

/* ---- 558d (V3.8): the space (VincModelSetState -> VincSpaceModelSetState2, VincSpaceModelIsFrozen
 * (V3.2's VincSpaceModelTestFlag)): Vinc/VincSpace.h ---- */
#ifdef __cplusplus
}
#endif
#include "Vinc/VincSpace.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- 559a (V3.7): contact simplification: Vinc/VincContactSimplify.h ---- */
#ifdef __cplusplus
}
#endif
#include "Vinc/VincContactSimplify.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- 558c (V3.7): height field (VincHeightFieldGetTypeId / GetCellRange / GetVertex ...):
 * Vinc/VincHeightField.h ---- */
#ifdef __cplusplus
}
#endif
#include "Vinc/VincHeightField.h"
#ifdef __cplusplus
extern "C" {
#endif

/* ---- 558a (V3.6): triangle contact generators: Vinc/VincTriangleContacts.h ---- */

/* pair-data pool of the convex pair tests (0x10c-byte objects, VincConvexMeshInteractions.cpp) */
int MEAPI VincConvexPairDataRelease(VincModelID m1, VincModelID m2, void* data);   /* Mac 0x4bd6c2 */
int MEAPI VincConvexPairDataAcquire(VincModelID m1, VincModelID m2, void** data);  /* Mac 0x4bd6ee */

#ifdef __cplusplus
}
#endif

#endif
