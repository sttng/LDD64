/* Vortex 5.0.0 build 800 "vortexldd41" - a planar convex polygon against the vertical cylinder
 * x^2 + y^2 <= r^2 (restored; src/Vinc/VincPolygonCircle.c). Name inferred. */
#ifndef VINC_VINCPOLYGONCIRCLE_H
#define VINC_VINCPOLYGONCIRCLE_H

#include "Me/MeMath.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac 0x4fa415, Win 0x80abb0 (cdecl): poly = *n points (x, y, z), n >= 3. Writes up to *maxPoints
 * points to out (the polygon edges clipped to the circle; with exactly two of them, one or two
 * circle points beyond that edge; with none and the z axis inside the polygon, the four points
 * (+-r, 0), (0, +-r)), each lifted to the polygon's plane, and their number to *count (not
 * written when n < 3). NOTE: the extra points ignore *maxPoints (out must hold 4 points).
 * Returns *count > 0. */
int VincPolygonCircleIntersect(const MeReal* poly, const int* n, const MeReal* r, const int* maxPoints, MeReal* out,
                               int* count);

#ifdef __cplusplus
}
#endif

#endif
