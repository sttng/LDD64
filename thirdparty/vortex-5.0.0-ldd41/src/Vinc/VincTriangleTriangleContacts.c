/* Vortex 5.0.0 build 800 "vortexldd41" - triangle / triangle intersection points (Moeller's
 * "A Fast Triangle-Triangle Intersection Test" with the intersection segment, plus a coplanar
 * variant that returns the overlap polygon) and the contact merge used by the triangle-mesh pair
 * tests (restored; Mac 0x4cc248, 0x4d6c11 (557d), 0x4d6cd9, 0x4da2f7 (557e);
 * Win 0x767520, 0x7d3020 ...). All names (name inferred). Plain cdecl helpers on Win. */
#include "Vinc/VincTriangleMeshContacts.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* |x| < 1e-14 -> 0 (Moeller's USE_EPSILON_TEST) */
static MeReal FuzzyZero(MeReal x)
{
    return 1e-14 <= fabs(x) ? x : 0.0;
}

/* Mac 0x4d6c11: on the intersection line of the planes n1 . p + d1 = 0 and n2 . p + d2 = 0 with
 * p[i] = t, the coordinate p[j] (k: the third axis); 0 when the line is (nearly) parallel */
int VincTriTriSolveLine(int i, int j, int k, MeReal t, const MeReal* n1, const MeReal* n2, MeReal d1, MeReal d2,
                        MeReal* out)
{
    MeReal b = n2[k], a = n1[k];
    MeReal det = b * n1[j] - a * n2[j];
    int ok = 1e-14 <= fabs(det);
    if (ok)
        *out = ((a * n2[i] - n1[i] * b) * t + (a * d2 - b * d1)) / det;
    return ok;
}

/* append p (3 doubles at pts[*count]) unless it is within 1e-7 of an earlier point */
static void AddUnique(MeReal* pts, int* count)
{
    int n = *count, i;
    const MeReal* p = pts + n * 3;
    for (i = 0; i < n; ++i) {
        const MeReal* q = pts + i * 3;
        if ((q[0] - p[0]) * (q[0] - p[0]) + (q[1] - p[1]) * (q[1] - p[1]) + (q[2] - p[2]) * (q[2] - p[2]) < 1e-14)
            return;
    }
    *count = n + 1;
}

/* coplanar case: edge a0-a1 of the first triangle against edge b0-b1 of the second in the
 * projection (i0, i1); the crossing point is lifted back onto plane n . p + d = 0 (axis i2) */
static void EdgeEdge(const MeReal* a0, const MeReal* a1, const MeReal* b0, const MeReal* b1, int i0, int i1,
                     int i2, const MeReal* n, MeReal d, MeReal* pts, int* count)
{
    MeReal ax = a1[i0] - a0[i0], ay = a1[i1] - a0[i1];
    MeReal bx = b0[i0] - b1[i0], by = b0[i1] - b1[i1];
    MeReal cx = a0[i0] - b0[i0], cy = a0[i1] - b0[i1];
    MeReal f = ay * bx - ax * by;
    MeReal e = by * cx - bx * cy;
    MeReal g;
    if (!(f <= 0.0 || e < 0.0 || f < e)) {
        g = ax * cy - ay * cx;
        if (!(0.0 <= g && g <= f))
            return;
    } else if (f < 0.0 && e <= 0.0 && f <= e) {
        g = ax * cy - ay * cx;
        if (!(g <= 0.0 && f <= g))
            return;
    } else {
        return;
    }
    {
        MeReal x = ax * (e / f) + a0[i0];
        MeReal y = (e / f) * ay + a0[i1];
        MeReal* p = pts + *count * 3;
        p[i0] = x;
        p[i1] = y;
        p[i2] = -(x * n[i0] + y * n[i1] + d) / n[i2];
        AddUnique(pts, count);
    }
}

/* coplanar case: p (projected) strictly inside the triangle a b c */
static int PointInTriangle(const MeReal* p, const MeReal* a, const MeReal* b, const MeReal* c, int i0, int i1)
{
    MeReal px = p[i0], py = p[i1];
    MeReal ax = a[i0], ay = a[i1], bx = b[i0], by = b[i1], cx = c[i0], cy = c[i1];
    MeReal d0 = px * (by - ay) + py * -(bx - ax) + (ax * -(by - ay) - -(bx - ax) * ay);
    MeReal d1 = px * (cy - by) + py * -(cx - bx) + (bx * -(cy - by) - by * -(cx - bx));
    MeReal d2 = px * (ay - cy) + py * -(ax - cx) + (cx * -(ay - cy) - cy * -(ax - cx));
    return 0.0 < d0 * d1 && 0.0 < d0 * d2;
}

static void AddPoint(MeReal* pts, int* count, const MeReal* v)
{
    MeReal* p = pts + *count * 3;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];
    AddUnique(pts, count);
}

static void PushPoint(MeReal* pts, int* count, const MeReal* v)
{
    MeReal* p = pts + *count * 3;
    *count = *count + 1;
    p[0] = v[0];
    p[1] = v[1];
    p[2] = v[2];
}

static void Swap3(MeReal* a, MeReal* b)
{
    MeReal x = a[0], y = a[1], z = a[2];
    a[0] = b[0];
    a[1] = b[1];
    a[2] = b[2];
    b[0] = x;
    b[1] = y;
    b[2] = z;
}

/* Mac 0x4d6cd9 (557e): coplanar triangles v0 v1 v2 / u0 u1 u2 on the plane n . p + d = 0: the
 * edge crossings and the vertices inside the other triangle (deduplicated), more than three
 * sorted around the point with the smallest i1 coordinate; returns 5 (0: no overlap). maxCount is
 * not checked (NOTE: the original's callers pass 6 and a 6-point buffer). */
int VincTriTriCoplanarPoints(MeReal d, const MeReal* n, const MeReal* v0, const MeReal* v1, const MeReal* v2,
                             const MeReal* u0, const MeReal* u1, const MeReal* u2, MeReal* pts, int maxCount,
                             int* count)
{
    int i0, i1, i2, k, m, nPts;
    (void)maxCount;
    if (fabs(n[0]) <= fabs(n[1])) {
        if (fabs(n[1]) < fabs(n[2])) {
            i0 = 0; i1 = 1; i2 = 2;
        } else {
            i0 = 0; i1 = 2; i2 = 1;
        }
    } else if (fabs(n[0]) <= fabs(n[2])) {
        i0 = 0; i1 = 1; i2 = 2;
    } else {
        i0 = 1; i1 = 2; i2 = 0;
    }
    EdgeEdge(v0, v1, u0, u1, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v0, v1, u1, u2, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v0, v1, u2, u0, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v1, v2, u0, u1, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v1, v2, u1, u2, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v1, v2, u2, u0, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v2, v0, u0, u1, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v2, v0, u1, u2, i0, i1, i2, n, d, pts, count);
    EdgeEdge(v2, v0, u2, u0, i0, i1, i2, n, d, pts, count);
    if (*count < 1) {
        /* no edge crossings: one triangle inside the other, or apart */
        if (PointInTriangle(v0, u0, u1, u2, i0, i1)) {
            PushPoint(pts, count, v0);
            PushPoint(pts, count, v1);
            PushPoint(pts, count, v2);
            return 5;
        }
        if (PointInTriangle(u0, v0, v1, v2, i0, i1)) {
            PushPoint(pts, count, u0);
            PushPoint(pts, count, u1);
            PushPoint(pts, count, u2);
            return 5;
        }
        return 0;
    }
    if (PointInTriangle(v0, u0, u1, u2, i0, i1))
        AddPoint(pts, count, v0);
    if (PointInTriangle(v1, u0, u1, u2, i0, i1))
        AddPoint(pts, count, v1);
    if (PointInTriangle(v2, u0, u1, u2, i0, i1))
        AddPoint(pts, count, v2);
    if (PointInTriangle(u0, v0, v1, v2, i0, i1))
        AddPoint(pts, count, u0);
    if (PointInTriangle(u1, v0, v1, v2, i0, i1))
        AddPoint(pts, count, u1);
    if (PointInTriangle(u2, v0, v1, v2, i0, i1))
        AddPoint(pts, count, u2);
    nPts = *count;
    if (3 < nPts) {
        /* order the polygon: lowest point (i1) first, then by the cosine of the direction to it */
        MeReal key[16];
        MeReal low = pts[i1], bx, by;
        int lowest = 0;
        for (k = 1; k < nPts; ++k) {
            if (pts[k * 3 + i1] < low) {
                lowest = k;
                low = pts[k * 3 + i1];
            }
        }
        bx = pts[lowest * 3 + i0];
        by = pts[lowest * 3 + i1];
        if (lowest != 0)
            Swap3(pts, pts + lowest * 3);
        for (k = 1; k < nPts; ++k) {
            MeReal dx = pts[k * 3 + i0] - bx, dy = pts[k * 3 + i1] - by;
            MeReal len = sqrt(dx * dx + dy * dy);
            if (0.0 < len) {
                dx = dx / len;
                dy = dy / len;
            }
            key[k] = dy * 0.0 + dx;
        }
        for (k = 1; k + 1 < nPts; ++k) {
            int sel = k;
            MeReal best = key[k], kk = key[k];
            for (m = k + 1; m < nPts; ++m) {
                if (best < key[m]) {
                    sel = m;
                    best = key[m];
                }
            }
            if (sel != k) {
                key[k] = key[sel];
                key[sel] = kk;
                Swap3(pts + k * 3, pts + sel * 3);
            }
        }
    }
    return 5;
}

/* Moeller's ISECT: the interval end points on the projection axis */
static void Isect(MeReal vv0, MeReal vv1, MeReal vv2, MeReal d0, MeReal d1, MeReal d2, MeReal* a, MeReal* b)
{
    *a = ((vv1 - vv0) * d0) / (d0 - d1) + vv0;
    *b = ((vv2 - vv0) * d0) / (d0 - d2) + vv0;
}

/* Moeller's COMPUTE_INTERVALS (vertex vv0 / distance d0 is the one alone on its side);
 * returns 0 when the triangle lies in the other plane */
static int Interval(MeReal vv0, MeReal vv1, MeReal vv2, MeReal d0, MeReal d1, MeReal d2, MeReal* a, MeReal* b)
{
    if (d0 * d1 <= 0.0) {
        if (d0 * d2 <= 0.0) {
            if (0.0 < d1 * d2 || d0 != 0.0) {
                Isect(vv0, vv1, vv2, d0, d1, d2, a, b);
            } else if (d1 == 0.0) {
                if (d2 == 0.0)
                    return 0;
                Isect(vv2, vv0, vv1, d2, d0, d1, a, b);
            } else {
                Isect(vv1, vv0, vv2, d1, d0, d2, a, b);
            }
        } else {
            Isect(vv1, vv0, vv2, d1, d0, d2, a, b);
        }
    } else {
        Isect(vv2, vv0, vv1, d2, d0, d1, a, b);
    }
    return 1;
}

/* the two end points of the overlap [s, e] on axis i: pts[0][i] = s, pts[1][i] = e and the
 * other coordinates on the planes' intersection line */
static int SegmentPoints(int i, int j, int k, MeReal s, MeReal e, const MeReal* n1, const MeReal* n2, MeReal d1,
                         MeReal d2, MeReal* pts)
{
    MeReal x;
    pts[i] = s;
    pts[3 + i] = e;
    if (!VincTriTriSolveLine(i, j, k, s, n1, n2, d1, d2, &x))
        return 0;
    pts[j] = x;
    if (!VincTriTriSolveLine(i, k, j, s, n1, n2, d1, d2, &x))
        return 0;
    pts[k] = x;
    if (!VincTriTriSolveLine(i, j, k, e, n1, n2, d1, d2, &x))
        return 0;
    pts[3 + j] = x;
    if (!VincTriTriSolveLine(i, k, j, e, n1, n2, d1, d2, &x))
        return 0;
    pts[3 + k] = x;
    return 1;
}

/* Mac 0x4da2f7 (557e), Win 0x7d3020: intersection of the triangles v0 v1 v2 and u0 u1 u2.
 * Returns 0: none; otherwise *count points in pts (3 doubles each): 2: the first triangle's
 * segment lies within the second's, 3: the other way round, 4: the segments overlap partly,
 * 5: coplanar (VincTriTriCoplanarPoints). */
int VincTriTriIntersectPoints(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* u0,
                              const MeReal* u1, const MeReal* u2, MeReal* pts, int maxCount, int* count)
{
    MeReal e1x, e1y, e1z, e2x, e2y, e2z, d1, d2, du0, du1, du2, dv0, dv1, dv2;
    MeReal n1[3], n2[3], dx, dy, mx, vp0, vp1, vp2, up0, up1, up2, a0, a1, b0, b1;
    int index, j, k;
    *count = 0;
    e1x = v1[0] - v0[0];
    e1y = v1[1] - v0[1];
    e1z = v1[2] - v0[2];
    e2x = v2[0] - v0[0];
    e2y = v2[1] - v0[1];
    e2z = v2[2] - v0[2];
    n1[0] = e1y * e2z - e1z * e2y;
    n1[1] = e1z * e2x - e1x * e2z;
    n1[2] = e1x * e2y - e1y * e2x;
    d1 = -(n1[0] * v0[0] + n1[1] * v0[1] + n1[2] * v0[2]);
    du0 = n1[0] * u0[0] + n1[1] * u0[1] + u0[2] * n1[2] + d1;
    du1 = n1[0] * u1[0] + u1[1] * n1[1] + u1[2] * n1[2] + d1;
    du2 = n1[0] * u2[0] + n1[1] * u2[1] + n1[2] * u2[2] + d1;
    du0 = FuzzyZero(du0);
    du1 = FuzzyZero(du1);
    du2 = FuzzyZero(du2);
    if (!(du0 * du1 <= 0.0 || du0 * du2 <= 0.0))
        return 0;
    e1x = u1[0] - u0[0];
    e1y = u1[1] - u0[1];
    e1z = u1[2] - u0[2];
    e2x = u2[0] - u0[0];
    e2y = u2[1] - u0[1];
    e2z = u2[2] - u0[2];
    n2[0] = e1y * e2z - e1z * e2y;
    n2[1] = e1z * e2x - e1x * e2z;
    n2[2] = e1x * e2y - e1y * e2x;
    d2 = -(n2[0] * u0[0] + n2[1] * u0[1] + n2[2] * u0[2]);
    dv0 = n2[0] * v0[0] + n2[1] * v0[1] + n2[2] * v0[2] + d2;
    dv1 = n2[0] * v1[0] + n2[1] * v1[1] + n2[2] * v1[2] + d2;
    dv2 = n2[0] * v2[0] + n2[1] * v2[1] + n2[2] * v2[2] + d2;
    dv0 = FuzzyZero(dv0);
    dv1 = FuzzyZero(dv1);
    dv2 = FuzzyZero(dv2);
    if (!(dv0 * dv1 <= 0.0 || dv0 * dv2 <= 0.0))
        return 0;
    /* the largest component of the direction of the intersection line */
    dx = fabs(n1[1] * n2[2] - n2[1] * n1[2]);
    dy = fabs(n2[0] * n1[2] - n1[0] * n2[2]);
    mx = dx;
    if (dx < dy)
        mx = dy;
    index = 2;
    if (fabs(n2[1] * n1[0] - n2[0] * n1[1]) <= mx)
        index = dx < dy;
    vp0 = v0[index];
    vp1 = v1[index];
    vp2 = v2[index];
    up0 = u0[index];
    up1 = u1[index];
    up2 = u2[index];
    if (!Interval(vp0, vp1, vp2, dv0, dv1, dv2, &a0, &a1))
        return VincTriTriCoplanarPoints(d1, n1, v0, v1, v2, u0, u1, u2, pts, maxCount, count);
    if (!Interval(up0, up1, up2, du0, du1, du2, &b0, &b1))
        return VincTriTriCoplanarPoints(d1, n1, v0, v1, v2, u0, u1, u2, pts, maxCount, count);
    if (a1 < a0) {
        MeReal t = a0;
        a0 = a1;
        a1 = t;
    }
    if (b1 < b0) {
        MeReal t = b0;
        b0 = b1;
        b1 = t;
    }
    if (!(b0 <= a1 && a0 <= b1))
        return 0;
    if (index == 0) {
        j = 2; k = 1;
    } else if (index == 1) {
        j = 2; k = 0;
    } else {
        j = 1; k = 0;
    }
    *count = 2;
    if (a0 < b0 || b1 < a1) {
        if (a0 <= b0) {
            if (b1 <= a1)
                return SegmentPoints(index, k, j, b0, b1, n1, n2, d1, d2, pts) ? 3 : 0;
            if (a1 <= b1)
                return SegmentPoints(index, k, j, a1, b0, n1, n2, d1, d2, pts) ? 4 : 0;
        }
        if (a0 < b0 || a1 < b1)
            return 4;   /* NOTE: only reachable with NaN interval ends: no points written */
        return SegmentPoints(index, k, j, b1, a0, n1, n2, d1, d2, pts) ? 4 : 0;
    }
    return SegmentPoints(index, k, j, a0, a1, n1, n2, d1, d2, pts) ? 2 : 0;
}

/* Mac 0x4cc248, Win 0x767520: the contact just written at contacts[count] is merged into a
 * nearby one (distance^2 < params->distanceSq: same normal within cosTolerance -> the smaller
 * separation, opposite (< -0.9) -> dropped, else averaged) or appended; at the limit the two
 * closest contacts are merged instead (the new one takes the freed slot). Returns 1 when the
 * contact count stays at the limit, 0 otherwise. */
int VincContactsMergeAdd(VincIntersectResult* r, const VincContactMergeParams* params, int limit)
{
    int n = r->contactCount, i, j, a = 0, b = 1;
    VincContact* c = r->contacts;
    VincContact* nc = c + n;
    MeReal best = DBL_MAX, dot;
    for (i = 0; i < n; ++i) {
        VincContact* o = c + i;
        MeReal d = (o->position[0] - nc->position[0]) * (o->position[0] - nc->position[0])
                   + (o->position[1] - nc->position[1]) * (o->position[1] - nc->position[1])
                   + (o->position[2] - nc->position[2]) * (o->position[2] - nc->position[2]);
        if (d < params->distanceSq) {
            dot = o->normal[0] * nc->normal[0] + o->normal[1] * nc->normal[1] + o->normal[2] * nc->normal[2];
            if (params->cosTolerance <= dot) {
                MeReal s = o->separation;
                if (nc->separation <= o->separation)
                    s = nc->separation;
                o->separation = s;
                return 0;
            }
            if (dot < -0.9)
                return 0;
            o->normal[0] = o->normal[0] + nc->normal[0];
            o->normal[1] = o->normal[1] + nc->normal[1];
            o->normal[2] = o->normal[2] + nc->normal[2];
            {
                MeReal x = o->normal[0], y = o->normal[1], z = o->normal[2];
                MeReal len = sqrt(x * x + y * y + z * z);
                if (len != 0.0) {
                    len = 1.0 / len;
                    o->normal[0] = x * len;
                    o->normal[1] = y * len;
                    o->normal[2] = len * z;
                }
            }
            o->separation = (o->separation + nc->separation) * 0.5;
            return 0;
        }
        if (d < best) {
            best = d;
            a = n;
            b = i;
        }
    }
    if (n < limit) {
        r->contactCount = n + 1;
        return 0;
    }
    for (i = 1; i < n; ++i) {
        for (j = 0; j < i; ++j) {
            MeReal d = (c[i].position[0] - c[j].position[0]) * (c[i].position[0] - c[j].position[0])
                       + (c[i].position[1] - c[j].position[1]) * (c[i].position[1] - c[j].position[1])
                       + (c[i].position[2] - c[j].position[2]) * (c[i].position[2] - c[j].position[2]);
            if (d < best) {
                best = d;
                a = j;
                b = i;
            }
        }
    }
    {
        VincContact* kb = c + b;
        VincContact* ka = c + a;
        kb->position[0] = (kb->position[0] + ka->position[0]) * 0.5;
        kb->position[1] = (kb->position[1] + ka->position[1]) * 0.5;
        kb->position[2] = (kb->position[2] + ka->position[2]) * 0.5;
        dot = kb->normal[0] * ka->normal[0] + kb->normal[1] * ka->normal[1] + kb->normal[2] * ka->normal[2];
        if (params->cosTolerance <= dot) {
            MeReal s = kb->separation;
            if (ka->separation <= kb->separation)
                s = ka->separation;
            kb->separation = s;
        } else if (-0.9 <= dot) {
            MeReal x = kb->normal[0] + ka->normal[0];
            MeReal len;
            kb->normal[0] = x;
            kb->normal[1] = kb->normal[1] + ka->normal[1];
            kb->normal[2] = kb->normal[2] + ka->normal[2];
            len = sqrt(x * kb->normal[0] + kb->normal[1] * kb->normal[1] + kb->normal[2] * kb->normal[2]);
            if (len != 0.0) {
                len = 1.0 / len;
                kb->normal[0] = x * len;
                kb->normal[1] = kb->normal[1] * len;
                kb->normal[2] = len * kb->normal[2];
            }
            kb->separation = (kb->separation + ka->separation) * 0.5;
        }
        if (a != n)
            memcpy(ka, nc, sizeof(VincContact));
    }
    return 1;
}

/* ---- boolean triangle / triangle test (Moeller's tri_tri_intersect; the touch-only path of the
 * mesh / mesh pair) ---- */

/* coplanar case of the boolean test: an edge crossing in the projection */
static int EdgeEdgeHit(const MeReal* a0, const MeReal* a1, const MeReal* b0, const MeReal* b1, int i0, int i1)
{
    MeReal ax = a1[i0] - a0[i0], ay = a1[i1] - a0[i1];
    MeReal bx = b0[i0] - b1[i0], by = b0[i1] - b1[i1];
    MeReal cx = a0[i0] - b0[i0], cy = a0[i1] - b0[i1];
    MeReal f = ay * bx - ax * by;
    MeReal e = by * cx - bx * cy;
    MeReal g;
    if (!(f <= 0.0 || e < 0.0 || f < e)) {
        g = ax * cy - ay * cx;
        return 0.0 <= g && g <= f;
    }
    if (f < 0.0 && e <= 0.0 && f <= e) {
        g = ax * cy - ay * cx;
        return g <= 0.0 && f <= g;
    }
    return 0;
}

/* p (projected) inside the triangle a b c (0x4d5531's form: NaN counts as inside) */
static int PointInTriangleB(const MeReal* p, const MeReal* a, const MeReal* b, const MeReal* c, int i0, int i1)
{
    MeReal px = p[i0], py = p[i1];
    MeReal ax = a[i0], ay = a[i1], bx = b[i0], by = b[i1], cx = c[i0], cy = c[i1];
    MeReal d0 = px * (by - ay) + py * -(bx - ax) + (ax * -(by - ay) - -(bx - ax) * ay);
    MeReal d1 = px * (cy - by) + py * -(cx - bx) + (bx * -(cy - by) - by * -(cx - bx));
    MeReal d2 = px * (ay - cy) + py * -(ax - cx) + (cx * -(ay - cy) - cy * -(ax - cx));
    return !(d0 * d1 <= 0.0 || d0 * d2 <= 0.0);
}

/* Mac 0x4d5531, Win 0x7d09a0: coplanar triangles (plane normal n) overlap; 5 / 0 */
int VincTriTriCoplanarOverlap(const MeReal* n, const MeReal* v0, const MeReal* v1, const MeReal* v2,
                              const MeReal* u0, const MeReal* u1, const MeReal* u2)
{
    int i0, i1;
    if (fabs(n[0]) <= fabs(n[1])) {
        if (fabs(n[1]) < fabs(n[2])) {
            i0 = 0; i1 = 1;
        } else {
            i0 = 0; i1 = 2;
        }
    } else if (fabs(n[0]) <= fabs(n[2])) {
        i0 = 0; i1 = 1;
    } else {
        i0 = 1; i1 = 2;
    }
    if (EdgeEdgeHit(v0, v1, u0, u1, i0, i1) || EdgeEdgeHit(v0, v1, u1, u2, i0, i1)
        || EdgeEdgeHit(v0, v1, u2, u0, i0, i1) || EdgeEdgeHit(v1, v2, u0, u1, i0, i1)
        || EdgeEdgeHit(v1, v2, u1, u2, i0, i1) || EdgeEdgeHit(v1, v2, u2, u0, i0, i1)
        || EdgeEdgeHit(v2, v0, u0, u1, i0, i1) || EdgeEdgeHit(v2, v0, u1, u2, i0, i1)
        || EdgeEdgeHit(v2, v0, u2, u0, i0, i1))
        return 5;
    if (PointInTriangleB(v0, u0, u1, u2, i0, i1) || PointInTriangleB(u0, v0, v1, v2, i0, i1))
        return 5;
    return 0;
}

/* Mac 0x4d615c, Win 0x7d1480: the triangles v0 v1 v2 and u0 u1 u2 intersect (or touch) */
int VincTriTriOverlap(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* u0, const MeReal* u1,
                      const MeReal* u2)
{
    MeReal n1[3], n2[3], d1, d2, du0, du1, du2, dv0, dv1, dv2, dx, dy, mx, a0, a1, b0, b1, e1x, e1y, e1z, e2x,
        e2y, e2z;
    int index;
    n1[0] = (v1[1] - v0[1]) * (v2[2] - v0[2]) - (v1[2] - v0[2]) * (v2[1] - v0[1]);
    n1[1] = (v1[2] - v0[2]) * (v2[0] - v0[0]) - (v1[0] - v0[0]) * (v2[2] - v0[2]);
    n1[2] = (v1[0] - v0[0]) * (v2[1] - v0[1]) - (v1[1] - v0[1]) * (v2[0] - v0[0]);
    d1 = -(n1[0] * v0[0] + n1[1] * v0[1] + n1[2] * v0[2]);
    du0 = FuzzyZero(n1[0] * u0[0] + n1[1] * u0[1] + n1[2] * u0[2] + d1);
    du1 = FuzzyZero(n1[0] * u1[0] + n1[1] * u1[1] + n1[2] * u1[2] + d1);
    du2 = FuzzyZero(n1[0] * u2[0] + n1[1] * u2[1] + n1[2] * u2[2] + d1);
    if (!(du0 * du1 <= 0.0 || du0 * du2 <= 0.0))
        return 0;
    e1x = u1[0] - u0[0];
    e1y = u1[1] - u0[1];
    e1z = u1[2] - u0[2];
    e2x = u2[0] - u0[0];
    e2y = u2[1] - u0[1];
    e2z = u2[2] - u0[2];
    n2[0] = e1y * e2z - e1z * e2y;
    n2[1] = e1z * e2x - e1x * e2z;
    n2[2] = e1x * e2y - e1y * e2x;
    d2 = -(u0[0] * n2[0] + u0[1] * n2[1] + u0[2] * n2[2]);
    dv0 = FuzzyZero(v0[0] * n2[0] + v0[1] * n2[1] + v0[2] * n2[2] + d2);
    dv1 = FuzzyZero(n2[0] * v1[0] + n2[1] * v1[1] + n2[2] * v1[2] + d2);
    dv2 = FuzzyZero(n2[0] * v2[0] + n2[1] * v2[1] + n2[2] * v2[2] + d2);
    if (!(dv0 * dv1 <= 0.0 || dv0 * dv2 <= 0.0))
        return 0;
    dx = fabs(n1[1] * n2[2] - n1[2] * n2[1]);
    dy = fabs(n1[2] * n2[0] - n1[0] * n2[2]);
    mx = dx;
    if (dx < dy)
        mx = dy;
    index = dx < dy;
    if (mx < fabs(n1[0] * n2[1] - n1[1] * n2[0]))
        index = 2;
    if (!Interval(v0[index], v1[index], v2[index], dv0, dv1, dv2, &a0, &a1))
        return VincTriTriCoplanarOverlap(n1, v0, v1, v2, u0, u1, u2);
    if (!Interval(u0[index], u1[index], u2[index], du0, du1, du2, &b0, &b1))
        return VincTriTriCoplanarOverlap(n1, v0, v1, v2, u0, u1, u2);
    if (a1 < a0) {
        MeReal t = a0;
        a0 = a1;
        a1 = t;
    }
    if (b1 < b0) {
        MeReal t = b0;
        b0 = b1;
        b1 = t;
    }
    if (b0 <= a1)
        return a0 <= b1;
    return 0;
}

/* ---- OBB / OBB separating axis tests on the mesh trees (Mac 0x4d20a8, 0x4d3204, 0x4d43fd) ----
 * box = {half extents [3], axes [3][3] (rows), centre [3]} (an OBB node from its half extents on),
 * tm = compact 3x4 transform {rotation rows [9], position [3]}. A flat box (a leaf: one triangle)
 * leaves its third half extent out of the axis tests (but not out of the sphere test). Returns 0
 * (overlap), the separating axis 1..15 or 16 (the bounding spheres are apart). The tests run in
 * the order A2, B2, A1, A0, B1, B0, then the edge pairs (as in the original). */
static int OBBDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB, int flatA, int flatB)
{
    const MeReal* R = tmA;
    const MeReal* S = tmB;
    MeReal D[3], A[3][3], B[3][3], Rm[3][3], Ra[3][3], tA[3], tB[3], r, s, d;
    MeReal a0 = a[0], a1 = a[1], a2 = a[2], b0 = b[0], b1 = b[1], b2 = b[2];
    int i;
    D[0] = (S[0] * b[12] + S[3] * b[13] + S[6] * b[14] + S[9]) - (R[0] * a[12] + R[3] * a[13] + a[14] * R[6] + R[9]);
    D[1] = (S[1] * b[12] + S[4] * b[13] + S[7] * b[14] + S[10]) - (R[1] * a[12] + R[4] * a[13] + a[14] * R[7] + R[10]);
    D[2] = (b[12] * S[2] + b[13] * S[5] + b[14] * S[8] + S[11]) - (a[12] * R[2] + a[13] * R[5] + a[14] * R[8] + R[11]);
    r = a0 * a[0] + a1 * a[1] + a2 * a[2] + b0 * b[0] + b1 * b[1] + b2 * b[2];
    if (!(D[0] * D[0] + D[1] * D[1] + D[2] * D[2] <= r + r))
        return 16;
    /* the axes in world space */
    for (i = 0; i < 3; ++i) {
        const MeReal* x = a + 3 + i * 3;
        const MeReal* y = b + 3 + i * 3;
        A[i][0] = R[0] * x[0] + R[3] * x[1] + R[6] * x[2];
        A[i][1] = R[1] * x[0] + R[4] * x[1] + R[7] * x[2];
        A[i][2] = R[2] * x[0] + R[5] * x[1] + R[8] * x[2];
        B[i][0] = S[0] * y[0] + S[3] * y[1] + S[6] * y[2];
        B[i][1] = S[1] * y[0] + S[4] * y[1] + S[7] * y[2];
        B[i][2] = S[2] * y[0] + S[5] * y[1] + S[8] * y[2];
    }
    for (i = 0; i < 3; ++i) {
        int j;
        for (j = 0; j < 3; ++j) {
            Rm[i][j] = A[i][0] * B[j][0] + A[i][1] * B[j][1] + A[i][2] * B[j][2];
            Ra[i][j] = fabs(Rm[i][j]);
        }
        tA[i] = D[0] * A[i][0] + D[1] * A[i][1] + D[2] * A[i][2];
        tB[i] = D[0] * B[i][0] + D[1] * B[i][1] + D[2] * B[i][2];
    }
#define VINC_SEP(code) if (s < d || d < -s) return code
    /* A2 */
    d = tA[2];
    s = b0 * Ra[2][0] + b1 * Ra[2][1];
    if (!flatB) s = s + b2 * Ra[2][2];
    if (!flatA) s = s + a2;
    s = s + 1e-06;
    VINC_SEP(3);
    /* B2 */
    d = tB[2];
    s = a0 * Ra[0][2] + a1 * Ra[1][2];
    if (!flatA) s = s + a2 * Ra[2][2];
    if (!flatB) s = s + b2;
    s = s + 1e-06;
    VINC_SEP(6);
    /* A1 */
    d = tA[1];
    s = b0 * Ra[1][0] + b1 * Ra[1][1];
    if (!flatB) s = s + b2 * Ra[1][2];
    s = s + a1 + 1e-06;
    VINC_SEP(2);
    /* A0 */
    d = tA[0];
    s = b0 * Ra[0][0] + b1 * Ra[0][1];
    if (!flatB) s = s + b2 * Ra[0][2];
    s = s + a0 + 1e-06;
    VINC_SEP(1);
    /* B1 */
    d = tB[1];
    s = a0 * Ra[0][1] + a1 * Ra[1][1];
    if (!flatA) s = s + a2 * Ra[2][1];
    s = s + b1 + 1e-06;
    VINC_SEP(5);
    /* B0 */
    d = tB[0];
    s = a0 * Ra[0][0] + a1 * Ra[1][0];
    if (!flatA) s = s + a2 * Ra[2][0];
    s = s + b0 + 1e-06;
    VINC_SEP(4);
    /* 7: A0 x B0 */
    d = tA[2] * Rm[1][0] - Rm[2][0] * tA[1];
    s = a1 * Ra[2][0];
    if (!flatA) s = s + a2 * Ra[1][0];
    s = s + b1 * Ra[0][2];
    if (!flatB) s = s + b2 * Ra[0][1];
    s = s + 1e-06;
    VINC_SEP(7);
    /* 8: A0 x B1 */
    d = tA[2] * Rm[1][1] - Rm[2][1] * tA[1];
    s = a1 * Ra[2][1];
    if (!flatA) s = s + a2 * Ra[1][1];
    s = s + b0 * Ra[0][2];
    if (!flatB) s = s + b2 * Ra[0][0];
    s = s + 1e-06;
    VINC_SEP(8);
    /* 9: A0 x B2 */
    d = tA[2] * Rm[1][2] - Rm[2][2] * tA[1];
    s = a1 * Ra[2][2];
    if (!flatA) s = s + a2 * Ra[1][2];
    s = s + b0 * Ra[0][1] + b1 * Ra[0][0] + 1e-06;
    VINC_SEP(9);
    /* 10: A1 x B0 */
    d = Rm[2][0] * tA[0] - tA[2] * Rm[0][0];
    s = a0 * Ra[2][0];
    if (!flatA) s = s + a2 * Ra[0][0];
    s = s + b1 * Ra[1][2];
    if (!flatB) s = s + b2 * Ra[1][1];
    s = s + 1e-06;
    VINC_SEP(10);
    /* 11: A1 x B1 */
    d = Rm[2][1] * tA[0] - tA[2] * Rm[0][1];
    s = a0 * Ra[2][1];
    if (!flatA) s = s + a2 * Ra[0][1];
    s = s + b0 * Ra[1][2];
    if (!flatB) s = s + b2 * Ra[1][0];
    s = s + 1e-06;
    VINC_SEP(11);
    /* 12: A1 x B2 */
    d = Rm[2][2] * tA[0] - tA[2] * Rm[0][2];
    s = a0 * Ra[2][2];
    if (!flatA) s = s + a2 * Ra[0][2];
    s = s + b0 * Ra[1][1] + b1 * Ra[1][0] + 1e-06;
    VINC_SEP(12);
    /* 13: A2 x B0 */
    d = Rm[0][0] * tA[1] - Rm[1][0] * tA[0];
    s = a0 * Ra[1][0] + a1 * Ra[0][0] + b1 * Ra[2][2];
    if (!flatB) s = s + b2 * Ra[2][1];
    s = s + 1e-06;
    VINC_SEP(13);
    /* 14: A2 x B1 */
    d = Rm[0][1] * tA[1] - Rm[1][1] * tA[0];
    s = a0 * Ra[1][1] + a1 * Ra[0][1] + b0 * Ra[2][2];
    if (!flatB) s = s + b2 * Ra[2][0];
    s = s + 1e-06;
    VINC_SEP(14);
    /* 15: A2 x B2 */
    d = Rm[0][2] * tA[1] - Rm[1][2] * tA[0];
    s = a0 * Ra[1][2] + a1 * Ra[0][2] + b0 * Ra[2][1] + b1 * Ra[2][0] + 1e-06;
    VINC_SEP(15);
#undef VINC_SEP
    return 0;
}

/* Mac 0x4d3204, Win 0x7d5190 */
int VincOBBDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB)
{
    return OBBDisjointTM(a, tmA, b, tmB, 0, 0);
}

/* Mac 0x4d20a8, Win 0x7d6420: a flat (a leaf) */
int VincOBBFlatDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB)
{
    return OBBDisjointTM(a, tmA, b, tmB, 1, 0);
}

/* Mac 0x4d43fd, Win 0x7d5b40: both flat */
int VincOBBFlatFlatDisjointTM(const MeReal* a, const MeReal* tmA, const MeReal* b, const MeReal* tmB)
{
    return OBBDisjointTM(a, tmA, b, tmB, 1, 1);
}
