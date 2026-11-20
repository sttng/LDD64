/* Vortex 5.0.0 build 800 "vortexldd41" - line segment query against a triangle list (restored).
 * Mac 0x4f527d-0x4f52ad (557g), Win 0x759060-0x759890; plus the segment /
 * triangle test Mac 0x4c8af1 (first function of sub-TU 557d; kept here with its callers 0x4f52ad,
 * 0x4fc3e2) / Win 0x775430. Names (name inferred). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4c8af1, Win 0x775430 (Win splits it: 0x8223b0 projection axes, 0x775340 inside test).
 * Segment p0 + t d (t in [0, 1], t <= *tBest) against the triangle v0 v1 v2 with the given normal;
 * on a hit: hit point, *tBest = t, *distance = t * *len. The inside test projects on the plane of
 * the two coordinates other than the normal's largest one. */
int VincLineSegmentTriangle(const MeReal* v0, const MeReal* v1, const MeReal* v2, const MeReal* normal,
                                  const MeReal* p0, const MeReal* p1, const MeReal* d, const MeReal* len,
                                  MeReal* tBest, MeReal* hit, MeReal* distance)
{
    MeReal nx = normal[0], ny = normal[1], nz = normal[2];
    MeReal nd = nx * d[0] + ny * d[1] + nz * d[2];
    MeReal t, ax, ay, m, a0, b0, e1w, e1u, e2u, e2w, det, s, r;
    MeVector3 q;
    int u, w;
    (void)p1;
    if (!(1e-14 <= fabs(nd)))
        return 0;
    t = -((nx * p0[0] + ny * p0[1] + nz * p0[2]) - (v0[0] * nx + v0[1] * ny + v0[2] * nz)) / nd;
    if (!(t <= *tBest) || !(0.0 <= t) || !(t <= 1.0))
        return 0;
    q[2] = t * d[2] + p0[2];
    q[1] = t * d[1] + p0[1];
    q[0] = t * d[0] + p0[0];
    ax = fabs(nx);
    ay = fabs(ny);
    u = ay < ax;
    m = u ? ax : ay;
    w = 2;
    if (m <= fabs(nz)) {
        u = 0;
        w = 1;
    }
    a0 = v0[u];
    b0 = v0[w];
    e1w = v1[w] - b0;
    e1u = v1[u] - a0;
    e2u = v2[u] - a0;
    e2w = v2[w] - b0;
    det = e2u * e1w - e2w * e1u;
    if (!(1e-14 <= fabs(det)))
        return 0;
    det = 1.0 / det;
    s = (e1w * (q[u] - a0) - e1u * (q[w] - b0)) * det;
    if (!(-1e-14 <= s))
        return 0;
    r = (e2u * (q[w] - b0) - e2w * (q[u] - a0)) * det;
    if (!(-1e-14 <= r) || !(-1e-14 <= (1.0 - s) - r))
        return 0;
    hit[0] = q[0];
    hit[1] = q[1];
    hit[2] = q[2];
    *tBest = t;
    *distance = t * *len;
    return 1;
}

static void HitResult(VincLineSegResult* result, const MeReal* tm, const MeReal* hit, const MeReal* n)
{
    VincTMTransform(tm, hit, result->position);
    VincTMRotate(tm, n, result->normal);
}

/* Mac 0x4f52ad, Win 0x759060: walks the segment through the list's (padded) box in up to 100
 * pieces, asks the line-segment generator for the triangles near each piece and keeps the
 * nearest hit (or the first one when the result's first-hit flag is set). result->element holds the
 * hit triangle's user id (-1: none); when it already names a triangle of this model that triangle
 * is tried first (getTriangle callback). */
int MEAPI VincTriangleListLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1,
                                      VincLineSegResult* result)
{
    VincTriangleList* g = (VincTriangleList*)VincModelGetGeometry(model);
    MeReal* tm = VincModelGetTransformPtr(model);
    MeVector3 a, b, d, u, hit, entry, exitP, bmin, bmax, nrm;
    MeReal len, inv, tBest = 2.0, tA, tB, span, h;
    MeI16 face[2];
    VincModelPair pair;
    int n, k;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    d[2] = b[2] - a[2];
    d[1] = b[1] - a[1];
    d[0] = b[0] - a[0];
    len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
    if (!(1e-14 <= len))
        return 0;
    inv = 1.0 / len;
    u[0] = d[0] * inv;
    u[1] = d[1] * inv;
    u[2] = inv * d[2];
    memset(&pair, 0, sizeof pair); /* NOTE: the original passes an uninitialised pair (only
                                      model2 is set) */
    if (result->model == model && result->element >= 0 && g->getTriangle) {
        int idx = result->element;
        pair.model2 = model;
        if (g->getTriangle(&pair, 1, &idx, g) == 1) {
            VincTriangleListTri* tri = g->triangles;
            if (!VincLineSegmentTriangle(tri->vertices[0], tri->vertices[1], tri->vertices[2], tri->normal, a, b,
                                         d, &len, &tBest, hit, &result->distance)) {
                result->element = -1;
            } else {
                if (result->firstHit > 0) {
                    result->element = tri->userId;
                    HitResult(result, tm, hit, tri->normal);
                    return 1;
                }
                /* continue with the segment shortened to the hit (tBest stays relative to the
                   old segment, as in the original). NOTE: when no triangle of the walk below is
                   hit, the original returns this hit with an uninitialised normal (stack garbage);
                   the restored code returns this triangle's normal instead (UB guard). */
                nrm[0] = tri->normal[0];
                nrm[1] = tri->normal[1];
                nrm[2] = tri->normal[2];
                b[0] = hit[0];
                b[1] = hit[1];
                b[2] = hit[2];
                d[2] = hit[2] - a[2];
                d[1] = hit[1] - a[1];
                d[0] = hit[0] - a[0];
                len = sqrt(d[0] * d[0] + d[1] * d[1] + d[2] * d[2]);
                if (len < 1e-14) {
                    result->element = tri->userId;
                    HitResult(result, tm, hit, tri->normal);
                    return 1;
                }
                inv = 1.0 / len;
                u[0] = d[0] * inv;
                u[1] = d[1] * inv;
                u[2] = inv * d[2];
            }
        }
    }
    tA = 0.0;
    tB = 0.0;
    bmin[2] = g->min[2] - 0.1;
    bmin[1] = g->min[1] - 0.1;
    bmin[0] = g->min[0] - 0.1;
    bmax[2] = g->max[2] + 0.1;
    bmax[1] = g->max[1] + 0.1;
    bmax[0] = g->max[0] + 0.1;
    if (!VincHitBoundingBox(bmin, bmax, a, u, entry, &tA, face) || !(tA <= len))
        return 0;
    u[0] = -u[0];
    u[1] = -u[1];
    u[2] = -u[2];
    VincHitBoundingBox(bmin, bmax, b, u, exitP, &tB, face);
    u[0] = -u[0];
    u[1] = -u[1];
    u[2] = -u[2];
    span = sqrt((entry[2] - exitP[2]) * (entry[2] - exitP[2]) + (entry[1] - exitP[1]) * (entry[1] - exitP[1]) +
                (entry[0] - exitP[0]) * (entry[0] - exitP[0]) + 0.0);
    n = 1;
    if (1.0 < len - (tA + tB)) {
        MeReal step = g->stepLength;
        int c;
        if (step <= 0.0) {
            step = DBL_MAX;
            if (bmax[0] - bmin[0] <= step)
                step = bmax[0] - bmin[0];
            if (bmax[1] - bmin[1] <= step)
                step = bmax[1] - bmin[1];
            if (bmax[2] - bmin[2] <= step)
                step = bmax[2] - bmin[2];
        }
        c = (int)(span / step);
        n = 10;
        if (9 < c) {
            n = 100;
            if (c < 101)
                n = c;
        }
    }
    h = span / (MeReal)n;
    exitP[0] = h * u[0] + entry[0];
    exitP[1] = entry[1] + h * u[1];
    exitP[2] = entry[2] + h * u[2];
    for (k = 0; k < n; ++k) {
        int cnt, i;
        g->triangleCount = 0;
        pair.model2 = model;
        g->lineSegmentGenerator(&pair, entry, exitP, 0);
        cnt = g->triangleCount;
        if (g->triangleMaxCount <= cnt)
            cnt = g->triangleMaxCount;
        for (i = 0; i < cnt; ++i) {
            VincTriangleListTri* tri = &g->triangles[i];
            if (VincLineSegmentTriangle(tri->vertices[0], tri->vertices[1], tri->vertices[2], tri->normal, a, b, d,
                                        &len, &tBest, hit, &result->distance)) {
                result->element = tri->userId;
                nrm[0] = tri->normal[0];
                nrm[1] = tri->normal[1];
                nrm[2] = tri->normal[2];
                if (result->firstHit > 0)
                    break;
            }
        }
        if (tBest <= 1.0) {
            HitResult(result, tm, hit, nrm);
            return 1;
        }
        entry[0] = entry[0] + h * u[0];
        entry[1] = entry[1] + h * u[1];
        entry[2] = entry[2] + h * u[2];
        exitP[0] = exitP[0] + h * u[0];
        exitP[1] = exitP[1] + h * u[1];
        exitP[2] = exitP[2] + h * u[2];
    }
    result->element = -1;
    return 0;
}

/* Mac 0x4f527d, Win 0x759890 */
int MEAPI VincTriangleListRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincTriangleListGetTypeId(), VincTriangleListLineSegment);
    return 1;
}
