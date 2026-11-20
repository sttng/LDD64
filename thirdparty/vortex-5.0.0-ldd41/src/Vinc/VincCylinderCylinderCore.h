/* Vortex 5.0.0 build 800 "vortexldd41" - separating-axis test of two cylinders (restored).
 * Included by VincCylinderCylinder.c and VincCylinderCapsule.c: the original
 * has one identical copy per file (Mac 0x4ecc33 / 0x4e807c, 11098 bytes each; Win 0x7de480 /
 * 0x7d8450), each with its own static rim data (Mac 0x7803a8.. / 0x780358.., Win 0xafa198.. /
 * 0xafa148..). Names (name inferred). Rim sines / cosines are multiplied by the radii at once
 * (MeSinMul / MeCosMul: on Win the unrounded x87 results of very large angles enter the products).
 * NOTE: VincCylinderRimObjective / Gradient (defined once, VincCylinderCylinder.c) read that file's
 * rim data; the VincCylinderCapsule.c copy fills its own, never-read data, so a cylinder / capsule
 * rim test minimises over the last cylinder / cylinder rim test's data. Both builds; kept.
 * Frame: the first cylinder's (axis z, radius *r1, half height *hh1); the second (radius *r2,
 * half height *hh2) is given by rel (axis a = rel row 2, centre c = rel row 3). Candidate axes in
 * this order, each replacing the best one when its separation is larger (kind codes):
 *   z (12), a (3), the axis-axis direction (10), cap 2 rim vs side 1 (6), cap 1 rim vs side 2
 *   (9), side-side (8), cap 1 rim vs cap 2 face side (2), rim / rim (best | ib << 2: 0, 1, 4, 5).
 * As soon as a separation exceeds *pad: *sep / *depth are written and 0 is returned. Otherwise
 * *normal (from the second shape to the first, first frame), *dims and the contact points
 * (appended at *cursor) are produced; returns 1 when points were added. */

#include "Vinc/VincCylinderCylinder.h"
#include "Vinc/VincPairSupport.h"
#include "Me/MeCrtMath.h"

#include <float.h>
#include <math.h>

/* the rim / rim search data read by VincCylinderRimObjective / Gradient:
 *   [0], [1] rim 2 centre (x, y) in frame 1   [2..5] rel[0], rel[4], rel[1], rel[5]
 *   [6] r1   [7] r2   [8], [9] rim 2 centre . (rel row 0 / row 1 restricted to x, y) + dz terms */
static MeReal s_cylinderRim[10];

static int VincCylinderCylinderSAT(MeReal* sep, MeReal* normal, MeReal* depth, MeReal** cursor, MeI16* dims,
                                   const MeReal* pad, const MeReal* r1p, const MeReal* hh1p, const MeReal* r2p,
                                   const MeReal* hh2p, const MeReal* rel)
{
    const MeReal* a = rel + 8;
    const MeReal* c = rel + 12;
    MeReal* start = *cursor;
    MeReal a0 = a[0], a1 = a[1], a2 = a[2], absA2, c0, c1, c2;
    MeReal hh1, hh2, r1, r2, padV, best, sign, bestDepth, l2, s, inv, d, sp;
    /* u = (a1, -a0, -0) normalised, w = a x u */
    MeReal ux, uy, uz, w0, w1, w2;
    /* axis a test: v, p */
    MeReal vx, vy, vz, px, py, pz;
    MeReal xyLen2, proj, offset, endZ, hsel;
    /* axis-axis direction (kind 10) */
    MeReal nAx, nAy, nAz;
    /* kind 6 */
    MeReal k6n0 = 0.0, k6n1 = 0.0, k6n2 = 0.0, k6b0 = 0.0, k6b1 = 0.0, k6b2 = 0.0, k6t0 = 0.0, k6t1 = 0.0, k6t2 = 0.0;
    /* kind 9 */
    MeReal k9n0 = 0.0, k9n1 = 0.0, k9n2 = 0.0, k9u0 = 0.0, k9u1 = 0.0, k9v0 = 0.0, k9v1 = 0.0, k9v2 = 0.0;
    /* kind 8 */
    MeReal k8m0 = 0.0, k8m1 = 0.0, k8m2 = 0.0, k8t0 = 0.0, k8t1 = 0.0, k8t2 = 0.0, k8n0 = 0.0, k8n1 = 0.0, k8n2 = 0.0;
    /* kind 2 */
    MeReal k2r0 = 0.0, k2r1 = 0.0, k2r2 = 0.0, k2n0 = 0.0, k2n1 = 0.0, k2n2 = 0.0;
    /* rim / rim */
    MeReal rn0 = 0.0, rn1 = 0.0, rn2 = 0.0, rp0 = 0.0, rp1 = 0.0, rp2 = 0.0;
    MeReal A1[3], B1[3], A2[3], B2[3], e[3];
    unsigned char kind;
    int over;

    /* u = (a1, -a0, -0) normalised; w = a x u */
    ux = a1;
    uy = -a0;
    l2 = ux * ux + uy * uy + 0.0;
    if (!(l2 < 1e-15)) {
        s = sqrt(l2);
        if (s != 0.0) {
            inv = 1.0 / s;
            ux = a1 * inv;
            uy = uy * inv;
            uz = inv * -0.0;
        } else {
            uz = -0.0;
        }
        w0 = a[2] * uy - a[1] * uz;
        w1 = a[0] * uz - a[2] * ux;
        w2 = a[1] * ux - a[0] * uy;
    } else {
        uz = 0.0;
        uy = 1.0;
        ux = 0.0;
        w2 = 0.0;
        w1 = 0.0;
        w0 = 1.0;
    }

    /* (12) the first cylinder's axis z */
    absA2 = fabs(a2);
    hh2 = *hh2p;
    r2 = *r2p;
    hh1 = *hh1p;
    c2 = c[2];
    best = fabs(c2) - (hh1 + (absA2 * hh2 + fabs(w2) * r2));
    sign = c2 <= 0.0 ? 1.0 : -1.0;
    bestDepth = -hh1 - best;
    padV = *pad;
    if (padV < best)
        goto separated;

    /* v = (-a1, a0, 0) normalised, p = (v1, -v0, 0); (rows 1 / 0 of rel when a ~ z) */
    xyLen2 = -a1 * -a1 + a0 * a0 + 0.0;
    if (!(xyLen2 < 1e-15)) {
        s = sqrt(xyLen2);
        if (s != 0.0) {
            inv = 1.0 / s;
            vx = -a1 * inv;
            vy = a0 * inv;
            vz = inv * 0.0;
        } else {
            vz = 0.0;
            vy = a0;
            vx = -a1;
        }
        px = vy;
        py = -vx;
        pz = 0.0;
    } else {
        vx = rel[4];
        vy = rel[5];
        vz = rel[6];
        px = rel[0];
        py = rel[1];
        pz = rel[2];
    }

    /* (3) the second cylinder's axis a */
    r1 = *r1p;
    c0 = c[0];
    c1 = c[1];
    proj = c0 * a0 + c1 * a1 + a2 * c2;
    d = fabs(proj) - (hh2 + (absA2 * hh1 + fabs(px * a0 + py * a1 + a2 * pz) * r1));
    kind = 12;
    if (best < d) {
        sign = proj <= 0.0 ? 1.0 : -1.0;
        bestDepth = hh2 - fabs(proj);
        best = d;
        if (padV < d)
            goto separated;
        kind = 3;
    }
    over = padV < best;

    /* (10) the direction between the axes: (-a1, a0, 0), or (c0, c1, 0) for parallel axes */
    nAx = -a1;
    nAy = a0;
    if (xyLen2 < 1e-15) {
        nAx = c[0];
        nAy = c[1];
    }
    l2 = nAx * nAx + nAy * nAy + 0.0;
    nAz = 0.0;
    if (1e-15 <= l2) {
        s = sqrt(l2);
        nAz = 0.0;
        if (s != 0.0) {
            inv = 1.0 / s;
            nAx = nAx * inv;
            nAy = nAy * inv;
            nAz = inv * 0.0;
        }
        d = c0 * nAx + c1 * nAy + c2 * nAz;
        sp = fabs(d) - (r2 + r1);
        if (best < sp) {
            sign = d <= 0.0 ? 1.0 : -1.0;
            bestDepth = -r1 - sp;
            over = padV < sp;
            best = sp;
            if (padV < sp)
                goto separated;
            kind = 10;
        }
    }

    /* (6) the second cylinder's cap rim nearest to the first cylinder's mid plane against its
     * side */
    offset = proj - a2 * c2;
    if (offset <= 0.0) {
        e[0] = c0 + hh2 * a0;
        e[1] = c1 + hh2 * a1;
        e[2] = a2 * hh2 + c2;
        endZ = hh2 + proj;
    } else {
        e[0] = c0 - hh2 * a0;
        e[1] = c1 - hh2 * a1;
        e[2] = c2 - a2 * hh2;
        endZ = proj - hh2;
    }
    if (1e-8 < absA2) {
        MeReal t0 = 0.0 - e[0], t1 = 0.0 - e[1], t2 = endZ / a2 - e[2];
        s = sqrt(t0 * t0 + t1 * t1 + t2 * t2);
        if (s != 0.0) {
            inv = 1.0 / s;
            t0 = t0 * inv;
            t1 = t1 * inv;
        }
        k6n0 = r2 * t0;
        k6n1 = r2 * t1;
        l2 = k6n0 * k6n0 + k6n1 * k6n1 + 0.0;
        if (1e-15 < l2) {
            s = sqrt(l2);
            if (s != 0.0) {
                inv = 1.0 / s;
                k6n0 = k6n0 * inv;
                k6n1 = k6n1 * inv;
                k6n2 = inv * 0.0;
            } else {
                k6n2 = 0.0;
            }
            k6t0 = k6n2 * a1 - k6n1 * a2;
            k6t1 = k6n0 * a2 - k6n2 * a0;
            k6t2 = k6n1 * a0 - k6n0 * a1;
            l2 = k6t0 * k6t0 + k6t1 * k6t1 + k6t2 * k6t2;
            if (1e-15 < l2) {
                s = sqrt(l2);
                if (s != 0.0) {
                    inv = 1.0 / s;
                    k6t0 = k6t0 * inv;
                    k6t1 = k6t1 * inv;
                    k6t2 = inv * k6t2;
                }
                k6b0 = k6t1 * a2 - k6t2 * a1;
                k6b1 = k6t2 * a0 - k6t0 * a2;
                k6b2 = k6t0 * a1 - k6t1 * a0;
                d = c0 * k6n0 + c1 * k6n1 + c2 * k6n2;
                sp = fabs(d) - (r1 + (hh2 * fabs(k6n0 * a0 + k6n1 * a1 + k6n2 * a2)
                                      + fabs(k6b0 * k6n0 + k6b1 * k6n1 + k6b2 * k6n2) * r2));
                if (best < sp) {
                    sign = d <= 0.0 ? 1.0 : -1.0;
                    bestDepth = -r1 - sp;
                    over = padV < sp;
                    best = sp;
                    if (padV < sp)
                        goto separated;
                    kind = 6;
                }
            }
        } else {
            k6n2 = 0.0;
        }
    }

    /* (9) the first cylinder's cap rim nearest to the second cylinder against its side */
    hsel = (0.0 < endZ * a2 - c2) ? -hh1 : hh1;
    if (1e-8 < absA2) {
        MeReal t = (hsel - c2) / a2, q0, q1, q2, dq;
        q1 = c1 + a1 * t;
        q0 = c0 + a0 * t;
        q2 = (t * a2 + c2) - hsel;
        s = sqrt(q0 * q0 + q1 * q1 + q2 * q2);
        if (s != 0.0) {
            inv = 1.0 / s;
            q0 = q0 * inv;
            q1 = q1 * inv;
            q2 = q2 * inv;
        }
        q0 = q0 * r1;
        q1 = q1 * r1;
        q2 = q2 * r1;
        dq = a0 * q0 + a1 * q1 + a2 * q2;
        k9n0 = q0 - a0 * dq;
        k9n1 = q1 - a1 * dq;
        k9n2 = q2 - dq * a2;
        l2 = k9n0 * k9n0 + k9n1 * k9n1 + k9n2 * k9n2;
        if (1e-15 < l2) {
            s = sqrt(l2);
            if (s != 0.0) {
                inv = 1.0 / s;
                k9n0 = k9n0 * inv;
                k9n1 = k9n1 * inv;
                k9n2 = k9n2 * inv;
            }
            k9v0 = -k9n1;
            l2 = k9v0 * k9v0 + k9n0 * k9n0 + 0.0;
            k9v1 = k9n0;
            k9v2 = 0.0;
            if (1e-15 < l2) {
                s = sqrt(l2);
                k9v2 = 0.0;
                if (s != 0.0) {
                    inv = 1.0 / s;
                    k9v0 = k9v0 * inv;
                    k9v1 = k9n0 * inv;
                    k9v2 = inv * 0.0;
                }
                k9u1 = -k9v0;
                d = c0 * k9n0 + c1 * k9n1 + c2 * k9n2;
                sp = fabs(d) - (r2 + (hh1 * fabs(k9n2) + fabs(k9v1 * k9n0 + k9u1 * k9n1 + k9n2 * 0.0) * r1));
                k9u0 = k9v1;
                if (best < sp) {
                    sign = d <= 0.0 ? 1.0 : -1.0;
                    bestDepth = r2 - fabs(d);
                    over = padV < sp;
                    best = sp;
                    if (padV < sp)
                        goto separated;
                    kind = 9;
                }
            }
        }
    }

    /* (8) side against side: the perpendicular from the second axis' end towards the first axis */
    {
        MeReal m0, m1, dz, len;
        if (offset <= 0.0) {
            m0 = hh2 * a0 + c0;
            m1 = c1 + hh2 * a1;
            proj = hh2 + proj;
        } else {
            m0 = c0 - hh2 * a0;
            m1 = c1 - hh2 * a1;
            proj = proj - hh2;
        }
        dz = a0 * -m0 + a1 * -m1 + a2 * 0.0;
        k8m0 = -m0 - a0 * dz;
        k8m1 = -m1 - a1 * dz;
        k8m2 = 0.0 - dz * a2;
        len = k8m0 * k8m0 + k8m1 * k8m1 + k8m2 * k8m2;
        if (1e-15 < len) {
            s = sqrt(len);
            if (s != 0.0) {
                inv = 1.0 / s;
                k8m0 = k8m0 * inv;
                k8m1 = k8m1 * inv;
                k8m2 = k8m2 * inv;
            }
            k8t0 = a1 * k8m2 - a2 * k8m1;
            k8t1 = a2 * k8m0 - a0 * k8m2;
            k8t2 = a0 * k8m1 - a1 * k8m0;
            k8n0 = m0 + r2 * k8m0;
            k8n1 = r2 * k8m1 + m1;
            k8n2 = 0.0;
            l2 = k8n0 * k8n0 + k8n1 * k8n1 + 0.0;
            if (1e-15 < l2) {
                s = sqrt(l2);
                if (s != 0.0) {
                    inv = 1.0 / s;
                    k8n0 = k8n0 * inv;
                    k8n1 = k8n1 * inv;
                    k8n2 = inv * 0.0;
                }
                d = k8n0 * c[0] + k8n1 * c[1] + k8n2 * c[2];
                sp = fabs(d) - (fabs(k8n0 * a[0] + k8n1 * a[1] + k8n2 * a[2]) * *hh2p
                                + fabs(k8n0 * k8m0 + k8n1 * k8m1 + k8n2 * k8m2) * *r2p + *r1p);
                if (best < sp) {
                    sign = d <= 0.0 ? 1.0 : -1.0;
                    bestDepth = -*r1p - sp;
                    over = *pad < sp;
                    best = sp;
                    if (*pad < sp)
                        goto separated;
                    kind = 8;
                }
            }
        }
    }

    /* (2) the first cylinder's cap rim against the second cylinder's cap face side */
    {
        MeReal hs, dc, w0v, w1v, w2v, dd, len;
        c2 = c[2];
        hs = (proj * a2 - c2 <= 0.0) ? *hh1p : -*hh1p;
        c0 = c[0];
        c1 = c[1];
        dc = c0 * a0 + a1 * c1 + (c2 - hs) * a2;
        k2r0 = c0 - a0 * dc;
        k2r1 = c1 - a1 * dc;
        l2 = k2r0 * k2r0 + k2r1 * k2r1 + 0.0;
        if (1e-15 < l2) {
            s = sqrt(l2);
            k2r2 = 0.0;
            if (s != 0.0) {
                inv = 1.0 / s;
                k2r0 = k2r0 * inv;
                k2r1 = k2r1 * inv;
                k2r2 = inv * 0.0;
            }
            r1 = *r1p;
            w0v = k2r0 * r1 + (0.0 - c0);
            w1v = k2r1 * r1 + (0.0 - c1);
            w2v = r1 * k2r2 + (hs - c2);
            dd = a0 * w0v + a1 * w1v + a2 * w2v;
            k2n2 = w2v - a2 * dd;
            k2n0 = w0v - a0 * dd;
            k2n1 = w1v - dd * a1;
            len = k2n0 * k2n0 + k2n1 * k2n1 + k2n2 * k2n2;
            if (1e-15 < len) {
                s = sqrt(len);
                if (s != 0.0) {
                    inv = 1.0 / s;
                    k2n0 = k2n0 * inv;
                    k2n1 = k2n1 * inv;
                    k2n2 = inv * k2n2;
                }
                d = k2n0 * c[0] + k2n1 * c[1] + k2n2 * c[2];
                sp = fabs(d) - (*r2p + (fabs(k2n2) * *hh1p + fabs(k2n0 * k2r0 + k2n1 * k2r1 + k2n2 * k2r2) * *r1p));
                if (best < sp) {
                    sign = d <= 0.0 ? 1.0 : -1.0;
                    bestDepth = *r2p - fabs(d);
                    over = *pad < sp;
                    best = sp;
                    if (*pad < sp)
                        goto separated;
                    kind = 2;
                }
            }
        }
    }

    /* rim / rim: the rim pair whose centres are closest, then the closest points of the two
     * circles (minimised over both angles) */
    {
        MeReal t, z, gx, gy, dxy, dist, dmin, hsel1, hsel2, zz, sa, pr, x[2], s0, c0a, s1, c1a, P0, P1, P2, len;
        MeReal h2 = *hh2p, h1 = *hh1p, twoH2 = h2 + h2, negH1 = -h1, upper = (h1 + h1) + -h1;
        int i, ib = 0;
        unsigned char ibest = 0;
        t = -h2;
        dmin = DBL_MAX;
        for (i = 0;;) {
            z = c[2] + a[2] * t;
            gy = c[1] + a[1] * t;
            gx = c[0] + a[0] * t;
            dxy = gx * gx + gy * gy;
            dist = (z - negH1) * (z - negH1) + dxy;
            if (dist < dmin) {
                ib = i;
                dmin = dist;
                ibest = 0;
            }
            dist = (z - upper) * (z - upper) + dxy;
            if (dist < dmin) {
                ib = i;
                dmin = dist;
                ibest = 1;
            }
            if (++i == 2)
                break;
            t = t + twoH2;
        }
        s_cylinderRim[1] = gy;
        s_cylinderRim[0] = gx;
        hsel1 = h1 * (MeReal)((signed char)ibest * 2 - 1);
        hsel2 = (MeReal)((signed char)ib * 2 - 1) * h2;
        s_cylinderRim[2] = rel[0];
        s_cylinderRim[3] = rel[4];
        s_cylinderRim[4] = rel[1];
        s_cylinderRim[5] = rel[5];
        s_cylinderRim[6] = *r1p;
        s_cylinderRim[7] = *r2p;
        zz = c[2] + a[2] * hsel2;
        sa = sqrt(1.0 - a2 * a2);
        if (fabs(zz) < fabs(hsel1))
            goto output;
        zz = zz - hsel1;
        if (fabs(zz) < *r2p * sa)
            goto output;
        pr = a[0] * (0.0 - c[0]) + (0.0 - c[1]) * a[1] + (hsel1 - c[2]) * a[2];
        if (fabs(pr) < fabs(hsel2))
            goto output;
        if (fabs(pr - hsel2) < *r1p * sa)
            goto output;
        s_cylinderRim[1] = c[1] + a[1] * hsel2;
        s_cylinderRim[0] = c[0] + a[0] * hsel2;
        s_cylinderRim[8] = s_cylinderRim[2] * s_cylinderRim[0] + s_cylinderRim[4] * s_cylinderRim[1] + zz * rel[2];
        s_cylinderRim[9] = s_cylinderRim[3] * s_cylinderRim[0] + s_cylinderRim[5] * s_cylinderRim[1] + zz * rel[6];
        x[0] = 0.0;
        x[1] = 0.0;
        VincConjugateGradientMinimize(2, x, 0.001, 0.001, VincCylinderRimObjective, VincCylinderRimGradient);
        r1 = *r1p;
        s0 = MeSinMul(x[0], r1);
        c0a = MeCosMul(x[0], r1);
        r2 = *r2p;
        s1 = MeSinMul(x[1], r2);
        c1a = MeCosMul(x[1], r2);
        P2 = hsel2 * a[2] + (s1 * rel[6] + (c1a * rel[2] + c[2]));
        P1 = hsel2 * a[1] + (s1 * rel[5] + (c1a * rel[1] + c[1]));
        P0 = hsel2 * a[0] + (s1 * rel[4] + (c1a * rel[0] + c[0]));
        rn2 = P2 - hsel1;
        rn1 = P1 - s0;
        rn0 = P0 - c0a;
        len = sqrt(rn0 * rn0 + rn1 * rn1 + rn2 * rn2);
        if (len != 0.0) {
            inv = 1.0 / len;
            rn0 = rn0 * inv;
            rn1 = rn1 * inv;
            rn2 = rn2 * inv;
        }
        rp2 = (hsel1 + P2) * 0.5;
        rp1 = (s0 + P1) * 0.5;
        rp0 = (c0a + P0) * 0.5;
        {
            MeReal q0 = -rn1, q1, ext1, ext2, m0, m1, m2, A0 = a[0], A1 = a[1], A2 = a[2];
            l2 = q0 * q0 + rn0 * rn0 + 0.0;
            if (!(l2 < 1e-15)) {
                q1 = rn0;
                if (sqrt(l2) != 0.0) {
                    q1 = 1.0 / sqrt(l2);
                    q0 = q0 * q1;
                    q1 = q1 * rn0;
                }
                ext1 = fabs(rn2) * *hh1p + fabs((q1 * rn0 - rn1 * q0) + rn2 * 0.0) * r1;
            } else {
                ext1 = *hh1p;
            }
            m0 = A1 * rn2 - rn1 * A2;
            m1 = rn0 * A2 - rn2 * A0;
            m2 = rn1 * A0 - A1 * rn0;
            l2 = m0 * m0 + m1 * m1 + m2 * m2;
            if (!(l2 < 1e-15)) {
                if (sqrt(l2) != 0.0) {
                    inv = 1.0 / sqrt(l2);
                    m0 = m0 * inv;
                    m1 = m1 * inv;
                    m2 = m2 * inv;
                }
                ext2 = fabs(rn0 * A0 + A1 * rn1 + rn2 * A2) * *hh2p
                       + r2 * fabs((A2 * m1 - A1 * m2) * rn0 + (A0 * m2 - A2 * m0) * rn1 + (A1 * m0 - A0 * m1) * rn2);
            } else {
                ext2 = *hh2p;
            }
            d = c[0] * rn0 + c[1] * rn1 + c[2] * rn2;
            sp = fabs(d) - (ext1 + ext2);
            if (best < sp) {
                sign = d <= 0.0 ? 1.0 : -1.0;
                kind = (unsigned char)(ibest | (ib << 2));
                bestDepth = ext2 - fabs(d);
                over = *pad < sp;
                best = sp;
            }
        }
    }

output:
    *sep = best;
    *depth = bestDepth;
    if (over)
        return *cursor != start;
    /* the clipping directions: A1 / B1 of the first cylinder (cap diameter / side lines), A2 / B2
     * of the second */
    if (kind == 12) {
        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = sign;
        *dims = 2;
        B2[0] = ux;
        B2[1] = uy;
        B2[2] = uz;
        A2[0] = w0;
        A2[1] = w1;
        A2[2] = w2;
        e[0] = 0.0;
        e[1] = 0.0;
        e[2] = 1.0;
        VincCylinderFrame(A1, B1, e, a, c);
    } else if (kind == 3) {
        normal[0] = sign * a[0];
        normal[1] = sign * a[1];
        normal[2] = sign * a[2];
        *dims = 0x200;
        B1[0] = vx;
        B1[1] = vy;
        B1[2] = vz;
        A1[0] = px;
        A1[1] = py;
        A1[2] = pz;
        e[0] = 0.0;
        e[1] = 0.0;
        e[2] = 1.0;
        VincCylinderFrame(A2, B2, a, e, c);
    } else {
        int hi = (kind & 0xc) >> 2;
        if ((kind & 3) == 2) {
            if (hi == 2) {
                normal[0] = nAx * sign;
                normal[1] = nAy * sign;
                normal[2] = nAz * sign;
                *dims = 0x303;
                e[0] = 0.0;
                e[1] = 0.0;
                e[2] = 1.0;
                VincCylinderFrame(A1, B1, e, a, c);
                e[0] = 0.0;
                e[1] = 0.0;
                e[2] = 1.0;
                VincCylinderFrame(A2, B2, a, e, c);
            } else if (hi == 0) {
                MeReal n2[3];
                A1[0] = k2r0;
                A1[1] = k2r1;
                A1[2] = k2r2;
                B1[0] = -k2r1;
                B1[1] = k2r0;
                B1[2] = 0.0;
                n2[0] = k2n0;
                n2[1] = k2n1;
                n2[2] = k2n2;
                VincCylinderFrame(A2, B2, a, n2, c);
                normal[0] = sign * k2n0;
                normal[1] = sign * k2n1;
                normal[2] = sign * k2n2;
                *dims = 0x301;
            } else {
                A1[0] = k6n0;
                A1[1] = k6n1;
                A1[2] = k6n2;
                B1[0] = -k6n1;
                B1[1] = k6n0;
                B1[2] = 0.0;
                A2[0] = k6b0;
                A2[1] = k6b1;
                A2[2] = k6b2;
                B2[0] = k6t0;
                B2[1] = k6t1;
                B2[2] = k6t2;
                normal[0] = k6n0 * sign;
                normal[1] = k6n1 * sign;
                normal[2] = k6n2 * sign;
                *dims = 0x303;
            }
        } else {
            if (hi != 2) {
                MeReal* p;
                normal[0] = rn0 * sign;
                normal[1] = rn1 * sign;
                normal[2] = rn2 * sign;
                p = *cursor;
                *cursor = p + 3;
                p[0] = rp0;
                p[1] = rp1;
                p[2] = rp2;
                *dims = 0;
                return 1;
            }
            if ((kind & 3) == 0) {
                MeReal n8[3];
                n8[0] = k8n0;
                n8[1] = k8n1;
                n8[2] = k8n2;
                e[0] = 0.0;
                e[1] = 0.0;
                e[2] = 1.0;
                VincCylinderFrame(A1, B1, e, n8, c);
                A2[0] = k8m0;
                A2[1] = k8m1;
                A2[2] = k8m2;
                B2[0] = k8t0;
                B2[1] = k8t1;
                B2[2] = k8t2;
                normal[0] = sign * k8n0;
                normal[1] = sign * k8n1;
                normal[2] = sign * k8n2;
                *dims = 0x103;
            } else {
                A1[0] = k9u0;
                A1[1] = k9u1;
                A1[2] = 0.0;
                B1[0] = k9v0;
                B1[1] = k9v1;
                B1[2] = k9v2;
                A2[0] = k9n0;
                A2[1] = k9n1;
                A2[2] = k9n2;
                B2[0] = k9n2 * a[1] - k9n1 * a[2];
                B2[1] = k9n0 * a[2] - k9n2 * a[0];
                B2[2] = k9n1 * a[0] - k9n0 * a[1];
                normal[0] = k9n0 * sign;
                normal[1] = k9n1 * sign;
                normal[2] = k9n2 * sign;
                *dims = 0x303;
            }
        }
    }
    VincCylinderClipEdges(cursor, rel, r1p, hh1p, A1, B1, 1, r2p, hh2p, A2, B2, 1);
    return *cursor != start;

separated:
    *sep = best;
    *depth = bestDepth;
    return *cursor != start;
}
