/* Vortex 5.0.0 build 800 "vortexldd41" - swept separating-axis test of two oriented boxes
 * (restored; src/Vinc/VincBoxSweep.c). Name inferred. */
#ifndef VINC_VINCBOXSWEEP_H
#define VINC_VINCBOXSWEEP_H

#include "Me/MeMath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac 0x4faa70, Win 0x8517a0 (cdecl): boxes with half extents radiiA / radiiB, transforms tmA / tmB
 * (4x4, rows = axes, row 3 = position) moving with velA / velB for maxTime. Returns 0 when one of
 * the 15 separating axes separates them during the whole interval; otherwise 1 and *t = the
 * latest time any of the axes stops separating them (0 when they overlap at time 0), a lower bound
 * of the first contact. */
int VincBoxSweepTime(const MeReal* radiiA, const MeReal* tmA, const MeReal* velA, const MeReal* radiiB,
                     const MeReal* tmB, const MeReal* velB, MeReal maxTime, MeReal* t);

#ifdef __cplusplus
}
#endif

#endif
