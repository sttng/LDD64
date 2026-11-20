/* Vortex 5.0.0 build 800 "vortexldd41" - GJK distance between two convex geometries (restored).
 * Mac 0x4f90c0-0x4f9d8a (558a), Win 0x81e7f0-0x81f3b0. Callers: the cone pair
 * tests (Mac 0x4e5e19 / 0x4e5ed7 / 0x4e5f95, Win 0x7e2380 / 0x7e2410 / 0x7dd3f0). All names
 * (name inferred). Johnson's sub-algorithm: separate functions on Win (0x81e910 / 0x81edb0 /
 * 0x81e840), inlined into VincGJK on Mac; Mac's other copy is inlined in cnvGJK 0x4b7adc
 * (VincConvexMesh.cpp). */
#include "Vinc/VincGJK.h"

#include <math.h>
#include <string.h>

/* Mac 0x77ce78 / Win 0xafcf00 */
MeReal VincGJKLastClosest[2][3];

/* ---- Johnson tables (Mac 0x702774.. through pointers, Win 0xad1078..0xad14f8): the 15 non-empty
 * subsets of {0, 1, 2, 3} in the order of their bit masks ---- */
/* Win 0xad1078: number of members */
static const int kSize[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
/* Win 0xad10b8: largest member */
static const int kMaxIndex[16] = {-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};
/* Win 0xad10f8: members */
static const int kMembers[16][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0},
                                    {2, 0, 0, 0}, {0, 2, 0, 0}, {1, 2, 0, 0}, {0, 1, 2, 0},
                                    {3, 0, 0, 0}, {0, 3, 0, 0}, {1, 3, 0, 0}, {0, 1, 3, 0},
                                    {2, 3, 0, 0}, {0, 2, 3, 0}, {1, 2, 3, 0}, {0, 1, 2, 3}};
/* Win 0xad11f8: non-members (of {0..3}) */
static const int kOthers[16][4] = {{0, 1, 2, 3}, {1, 2, 3, 0}, {0, 2, 3, 0}, {2, 3, 0, 0},
                                   {0, 1, 3, 0}, {1, 3, 0, 0}, {0, 3, 0, 0}, {3, 0, 0, 0},
                                   {0, 1, 2, 0}, {1, 2, 0, 0}, {0, 2, 0, 0}, {2, 0, 0, 0},
                                   {0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
/* Win 0xad12f8: the subset without members[t] */
static const int kWithout[16][4] = {{0, 0, 0, 0}, {0, 0, 0, 0},    {0, 0, 0, 0},     {2, 1, 0, 0},
                                    {0, 0, 0, 0}, {4, 1, 0, 0},    {4, 2, 0, 0},     {6, 5, 3, 0},
                                    {0, 0, 0, 0}, {8, 1, 0, 0},    {8, 2, 0, 0},     {10, 9, 3, 0},
                                    {8, 4, 0, 0}, {12, 9, 5, 0},   {12, 10, 6, 0},   {14, 13, 11, 7}};
/* Win 0xad13f8: the subset plus others[t] */
static const int kWith[16][4] = {{1, 2, 4, 8}, {3, 5, 9, 0},   {3, 6, 10, 0}, {7, 11, 0, 0},
                                 {5, 6, 12, 0}, {7, 13, 0, 0}, {7, 14, 0, 0}, {15, 0, 0, 0},
                                 {9, 10, 12, 0}, {11, 13, 0, 0}, {11, 14, 0, 0}, {15, 0, 0, 0},
                                 {13, 14, 0, 0}, {15, 0, 0, 0}, {15, 0, 0, 0}, {0, 0, 0, 0}};

typedef struct Johnson {
    MeReal delta[16][4]; /* +0x000 delta[subset][member] (Johnson's determinants) */
    MeReal dot[4][4];    /* +0x200 w_i . w_j */
    MeReal total[16];    /* +0x280 sum of a subset's deltas */
} Johnson;

/* Mac 0x4f90c0 (regparm EAX; Win: inlined): one point of weight 1 */
static void LambdaInit(VincGJKLambda* l)
{
    l->count = 1;
    l->lambda[0] = 1.0;
}

/* Mac 0x4f90d9, Win 0x81e7f0: every simplex point = the start pair */
static void SimplexInit(VincGJKSimplex* s, const VincGJKLambda* l, const MeReal* a, const MeReal* b)
{
    int i;
    for (i = 0; i < l->count; ++i) {
        s->p[i][0] = a[0];
        s->p[i][1] = a[1];
        s->p[i][2] = a[2];
        s->q[i][0] = b[0];
        s->q[i][1] = b[1];
        s->q[i][2] = b[2];
    }
}

/* Mac 0x4f9139, Win 0x81eef0 (regparm): out = sum lambda_i pts_i */
static void Combine(MeReal* out, int n, const MeVector3* pts, const MeReal* lambda)
{
    int c, i;
    out[2] = 0.0;
    out[1] = 0.0;
    out[0] = 0.0;
    for (c = 0; c < 3; ++c)
        for (i = 0; i < n; ++i)
            out[c] = pts[i][c] * lambda[i] + out[c];
}

/* Mac 0x4f91a4, Win 0x81efb0 (regparm ESI = dir, EDI = out): out = the maximum point of g along
 * the normalised dir; *h = out . dir */
static void Support(VincGeometryID g, MeReal* tm, MeReal* h, const MeReal* dir, MeReal* out)
{
    MeVector3 n;
    MeReal len;
    n[0] = dir[0];
    n[1] = dir[1];
    n[2] = dir[2];
    len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (0.0 != len) {
        MeReal inv = 1.0 / len;
        n[0] = n[0] * inv;
        n[1] = n[1] * inv;
        n[2] = inv * n[2];
    }
    VincGeometryMaximumPoint(g, (MeReal (*)[4])tm, n, out);
    *h = out[0] * dir[0] + out[1] * dir[1] + out[2] * dir[2];
}

/* Win 0x81e910 (Mac: inlined; Mac 0x4dd073 is BoxCapsuleEnds, not a copy of this): the dot
 * products of the Minkowski points and the deltas of every subset of the first n points */
static void JohnsonDeltas(Johnson* j, const VincGJKSimplex* s, int n)
{
    MeVector3 w[4];
    int i, k, set, t;
    for (i = 0; i < n; ++i) {
        w[i][0] = s->q[i][0] - s->p[i][0];
        w[i][1] = s->q[i][1] - s->p[i][1];
        w[i][2] = s->q[i][2] - s->p[i][2];
    }
    for (i = 0; i < n; ++i)
        for (k = i; k < n; ++k) {
            MeReal d = w[k][0] * w[i][0] + w[k][1] * w[i][1] + w[k][2] * w[i][2];
            j->dot[k][i] = d;
            j->dot[i][k] = d;
        }
    for (set = 1; set < 16; ++set) {
        int size = kSize[set];
        if (n <= kMaxIndex[set])
            return;
        if (size < 2) {
            j->delta[set][kMembers[set][0]] = 1.0;
        } else if (size == 2) {
            int a = kMembers[set][0], b = kMembers[set][1];
            j->delta[set][a] = j->dot[b][b] - j->dot[b][a];
            j->delta[set][b] = j->dot[a][a] - j->dot[a][b];
        } else {
            for (t = 0; t < size; ++t) {
                int m = kMembers[set][t];
                int sub = kWithout[set][t];
                int k0 = kMembers[sub][0];
                MeReal sum = 0.0;
                for (k = 0; k < kSize[sub]; ++k) {
                    int u = kMembers[sub][k];
                    sum = (j->dot[u][k0] - j->dot[u][m]) * j->delta[sub][u] + sum;
                }
                j->delta[set][m] = sum;
            }
        }
    }
}

/* Win 0x81e840 (Mac: inlined): reduce the simplex to subset `set`, weights delta / total */
static void JohnsonReduce(int set, VincGJKLambda* l, VincGJKSimplex* s, const Johnson* j)
{
    int t, size;
    if (set >= 16) {
        /* NOTE: the original indexes its size table with 16 and reads the next table's first entry
         * (the largest member of the empty set, -1): the simplex count becomes -1 */
        l->count = -1;
        return;
    }
    size = kSize[set];
    for (t = 0; t < size; ++t) {
        int m = kMembers[set][t];
        if (m != t) {
            s->p[t][0] = s->p[m][0];
            s->p[t][1] = s->p[m][1];
            s->p[t][2] = s->p[m][2];
            s->q[t][0] = s->q[m][0];
            s->q[t][1] = s->q[m][1];
            s->q[t][2] = s->q[m][2];
        }
        l->lambda[t] = j->delta[set][m] / j->total[set];
    }
    l->count = size;
}

/* Win 0x81edb0 (Mac: inlined): the first subset whose deltas are all positive and that no further
 * point would improve. NOTE: a subset that passes with a delta total below 1e-20 does not stop
 * the search, yet stays "found" when it is the last one tried: the subset after it is used (deltas
 * not computed for this simplex: an earlier iteration's, or zero here where the original reads
 * uninitialised stack; full simplex: see JohnsonReduce). Returns 0 when there is none (ignored). */
static int JohnsonSelect(VincGJKLambda* l, VincGJKSimplex* s, Johnson* j)
{
    int n = l->count;
    int set = 1, ok = 1, t;
    for (;;) {
        int size;
        if (!(kMaxIndex[set] < n))
            break;
        size = kSize[set];
        j->total[set] = 0.0;
        ok = 1;
        for (t = 0; t < size; ++t) {
            MeReal d = j->delta[set][kMembers[set][t]];
            if (!(0.0 < d)) {
                ok = 0;
                break;
            }
            j->total[set] = d + j->total[set];
        }
        if (ok) {
            for (t = 0; t < n - size; ++t)
                if (0.0 < j->delta[kWith[set][t]][kOthers[set][t]]) {
                    ok = 0;
                    break;
                }
            if (ok && 1e-20 <= j->total[set])
                break;
        }
        ++set;
        if (set >= 16)
            break;
    }
    if (!ok)
        return 0;
    JohnsonReduce(set, l, s, j);
    return 1;
}

/* Mac 0x4f9c5a, Win 0x81f040 */
void VincGJKInit(VincModelID a, VincModelID b, MeReal* dir, int useDir, MeReal* pointA, MeReal* pointB)
{
    VincGeometryID ga = VincModelGetGeometry(a);
    VincGeometryID gb = VincModelGetGeometry(b);
    MeReal* tmA = VincModelGetTransformPtr(a);
    MeReal* tmB = VincModelGetTransformPtr(b);
    MeVector3 d, nd;
    MeReal h;
    if (useDir) {
        d[0] = dir[0];
        d[1] = dir[1];
        d[2] = dir[2];
    } else {
        d[0] = tmB[12] - tmA[12];
        d[1] = tmB[13] - tmA[13];
        d[2] = tmB[14] - tmA[14];
        dir[0] = d[0];
        dir[1] = d[1];
        dir[2] = d[2];
    }
    Support(ga, tmA, &h, d, pointA);
    nd[0] = -d[0];
    nd[1] = -d[1];
    nd[2] = -d[2];
    Support(gb, tmB, &h, nd, pointB);
}

/* Mac 0x4f928d, Win 0x81f100 */
int VincGJK(VincGeometryID ga, VincGeometryID gb, MeReal* tmA, MeReal* tmB, MeReal* dir, MeReal* pointA,
            MeReal* pointB, VincGJKLambda* lambda, VincGJKSimplex* simplex, MeReal* distance2)
{
    Johnson j;
    MeVector3 pa, pb, v, nv;
    MeReal dist2 = 0.0, prev = 1e30f; /* (a float constant in both binaries) */
    MeReal ha, hb, sum;
    int iter = 40, result;
    memset(&j, 0, sizeof j); /* (uninitialised stack in the original; it keeps its values between the
                                iterations, as here; see JohnsonSelect) */
    VincGJKLastClosest[0][0] = simplex->p[0][0];
    VincGJKLastClosest[0][1] = simplex->p[0][1];
    VincGJKLastClosest[0][2] = simplex->p[0][2];
    VincGJKLastClosest[1][0] = simplex->q[0][0];
    VincGJKLastClosest[1][1] = simplex->q[0][1];
    VincGJKLastClosest[1][2] = simplex->q[0][2];
    for (;;) {
        --iter;
        if (lambda->count == 1) {
            lambda->lambda[0] = 1.0;
        } else {
            JohnsonDeltas(&j, simplex, lambda->count);
            JohnsonSelect(lambda, simplex, &j);
        }
        if (lambda->count > 3) {
            simplex->m_c0 = 1e-8;
            result = 1;
            break;
        }
        Combine(pa, lambda->count, (const MeVector3*)simplex->p, lambda->lambda);
        Combine(pb, lambda->count, (const MeVector3*)simplex->q, lambda->lambda);
        v[0] = pb[0] - pa[0];
        v[1] = pb[1] - pa[1];
        v[2] = pb[2] - pa[2];
        dist2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        if (dist2 < 1e-8) {
            result = 1;
            break;
        }
        nv[0] = -v[0];
        nv[1] = -v[1];
        nv[2] = -v[2];
        Support(ga, tmA, &ha, v, simplex->p[lambda->count]);
        Support(gb, tmB, &hb, nv, simplex->q[lambda->count]);
        sum = hb + ha;
        if (!distance2 && sum <= 0.0) {
            result = 0;
            goto done; /* separated: *distance2 not written */
        }
        sum = sum + dist2;
        if (sum < 0.0 || sum < 1e-8 || !(dist2 < prev)) {
            result = 0;
            break;
        }
        lambda->lambda[lambda->count] = 0.0;
        ++lambda->count;
        prev = dist2;
        if (pointA) {
            pointA[0] = pa[0];
            pointA[1] = pa[1];
            pointA[2] = pa[2];
            pointB[0] = pb[0];
            pointB[1] = pb[1];
            pointB[2] = pb[2];
        }
        if (dir) {
            dir[0] = v[0];
            dir[1] = v[1];
            dir[2] = v[2];
        }
        if (iter <= 0) {
            result = 1;
            break;
        }
    }
    if (distance2)
        *distance2 = dist2;
done:
    if (pointA) {
        VincGJKLastClosest[0][0] = pointA[0];
        VincGJKLastClosest[0][1] = pointA[1];
        VincGJKLastClosest[0][2] = pointA[2];
        VincGJKLastClosest[1][0] = pointB[0];
        VincGJKLastClosest[1][1] = pointB[1];
        VincGJKLastClosest[1][2] = pointB[2];
    }
    return result;
}

/* Mac 0x4f9d8a, Win 0x81f3b0 */
int VincGJKIntersect(VincModelID a, VincModelID b, void* unused, MeReal* pointA, MeReal* pointB)
{
    MeReal* tmA = VincModelGetTransformPtr(a);
    MeReal* tmB = VincModelGetTransformPtr(b);
    VincGeometryID ga = VincModelGetGeometry(a);
    VincGeometryID gb = VincModelGetGeometry(b);
    VincGJKLambda lambda;
    VincGJKSimplex simplex;
    MeVector3 dir, startA, startB, localA, localB;
    MeReal dist2;
    (void)unused;
    LambdaInit(&lambda);
    VincGJKInit(a, b, dir, 0, startA, startB);
    SimplexInit(&simplex, &lambda, startA, startB);
    /* NOTE: without pointA / pointB the original uses uninitialised locals; when GJK stops in its
     * first iteration they are copied into VincGJKLastClosest unwritten (zeroed here) */
    memset(localA, 0, sizeof localA);
    memset(localB, 0, sizeof localB);
    if (!pointB)
        pointB = localB;
    if (!pointA)
        pointA = localA;
    return VincGJK(ga, gb, tmA, tmB, dir, pointA, pointB, &lambda, &simplex, &dist2);
}
