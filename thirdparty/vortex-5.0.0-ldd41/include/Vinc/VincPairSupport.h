/* Vortex 5.0.0 build 800 "vortexldd41" - functions of other Vinc items that the V3.5 / V3.6 pair
 * tests call (implemented by the owning items below). Names inferred except VincContactSimplify
 * (string in both binaries).
 *
 *   function                       Mac        Win        owner
 *   VincContactSimplify            0x54f6a8   0x756dc0   V3.7 (559a)  MEAPI
 *   VincClipSegmentAddPoints       0x544db7   0x7e7100   V3.8 (558e)  cdecl
 *   VincClipSegmentAddInnerPoints  0x544e97   0x7e7030   V3.8 (558e)  cdecl
 *   VincCylinderTriangleContacts   0x545a7e   0x7e76c0   V3.8 (558e)  cdecl (VincTriangleContacts.h)
 *   VincSphereTriangleContact      0x548776   0x7e7230   V3.8 (558e)  cdecl (VincTriangleContacts.h)
 *   VincEllipsoidSafeTime          0x4dc9ee   0x81c740   V3.4 (557e)  MEAPI
 * declared by V3.5 (557f callers):
 *   VincContactSimplifyResult      0x5501e5   0x757180   V3.7 (559a)  MEAPI
 *   VincClipSegmentCylinderAddPoints      0x544fa2  0x7e60f0  V3.8 (558e)  cdecl
 *   VincClipSegmentCylinderAddInnerPoints 0x545088  0x7e6010  V3.8 (558e)  cdecl
 *   VincConjugateGradientMinimize  0x54ac77   0x81e0e0   V3.8 (558e)  cdecl
 * The Win conventions are from the disassembly (ret N); the cdecl ones are internal helpers
 * shared between Vinc files. */
#ifndef VINC_VINCPAIRSUPPORT_H
#define VINC_VINCPAIRSUPPORT_H

#include "Vinc/VincIntersect.h"
#include "Vinc/VincContactSimplify.h"

#ifdef __cplusplus
extern "C" {
#endif

/* VincContactSimplify / VincContactSimplifyResult (V3.7): Vinc/VincContactSimplify.h */

/* Clip the segment p + t dir, t in [-1, 1], against the half space z <= 0 (invDirZ = 1 / dir z)
 * and append both end points of the remaining part to *cursor (advanced by 2 points). */
void VincClipSegmentAddPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* invDirZ);
/* the same, but an end point is appended only when it was clipped (t > -1 / t < 1) */
void VincClipSegmentAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* invDirZ);

/* Safe time of two moving ellipsoids (half axes radii1 / radii2 along the models' axes) */
int MEAPI VincEllipsoidSafeTime(VincModelPair* pair, MeReal maxTime, const MeReal* radii1, const MeReal* radii2,
                                VincSafeTimeResult* result);


/* Mac 0x544fa2 / 0x545088, Win 0x7e60f0 / 0x7e6010: clip the segment p + t dir, t in [*tLo, *tHi],
 * against the z-axis cylinder (radius *r, half height h) and append both end points of the part
 * inside / only the end points that were clipped (the inner ones) to *cursor (names inferred) */
void VincClipSegmentCylinderAddPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* r, MeReal h,
                                      const MeReal* tLo, const MeReal* tHi);
void VincClipSegmentCylinderAddInnerPoints(MeReal** cursor, const MeReal* p, const MeReal* dir, const MeReal* r,
                                           MeReal h, const MeReal* tLo, const MeReal* tHi);

/* Mac 0x54ac77, Win 0x81e0e0 (V3.8, src/Vinc/VincMinimize.c): Polak-Ribiere conjugate-gradient
 * minimisation of f over n <= 4 variables starting at x (at most 100 line searches with tolerance
 * lineTol; stops when 2 |f1 - f0| <= ftol (|f1| + |f0| + 1e-10)); returns the minimum, x = its
 * position (Numerical Recipes frprmn, with the quirks listed in VincMinimize.c) (name inferred) */
typedef MeReal (*VincMinimizeFn)(const MeReal* x);
typedef void (*VincMinimizeGradientFn)(MeReal* gradient, const MeReal* x);
MeReal VincConjugateGradientMinimize(int n, MeReal* x, MeReal lineTol, MeReal ftol, VincMinimizeFn f,
                                     VincMinimizeGradientFn df);
/* Win 0x81e070 (Win only): minimum of f near [0, 1] (bracket from 0 / 1, then the line minimiser) */
MeReal VincMinimize1D(MeReal* xmin, MeReal tol, MeReal (*f)(MeReal), MeReal (*df)(MeReal));

#ifdef __cplusplus
}
#endif

#endif
