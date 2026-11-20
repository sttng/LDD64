/* Vortex 5.0.0 build 800 "vortexldd41" - box / cylinder pair test (restored).
 * Mac 0x4ddbf5-0x4dff90 (557e), Win 0x7e0b20-0x7e1f00. Names (name inferred). The sums keep the
 * association order of the Win build (bit-exact contract), which differs from the Mac's in places
 * (the Win compiler reassociated some sums; marked "Win order"). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4ddbf5 (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

#define NEXT(i) ((((i) >> 1) + 1 + (i)) & 3)

/* Mac 0x4ddc08, Win 0x7e0b20: the box and the cylinder (as a box r, r, hh) swept */
int MEAPI VincBoxCylinderSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    VincGeometryID g1 = VincModelGetGeometry(pair->model1);
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCylinderGetRadius(g2);
    MeReal hh = VincCylinderGetHalfHeight(g2);
    MeReal* radii = VincBoxGetRadii(g1);
    MeVector3 e2;
    e2[0] = r;
    e2[1] = r;
    e2[2] = hh;
    return VincEllipsoidSafeTime(pair, maxTime, radii, e2, result);
}

/* Mac 0x4ddd33, Win 0x7e0b90 (cdecl): the 12 box edges clipped against the cylinder (radius *r,
 * half height *hh, z axis), in the cylinder's frame (rel = cylinder in the box frame), transformed
 * back to the box frame and appended at *cursor (V3.8's segment / cylinder clippers) */
void VincBoxCylinderEdgePoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r,
                               const MeReal* hh)
{
    MeReal inv[16], local[24 * 3], a[3], b[3], c[3], t[3], p[3], t0, t1;
    MeReal *end = local, *q;
    inv[0] = rel[0];
    inv[1] = rel[4];
    inv[2] = rel[8];
    inv[4] = rel[1];
    inv[5] = rel[5];
    inv[6] = rel[9];
    inv[8] = rel[2];
    inv[9] = rel[6];
    inv[10] = rel[10];
    t[2] = -rel[14];
    t[1] = -rel[13];
    t[0] = -rel[12];
    inv[12] = t[0] * inv[0] + t[1] * inv[4] + t[2] * inv[8];
    inv[13] = t[0] * inv[1] + t[1] * inv[5] + t[2] * inv[9];
    inv[14] = t[0] * inv[2] + t[1] * inv[6] + t[2] * inv[10];
    a[0] = r1[0] * inv[0];
    a[1] = r1[0] * inv[1];
    a[2] = r1[0] * inv[2];
    b[0] = r1[1] * inv[4];
    b[1] = r1[1] * inv[5];
    b[2] = r1[1] * inv[6];
    c[0] = r1[2] * inv[8];
    c[1] = r1[2] * inv[9];
    c[2] = r1[2] * inv[10];
#define CORNER(s1, v1, s2, v2) \
    p[0] = (inv[12] s1 v1[0]) s2 v2[0]; \
    p[1] = (inv[13] s1 v1[1]) s2 v2[1]; \
    p[2] = (inv[14] s1 v1[2]) s2 v2[2]; \
    t0 = -1.0; \
    t1 = 1.0
    CORNER(+, b, +, c); VincClipSegmentCylinderAddInnerPoints(&end, p, a, r, *hh, &t0, &t1);
    CORNER(+, b, -, c); VincClipSegmentCylinderAddInnerPoints(&end, p, a, r, *hh, &t0, &t1);
    CORNER(-, b, -, c); VincClipSegmentCylinderAddInnerPoints(&end, p, a, r, *hh, &t0, &t1);
    CORNER(-, b, +, c); VincClipSegmentCylinderAddInnerPoints(&end, p, a, r, *hh, &t0, &t1);
    CORNER(+, c, +, a); VincClipSegmentCylinderAddInnerPoints(&end, p, b, r, *hh, &t0, &t1);
    CORNER(+, c, -, a); VincClipSegmentCylinderAddInnerPoints(&end, p, b, r, *hh, &t0, &t1);
    CORNER(-, c, -, a); VincClipSegmentCylinderAddInnerPoints(&end, p, b, r, *hh, &t0, &t1);
    CORNER(-, c, +, a); VincClipSegmentCylinderAddInnerPoints(&end, p, b, r, *hh, &t0, &t1);
    CORNER(+, a, +, b); VincClipSegmentCylinderAddPoints(&end, p, c, r, *hh, &t0, &t1);
    CORNER(+, a, -, b); VincClipSegmentCylinderAddPoints(&end, p, c, r, *hh, &t0, &t1);
    CORNER(-, a, -, b); VincClipSegmentCylinderAddPoints(&end, p, c, r, *hh, &t0, &t1);
    CORNER(-, a, +, b); VincClipSegmentCylinderAddPoints(&end, p, c, r, *hh, &t0, &t1);
#undef CORNER
    for (q = local; q != end; q += 3) {
        MeReal* o = *cursor;
        *cursor = o + 3;
        VincTMTransform(rel, q, o);
    }
}

/* Mac 0x4de961, Win 0x7e1130 (cdecl): separating axes of a box (half extents r1) and a cylinder
 * (radius *r, half height *hh; rel = cylinder in the box frame, its axis z = rel row 2): the box
 * faces, the cylinder's axis, box edge x cylinder axis and the radial direction towards the box
 * centre. Returns 0 at the first axis separating by more than *pad. Otherwise *sep / n / *d as for
 * VincBoxBoxSeparation and the contact point candidates (box frame) at *cursor: the cylinder's
 * rims / generators clipped against the box (V3.8's segment / box clippers) and the box edges
 * against the cylinder; returns 1 when there are any. padValue: passed on, unused. */
int VincBoxCylinderSeparation(MeReal* sep, MeReal* n, MeReal* d, MeReal** cursor, const MeReal* pad,
                              const MeReal* r1, const MeReal* r, const MeReal* hh, const MeReal* rel)
{
    const MeReal* z = rel + 8;
    const MeReal* T = rel + 12;
    MeReal hv = *hh, rv = *r;
    MeReal best = -DBL_MAX, sign = 1.0, dBest = 0.0, s;
    MeReal perp[3][3], cr[3][3], edge[3][3], v[3], radial[3], A[3], B[3], A2;
    MeReal* start;
    int separated = 1, i;
    unsigned char code = 0xf;
    /* box faces: perp[i] = z x e_i normalised, cr[i] = perp[i] x z (e_i without its z part) */
    for (i = 0; i < 3; ++i) {
        int u = NEXT(i), w = NEXT(u);
        MeReal len2, t;
        v[i] = 0.0;
        v[u] = z[w];
        v[w] = -z[u];
        perp[i][0] = v[0];
        perp[i][1] = v[1];
        perp[i][2] = v[2];
        len2 = v[0] * perp[i][0] + v[1] * perp[i][1] + v[2] * v[2];
        if (1e-15 <= len2) {
            MeReal len = sqrt(len2);
            if (len != 0.0) {
                MeReal inv = 1.0 / len;
                perp[i][0] = v[0] * inv;
                perp[i][1] = perp[i][1] * inv;
                perp[i][2] = perp[i][2] * inv;
            }
            cr[i][0] = perp[i][1] * z[2] - perp[i][2] * z[1];
            cr[i][1] = perp[i][2] * z[0] - z[2] * perp[i][0];
            cr[i][2] = z[1] * perp[i][0] - perp[i][1] * z[0];
        } else {
            perp[i][0] = 0.0;
            perp[i][1] = 0.0;
            perp[i][2] = 0.0;
            perp[i][w] = 1.0;
            cr[i][0] = 0.0;
            cr[i][1] = 0.0;
            cr[i][2] = 0.0;
            cr[i][u] = 1.0;
        }
        t = fabs(T[i]) - ((fabs(z[i]) * hv + fabs(cr[i][i]) * rv) + r1[i]); /* Win order (Mac: (r1 + hh) + r) */
        if (best < t) {
            sign = (T[i] <= 0.0) ? 1.0 : -1.0;
            dBest = -r1[i] - t;
            if (*pad < t) {
                s = t;
                goto separatedExit;
            }
            code = (unsigned char)(i | 0xc);
            separated = 0;
            best = t;
        }
    }
    /* the cylinder's axis */
    {
        MeReal p = (z[0] * T[0] + z[1] * T[1]) + z[2] * T[2];
        MeReal ap = fabs(p);
        MeReal t = ap - ((fabs(z[1]) * r1[1] + (fabs(z[0]) * r1[0] + hv)) + fabs(z[2]) * r1[2]); /* Win order */
        if (best < t) {
            sign = (p <= 0.0) ? 1.0 : -1.0;
            dBest = hv - ap;
            if (*pad < t) {
                s = t;
                goto separatedExit;
            }
            code = 0xb;
            separated = 0;
            best = t;
        }
    }
    /* box edge k x cylinder axis */
    for (i = 0; i < 3; ++i) {
        int u = NEXT(i), w = NEXT(u);
        MeReal len;
        v[i] = 0.0;
        v[u] = -z[w];
        v[w] = z[u];
        edge[i][0] = v[0];
        edge[i][1] = v[1];
        edge[i][2] = v[2];
        len = sqrt(v[0] * v[0] + v[1] * v[1] + v[2] * v[2]);
        if (len != 0.0) {
            MeReal inv = 1.0 / len, e0, e1, e2;
            e0 = v[0] * inv;
            edge[i][0] = e0;
            e1 = v[1] * inv;
            edge[i][1] = e1;
            e2 = v[2] * inv;
            edge[i][2] = e2;
            if (1e-15 < len) {
                MeReal p = (T[0] * e0 + T[1] * e1) + T[2] * e2;
                MeReal ap = fabs(p);
                MeReal t = ap - (((rv + fabs(e0) * r1[0]) + fabs(e1) * r1[1]) + fabs(e2) * r1[2]);
                if (best < t) {
                    sign = (p <= 0.0) ? 1.0 : -1.0;
                    dBest = rv - ap;
                    if (*pad < t) {
                        s = t;
                        goto separatedExit;
                    }
                    code = (unsigned char)(i | 8);
                    separated = 0;
                    best = t;
                }
            }
        }
    }
    /* the radial direction from the cylinder's axis towards the box centre */
    {
        MeReal q = (z[0] * -T[0] + z[1] * -T[1]) + z[2] * -T[2];
        MeReal len;
        radial[0] = -T[0] - z[0] * q;
        radial[1] = -T[1] - z[1] * q;
        radial[2] = -T[2] - q * z[2];
        len = sqrt(radial[0] * radial[0] + radial[1] * radial[1] + radial[2] * radial[2]);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            radial[0] = radial[0] * inv;
            radial[1] = radial[1] * inv;
            radial[2] = radial[2] * inv;
            if (1e-15 < len) {
                MeReal p = (T[0] * radial[0] + T[1] * radial[1]) + T[2] * radial[2];
                MeReal ap = fabs(p);
                MeReal t = ap - (((rv + fabs(radial[0]) * r1[0]) + fabs(radial[1]) * r1[1]) + fabs(radial[2]) * r1[2]);
                if (best < t) {
                    sign = (p <= 0.0) ? 1.0 : -1.0;
                    separated = *pad < t;
                    dBest = rv - ap;
                    code = 3;
                    best = t;
                }
            }
        }
    }
    *sep = best;
    *d = dBest;
    start = *cursor;
    if (separated)
        return *cursor != start;
    /* A, B: the directions spanning the cylinder's rim for the chosen axis */
    if ((code & 0xc) == 0xc) {
        int a = code & 3;
        A[0] = perp[a][0];
        A[1] = perp[a][1];
        A2 = perp[a][2];
        B[0] = cr[a][0];
        B[1] = cr[a][1];
        B[2] = cr[a][2];
        n[0] = 0.0;
        n[1] = 0.0;
        n[2] = 0.0;
        n[a] = sign;
    } else if ((code & 3) == 3) {
        if (((code & 0xc) >> 2) == 2) {
            A[0] = rel[0];
            A[1] = rel[1];
            A2 = rel[2];
            B[0] = rel[4];
            B[1] = rel[5];
            B[2] = rel[6];
            n[0] = sign * z[0];
            n[1] = sign * z[1];
            n[2] = sign * z[2];
        } else {
            B[0] = radial[0];
            B[1] = radial[1];
            B[2] = radial[2];
            A[0] = z[1] * B[2] - z[2] * B[1];
            A[1] = z[2] * B[0] - z[0] * B[2];
            A2 = z[0] * B[1] - z[1] * B[0];
            n[0] = B[0];
            n[1] = B[1];
            n[2] = B[2];
        }
    } else {
        int a = code & 3;
        B[0] = edge[a][0];
        B[1] = edge[a][1];
        B[2] = edge[a][2];
        A[0] = B[2] * z[1] - B[1] * z[2];
        A[1] = B[0] * z[2] - B[2] * z[0];
        A2 = B[1] * z[0] - B[0] * z[1];
        n[0] = sign * B[0];
        n[1] = sign * B[1];
        n[2] = sign * B[2];
    }
    {
        MeReal rB[3], invB[3], Z[3], invZ[3], p[3];
        rB[0] = rv * B[0];
        rB[1] = rv * B[1];
        rB[2] = rv * B[2];
        Z[0] = hv * z[0];
        Z[1] = hv * z[1];
        Z[2] = hv * z[2];
        invB[2] = 1.0 / rB[2];
        invB[1] = 1.0 / rB[1];
        invB[0] = 1.0 / rB[0];
        invZ[2] = 1.0 / Z[2];
        invZ[1] = 1.0 / Z[1];
        invZ[0] = 1.0 / Z[0];
        p[2] = A2 * rv + T[2];
        p[1] = A[1] * rv + T[1];
        p[0] = A[0] * rv + T[0];
        VincClipSegmentBoxAddPoints(cursor, p, Z, invZ, r1, *pad);
        p[2] = T[2] + rB[2];
        p[1] = T[1] + rB[1];
        p[0] = T[0] + rB[0];
        VincClipSegmentBoxAddPoints(cursor, p, Z, invZ, r1, *pad);
        p[2] = T[2] - A2 * rv;
        p[1] = T[1] - A[1] * rv;
        p[0] = T[0] - A[0] * rv;
        VincClipSegmentBoxAddPoints(cursor, p, Z, invZ, r1, *pad);
        p[2] = T[2] - rB[2];
        p[1] = T[1] - rB[1];
        p[0] = T[0] - rB[0];
        VincClipSegmentBoxAddPoints(cursor, p, Z, invZ, r1, *pad);
        p[2] = T[2] + Z[2];
        p[1] = T[1] + Z[1];
        p[0] = T[0] + Z[0];
        VincClipSegmentBoxAddInnerPoints(cursor, p, rB, invB, r1, *pad);
        p[2] = T[2] - Z[2];
        p[1] = T[1] - Z[1];
        p[0] = T[0] - Z[0];
        VincClipSegmentBoxAddInnerPoints(cursor, p, rB, invB, r1, *pad);
        VincBoxCylinderEdgePoints(cursor, rel, r1, r, hh);
    }
    return *cursor != start;
separatedExit:
    *sep = s;
    *d = dBest;
    return 0;
}

/* dims of the box / cylinder and box / capsule contacts: low digits = number of the normal's
 * near-zero components (box frame), (k << 2): 2 the normal is along the axis, 3 across it, 1 else */
MeI16 VincBoxCylinderDims(const MeReal* n, const MeReal* z)
{
    int cnt = (n[1] < 0.0001) + (n[0] < 0.0001) + (n[2] < 0.0001);
    MeReal c = n[0] * z[0] + n[1] * z[1] + n[2] * z[2];
    int k = 2;
    c = c * c;
    if (c <= 0.999999)
        k = (c < 1e-06) * 2 + 1;
    return (MeI16)(cnt | (k << 2));
}

/* Mac 0x4df902, Win 0x7e1ba0 (model1 = box, model2 = cylinder): contacts at the candidate points
 * no deeper than 1 % of the separation behind the support plane, up to 48 */
int MEAPI VincBoxCylinderIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* r1 = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    VincGeometryID g2 = VincModelGetGeometry(pair->model2);
    MeReal r = VincCylinderGetRadius(g2);
    MeReal hh = VincCylinderGetHalfHeight(g2);
    MeReal rel[16], points[96 * 3], n[3], sep, d;
    MeReal *cursor = points, *p;
    VincContact contacts[48];
    VincRequest* rq;
    MeI16 dims;
    int cnt;
    VincTMMakeRelative(tm2, tm1, rel);
    if (!VincBoxCylinderSeparation(&sep, n, &d, &cursor, &pad, r1, &r, &hh, rel)) {
        ResultClear(result);
        return 0;
    }
    rq = VincModelPairGetRequestPtr(pair);
    cnt = rq->contactMaxCount;
    if (result->contactMaxCount < cnt)
        cnt = result->contactMaxCount;
    if (cnt == 0) {
        ResultClear(result);
        return 1;
    }
    dims = VincBoxCylinderDims(n, rel + 8);
    VincTMRotate(tm1, n, result->normal);
    ResultClear(result);
    for (p = points; p != cursor; p += 3) {
        MeReal depth;
        if (result->contactCount >= 48)
            break;
        depth = (p[0] * n[0] + p[1] * n[1] + p[2] * n[2]) - d;
        if (depth < sep * 0.01) {
            VincContact* c = &contacts[result->contactCount];
            VincTMTransform(tm1, p, c->position);
            c->dims = dims;
            c->separation = depth;
            c->normal[0] = result->normal[0];
            c->normal[1] = result->normal[1];
            c->normal[2] = result->normal[2];
            ++result->contactCount;
        }
    }
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    return result->contactCount > 0;
}

/* Mac 0x4ddcb9, Win 0x7e1eb0 */
int MEAPI VincBoxCylinderRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincBoxCylinderIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincBoxCylinderSafeTime;
    VincGeometryPairRegisterInteractions(VincBoxGetTypeId(), VincCylinderGetTypeId(), &i, 0);
    return 1;
}
