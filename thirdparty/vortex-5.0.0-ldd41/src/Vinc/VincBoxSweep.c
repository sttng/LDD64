/* Vortex 5.0.0 build 800 "vortexldd41" - earliest time two linearly moving oriented boxes can touch,
 * by the 15 separating axes (restored). Mac 0x4faa70 (558a), Win 0x8517a0.
 * Caller: VincEllipsoidSafeTime (Mac 0x4dc9ee, Win 0x81c740; V3.4). Name inferred. Mac 0x4faa54
 * (empty) / 0x4faa59 (calls it, via the module initialiser table) are this C++ file's compiler
 * static-initialisation stubs: nothing to restore. */
#include "Vinc/VincBoxSweep.h"

#include <math.h>

/* one axis: s0 / s1 = the centre distance along it at time 0 / maxTime, r = the radius sum.
 * Returns 0 when the boxes stay separated on this axis over the whole interval; otherwise raises
 * *t to the time the axis stops separating them (first: only when that time is > 0) */
static int Axis(MeReal s0, MeReal s1, MeReal r, MeReal maxTime, MeReal* t, int first)
{
    MeReal u;
    if (s0 <= r) {
        if (!(s0 < -r))
            return 1; /* overlapping on this axis at time 0 */
        if (s1 < -r)
            return 0;
        u = -(((s0 + r) * maxTime) / (s1 - s0));
    } else {
        if (r < s1)
            return 0;
        u = ((r - s0) * maxTime) / (s1 - s0);
    }
    if (first ? 0.0 < u : *t < u)
        *t = u;
    return 1;
}

/* Mac 0x4faa70, Win 0x8517a0 */
int VincBoxSweepTime(const MeReal* radiiA, const MeReal* tmA, const MeReal* velA, const MeReal* radiiB,
                     const MeReal* tmB, const MeReal* velB, MeReal maxTime, MeReal* t)
{
    const MeReal *a0 = tmA, *a1 = tmA + 4, *a2 = tmA + 8;
    const MeReal *b0 = tmB, *b1 = tmB + 4, *b2 = tmB + 8;
    MeReal dx = tmB[12] - tmA[12], dy = tmB[13] - tmA[13], dz = tmB[14] - tmA[14];
    MeReal ex = (velB[0] - velA[0]) * maxTime + dx;
    MeReal ey = (velB[1] - velA[1]) * maxTime + dy;
    MeReal ez = (velB[2] - velA[2]) * maxTime + dz;
    MeReal c00, c01, c02, c10, c11, c12, c20, c21, c22;
    MeReal f00, f01, f02, f10, f11, f12, f20, f21, f22;
    MeReal s0, s1, s2, e0, e1, e2;
    *t = 0.0;

    /* A's axes */
    c00 = a0[0] * b0[0] + a0[1] * b0[1] + a0[2] * b0[2];
    c01 = a0[0] * b1[0] + a0[1] * b1[1] + a0[2] * b1[2];
    c02 = a0[0] * b2[0] + a0[1] * b2[1] + a0[2] * b2[2];
    s0 = dx * a0[0] + dy * a0[1] + dz * a0[2];
    e0 = ex * a0[0] + ey * a0[1] + ez * a0[2];
    f00 = fabs(c00);
    f01 = fabs(c01);
    f02 = fabs(c02);
    if (!Axis(s0, e0, f00 * radiiB[0] + f01 * radiiB[1] + f02 * radiiB[2] + radiiA[0], maxTime, t, 1))
        return 0;
    c10 = a1[0] * b0[0] + a1[1] * b0[1] + a1[2] * b0[2];
    c11 = a1[0] * b1[0] + a1[1] * b1[1] + a1[2] * b1[2];
    c12 = a1[0] * b2[0] + a1[1] * b2[1] + a1[2] * b2[2];
    s1 = dx * a1[0] + dy * a1[1] + dz * a1[2];
    e1 = ex * a1[0] + ey * a1[1] + ez * a1[2];
    f10 = fabs(c10);
    f11 = fabs(c11);
    f12 = fabs(c12);
    if (!Axis(s1, e1, f10 * radiiB[0] + f11 * radiiB[1] + f12 * radiiB[2] + radiiA[1], maxTime, t, 0))
        return 0;
    c20 = a2[0] * b0[0] + a2[1] * b0[1] + a2[2] * b0[2];
    c21 = a2[0] * b1[0] + a2[1] * b1[1] + a2[2] * b1[2];
    c22 = a2[0] * b2[0] + a2[1] * b2[1] + a2[2] * b2[2];
    s2 = dx * a2[0] + dy * a2[1] + dz * a2[2];
    e2 = ex * a2[0] + ey * a2[1] + ez * a2[2];
    f20 = fabs(c20);
    f21 = fabs(c21);
    f22 = fabs(c22);
    if (!Axis(s2, e2, f20 * radiiB[0] + f21 * radiiB[1] + f22 * radiiB[2] + radiiA[2], maxTime, t, 0))
        return 0;

    /* B's axes */
    if (!Axis(dx * b0[0] + dy * b0[1] + dz * b0[2], ex * b0[0] + ey * b0[1] + ez * b0[2],
              f00 * radiiA[0] + f10 * radiiA[1] + f20 * radiiA[2] + radiiB[0], maxTime, t, 0))
        return 0;
    if (!Axis(dx * b1[0] + dy * b1[1] + dz * b1[2], ex * b1[0] + ey * b1[1] + ez * b1[2],
              f01 * radiiA[0] + f11 * radiiA[1] + f21 * radiiA[2] + radiiB[1], maxTime, t, 0))
        return 0;
    if (!Axis(dx * b2[0] + dy * b2[1] + dz * b2[2], ex * b2[0] + ey * b2[1] + ez * b2[2],
              f02 * radiiA[0] + f12 * radiiA[1] + f22 * radiiA[2] + radiiB[2], maxTime, t, 0))
        return 0;

    /* A_i x B_j (the radius as two pairs: (A's terms) + (B's terms)) */
    if (!Axis(c10 * s2 - s1 * c20, c10 * e2 - e1 * c20,
              (f20 * radiiA[1] + f10 * radiiA[2]) + (f02 * radiiB[1] + f01 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(c11 * s2 - s1 * c21, c11 * e2 - e1 * c21,
              (f21 * radiiA[1] + f11 * radiiA[2]) + (f02 * radiiB[0] + f00 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(c12 * s2 - s1 * c22, c12 * e2 - e1 * c22,
              (f22 * radiiA[1] + f12 * radiiA[2]) + (f01 * radiiB[0] + f00 * radiiB[1]), maxTime, t, 0))
        return 0;
    if (!Axis(s0 * c20 - c00 * s2, e0 * c20 - c00 * e2,
              (f20 * radiiA[0] + f00 * radiiA[2]) + (f12 * radiiB[1] + f11 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(s0 * c21 - c01 * s2, e0 * c21 - c01 * e2,
              (f21 * radiiA[0] + f01 * radiiA[2]) + (f12 * radiiB[0] + f10 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(s0 * c22 - c02 * s2, e0 * c22 - c02 * e2,
              (f22 * radiiA[0] + f02 * radiiA[2]) + (f11 * radiiB[0] + f10 * radiiB[1]), maxTime, t, 0))
        return 0;
    if (!Axis(c00 * s1 - s0 * c10, c00 * e1 - e0 * c10,
              (f10 * radiiA[0] + f00 * radiiA[1]) + (f22 * radiiB[1] + f21 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(c01 * s1 - s0 * c11, c01 * e1 - e0 * c11,
              (f11 * radiiA[0] + f01 * radiiA[1]) + (f22 * radiiB[0] + f20 * radiiB[2]), maxTime, t, 0))
        return 0;
    if (!Axis(c02 * s1 - s0 * c12, c02 * e1 - e0 * c12,
              (f12 * radiiA[0] + f02 * radiiA[1]) + (f21 * radiiB[0] + f20 * radiiB[1]), maxTime, t, 0))
        return 0;
    return 1;
}
