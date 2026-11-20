/* Vortex 5.0.0 build 800 "vortexldd41" - a planar polygon against the vertical cylinder x^2 + y^2 <= r^2
 * (restored). Mac 0x4f9ee1-0x4fa415 (558a), Win 0x80a620-0x80abb0. Caller: a
 * cylinder contact generator (Mac 0x4b45ab, Win 0x7b1470). All names (name inferred). */
#include "Vinc/VincPolygonCircle.h"

#include <math.h>

/* Mac 0x4f9ee1, Win 0x80a620: whether the z axis passes through the polygon (crossing test of the
 * origin in the xy plane) */
static int OriginInPolygon(const int* n, const MeReal* poly)
{
    int i, j = *n - 1, inside = 0;
    for (i = 0; i < *n; j = i, ++i) {
        MeReal yi = poly[i * 3 + 1], yj = poly[j * 3 + 1];
        if (((yi <= 0.0 && 0.0 < yj) || (yj <= 0.0 && 0.0 < yi))
            && 0.0 < poly[i * 3] + ((poly[j * 3] - poly[i * 3]) * -yi) / (yj - yi))
            inside ^= 1;
    }
    return inside;
}

/* Mac 0x4f9fc1, Win 0x80a870: plane through a, b, c: normal (b - a) x (c - a), offset normal . a */
static void PlaneFromPoints(const MeReal* a, const MeReal* b, const MeReal* c, MeReal* normal, MeReal* d)
{
    MeReal e1x = b[0] - a[0], e1y = b[1] - a[1], e1z = b[2] - a[2];
    MeReal e2x = c[0] - a[0], e2y = c[1] - a[1], e2z = c[2] - a[2];
    normal[0] = e1y * e2z - e1z * e2y;
    normal[1] = e2x * e1z - e1x * e2z;
    normal[2] = e1x * e2y - e1y * e2x;
    *d = normal[0] * a[0] + normal[1] * a[1] + normal[2] * a[2];
}

/* Mac 0x4fa071, Win 0x80a950: clip the segment seg[0..2] - seg[3..5] to x^2 + y^2 <= r^2 (in place);
 * clipped[0] / clipped[1]: whether the start / end point was moved. 0: the segment misses (or its
 * xy length is below 1e-2) */
static int ClipSegmentToCircle(const MeReal* r, MeReal* seg, int* clipped)
{
    MeReal x0 = seg[0], y0 = seg[1], z0 = seg[2];
    MeReal dx = seg[3] - x0, dy = seg[4] - y0, dz = seg[5] - z0;
    MeReal a = dx * dx + dy * dy, b, disc, s, t1, t2;
    if (a < 0.0001)
        return 0;
    b = x0 * dx + y0 * dy;
    b = b + b;
    disc = b * b - a * 4.0 * ((x0 * x0 + y0 * y0) - *r * *r);
    if (disc < 0.0)
        return 0;
    s = sqrt(disc);
    t1 = (-b - s) / (a + a);
    t2 = (s - b) / (a + a);
    if (t1 < 0.0 && t2 < 0.0)
        return 0;
    if (1.0 < t1 && 1.0 < t2)
        return 0;
    if (t1 <= 0.0 && 1.0 <= t2) { /* inside */
        clipped[0] = 0;
        clipped[1] = 0;
        return 1;
    }
    if (0.0 <= t1 && t2 <= 1.0) {
        clipped[0] = t1 != 0.0;
        clipped[1] = t2 != 1.0;
        seg[0] = dx * t1 + x0;
        seg[1] = dy * t1 + y0;
        seg[2] = dz * t1 + z0;
        seg[3] = dx * t2 + x0;
        seg[4] = dy * t2 + y0;
        seg[5] = dz * t2 + z0;
        return 1;
    }
    if (0.0 < t1) {
        clipped[0] = 1;
        clipped[1] = 0;
        seg[0] = dx * t1 + x0;
        seg[1] = dy * t1 + y0;
        seg[2] = dz * t1 + z0;
    } else {
        clipped[0] = 0;
        clipped[1] = 1;
        seg[3] = dx * t2 + x0;
        seg[4] = dy * t2 + y0;
        seg[5] = dz * t2 + z0;
    }
    return 1;
}

/* Mac 0x4fa370, Win 0x80a8f0: the mean of the n points */
static void Centroid(const int* n, const MeReal* poly, MeReal* c)
{
    int i;
    MeReal inv;
    c[0] = 0.0;
    c[1] = 0.0;
    c[2] = 0.0;
    for (i = 0; i < *n; ++i) {
        c[0] = c[0] + poly[i * 3];
        c[1] = c[1] + poly[i * 3 + 1];
        c[2] = c[2] + poly[i * 3 + 2];
    }
    inv = 1.0 / (MeReal)*n;
    c[0] = c[0] * inv;
    c[1] = c[1] * inv;
    c[2] = inv * c[2];
}

/* z of the polygon's plane at (x, y) */
static MeReal PlaneZ(const MeReal* normal, MeReal d, MeReal x, MeReal y)
{
    /* negated after the division (the sign of a 0 / 0 NaN as in the original); volatile keeps the
     * compiler from moving the negation into the numerator */
    volatile MeReal q = ((normal[0] * x + normal[1] * y) - d) / normal[2];
    return -q;
}

/* Mac 0x4fa415, Win 0x80abb0 */
int VincPolygonCircleIntersect(const MeReal* poly, const int* n, const MeReal* r, const int* maxPoints, MeReal* out,
                               int* count)
{
    MeReal seg[6], c[3], normal[3], d;
    int clipped[2];
    int i, k = 0;
    if (*n < 3)
        return 0;
    for (i = 0; i < *n; ++i) {
        const MeReal* p = poly + i * 3;
        const MeReal* q = poly + ((i + 1) % *n) * 3;
        seg[0] = p[0];
        seg[1] = p[1];
        seg[2] = p[2];
        seg[3] = q[0];
        seg[4] = q[1];
        seg[5] = q[2];
        if (!ClipSegmentToCircle(r, seg, clipped) || !(k < *maxPoints))
            continue;
        if (clipped[0]) {
            out[k * 3] = seg[0];
            out[k * 3 + 1] = seg[1];
            out[k * 3 + 2] = seg[2];
            ++k;
        }
        if (k < *maxPoints) {
            out[k * 3] = seg[3];
            out[k * 3 + 1] = seg[4];
            out[k * 3 + 2] = seg[5];
            ++k;
        }
    }
    if (k == 2) {
        /* one edge crosses the circle: add the circle points beyond it (away from the polygon) */
        MeReal dx = out[3] - out[0], dy = out[4] - out[1], nx, ny, rx, ry;
        Centroid(n, poly, c);
        if (fabs(dx) <= 0.0001) {
            volatile MeReal q; /* negated last: the sign of a NaN as in the original */
            nx = sqrt((dy * dy) / (dx * dx + dy * dy));
            q = (dx / dy) * nx;
            ny = -q;
        } else {
            volatile MeReal q;
            ny = sqrt((dx * dx) / (dy * dy + dx * dx));
            q = (dy / dx) * ny;
            nx = -q;
        }
        if ((c[0] - out[3]) * nx + (c[1] - out[4]) * ny < 0.0) {
            nx = -nx;
            ny = -ny;
        }
        PlaneFromPoints(poly, poly + 3, poly + (*n - 1) * 3, normal, &d);
        rx = *r * nx;
        ry = *r * ny;
        if (!(rx * out[0] + ry * out[1] < 0.0)) { /* (NaN: this branch, as in the original) */
            /* NOTE: with a near-vertical plane (|normal z| <= 1e-4) the third point is counted but
             * not written */
            if (0.0001 < fabs(normal[2])) {
                out[6] = rx;
                out[7] = ny * *r;
                out[8] = PlaneZ(normal, d, out[6], out[7]);
            }
            k = 3;
        } else {
            MeReal ax = rx + out[3], ay = ry + out[4];
            MeReal bx = rx + out[0], by = out[1] + ry;
            MeReal len = sqrt(ax * ax + ay * ay + 0.0 * 0.0);
            if (0.0 != len) {
                MeReal inv = 1.0 / len;
                ax = ax * inv;
                ay = inv * ay;
            }
            len = sqrt(bx * bx + by * by + 0.0 * 0.0);
            if (0.0 != len) {
                MeReal inv = 1.0 / len;
                bx = bx * inv;
                by = by * inv;
            }
            out[6] = *r * ax;
            out[7] = *r * ay;
            out[8] = PlaneZ(normal, d, out[6], out[7]);
            out[9] = *r * bx;
            out[10] = *r * by;
            out[11] = PlaneZ(normal, d, out[9], out[10]);
            k = 4;
        }
    } else if (k == 0 && OriginInPolygon(n, poly)) {
        /* the circle lies inside the polygon: its four axis points */
        PlaneFromPoints(poly, poly + 3, poly + (*n - 1) * 3, normal, &d);
        if (0.0001 < fabs(normal[2])) {
            MeReal rr = *r, mr = -*r;
            out[0] = rr;
            out[1] = 0.0;
            out[2] = PlaneZ(normal, d, rr, 0.0);
            out[3] = 0.0;
            out[4] = rr;
            out[5] = PlaneZ(normal, d, 0.0, rr);
            out[6] = mr;
            out[7] = 0.0;
            out[8] = PlaneZ(normal, d, mr, 0.0);
            out[9] = 0.0;
            out[10] = mr;
            out[11] = PlaneZ(normal, d, 0.0, mr);
            k = 4;
        }
    }
    *count = k;
    return 0 < k;
}
