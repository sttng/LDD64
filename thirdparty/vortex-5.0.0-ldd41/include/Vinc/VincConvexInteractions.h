/* Vortex 5.0.0 build 800 "vortexldd41" - registration functions and helpers of the Convex (type 9)
 * pair tests (src/Vinc/VincConvexMeshInteractions.cpp; Mac 557c 0x4ba87b-0x4bf81e).
 * All names inferred. The hull itself is (557b). */
#ifndef VINC_VINCCONVEXINTERACTIONS_H
#define VINC_VINCCONVEXINTERACTIONS_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

int MEAPI VincConvexGetTypeId(void);                          /* Mac 0x4bf149, Win 0x75c6d0 */
void MEAPI VincConvexRegisterType(void);                      /* Mac 0x4bf157, Win 0x75c6c0 */
/* pool + terminate action + convex-convex + box / sphere / cylinder / plane + line segment
 * (Mac 0x4bf077; called by the Vx frame setup) */
int MEAPI VincConvexMeshRegisterInteractions(void);
int MEAPI VincConvexMeshHeightFieldRegisterInteraction(void);   /* Mac 0x4bb5a5, Win 0x763d10 */
int MEAPI VincConvexMeshTriangleListRegisterInteraction(void);  /* Mac 0x4bc46d */
int MEAPI VincCapsuleConvexRegisterInteraction(void);           /* Mac 0x4bd8b2 */
int MEAPI VincBoxConvexRegisterInteraction(void);               /* Mac 0x4bd847 */
int MEAPI VincCylinderConvexRegisterInteraction(void);          /* Mac 0x4bd91d */
int MEAPI VincSphereConvexRegisterInteraction(void);            /* Mac 0x4bd988 */
int MEAPI VincConvexMeshPlaneRegisterInteraction(void);         /* Mac 0x4baf0f */
void MEAPI VincConvexMeshRegisterLineSegment(void);             /* Mac 0x4ba87b */

/* pair tests */
int MEAPI VincConvexMeshLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1,
                                    VincLineSegResult* r);                          /* Mac 0x4ba8a6 */
int MEAPI VincConvexMeshPlaneIntersect(VincModelPair* p, VincIntersectResult* r);         /* Mac 0x4baf7a */
int MEAPI VincConvexMeshHeightFieldIntersect(VincModelPair* p, VincIntersectResult* r);   /* Mac 0x4bb6c4, Win 0x763600 */
int MEAPI VincConvexMeshTriangleListIntersect(VincModelPair* p, VincIntersectResult* r);  /* Mac 0x4bc96d */
int MEAPI VincConvexConvexPairIntersect(VincModelPair* p, VincIntersectResult* r);        /* Mac 0x4bdb96 */
int MEAPI VincCylinderConvexIntersect(VincModelPair* p, VincIntersectResult* r);          /* Mac 0x4bdc61 */
int MEAPI VincBoxConvexIntersect(VincModelPair* p, VincIntersectResult* r);               /* Mac 0x4bdd44 */
int MEAPI VincSphereConvexIntersect(VincModelPair* p, VincIntersectResult* r);            /* Mac 0x4bde34 */
int MEAPI VincCapsuleConvexIntersect(VincModelPair* p, VincIntersectResult* r);           /* Mac 0x4bdf0f */

/* segment helpers (used by the convex hull code) */
MeReal MEAPI VincPointSegmentDistance(const MeReal* p, const MeReal* a, const MeReal* b, MeReal* out); /* Mac 0x4be1be */
MeReal MEAPI VincSegmentSegmentParameters(const MeReal* a0, const MeReal* a1, const MeReal* b0,
                                          const MeReal* b1, MeReal* s, MeReal* t, char* parallel); /* Mac 0x4be470 */
MeReal MEAPI VincSegmentSegmentClosestPoints(const MeReal* a0, const MeReal* a1, const MeReal* b0,
                                             const MeReal* b1, MeReal* pa, MeReal* pb, char* parallel,
                                             int* count);                                       /* Mac 0x4be710 */

/* hull accessors */
void MEAPI VincConvexGetFaceNormal(VincGeometryID g, int face, MeVector3 n);        /* Mac 0x4bf164 */
MeReal* MEAPI VincConvexGetFaceVertexPtr(VincGeometryID g, int face, int k);        /* Mac 0x4bf19b */
void MEAPI VincConvexGetFaceVertex(VincGeometryID g, int face, int k, MeVector3 v); /* Mac 0x4bf1ba */
int MEAPI VincConvexGetFaceVertexCount(VincGeometryID g, int face);                 /* Mac 0x4bf1f8 */
int MEAPI VincConvexGetFaceCount(VincGeometryID g);                                 /* Mac 0x4bf20f */

#ifdef __cplusplus
}
#endif

#endif
