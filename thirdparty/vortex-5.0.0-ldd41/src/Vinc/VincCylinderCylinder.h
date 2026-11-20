/* Vortex 5.0.0 build 800 "vortexldd41" - private declarations shared by the cylinder / cylinder
 * and cylinder / capsule pair tests (VincCylinderCylinder.c, VincCylinderCapsule.c; restored,
 * 557f). Names (name inferred). cdecl on Win. Both tests work in the first
 * cylinder's frame (axis z, r1, hh1); the second shape (r2, hh2; the capsule test passes its
 * segment and radius) is `rel` in that frame (rows 0..2 axes, row 3 centre). The separating-axis
 * core (VincCylinderCylinderCore.h) is compiled into both files with its own rim data. */
#ifndef VINC_VINCCYLINDERCYLINDER_H
#define VINC_VINCCYLINDERCYLINDER_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac 0x4ec9cf, Win 0x7ddcd0: v = normalised (a x b) (a x c when a x b vanishes, else a vector
 * perpendicular to a), u = v x a */
void VincCylinderFrame(MeReal* u, MeReal* v, const MeReal* a, const MeReal* b, const MeReal* c);

/* Mac 0x4ebd4f, Win 0x7dde80: the contact points of a face / edge contact. With clipSecond the
 * second cylinder's cap diameters (along r2 * u2) and side lines (at +-r2 u2, +-r2 v2) are
 * clipped against the first cylinder and appended to *cursor; with clipFirst the same for the
 * first cylinder (u1, v1) against the second, computed in the second's frame and moved back. */
void VincCylinderClipEdges(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* hh1,
                           const MeReal* u1, const MeReal* v1, char clipFirst, const MeReal* r2,
                           const MeReal* hh2, const MeReal* u2, const MeReal* v2, char clipSecond);

/* Mac 0x4ebbac / 0x4eba07, Win 0x7dda70 / 0x7ddb30: objective and gradient of the rim / rim
 * closest-point search, minimised over the two rim angles by VincConjugateGradientMinimize. They
 * read the rim data of VincCylinderCylinder.c only (see VincCylinderCylinderCore.h). */
MeReal VincCylinderRimObjective(const MeReal* angles);
void VincCylinderRimGradient(MeReal* gradient, const MeReal* angles);

/* (not in the binaries; tests) VincCylinderCylinder.c's copy of the separating-axis core
 * (VincCylinderCylinderCore.h, Win 0x7de480) */
int VincCylinderCylinderSATForTests(MeReal* sep, MeReal* normal, MeReal* depth, MeReal** cursor, MeI16* dims,
                                    const MeReal* pad, const MeReal* r1, const MeReal* hh1, const MeReal* r2,
                                    const MeReal* hh2, const MeReal* rel);
/* (not in the binaries; tests) the rim data of VincCylinderCylinder.c (Win 0xafa198, 10 doubles) */
const MeReal* VincCylinderRimDataForTests(void);

#ifdef __cplusplus
}
#endif

#endif
