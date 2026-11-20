/* Vortex 5.0.0 build 800 "vortexldd41" - the Vinc pair tests of 557e: box
 * against box / capsule / cylinder / plane / sphere / triangle list, capsule against capsule /
 * plane / sphere, the capsule line segment, and their contact helpers. Registered through
 * Vinc<A><B>RegisterInteraction (Vinc/VincPrimitives.h); exported for tests. All names
 * inferred. Win conventions from the disassembly: MEAPI = stdcall (ret N), the plain helpers
 * are cdecl. The swept-box safe time VincEllipsoidSafeTime (Mac 0x4dc9ee, Win 0x81c740) is
 * declared in Vinc/VincPairSupport.h. */
#ifndef VINC_VINCPAIRTESTSB_H
#define VINC_VINCPAIRTESTSB_H

#include "Vinc/VincIntersect.h"
#include "Vinc/VincPairSupport.h"

#ifdef __cplusplus
extern "C" {
#endif

/* ---- VincBoxBox.c ---- */
int MEAPI VincBoxBoxIntersect(VincModelPair* pair, VincIntersectResult* result);              /* Mac 0x4dc3d5, Win 0x81d0a0 */
int MEAPI VincBoxBoxSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r);     /* Mac 0x4dceb9, Win 0x81c950 */
/* Mac 0x4db832, Win 0x81ca90 (cdecl): separating axes of two boxes (rel = box 2 in box 1's frame) */
int VincBoxBoxSeparation(MeReal* sep, MeReal* n, MeReal* d, MeI16* dims, const MeReal* pad,
                         const MeReal* r1, const MeReal* r2, const MeReal* rel, MeReal padValue);
/* Mac 0x4dc122, Win 0x81c9a0: contact point candidates (box 1 frame) appended at *cursor */
void MEAPI VincBoxBoxContactPoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r2,
                                   MeReal pad);

/* ---- VincBoxCapsule.c ---- */
int MEAPI VincBoxCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result);          /* Mac 0x4dd2c4, Win 0x7da750 */
int MEAPI VincBoxCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4dcf40, Win 0x7da6e0 */

/* ---- VincBoxCylinder.c ---- */
int MEAPI VincBoxCylinderIntersect(VincModelPair* pair, VincIntersectResult* result);         /* Mac 0x4df902, Win 0x7e1ba0 */
int MEAPI VincBoxCylinderSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4ddc08, Win 0x7e0b20 */

/* ---- VincBoxPlane.c ---- */
int MEAPI VincBoxPlaneIntersect(VincModelPair* pair, VincIntersectResult* result);            /* Mac 0x4e0890, Win 0x81fe40 */
int MEAPI VincBoxPlaneSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r);   /* Mac 0x4e06b3, Win 0x81fd00 */

/* ---- VincBoxSphere.c ---- */
int MEAPI VincBoxSphereIntersect(VincModelPair* pair, VincIntersectResult* result);           /* Mac 0x4e11bb, Win 0x81e560 */
int MEAPI VincBoxSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r);  /* Mac 0x4e0e1d, Win 0x81e500 */
/* Mac 0x4e0ead, Win 0x81e240 (cdecl): sphere (centre c, box frame) against the box */
int VincBoxSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                         const MeReal* radii, const MeReal* sr, const MeReal* c);

/* ---- VincBoxTriangleList.c ---- */
int MEAPI VincBoxTriangleListIntersect(VincModelPair* pair, VincIntersectResult* result);     /* Mac 0x4e183f, Win 0x7dc050 */

/* ---- VincCapsuleCapsule.c ---- */
int MEAPI VincCapsuleCapsuleIntersect(VincModelPair* pair, VincIntersectResult* result);      /* Mac 0x4e348c, Win 0x7d8040 */
int MEAPI VincCapsuleCapsuleSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4e2742, Win 0x7d7a60 */
int MEAPI VincCapsuleLineSegment(VincModelID m, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* r); /* Mac 0x4e3b4e, Win 0x7db730 */

/* ---- VincCapsulePlane.c ---- */
int MEAPI VincCapsulePlaneIntersect(VincModelPair* pair, VincIntersectResult* result);        /* Mac 0x4e4315, Win 0x7db3f0 */

/* ---- VincCapsuleSphere.c ---- */
int MEAPI VincCapsuleSphereIntersect(VincModelPair* pair, VincIntersectResult* result);       /* Mac 0x4e4d52, Win 0x7daee0 */
int MEAPI VincCapsuleSphereSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* r); /* Mac 0x4e4956, Win 0x7dae60 */
/* Mac 0x4e4a99, Win 0x7dac70 (cdecl): sphere (centre c, capsule frame) against the capsule */
int VincCapsuleSphereContact(MeReal* sep, MeReal* n, MeReal* point, MeI16* dims, const MeReal* pad,
                             const MeReal* r, const MeReal* hh, const MeReal* sr, const MeReal* c, int* element);

/* ---- V3.8 (558e) helpers called here; implemented by V3.8 (cdecl on Win) ---- */
/* Mac 0x545192, Win 0x7e8ac0: candidate contact points of box 2 (rel = box 2 in box 1's frame,
 * half extents r2) against box 1 (r1), in box 1's frame, appended at *cursor */
void VincBoxClipBoxPoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r2, MeReal pad);
/* Mac 0x542449, Win 0x7e5a70: the segment p + t d, t in [-1, 1], clipped against the box of half
 * extents radii (invD = 1 / d per component): both end points of the inside part appended */
void VincClipSegmentBoxAddPoints(MeReal** cursor, const MeReal* p, const MeReal* d, const MeReal* invD,
                              const MeReal* radii, MeReal unused);
/* Mac 0x542337, Win 0x7e59a0: the same, only end points that were clipped (t > -1 / t < 1) */
void VincClipSegmentBoxAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* d, const MeReal* invD,
                                   const MeReal* radii, MeReal unused);
/* the segment / cylinder clippers (Mac 0x544fa2 / 0x545088, Win 0x7e60f0 / 0x7e6010) are
 * VincClipSegmentCylinderAdd(Inner)Points in Vinc/VincPairSupport.h */

/* ---- VincBoxCylinder.c helpers (cdecl) ---- */
/* Mac 0x4de961, Win 0x7e1130: separating axes of a box and a cylinder (rel = cylinder in the box
 * frame) + contact point candidates (box frame) at *cursor */
int VincBoxCylinderSeparation(MeReal* sep, MeReal* n, MeReal* d, MeReal** cursor, const MeReal* pad,
                              const MeReal* r1, const MeReal* r, const MeReal* hh, const MeReal* rel);
/* Mac 0x4ddd33, Win 0x7e0b90: the box edges clipped against the cylinder (box frame) */
void VincBoxCylinderEdgePoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r,
                               const MeReal* hh);
/* (inlined in both builds) contact dims of the box / cylinder and box / capsule tests */
MeI16 VincBoxCylinderDims(const MeReal* n, const MeReal* z);

#ifdef __cplusplus
}
#endif

#endif
