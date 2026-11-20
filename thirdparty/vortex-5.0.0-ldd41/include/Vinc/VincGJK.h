/* Vortex 5.0.0 build 800 "vortexldd41" - GJK distance between two convex geometries through their
 * maximum-point (support) functions (restored; src/Vinc/VincGJK.c,
 * Mac 0x4f90c0-0x4f9d8a, 558a). All names inferred. All cdecl on Win.
 * The simplex holds support point pairs (p[i] on A, q[i] on B; Minkowski point q[i] - p[i]);
 * Johnson's sub-algorithm (tables in VincGJK.c) reduces it to the smallest sub-simplex that
 * contains the point closest to the origin. */
#ifndef VINC_VINCGJK_H
#define VINC_VINCGJK_H

#include "Vinc/VincModel.h"
#include "Vinc/VincGeometry.h"

#ifdef __cplusplus
extern "C" {
#endif

/*                                    Mac    Win */
typedef struct VincGJKLambda {
    int count;                     /* +0x00 +0x00 points in the simplex */
    MeReal lambda[5];              /* +0x04 +0x08 barycentric weights ([4]: unused) */
} VincGJKLambda;                   /* Mac 0x2c, Win 0x30 */

typedef struct VincGJKSimplex {
    MeVector3 p[4];                /* +0x00 support points on A */
    MeVector3 q[4];                /* +0x60 support points on B */
    MeReal m_c0;                   /* +0xc0 set to 1e-8 when the simplex is full (never read here) */
} VincGJKSimplex;                  /* 0xc8 */

/* Mac 0x77ce78 / Win 0xafcf00: the closest points of the last VincGJK run (A, B) */
extern MeReal VincGJKLastClosest[2][3];

/* Mac 0x4f9c5a, Win 0x81f040: the start points: dir = (B's origin - A's origin) unless useDir
 * (then *dir is used as is and not written); pointA = support of A along dir, pointB = support
 * of B along -dir */
void VincGJKInit(VincModelID a, VincModelID b, MeReal* dir, int useDir, MeReal* pointA, MeReal* pointB);

/* Mac 0x4f928d, Win 0x81f100: at most 40 GJK iterations. On return pointA / pointB / dir (each
 * may be NULL) hold the closest points / their difference of the last improving iteration and
 * *distance2 (may be NULL) the squared distance. Returns 1 when the distance fell below 1e-8
 * (touching / penetrating), the simplex filled up or the iterations ran out; 0 when separation
 * was proven (and, with distance2 == NULL, as soon as a separating direction is found). */
int VincGJK(VincGeometryID ga, VincGeometryID gb, MeReal* tmA, MeReal* tmB, MeReal* dir, MeReal* pointA,
            MeReal* pointB, VincGJKLambda* lambda, VincGJKSimplex* simplex, MeReal* distance2);

/* Mac 0x4f9d8a, Win 0x81f3b0: 1 when the geometries of a and b touch (GJK distance < 1e-8);
 * unused: not read (the callers pass 0); pointA / pointB: closest points (may be NULL) */
int VincGJKIntersect(VincModelID a, VincModelID b, void* unused, MeReal* pointA, MeReal* pointB);

#ifdef __cplusplus
}
#endif

#endif
