/* Vortex 5.0.0 build 800 "vortexldd41" - the Vinc pair tests and line-segment queries of the
 * primitive types restored (557g). Registered through
 * Vinc<A><B>RegisterInteraction / Vinc<Type>RegisterLineSegment (Vinc/VincPrimitives.h); the
 * functions are exported for tests. All names inferred. */
#ifndef VINC_VINCPAIRTESTS_H
#define VINC_VINCPAIRTESTS_H

#include "Vinc/VincIntersect.h"
#include "Vinc/VincPrimitives.h"

#ifdef __cplusplus
extern "C" {
#endif

int MEAPI VincSphereSphereIntersect(VincModelPair* pair, VincIntersectResult* result);        /* Mac 0x4f441e, Win 0x820080 */
int MEAPI VincSphereSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4f4198, Win 0x820310 */
int MEAPI VincSpherePlaneIntersect(VincModelPair* pair, VincIntersectResult* result);         /* Mac 0x4f3bef, Win 0x81f660 */
int MEAPI VincSpherePlaneSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r);  /* Mac 0x4f3f34, Win 0x81f540 */

int MEAPI VincCylinderPlaneIntersect(VincModelPair* pair, VincIntersectResult* result);       /* Mac 0x4f0e18, Win 0x7e0870 */
int MEAPI VincCylinderSphereIntersect(VincModelPair* pair, VincIntersectResult* result);      /* Mac 0x4f188f, Win 0x7e0270 */
int MEAPI VincCylinderSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4f13f8, Win 0x7e01f0 */
int MEAPI VincSphereTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result);  /* Mac 0x4f4800, Win 0x7dc730 */
int MEAPI VincCylinderTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result); /* Mac 0x4f1f2e, Win 0x7db910 */
extern int VincTriangleListMaxContacts; /* Mac 0x7797bc, Win 0xac987c: 400 */
/* Mac 0x4f07fb, Win 0x7e0510 (cdecl): cylinder in the plane's frame against z = 0 */
int VincCylinderPlaneContactPoints(MeReal* depth, MeReal** cursor, const MeReal* pad, const MeReal* r,
                                   const MeReal* hh, const MeReal* rel);
/* Mac 0x4f14c1, Win 0x7dffd0 (cdecl): sphere against cylinder in the cylinder frame */
int VincCylinderSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, MeReal* m, const MeReal* pad,
                              const MeReal* r, const MeReal* hh, const MeReal* sr, const MeReal* c);

/* line segment queries (VincLineSegFn) and their shared helpers (the helpers are plain cdecl on
 * Win, not MEAPI: internal functions shared between Vinc files) */
int MEAPI VincSphereLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r);   /* Mac 0x4f3308, Win 0x7ad110 */
int MEAPI VincPlaneLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r);    /* Mac 0x4f335b, Win 0x7ad160 */
int MEAPI VincBoxLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r);      /* Mac 0x4f36f9, Win 0x7ad2f0 */
int MEAPI VincCylinderLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r); /* Mac 0x4f0783, Win 0x7dd9e0 */
int MEAPI VincTriangleListLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r); /* Mac 0x4f52ad, Win 0x759060 */
/* Mac 0x4f2e82, Win 0x7acd40: ray / axis-aligned box (Graphics Gems "HitBoundingBox"): 0 miss,
 * 1 hit (face[0] axis, face[1] 1 = min side), 2 origin inside */
int VincHitBoundingBox(const MeReal* minB, const MeReal* maxB, const MeReal* origin, const MeReal* dir,
                             MeReal* coord, MeReal* t, MeI16* face);
/* Mac 0x4f30ea, Win 0x7acf80: segment against a sphere at tm[12..14] */
int VincLineSegmentSphere(const MeReal* tm, MeReal r, const MeReal* p0, const MeReal* p1, VincLineSegResult* result);
/* Mac 0x4efdac, Win 0x7dd470: segment against a z-axis cylinder of tm */
int VincLineSegmentCylinder(const MeReal* tm, MeReal r, MeReal hh, const MeReal* p0, const MeReal* p1,
                                  VincLineSegResult* result);
/* Mac 0x4c8af1, Win 0x775430: segment p0 + t d against one triangle (t <= *tBest) */
int VincLineSegmentTriangle(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* normal,
                                  const MeReal* p0, const MeReal* p1, const MeReal* d, const MeReal* len,
                                  MeReal* tBest, MeReal* hit, MeReal* distance);


/* ---- 557f: cone, capsule / triangle list, cylinder / capsule, cylinder /
 * cylinder (src/Vinc/VincCone*.c, VincCapsuleTriangleList.c, VincCylinderCapsule.c,
 * VincCylinderCylinder.c). All names inferred. ---- */
int MEAPI VincCapsuleTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result);  /* Mac 0x4e53fd, Win 0x7d7400 */
int MEAPI VincConeBoxIntersect(VincModelPair* pair, VincIntersectResult* result);              /* Mac 0x4e5e19, Win 0x7e2410 */
int MEAPI VincConeConeIntersect(VincModelPair* pair, VincIntersectResult* result);             /* Mac 0x4e5ed7, Win 0x7e2380 */
int MEAPI VincConeCylinderIntersect(VincModelPair* pair, VincIntersectResult* result);         /* Mac 0x4e5f95, Win 0x7dd3f0 */
int MEAPI VincConeLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r); /* Mac 0x4e6008, Win 0x7e1f00 */
int MEAPI VincConePlaneIntersect(VincModelPair* pair, VincIntersectResult* result);            /* Mac 0x4e6c82, Win 0x7e2cd0 */
int MEAPI VincConeSphereIntersect(VincModelPair* pair, VincIntersectResult* result);           /* Mac 0x4e7311, Win 0x7e2530 */
int MEAPI VincCylinderCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result);      /* Mac 0x4eb2b2, Win 0x7da1a0 */
int MEAPI VincCylinderCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4e7f15, Win 0x7d83c0 */
int MEAPI VincCylinderCylinderIntersect(VincModelPair* pair, VincIntersectResult* result);     /* Mac 0x4ef78d, Win 0x7dfd00 */
int MEAPI VincCylinderCylinderSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4eb91a, Win 0x7ddc40 */
/* Mac 0x4e67c3, Win 0x7e2b40 (cdecl): the cone's deepest point(s) along dir (see VincConePlane.c) */
int VincConeDeepestPoints(const VincCone* cone, const MeReal* tm, const MeReal* dir, MeReal* points, MeReal eps);

#ifdef __cplusplus
}
#endif

#endif
