/* Vortex 5.0.0 build 800 "vortexldd41" - cylinder / one triangle contact generator (restored,
 * Mac 558e 0x543f1f, 0x54400f, 0x544a59, 0x545a7e; Win 0x7e6830, 0x7e68d0,
 * 0x7e65e0, 0x7e7590, 0x7e76c0). cdecl on Win. All names (name inferred).
 * Cylinder: z axis, radius *r, half height *hh, in the triangle's frame. A separating axis test
 * (triangle normal, caps, triangle edges vs side) picks the normal; the points are the deepest
 * cylinder point in the triangle's prism or the triangle vertices inside the cylinder, the rim
 * crossing the triangle, and the triangle edges clipped against the cylinder (with a deepest-point
 * search). */
#include "Vinc/VincIntersect.h"
#include "Vinc/VincTriangleContacts.h"
#include "VincTriangleClip.h"

#include <math.h>
#include "Me/MeCrtMath.h"

/* the search of EdgeSearch (Win globals 0xac9ad8 / 0xac9adc; constants on the Mac) */
static int s_searchSteps = 4;    /* Win 0xac9ad8 */
static int s_searchHalvings = 5; /* Win 0xac9adc */

/* Mac 0x543f1f, Win 0x7e6830 (register arguments in both builds): the depth of the cylinder at
 * p along -normal (the clip of p - t (r + hh) normal, t in [-1, 1]), at least best; 0 when the
 * ray misses */
static MeReal Depth(const MeReal* p, MeReal best, MeReal hh, MeReal r, const MeReal* normal)
{
    MeReal lo = -1.0, hi = 1.0, ext = hh + r, ray[3], d;
    ray[0] = -normal[0] * ext;
    ray[1] = -normal[1] * ext;
    ray[2] = -normal[2] * ext;
    if (!VincClipSegmentCylinder(&lo, &hi, p, ray, &r, &hh) || hi <= 0.0)
        return 0.0;
    d = -(hi * ext);
    return best < d ? d : best;
}

static void AddDepthPoint(MeReal** points, MeReal** seps, MeReal best, MeReal hh, MeReal r, const MeReal* normal,
                          int* count)
{
    MeReal s = Depth(*points, best, hh, r, normal);
    **seps = s;
    if (s < 0.0) {
        *points += 3;
        ++*seps;
        ++*count;
    }
}

/* Mac 0x54400f, Win 0x7e68d0: the cylinder's rim crossing the triangle: the cap diameters along
 * a and the side lines at +-r a and +-r b (a, b: the radial and tangential directions of the
 * normal); cross = v0 x v1, v1 x v2, v2 x v0, *d = v0 . n. Each crossing point is kept when its
 * depth (Depth) is negative. */
static void RimPoints(MeReal** points, const MeReal* r, const MeReal* hh, const MeReal* a, const MeReal* b,
                      const VincTriangle* tri, const MeReal (*cross)[3], const MeReal* d, MeReal** seps, MeReal best,
                      const MeReal* normal)
{
    const MeReal* n = tri->normal;
    MeReal dn = a[0] * n[0] + a[1] * n[1] + a[2] * n[2];
    MeReal* q;
    int count = 0, neg, m;
    if (dn != 0.0) {
        MeReal c[3], w[3], hz, t;
        for (m = 0; m < 3; ++m) {
            c[m] = a[0] * cross[m][0] + a[1] * cross[m][1] + a[2] * cross[m][2];
            w[m] = (a[0] * tri->edges[m][1] - a[1] * tri->edges[m][0]) * *hh;
        }
        hz = *hh * n[2];
        neg = dn < 0.0;
        if ((c[0] + w[0] < 0.0) == neg && (c[1] + w[1] < 0.0) == neg && (c[2] + w[2] < 0.0) == neg) {
            t = (1.0 / dn) * (*d - hz);
            if (-*r <= t && t <= *r) {
                q = *points;
                q[0] = a[0] * t;
                q[1] = a[1] * t;
                q[2] = a[2] * t;
                q[2] = q[2] + *hh;
                AddDepthPoint(points, seps, best, *hh, *r, normal, &count);
            }
        }
        if ((c[0] - w[0] < 0.0) == neg && (c[1] - w[1] < 0.0) == neg && (c[2] - w[2] < 0.0) == neg) {
            t = (1.0 / dn) * (hz + *d);
            if (-*r <= t && t <= *r) {
                q = *points;
                q[0] = t * a[0];
                q[1] = t * a[1];
                q[2] = t * a[2];
                q[2] = q[2] - *hh;
                AddDepthPoint(points, seps, best, *hh, *r, normal, &count);
            }
        }
        if (count == 2)
            return;
    }
    if (n[2] != 0.0) {
        MeReal inv = 1.0 / n[2], u[3], v[3], da, db, z;
        for (m = 0; m < 3; ++m) {
            const MeReal* e = tri->edges[m];
            u[m] = (a[0] * e[1] - a[1] * e[0]) * *r;
            v[m] = (e[1] * b[0] - e[0] * b[1]) * *r;
        }
        da = (a[0] * n[0] + a[1] * n[1] + n[2] * a[2]) * *r;
        db = (b[0] * n[0] + b[1] * n[1] + n[2] * b[2]) * *r;
        neg = n[2] < 0.0;
#define VINC_RIM_SIDE(s, x, dx, px, py)                                                                     \
    if ((cross[0][2] s x[0] < 0.0) == neg && (cross[1][2] s x[1] < 0.0) == neg &&                           \
        (cross[2][2] s x[2] < 0.0) == neg) {                                                                \
        z = inv * (*d s dx);                                                                                \
        if (-*hh <= z && z <= *hh) {                                                                        \
            q = *points;                                                                                    \
            q[0] = px;                                                                                      \
            q[1] = py;                                                                                      \
            q[2] = z;                                                                                       \
            AddDepthPoint(points, seps, best, *hh, *r, normal, &count);                                     \
        }                                                                                                   \
    }
        VINC_RIM_SIDE(-, u, da, *r * a[0], *r * a[1])
        VINC_RIM_SIDE(+, u, da, -*r * a[0], -*r * a[1])
        VINC_RIM_SIDE(-, v, db, *r * b[0], *r * b[1])
        VINC_RIM_SIDE(+, v, db, -*r * b[0], -*r * b[1])
#undef VINC_RIM_SIDE
    }
}

/* Mac 0x544a59, Win 0x7e65e0: the triangle edge p + t d, t in [*tLo, *tHi], clipped against the
 * cylinder; when the clip cut something off, a step search for the deepest point along the
 * inside part (depth along -normal, scaled by 0.9999) */
static void EdgeSearch(MeReal** points, const MeReal* p, const MeReal* d, const MeReal* r, const MeReal* hh,
                       const MeReal* tLo, const MeReal* tHi, MeReal** seps, MeReal unused, const MeReal* normal)
{
    MeReal lo = *tLo, hi = *tHi, t, step, ext, ray[3], q[3], sLo, sHi, sep;
    int found = 0, halvings = 0, i;
    (void)unused;
    if (!VincClipSegmentCylinder(&lo, &hi, p, d, r, hh) || !(*tLo + 1e-07 < lo || hi < *tHi - 1e-07))
        return;
    **seps = 0.0;
    t = lo + 1e-07;
    step = (1.0 / (double)s_searchSteps) * (hi - lo);
    ext = *r + *hh;
    ray[0] = -normal[0] * ext;
    ray[1] = -normal[1] * ext;
    ray[2] = -normal[2] * ext;
    for (i = 0; halvings < s_searchHalvings && i < 16; ++i) {
        q[0] = t * d[0] + p[0];
        q[1] = t * d[1] + p[1];
        q[2] = t * d[2] + p[2];
        sLo = -1.0;
        sHi = 1.0;
        if (!VincClipSegmentCylinder(&sLo, &sHi, q, ray, r, hh) || sHi <= 0.0 ||
            **seps <= (sep = -(sHi * ext * (double)0.9999f))) {
            if (found) {
                step = step * -0.5;
                ++halvings;
            }
        } else {
            (*points)[0] = q[0];
            (*points)[1] = q[1];
            (*points)[2] = q[2];
            **seps = sep;
            found = 1;
        }
        t = step + t;
        if (t < lo + 1e-07)
            t = lo + 1e-07;
        else if (hi - 1e-07 < t)
            t = hi - 1e-07;
    }
    if (found) {
        ++*seps;
        *points += 3;
    }
}

static void AddVertex(MeReal** points, MeReal** seps, const MeReal* v, MeReal s, MeReal* sep, MeReal* best)
{
    MeReal* q = *points;
    *sep = s;
    *points = q + 3;
    q[0] = v[0];
    q[1] = v[1];
    q[2] = v[2];
    **seps = s;
    ++*seps;
    *best = s;
}

/* the triangle edge a-b (dir e) with its vertices inside the cylinder: the vertices on the side
 * (when both lie on the same side of the axis along e) at their radial depth */
static void SideVertices(MeReal** points, MeReal** seps, const MeReal* a, const MeReal* b, const MeReal* e, MeReal ra,
                         MeReal rb, int inA, int inB, const MeReal* r, MeReal* sep, MeReal* best)
{
    if (0.0 < (a[0] * e[0] + a[1] * e[1]) * (b[0] * e[0] + b[1] * e[1])) {
        MeReal s = ra - *r;
        if (*best < s && inA)
            AddVertex(points, seps, a, ra - *r, sep, best);
        s = rb - *r;
        if (*best < s && inB)
            AddVertex(points, seps, b, rb - *r, sep, best);
    }
}

/* Mac 0x545a7e, Win 0x7e76c0 (see VincTriangleContacts.h) */
int VincCylinderTriangleContacts(MeReal* sep, MeReal* normal, MeReal* out2, MeReal** points, MeI16* dims,
                                 const MeReal* pad, const MeReal* r, const MeReal* hh, const VincTriangle* tri,
                                 MeReal cosTol, int flag, MeReal** seps)
{
    const MeReal* n = tri->normal;
    const MeReal* v0 = tri->vertices[0];
    MeReal tx, ty, tz, rx, l2, best, sign, d, zmin, zmax, mid, capSign, capSep, best2, sign2, depth = 0.0;
    MeReal scale = 1.0, len2[3], a[3], b[3], cross[3][3], dot;
    MeReal* start;
    char hits[3];
    int axis, edgeAxis = -1, i, overlap = 1, special = 0;
    /* the tangential direction of the normal in the xy plane */
    tx = -n[1];
    l2 = tx * tx + n[0] * n[0] + 0.0;
    if (1e-15 <= l2) {
        MeReal len = sqrt(l2);
        tz = 0.0;
        ty = n[0];
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            tx = tx * inv;
            ty = n[0] * inv;
            tz = inv * 0.0;
        }
        rx = -tx;
    } else {
        tz = 0.0;
        ty = 1.0;
        tx = 0.0;
        rx = 0.0;
    }
    /* the triangle's plane */
    d = n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2];
    best = fabs(d) - (fabs(n[2]) * *hh + fabs(ty * n[0] + rx * n[1] + n[2] * 0.0) * *r);
    axis = 0xc;
    sign = 1.0;
    if (*pad < best) {
        overlap = 0;
    } else {
        sign = d <= 0.0 ? 1.0 : -1.0;
        /* the caps */
        zmax = tri->vertices[0][2];
        zmin = tri->vertices[1][2];
        if (zmax <= zmin) {
            MeReal t0 = zmax;
            if (zmax < zmin)
                zmax = zmin;
            zmin = t0;
        }
        if (zmin <= tri->vertices[2][2]) {
            if (zmax < tri->vertices[2][2])
                zmax = tri->vertices[2][2];
        } else {
            zmin = tri->vertices[2][2];
        }
        mid = (zmax + zmin) * 0.5;
        capSign = mid <= 0.0 ? 1.0 : -1.0;
        capSep = fabs(mid) - ((zmax - zmin) * 0.5 + *hh);
        if (*pad < capSep) {
            overlap = 0;
        } else {
            /* the edges against the caps */
            best2 = best;
            sign2 = capSign;
            for (i = 0; i < 3; ++i) {
                const MeReal* e = tri->edges[i];
                const MeReal* vi = tri->vertices[i];
                MeReal tLo = 0.0, tHi = 1.0, dz = e[2], len, lim, z, s;
                int loKind = -1, hiKind = -1, steep;
                len = sqrt(e[0] * e[0] + e[1] * e[1] + dz * dz);
                if (len != 0.0)
                    dz = dz * (1.0 / len);
                steep = fabs(dz) < 0.5 && (fabs(vi[2]) < *hh || fabs(tri->vertices[(i + 1) % 3][2]) < *hh) &&
                        tri->edgeData0[i] == -2.0;
                lim = 0.1;
                if (tri->edgeData0[i] - 1.0 < 0.1)
                    lim = 1.0 - tri->edgeData0[i];
                hits[i] = (char)VincClipSegmentCylinderRange(&tLo, &tHi, vi, e, r, hh, &loKind, &hiKind);
                if (!hits[i])
                    continue;
                if (loKind == 1 && hiKind == 1) {
                    if (best2 < capSep) {
                        sign2 = capSign;
                        best2 = capSep;
                    }
                } else {
                    if (loKind == -1 || loKind == 0) {
                        z = loKind == -1 ? vi[2] : e[2] * tLo + vi[2];
                        s = fabs(z) - *hh;
                        if (best2 < s) {
                            best2 = s;
                            sign2 = 0.0 <= z ? -1.0 : 1.0;
                        }
                    }
                    if (hiKind == -1 || hiKind == 0) {
                        z = hiKind == -1 ? tri->vertices[(i + 1) % 3][2] : e[2] * tHi + vi[2];
                        s = fabs(z) - *hh;
                        if (best2 < s) {
                            best2 = s;
                            sign2 = 0.0 <= z ? -1.0 : 1.0;
                        }
                    }
                }
                if ((steep && sign2 * tri->vertices[(i + 2) % 3][2] < 0.0) ||
                    (fabs(dz) < 0.1 && 0.0 < tri->edgeData1[i] && tri->edgeData0[i] - lim < fabs(n[2]))) {
                    special = 1;
                } else if (best < best2 && 0.5 < fabs(n[2])) {
                    best = best2;
                }
                if (sign < 0.0 && -2.0 < tri->edgeData0[i] && !(tri->edgeData1[i] <= -0.2))
                    sign = 1.0;
            }
            if (special && best2 < 0.0 && best < best2) {
                best = best2;
                sign = sign2;
                axis = 0xb;
            }
            /* the edges against the side */
            for (i = 0; i < 3; ++i) {
                const MeReal* e = tri->edges[i];
                len2[i] = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
            }
            for (i = 0; i < 3; ++i) {
                const MeReal* e = tri->edges[i];
                const MeReal* e2 = tri->edges[(i + 1) % 3];
                const MeReal* vi = tri->vertices[i];
                MeReal q = len2[i] - e[2] * e[2];
                if (*pad * *pad < q) {
                    MeReal h = (e[1] * e2[0] - e[0] * e2[1]) * 0.5;
                    MeReal s = (e[1] * vi[0] - vi[1] * e[0]) + h;
                    MeReal inv = 1.0 / sqrt(q), as = fabs(s) * inv, ah = fabs(h) * inv;
                    MeReal sepE = as - (ah + *r);
                    if (*pad < sepE) {
                        overlap = 0;
                        break;
                    }
                    if (*pad < sepE - best) {
                        MeReal sg = s <= 0.0 ? 1.0 : -1.0, k = sg * inv, accept = 0, reject = 0;
                        MeReal nx = e[1] * k, ny = -e[0] * k, nz = k * 0.0;
                        MeReal ed0 = tri->edgeData0[i];
                        if (ed0 != -2.0) {
                            MeReal lim = 1.0 - ed0, de;
                            if (0.1 <= lim)
                                lim = 0.1;
                            de = (nz * n[1] - ny * n[2]) * e[0] + (nx * n[2] - nz * n[0]) * e[1] +
                                 (ny * n[0] - nx * n[1]) * e[2];
                            if (0.0 < de) {
                                MeReal dn = nz * n[2] + nx * n[0] + ny * n[1];
                                if (ed0 - lim <= dn)
                                    accept = 1;
                                else if (0.5 < dn)
                                    reject = 1;
                            }
                        } else if (0.0 < nx * (e[1] * n[2] - e[2] * n[1]) + -e[0] * k * (e[2] * n[0] - n[2] * e[0]) +
                                             nz * (n[1] * e[0] - e[1] * n[0])) {
                            accept = 1;
                        } else {
                            reject = 1;
                        }
                        if (accept) {
                            axis = i | 8;
                            depth = ah - as;
                            sign = sg;
                            best = sepE;
                            scale = inv;
                        } else if (reject) {
                            best = sepE;
                            edgeAxis = i;
                            if (axis == 0xc && sign == -1.0)
                                sign = 1.0;
                        }
                    }
                }
            }
        }
    }
    *sep = best;
    start = *points;
    if (!overlap)
        return 0;
    if (axis == 0xc) {
        /* the triangle's face */
        MeReal en[3][3];
        b[0] = tx;
        b[1] = ty;
        b[2] = tz;
        a[0] = ty;
        a[1] = rx;
        a[2] = 0.0;
        normal[0] = sign * n[0];
        normal[1] = sign * n[1];
        normal[2] = sign * n[2];
        {
            MeReal p[3];
            p[2] = normal[2] <= 0.0 ? *hh + 0.0 : 0.0 - *hh;
            dot = normal[0] * ty + normal[1] * rx + normal[2] * 0.0;
            if (0.0 <= dot) {
                p[0] = 0.0 - *r * ty;
                p[1] = 0.0 - *r * rx;
                p[2] = p[2] - *r * 0.0;
            } else {
                p[0] = *r * ty + 0.0;
                p[1] = *r * rx + 0.0;
                p[2] = p[2] + *r * 0.0;
            }
            for (i = 0; i < 3; ++i) {
                const MeReal* e = tri->edges[i];
                en[i][0] = n[1] * e[2] - n[2] * e[1];
                en[i][1] = n[2] * e[0] - n[0] * e[2];
                en[i][2] = n[0] * e[1] - n[1] * e[0];
            }
            if (VincTriangleAddPointInPrism(points, seps, p, tri, (const MeReal(*)[3])en)) {
                best = (*seps)[-1];
            } else {
                for (i = 0; i < 3; ++i) {
                    const MeReal* va = tri->vertices[i];
                    const MeReal* vb = tri->vertices[(i + 1) % 3];
                    const MeReal* e = tri->edges[i];
                    MeReal ra = sqrt(va[0] * va[0] + va[1] * va[1]);
                    MeReal rb = sqrt(vb[0] * vb[0] + vb[1] * vb[1]);
                    int inA = ra < *r && fabs(va[2]) < *hh;
                    int inB = rb < *r && fabs(vb[2]) < *hh;
                    if (inA || inB) {
                        MeReal s;
                        SideVertices(points, seps, va, vb, e, ra, rb, inA, inB, r, sep, &best);
                        if (0.0 < va[2] * vb[2]) {
                            s = fabs(va[2]) - *hh;
                            if (best < s && inA)
                                AddVertex(points, seps, va, fabs(va[2]) - *hh, sep, &best);
                            s = fabs(vb[2]) - *hh;
                            if (best < s && inB)
                                AddVertex(points, seps, vb, fabs(vb[2]) - *hh, sep, &best);
                        }
                    } else if (hits[i] && 0.0 < sign) {
                        MeReal tLo = 0.0, tHi = 1.0, t, m[3], ray[3], ext;
                        int k0, k1;
                        if (VincClipSegmentCylinderRange(&tLo, &tHi, va, e, r, hh, &k0, &k1)) {
                            t = (tLo + tHi) * 0.5;
                            m[2] = t * e[2] + va[2];
                            m[1] = t * e[1] + va[1];
                            m[0] = t * e[0] + va[0];
                            ext = -(*r + *hh);
                            ray[2] = ext * n[2];
                            ray[1] = ext * n[1];
                            ray[0] = ext * n[0];
                            tLo = 0.0;
                            tHi = 1.0;
                            if (VincClipSegmentCylinderRange(&tLo, &tHi, m, ray, r, hh, &k0, &k1) && tHi < 1.0) {
                                MeReal s = -tHi * (*r + *hh);
                                if (best < s) {
                                    MeReal* q = *points;
                                    *points = q + 3;
                                    q[0] = m[0];
                                    q[1] = m[1];
                                    q[2] = m[2];
                                    **seps = s;
                                    ++*seps;
                                }
                            }
                        }
                    }
                }
            }
        }
        *out2 = (normal[0] * v0[0] + normal[1] * v0[1] + normal[2] * v0[2]) - best;
        {
            MeReal z2 = normal[2] * normal[2];
            int c = 2;
            if (z2 <= 0.999999)
                c = (z2 < 1e-06) * 2 + 1;
            *dims = (MeI16)(0x200 | c);
        }
    } else if ((axis & 3) == 3) {
        /* a cap */
        MeReal p[3], en[3][3];
        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = sign;
        a[0] = 1.0;
        a[1] = 0.0;
        a[2] = 0.0;
        b[0] = 0.0;
        b[1] = sign;
        b[2] = 0.0;
        *out2 = -sign * *hh;
        *dims = 0;
        p[2] = normal[2] <= 0.0 ? *hh + 0.0 : 0.0 - *hh;
        dot = normal[0] * 1.0 + normal[1] * 0.0 + normal[2] * 0.0;
        if (0.0 <= dot) {
            p[0] = 0.0 - *r * 1.0;
            p[1] = 0.0 - *r * 0.0;
            p[2] = p[2] - *r * 0.0;
        } else {
            p[0] = *r * 1.0 + 0.0;
            p[1] = *r * 0.0 + 0.0;
            p[2] = p[2] + *r * 0.0;
        }
        for (i = 0; i < 3; ++i) {
            const MeReal* e = tri->edges[i];
            en[i][0] = n[1] * e[2] - n[2] * e[1];
            en[i][1] = n[2] * e[0] - n[0] * e[2];
            en[i][2] = n[0] * e[1] - n[1] * e[0];
        }
        if (VincTriangleAddPointInPrism(points, seps, p, tri, (const MeReal(*)[3])en))
            best = (*seps)[-1];
    } else {
        /* a triangle edge against the side */
        const MeReal *e, *va, *vb;
        MeReal ra, rb;
        int inA, inB;
        i = axis & 3;
        e = tri->edges[i];
        a[1] = -e[0] * scale;
        a[0] = scale * e[1];
        a[2] = scale * 0.0;
        normal[0] = sign * a[0];
        normal[1] = sign * a[1];
        normal[2] = sign * a[2];
        b[0] = -a[1];
        b[1] = a[0];
        b[2] = 0.0;
        *out2 = depth;
        *dims = 0x103;
        va = tri->vertices[i];
        vb = tri->vertices[(i + 1) % 3];
        ra = sqrt(va[0] * va[0] + va[1] * va[1]);
        rb = sqrt(vb[0] * vb[0] + vb[1] * vb[1]);
        inA = ra < *r && fabs(va[2]) < *hh;
        inB = rb < *r && fabs(vb[2]) < *hh;
        if (inA || inB)
            SideVertices(points, seps, va, vb, e, ra, rb, inA, inB, r, sep, &best);
        edgeAxis = i;
    }
    /* the rim crossing the triangle */
    {
        const MeReal *p0 = tri->vertices[0], *p1 = tri->vertices[1], *p2 = tri->vertices[2];
        MeReal d0;
        int count;
        cross[0][0] = p0[1] * p1[2] - p0[2] * p1[1];
        cross[0][1] = p0[2] * p1[0] - p1[2] * p0[0];
        cross[0][2] = p1[1] * p0[0] - p0[1] * p1[0];
        cross[1][0] = p1[1] * p2[2] - p1[2] * p2[1];
        cross[1][1] = p1[2] * p2[0] - p2[2] * p1[0];
        cross[1][2] = p2[1] * p1[0] - p1[1] * p2[0];
        cross[2][0] = p2[1] * p0[2] - p2[2] * p0[1];
        cross[2][1] = p2[2] * p0[0] - p0[2] * p2[0];
        cross[2][2] = p0[1] * p2[0] - p2[1] * p0[0];
        d0 = p0[0] * n[0] + p0[1] * n[1] + p0[2] * n[2];
        RimPoints(points, r, hh, a, b, tri, (const MeReal(*)[3])cross, &d0, seps, best, normal);
        count = (int)((*points - start) / 3);
        if (flag != 0 && 0 <= edgeAxis && cosTol < tri->edgeData0[edgeAxis])
            return *points != start;
        for (i = 0; i < 3; ++i) {
            if (edgeAxis == i || tri->edgeData0[i] == -2.0 ||
                (count < 2 && 0.0 <= tri->edgeData1[i] && tri->edgeData0[i] < cosTol)) {
                static const MeReal kLo = 0.0, kHi = 1.0;
                EdgeSearch(points, tri->vertices[i], tri->edges[i], r, hh, &kLo, &kHi, seps, best, normal);
                count = (int)((*points - start) / 3);
            }
        }
    }
    return *points != start;
}
