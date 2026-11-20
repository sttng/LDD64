/* Vortex 5.0.0 build 800 "vortexldd41" - box / one triangle contact generator (restored,
 * Mac 558e 0x542085, 0x5426e3, 0x542d82, 0x5433de, 0x547a3e; Win 0x7e5590,
 * 0x7e5690, 0x7e61a0, 0x7e8fd0, 0x7e94c0, 0x7e9780). cdecl on Win. All names (name inferred).
 * A separating axis test (triangle normal, box faces, 9 edge cross products) picks the normal; the
 * points are the triangle edges clipped against the box (plus a deepest-point search), the box
 * corners inside the triangle's prism and, with fewer than 4 points, the box edges crossing it. */
#include "Vinc/VincIntersect.h"
#include "Vinc/VincTriangleContacts.h"
#include "VincTriangleClip.h"

#include <math.h>
#include "Me/MeCrtMath.h"

/* the search of EdgeSearch (Win globals 0xac9ac8 / 0xac9acc / 0xac9ad0; constants on the Mac) */
static int s_searchSteps = 4;           /* Win 0xac9ac8 */
static int s_searchHalvings = 5;        /* Win 0xac9acc */
static double s_depthScale = 0.9999f;   /* Win 0xac9ad0 (a float constant widened) */

static int Next3(int i)
{
    return (i + 1) % 3;
}

/* Mac 0x542085, Win 0x7e5590: the box corner p when it lies below the triangle's plane (d =
 * (p - v0) . n <= 0) and inside the prism of the triangle (edge normals en): p projected onto
 * the plane goes to *cursor, d to *seps (also used by VincCylinderTriangle.c) */
int VincTriangleAddPointInPrism(MeReal** cursor, MeReal** seps, const MeReal* p, const VincTriangle* tri,
                                const MeReal (*en)[3])
{
    const MeReal* n = tri->normal;
    const MeReal* v = tri->vertices[0];
    MeReal d = (p[0] - v[0]) * n[0] + (p[1] - v[1]) * n[1] + (p[2] - v[2]) * n[2];
    MeReal md, *q;
    int i;
    if (0.0 < d)
        return 0;
    md = -d;
    for (i = 0; i < 3; ++i) {
        v = tri->vertices[i];
        if (((p[0] - v[0]) + n[0] * md) * en[i][0] + ((p[1] - v[1]) + n[1] * md) * en[i][1] +
                ((p[2] - v[2]) + n[2] * md) * en[i][2] <
            0.0)
            return 0;
    }
    q = *cursor;
    q[0] = p[0] - n[0] * d;
    q[1] = p[1] - n[1] * d;
    q[2] = p[2] - n[2] * d;
    *cursor = q + 3;
    **seps = d;
    ++*seps;
    return 1;
}

/* Mac 0x5426e3, Win 0x7e61a0: the triangle edge p + t d, t in [*tLo, *tHi], clipped against the
 * box (radii): its clipped ends with their depths along -dir (the box's extent along dir scaled
 * by 0.9999), and the deepest point found by a step search along the inside part. `unused` is
 * the separation of the caller (not read). */
static void EdgeSearch(MeReal** cursor, MeReal** seps, MeReal unused, const MeReal* p, const MeReal* d,
                       const MeReal* invD, const MeReal* radii, const MeReal* tLo, const MeReal* tHi,
                       const MeReal* dir)
{
    MeReal lo = *tLo, hi = *tHi, ext, ray[3], invRay[3], q[3], sLo, sHi, t, step, sep;
    int found, halvings, i;
    (void)unused;
    ext = radii[0] + radii[1] + radii[2];
    ray[0] = -dir[0] * ext;
    ray[1] = -dir[1] * ext;
    ray[2] = -dir[2] * ext;
    invRay[0] = 1.0 / ray[0];
    invRay[1] = 1.0 / ray[1];
    invRay[2] = 1.0 / ray[2];
    if (!VincClipSegmentBoxRange(&lo, &hi, p, d, invD, radii)) {
        /* NOTE: the whole edge is searched when its start lies strictly inside the box */
        if (radii[0] <= fabs(p[0]) || radii[1] <= fabs(p[1]) || radii[2] <= fabs(p[2]))
            return;
        lo = 0.0;
        hi = 1.0;
    } else {
        int edgeHit = 0;
        if (0.0 < lo) {
            q[0] = lo * d[0] + p[0];
            q[1] = lo * d[1] + p[1];
            q[2] = lo * d[2] + p[2];
            sLo = -1.0;
            sHi = 1.0;
            if (!VincClipSegmentBoxRange(&sLo, &sHi, q, ray, invRay, radii) || sHi <= 1e-14) {
                sep = 0.0;
                edgeHit = 1;
            } else {
                sep = -(sHi * ext * s_depthScale);
            }
            (*cursor)[0] = q[0];
            (*cursor)[1] = q[1];
            (*cursor)[2] = q[2];
            *cursor += 3;
            **seps = sep;
            ++*seps;
        }
        if (hi < *tHi) {
            q[0] = hi * d[0] + p[0];
            q[1] = hi * d[1] + p[1];
            q[2] = hi * d[2] + p[2];
            sLo = -1.0;
            sHi = 1.0;
            if (!VincClipSegmentBoxRange(&sLo, &sHi, q, ray, invRay, radii) || sHi <= 1e-14) {
                (*cursor)[0] = q[0];
                (*cursor)[1] = q[1];
                (*cursor)[2] = q[2];
                *cursor += 3;
                **seps = 0.0;
                ++*seps;
                edgeHit = 1;
            } else {
                (*cursor)[0] = q[0];
                (*cursor)[1] = q[1];
                (*cursor)[2] = q[2];
                *cursor += 3;
                **seps = -(sHi * ext * s_depthScale);
                ++*seps;
            }
        }
        if (!edgeHit)
            return;
    }
    /* step search for the deepest point (1 / steps of the range, the step halved and reversed
     * after each miss once something was found) */
    **seps = 0.0;
    t = lo + 1e-07;
    step = (1.0 / (double)s_searchSteps) * (hi - lo);
    found = 0;
    halvings = 0;
    for (i = 0; halvings < s_searchHalvings && i < 16; ++i) {
        q[0] = t * d[0] + p[0];
        q[1] = t * d[1] + p[1];
        q[2] = t * d[2] + p[2];
        sLo = -1.0;
        sHi = 1.0;
        if (!VincClipSegmentBoxRange(&sLo, &sHi, q, ray, invRay, radii) || sHi <= 0.0 ||
            **seps <= (sep = -(sHi * ext * s_depthScale))) {
            if (found) {
                step = step * -0.5;
                ++halvings;
            }
        } else {
            (*cursor)[0] = q[0];
            (*cursor)[1] = q[1];
            (*cursor)[2] = q[2];
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
        *cursor += 3;
    }
}

/* Mac 0x542d82, Win 0x7e8fd0: the 4 box edges along axis i (j, k the other axes) crossing the
 * triangle (crosses = v0 x v1, v1 x v2, v2 x v0; *d = v0 . n): the crossing points, depth 0 */
static void EdgeAxis(MeReal** cursor, MeReal** seps, const MeReal* r, int i, int j, int k, const VincTriangle* tri,
                     const MeReal (*cross)[3], const MeReal* d)
{
    const MeReal* n = tri->normal;
    MeReal inv = 1.0 / n[i], a[3][3], w[3], nj, nk, t;
    int neg = n[i] < 0.0, m, jj = Next3(i), kk = Next3(jj);
    for (m = 0; m < 3; ++m) {
        w[i] = 0.0;
        w[jj] = tri->edges[m][kk];
        w[kk] = -tri->edges[m][jj];
        a[m][0] = w[0];
        a[m][1] = w[1];
        a[m][2] = w[2];
        a[m][j] = r[j] * a[m][j];
        a[m][k] = r[k] * a[m][k];
    }
    nj = n[j] * r[j];
    nk = n[k] * r[k];
#define VINC_EDGE_AXIS(s1, s2, pj, pk)                                                                     \
    if (((cross[0][i] s1 a[0][j]) s2 a[0][k] < 0.0) == neg && ((cross[1][i] s1 a[1][j]) s2 a[1][k] < 0.0) == neg && \
        ((cross[2][i] s1 a[2][j]) s2 a[2][k] < 0.0) == neg) {                                              \
        t = ((*d s1 nj) s2 nk) * inv;                                                                       \
        if (-r[i] <= t && t <= r[i]) {                                                                     \
            MeReal* q = *cursor;                                                                           \
            q[i] = t;                                                                                      \
            q[j] = pj r[j];                                                                                \
            q[k] = pk r[k];                                                                                \
            *cursor = q + 3;                                                                               \
            **seps = 0.0;                                                                                  \
            ++*seps;                                                                                       \
        }                                                                                                  \
    }
    VINC_EDGE_AXIS(-, -, +, +)
    VINC_EDGE_AXIS(-, +, +, -)
    VINC_EDGE_AXIS(+, +, -, -)
    VINC_EDGE_AXIS(+, -, -, +)
#undef VINC_EDGE_AXIS
}

/* Mac 0x5433de, Win 0x7e94c0: the contact points of the box (radii) and tri; dir = the contact
 * normal (box frame), sep (unused) the separation */
static void BoxTriangleClip(MeReal** cursor, MeReal** seps, const MeReal* radii, const VincTriangle* tri, MeReal sep,
                            const MeReal* dir)
{
    static const MeReal kLo = 0.0, kHi = 1.0;
    MeReal* start = *cursor;
    MeReal inv[3][3], en[3][3], c[3];
    const MeReal* n = tri->normal;
    int m;
    for (m = 0; m < 3; ++m) {
        inv[m][0] = 1.0 / tri->edges[m][0];
        inv[m][1] = 1.0 / tri->edges[m][1];
        inv[m][2] = 1.0 / tri->edges[m][2];
    }
    for (m = 0; m < 3; ++m)
        if (tri->edgeData0[m] < (double)0.9999f)
            EdgeSearch(cursor, seps, sep, tri->vertices[m], tri->edges[m], inv[m], radii, &kLo, &kHi, dir);
    /* Win 0x7e5690: the corners */
    for (m = 0; m < 3; ++m) {
        const MeReal* e = tri->edges[m];
        en[m][0] = n[1] * e[2] - n[2] * e[1];
        en[m][1] = n[2] * e[0] - n[0] * e[2];
        en[m][2] = n[0] * e[1] - n[1] * e[0];
    }
    for (m = 0; m < 8; ++m) {
        c[0] = m & 4 ? -radii[0] : radii[0];
        c[1] = m & 2 ? -radii[1] : radii[1];
        c[2] = m & 1 ? -radii[2] : radii[2];
        VincTriangleAddPointInPrism(cursor, seps, c, tri, (const MeReal(*)[3])en);
    }
    if ((*cursor - start) / 3 < 4) {
        const MeReal *a = tri->vertices[0], *b = tri->vertices[1], *e = tri->vertices[2];
        MeReal cross[3][3], d;
        cross[0][0] = a[1] * b[2] - a[2] * b[1];
        cross[0][1] = a[2] * b[0] - b[2] * a[0];
        cross[0][2] = b[1] * a[0] - a[1] * b[0];
        cross[1][0] = b[1] * e[2] - b[2] * e[1];
        cross[1][1] = b[2] * e[0] - e[2] * b[0];
        cross[1][2] = e[1] * b[0] - b[1] * e[0];
        cross[2][0] = e[1] * a[2] - e[2] * a[1];
        cross[2][1] = e[2] * a[0] - a[2] * e[0];
        cross[2][2] = a[1] * e[0] - e[1] * a[0];
        d = a[0] * n[0] + a[1] * n[1] + a[2] * n[2];
        if (n[0] != 0.0)
            EdgeAxis(cursor, seps, radii, 0, 1, 2, tri, (const MeReal(*)[3])cross, &d);
        if (n[1] != 0.0)
            EdgeAxis(cursor, seps, radii, 1, 2, 0, tri, (const MeReal(*)[3])cross, &d);
        if (n[2] != 0.0)
            EdgeAxis(cursor, seps, radii, 2, 0, 1, tri, (const MeReal(*)[3])cross, &d);
    }
}

static MeI16 FaceDims(const MeReal* an)
{
    return (MeI16)(0x200 | ((an[1] < 0.0001) + (an[0] < 0.0001) + (an[2] < 0.0001)));
}

/* Mac 0x547a3e, Win 0x7e9780 (see VincTriangleContacts.h): *sep = the separation along the
 * chosen axis, normal (box frame, from the triangle to the box), *out2 = the plane offset of a
 * face normal / the depth of an edge normal; the contact points (box frame) and their depths
 * are appended to *points / *seps. Returns 1 when points were added. */
int VincBoxTriangleContact(MeReal* sep, MeReal* normal, MeReal* out2, MeReal** points, MeReal** seps,
                           MeI16* dims, const MeReal* pad, const MeReal* radii, const VincTriangle* tri)
{
    const MeReal* n = tri->normal;
    const MeReal* v0 = tri->vertices[0];
    MeReal* start = *points;
    MeReal padC = *pad < 0.0 ? 0.0 : *pad;
    MeReal an[3], d, best, depth, sign, scale = 1.0, lo[3], ae[3][3], len2[3];
    int axis = 3, i, m;
    an[0] = fabs(n[0]);
    an[1] = fabs(n[1]);
    an[2] = fabs(n[2]);
    d = n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2];
    best = fabs(d) - (an[0] * radii[0] + an[1] * radii[1] + an[2] * radii[2]);
    sign = d <= 0.0 ? 1.0 : -1.0;
    if (*pad < best)
        return 0;
    depth = -fabs(d);
    /* the box faces */
    for (i = 0; i < 3; ++i) {
        MeReal a = tri->vertices[0][i], b = tri->vertices[1][i], c = tri->vertices[2][i], hi = a, mid, s;
        lo[i] = a;
        if (a <= b) {
            if (hi < b)
                hi = b;
        } else {
            lo[i] = b;
        }
        if (lo[i] <= c) {
            if (hi < c)
                hi = c;
        } else {
            lo[i] = c;
        }
        mid = (hi + lo[i]) * 0.5;
        s = fabs(mid) - ((hi - lo[i]) * 0.5 + radii[i]);
        if (best < s) {
            sign = mid <= 0.0 ? 1.0 : -1.0;
            if (*pad < s)
                return 0;
            depth = -radii[i] - s;
            axis = i | 0xc;
            best = s;
        }
    }
    /* the edge / axis cross products */
    for (m = 0; m < 3; ++m) {
        const MeReal* e = tri->edges[m];
        ae[m][0] = fabs(e[0]);
        ae[m][1] = fabs(e[1]);
        ae[m][2] = fabs(e[2]);
        len2[m] = e[0] * e[0] + e[1] * e[1] + e[2] * e[2];
    }
    for (m = 0; m < 3; ++m) {
        const MeReal* e = tri->edges[m];
        const MeReal* e2 = tri->edges[Next3(m)];
        const MeReal* v = tri->vertices[m];
        for (i = 0; i < 3; ++i) {
            int j = Next3(i), k = Next3(j);
            MeReal q = len2[m] - ae[m][i] * ae[m][i];
            if (padC * padC < q) {
                MeReal h = (e2[j] * e[k] - e2[k] * e[j]) * 0.5;
                MeReal ah = fabs(h), s, inv, as, sepE;
                s = h + (e[k] * v[j] - v[k] * e[j]);
                inv = 1.0 / sqrt(q);
                as = fabs(s) * inv;
                sepE = as - (radii[j] * ae[m][k] + radii[k] * ae[m][j] + ah) * inv;
                if (padC < sepE - best) {
                    sign = s <= 0.0 ? 1.0 : -1.0;
                    if (*pad < sepE)
                        return 0;
                    depth = ah * inv - as;
                    axis = m << 2 | i;
                    best = sepE;
                    scale = inv;
                }
            }
        }
    }
    *sep = best;
    if (axis == 3) {
        normal[0] = sign * n[0];
        normal[1] = sign * n[1];
        normal[2] = sign * n[2];
        *out2 = sign * n[0] * v0[0] + sign * n[1] * v0[1] + sign * n[2] * v0[2];
        *dims = FaceDims(an);
    } else if ((axis & 0xc) == 0xc) {
        i = axis & 3;
        normal[0] = 0.0;
        normal[1] = 0.0;
        normal[2] = 0.0;
        normal[i] = sign;
        *out2 = -radii[i] - best;
        *dims = (MeI16)((((tri->vertices[0][i] - lo[i] <= padC) - 1) + (tri->vertices[1][i] - lo[i] <= padC) +
                         (tri->vertices[2][i] - lo[i] <= padC)) * 0x100 | 2);
    } else {
        int j, k;
        MeReal dn, de;
        const MeReal* e;
        m = axis >> 2;
        i = axis & 3;
        j = Next3(i);
        k = Next3(j);
        e = tri->edges[m];
        normal[i] = 0.0;
        if (sign <= 0.0) {
            normal[j] = -e[k];
            normal[k] = e[j];
        } else {
            normal[j] = e[k];
            normal[k] = -e[j];
        }
        *out2 = depth;
        *dims = 0x101;
        normal[0] = scale * normal[0];
        normal[1] = scale * normal[1];
        normal[2] = scale * normal[2];
        dn = n[0] * normal[0] + n[1] * normal[1] + n[2] * normal[2];
        de = (n[1] * normal[2] - n[2] * normal[1]) * e[0] + (n[2] * normal[0] - n[0] * normal[2]) * e[1] +
             (n[0] * normal[1] - n[1] * normal[0]) * e[2];
        if (tri->edgeData0[m] <= dn + 0.0) {
            if (0.0 < dn && de * tri->edgeData1[m] < 0.0 && (0.0 <= tri->edgeData0[m] || 0.0 <= tri->edgeData1[m])) {
                MeReal sg = n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2] <= 0.0 ? 1.0 : -1.0;
                normal[0] = sg * n[0];
                normal[1] = sg * n[1];
                normal[2] = sg * n[2];
                *out2 = (sg * n[0] * v0[0] + sg * n[1] * v0[1] + sg * n[2] * v0[2]) - best;
                *dims = FaceDims(an);
            }
        } else {
            MeReal x, y, z, l;
            if (de * tri->edgeData1[m] < 0.0)
                return 0;
            x = normal[0] - n[0] * dn;
            normal[0] = x;
            y = normal[1] - n[1] * dn;
            normal[1] = y;
            z = normal[2] - n[2] * dn;
            normal[2] = z;
            l = sqrt(x * x + y * y + z * z);
            if (l != 0.0) {
                l = 1.0 / l;
                normal[0] = x * l;
                normal[1] = y * l;
                normal[2] = l * z;
            }
            normal[0] = normal[0] * tri->edgeData1[m];
            normal[1] = normal[1] * tri->edgeData1[m];
            normal[2] = normal[2] * tri->edgeData1[m];
            normal[0] = normal[0] + tri->edgeData0[m] * n[0];
            normal[1] = normal[1] + tri->edgeData0[m] * n[1];
            normal[2] = normal[2] + tri->edgeData0[m] * n[2];
        }
    }
    BoxTriangleClip(points, seps, radii, tri, best, normal);
    return *points != start;
}
