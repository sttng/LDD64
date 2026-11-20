/* Vortex 5.0.0 build 800 "vortexldd41" - 3x3 symmetric eigen decomposition by cyclic Jacobi
 * rotations (restored; Mac 557d 0x4d1b94-0x4d1e3e, Win 0x7c0cf0 / 0x7c0ea0).
 * Used by the Vx inertia code (Mac TU 491 0x45856a / Win 0x7e31e0, TU 503). Names (name inferred). */
#include "Vinc/VincTriangleMeshContacts.h"

#include <math.h>

/* Mac 0x4d1b94 (Win: inlined): columns p, q of m rotated by (c, s) */
static void JacobiRotateColumns(MeReal* m, int p, int q, MeReal c, MeReal s)
{
    MeReal a0 = m[p], b0 = m[q], a1 = m[3 + p], b1 = m[3 + q], a2 = m[6 + p], b2 = m[6 + q];
    m[p] = a0 * c - b0 * s;
    m[3 + p] = a1 * c - b1 * s;
    m[6 + p] = a2 * c - b2 * s;
    m[q] = a0 * s + c * b0;
    m[3 + q] = s * a1 + c * b1;
    m[6 + q] = s * a2 + c * b2;
}

/* Mac 0x4d1c72 (Win: inlined): rows p, q of m rotated by (c, s) */
static void JacobiRotateRows(MeReal* m, int p, int q, MeReal c, MeReal s)
{
    MeReal* rp = m + p * 3;
    MeReal* rq = m + q * 3;
    MeReal a0 = rp[0], b0 = rq[0], a1 = rp[1], b1 = rq[1], a2 = rp[2], b2 = rq[2];
    rp[0] = a0 * c - b0 * s;
    rp[1] = a1 * c - b1 * s;
    rp[2] = a2 * c - b2 * s;
    rq[0] = a0 * s + c * b0;
    rq[1] = s * a1 + c * b1;
    rq[2] = s * a2 + c * b2;
}

/* Mac 0x4d1d4b, Win 0x7c0cf0 */
void VincJacobiRotation(const MeReal* a, int p, int q, MeReal* s, MeReal* c)
{
    MeReal apq, theta, t, inv;
    *c = 1.0;
    *s = 0.0;
    apq = a[p * 3 + q];
    if (apq != 0.0) {
        theta = ((a[q * 4] - a[p * 4]) * 0.5) / apq;
        if (theta <= 0.0)
            t = -1.0 / (sqrt(theta * theta + 1.0) - theta);
        else
            t = 1.0 / (sqrt(theta * theta + 1.0) + theta);
        inv = 1.0 / sqrt(t * t + 1.0);
        *c = inv;
        *s = t * inv;
    }
}

/* Mac 0x4d1e3e, Win 0x7c0ea0 */
int VincJacobi3(MeReal* a, MeReal* v, int maxSweeps)
{
    int sweep, p, q;
    for (sweep = 0; 0.001 < sqrt(a[3] * a[3] + a[6] * a[6] + a[7] * a[7]) && sweep < maxSweeps; ++sweep) {
        for (p = 0; p < 3; ++p) {
            for (q = 0; q < p; ++q) {
                MeReal c = 1.0, s = 0.0;
                VincJacobiRotation(a, p, q, &s, &c);
                JacobiRotateColumns(v, p, q, c, s);
                JacobiRotateColumns(a, p, q, c, s);
                JacobiRotateRows(a, p, q, c, s);
            }
        }
    }
    return sweep;
}
