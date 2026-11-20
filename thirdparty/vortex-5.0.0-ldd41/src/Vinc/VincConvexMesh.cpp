// Vortex 5.0.0 build 800 "vortexldd41" - the Convex geometry (type 9) and the convex-convex core:
// support features, GJK with Johnson's sub-algorithm, the expanding-polytope penetration depth and
// the contact generator (restored). Mac 557b 0x4b30cb-0x4ba87b (one GCC TU with
// VincConvexHull.cpp; C++: cnvDistance uses new[]); Win 0x7b0120-0x7b2a00 (contacts),
// 0x7b5f10-0x7b6aa0 (geometry), 0x80b010-0x80d860 (GJK, penetration depth). Win only:
// VincConvexMeshCreate, CreateSegment, Init, GetType, the bounding sphere. Names inferred
// (VincConvexMesh.h).
// Every sum keeps the Mac SSE2 association (not Ghidra's flattened sums): dot = ((x + y) + z).
#include "Vinc/VincConvexMesh.h"
#include "Vinc/VincConvexInteractions.h"
#include "Vinc/VincMath.h"
#include "Vinc/VincPolygonCircle.h"
#include "Vinc/VincPrimitives.h"
#include "Me/MeMemory.h"

#include <float.h>
#include <math.h>
#include <string.h>

#include "VincInternal.h"

extern "C" {
// Win 0xafc838 (Mac: a constant 0 at 0x6449c0, the mode-1 block is unreachable there)
int cnvPenetrationMethod = 0;
}

namespace {

MeI16 s_convexTypeId = -1;               // Mac 0x779528, Win 0xac7ef4
const char* s_convexTypeName = "Convex"; // Mac 0x77952c, Win 0xac7ef8
// Mac 0x779520, Win 0xac7e88 (a float constant stored as double): face tolerance of the support
// features of the contact generator
MeReal s_featureEps = 0.01f;

// ---- Johnson tables (Mac 0x779800.. through the non-lazy pointers 0x702774-0x702788, shared with
// VincGJK.c's copy of the algorithm; Win 0xad1078..0xad14f8). The 15 non-empty subsets of
// {0, 1, 2, 3} in the order of their bit masks. ----
const int kSize[16] = {0, 1, 1, 2, 1, 2, 2, 3, 1, 2, 2, 3, 2, 3, 3, 4};
const int kMaxIndex[16] = {-1, 0, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 3, 3, 3, 3};
const int kMembers[16][4] = {{0, 0, 0, 0}, {0, 0, 0, 0}, {1, 0, 0, 0}, {0, 1, 0, 0},
                             {2, 0, 0, 0}, {0, 2, 0, 0}, {1, 2, 0, 0}, {0, 1, 2, 0},
                             {3, 0, 0, 0}, {0, 3, 0, 0}, {1, 3, 0, 0}, {0, 1, 3, 0},
                             {2, 3, 0, 0}, {0, 2, 3, 0}, {1, 2, 3, 0}, {0, 1, 2, 3}};
const int kOthers[16][4] = {{0, 1, 2, 3}, {1, 2, 3, 0}, {0, 2, 3, 0}, {2, 3, 0, 0},
                            {0, 1, 3, 0}, {1, 3, 0, 0}, {0, 3, 0, 0}, {3, 0, 0, 0},
                            {0, 1, 2, 0}, {1, 2, 0, 0}, {0, 2, 0, 0}, {2, 0, 0, 0},
                            {0, 1, 0, 0}, {1, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}};
const int kWithout[16][4] = {{0, 0, 0, 0}, {0, 0, 0, 0},  {0, 0, 0, 0},   {2, 1, 0, 0},
                             {0, 0, 0, 0}, {4, 1, 0, 0},  {4, 2, 0, 0},   {6, 5, 3, 0},
                             {0, 0, 0, 0}, {8, 1, 0, 0},  {8, 2, 0, 0},   {10, 9, 3, 0},
                             {8, 4, 0, 0}, {12, 9, 5, 0}, {12, 10, 6, 0}, {14, 13, 11, 7}};
const int kWith[16][4] = {{1, 2, 4, 8},  {3, 5, 9, 0},   {3, 6, 10, 0}, {7, 11, 0, 0},
                          {5, 6, 12, 0}, {7, 13, 0, 0},  {7, 14, 0, 0}, {15, 0, 0, 0},
                          {9, 10, 12, 0}, {11, 13, 0, 0}, {11, 14, 0, 0}, {15, 0, 0, 0},
                          {13, 14, 0, 0}, {15, 0, 0, 0}, {15, 0, 0, 0}, {0, 0, 0, 0}};

// Johnson's determinants (Win 0x80b450's block; Mac: GJK stack locals)
struct Johnson {
    MeReal delta[16][4];       // +0x000 delta[subset][member]
    MeReal dot[4][4];          // +0x200 w_i . w_j
    MeReal total[16];          // +0x280
};

inline VincConvexGeometry* Convex(VincGeometryID g)
{
    return (VincConvexGeometry*)g;
}

inline void Copy3(MeReal* out, const MeReal* a)
{
    out[0] = a[0];
    out[1] = a[1];
    out[2] = a[2];
}

// out = tm ? R^T v : v
inline void ToLocal(const MeReal* tm, const MeReal* v, MeReal* out)
{
    if (tm == 0)
        Copy3(out, v);
    else
        VincTMInverseRotate(tm, v, out);
}

// out = tm ? p R + T : p
inline void ToWorld(const MeReal* tm, const MeReal* p, MeReal* out)
{
    if (tm == 0)
        Copy3(out, p);
    else
        VincTMTransform(tm, p, out);
}

// Mac 0x4b30cb (regparm EAX)
void ResultClear(VincIntersectResult* r)
{
    r->contactGroupCount = 0;
    r->contactCount = 0;
}

// ---- polygon clipping (contact generator) ----

// the clipped points of ClipPolygon (Mac 24000 + 4 bytes of stack, Win 0x7b1470 locals)
struct PointPool {
    MeVector3 points[1000];    // +0x0000
    int count;                 // +0x5dc0
};

// Mac 0x4b30de, Win 0x7b0120 (regparm): Sutherland-Hodgman against the plane n . x = d (keeps
// n . x <= d); the cut points come from `pool`
void ClipPolygon(MeReal** out, int* outCount, MeReal** in, int inCount, const MeReal* n, MeReal d, PointPool* pool)
{
    *outCount = 0;
    if (inCount <= 0)
        return;              // (the original reads in[-1] first)
    MeReal* prev = in[inCount - 1];
    MeReal dPrev = (n[0] * prev[0] + n[1] * prev[1] + prev[2] * n[2]) - d;
    for (int i = 0; i != inCount; ++i) {
        MeReal* cur = in[i];
        MeReal dCur = (cur[0] * n[0] + cur[1] * n[1] + cur[2] * n[2]) - d;
        if ((0.0 < dPrev) != (0.0 < dCur)) {
            MeReal t = dPrev / (dPrev - dCur);
            MeReal* p = pool->points[pool->count++];
            p[0] = t * (cur[0] - prev[0]) + prev[0];
            p[1] = t * (cur[1] - prev[1]) + prev[1];
            p[2] = t * (cur[2] - prev[2]) + prev[2];
            out[(*outCount)++] = p;
        }
        if (dCur <= 0.0)
            out[(*outCount)++] = cur;
        prev = cur;
        dPrev = dCur;
    }
}

// Mac 0x4b373e, Win 0x7b12f0: p lies (within 0.1) on the segment a-b, measured in the plane of
// normal n
int PointNearSegment(const MeReal* p, const MeReal* a, const MeReal* b, const MeReal* n)
{
    MeReal dz = b[2] - a[2], dy = b[1] - a[1], dx = b[0] - a[0];
    MeReal mx = dy * n[2] - dz * n[1];
    MeReal my = dz * n[0] - dx * n[2];
    MeReal mz = dx * n[1] - dy * n[0];
    if ((MeReal)0.1f < fabs((mx * p[0] + p[1] * my + p[2] * mz) - (a[0] * mx + my * a[1] + a[2] * mz)))
        return 0;
    MeReal t = dx * p[0] + p[1] * dy + dz * p[2];
    if (t - (a[0] * dx + a[1] * dy + a[2] * dz) < -(MeReal)0.1f)
        return 0;
    return t - (dx * b[0] + dy * b[1] + dz * b[2]) <= (MeReal)0.1f;
}

// Mac 0x4b39b4, Win 0x7b0e50: the parameter range [*t0, *t1] of the segment p-q inside the prism
// over the convex polygon poly (n >= 3 points); 2 when found, 0 when not
int ClipSegmentToPolygon(MeReal* t0, MeReal* t1, const MeReal* p, const MeReal* q, const int* n,
                         const MeReal* poly, const MeReal* /*normal: unused*/)
{
    if (*n <= 2)
        return 0;
    *t0 = 2.0;
    *t1 = -2.0;
    MeReal qz = q[2], pz = p[2], qy = q[1], py = p[1], qx = q[0], px = p[0];
    const MeReal* P0 = poly;
    const MeReal* P1 = poly + 3;
    const MeReal* P2 = poly + 6;
    MeReal nx = (P2[2] - P1[2]) * (P1[1] - P0[1]) - (P2[1] - P1[1]) * (P1[2] - P0[2]);
    MeReal ny = (P2[0] - P1[0]) * (P1[2] - P0[2]) - (P2[2] - P1[2]) * (P1[0] - P0[0]);
    MeReal nz = (P2[1] - P1[1]) * (P1[0] - P0[0]) - (P2[0] - P1[0]) * (P1[1] - P0[1]);
    int inP = 1, inQ = 1;
    const MeReal* cur = poly;
    for (int i = 0; i < *n;) {
        ++i;
        const MeReal* nxt = poly + (i % *n) * 3;
        MeReal cz = cur[2], cy = cur[1], cx = cur[0];
        MeReal ez = nxt[2] - cz, ey = nxt[1] - cy, ex = nxt[0] - cx;
        MeReal mx = nz * ey - ny * ez;
        MeReal my = nx * ez - nz * ex;
        MeReal mz = ny * ex - nx * ey;
        MeReal off = -(cx * mx + cy * my + cz * mz);
        MeReal dp = mx * p[0] + my * p[1] + mz * p[2];
        MeReal dq = mx * q[0] + my * q[1] + mz * q[2];
        MeReal denom = dq - dp;
        MeReal sp = off + dp;
        inP = sp <= 0.0 ? inP : 0;
        inQ = off + dq <= 0.0 ? inQ : 0;
        if (1e-14 <= fabs(denom)) {
            MeReal t = -sp / denom;
            if (t <= 1.0 && 0.0 <= t) {
                MeReal len = sqrt(ex * ex + ey * ey + ez * ez);
                if (len != 0.0) {
                    MeReal inv = 1.0 / len;
                    ex = ex * inv;
                    ey = ey * inv;
                    ez = inv * ez;
                }
                MeReal s = (((qz - pz) * t + p[2]) - cz) * ez + ex * (((qx - px) * t + p[0]) - cx)
                           + (((qy - py) * t + p[1]) - cy) * ey;
                if (0.0 <= s && s <= len) {
                    if (t < *t0)
                        *t0 = t;
                    if (*t1 < t)
                        *t1 = t;
                }
            }
        }
        cur += 3;
    }
    if (inP)
        *t0 = 0.0;
    if (inQ)
        *t1 = 1.0;
    if (*t0 != 2.0 && *t1 != -2.0)
        return 2;
    return 0;
}

// Mac 0x4b3db2, Win 0x7b0cb0 (+ the quadratic 0x7b0c40) (regparm): the segment a-b clipped to the
// cylinder x^2 + y^2 <= r^2 (out: 2 points), or one point
void ClipSegmentToCircle(const MeReal* a, const MeReal* b, MeReal* out, MeReal r, int* count)
{
    MeReal bz = b[2], az = a[2];
    MeReal dy = b[1] - a[1];
    MeReal dx = b[0] - a[0];
    MeReal A = dx * dx + dy * dy;
    MeReal B = dx * a[0] + a[1] * dy;
    B = B + B;
    MeReal C = (a[0] * a[0] + a[1] * a[1]) - r * r;
    if (A != 0.0) {
        MeReal disc = (B * B) / (A * 4.0 * A) - C / A;
        if (disc < -1e-08)
            goto none;
        MeReal mid = B / (A * -2.0);
        MeReal dz = bz - az;
        if (!(disc < 1e-08)) {
            MeReal t1 = mid + sqrt(disc);
            MeReal t0 = mid - sqrt(disc);
            if (1.0 <= t1)
                t1 = 1.0;
            if (t0 <= 0.0)
                t0 = 0.0;
            out[0] = t0 * dx + a[0];
            out[1] = t0 * dy + a[1];
            out[2] = t0 * dz + a[2];
            out[3] = t1 * dx + a[0];
            out[4] = t1 * dy + a[1];
            out[5] = t1 * dz + a[2];
            *count = 2;
            return;
        }
        // NOTE: a tangent segment uses the root of the linear equation (as in both binaries)
    } else if (B == 0.0) {
none:
        MeReal origin[3] = {0.0, 0.0, 0.0};
        VincPointSegmentDistance(origin, a, b, out);
        *count = 1;
        return;
    }
    {
        MeReal t = -C / B;
        out[0] = t * dx + a[0];
        out[1] = dy * t + a[1];
        out[2] = (bz - az) * t + a[2];
        *count = 1;
    }
}

// Mac 0x4b3fe6, Win 0x7b0a10 (+ the cylinder part 0x7b0250): the support feature of one side in
// direction dir (unit, local): a hull's face / edge / vertex (cnvPlaneSupport) or, for a cylinder
// (cylBuf != NULL: 2 points), a rim point (1), a side segment (2) or the cap (-1, *capRadius)
MeReal SupportFeature(const MeReal* dir, int /*maxCount: unused*/, int* count, MeReal** points, void* convex,
                      MeReal* capRadius, MeReal* cylBuf, void* /*hint: unused*/, MeReal eps, MeReal* normal,
                      int* isFace)
{
    *isFace = 0;
    if (cylBuf == 0) {
        int maxCount = 100;
        MeReal dots[100];
        return cnvPlaneSupport(points, dots, &maxCount, dir, count, (CnvHull*)convex, eps, normal, isFace);
    }
    points[0] = cylBuf;
    points[1] = cylBuf + 3;
    MeReal r = VincCylinderGetRadius((VincGeometryID)convex);
    MeReal h = VincCylinderGetHalfHeight((VincGeometryID)convex);
    MeReal dx = dir[0], dy = dir[1];
    MeReal len = sqrt(dx * dx + dy * dy);
    MeReal x = 0.0, y = 0.0;
    if (1e-08 < len) {
        MeReal s = r / len;
        x = dx * s;
        y = dy * s;
    }
    MeReal* p0 = points[0];
    if (!(len <= 1e-08) && !(x * dx + y * dy < eps * 0.5)) {
        if (eps > fabs((h + h) * dir[2])) {
            *count = 2;
            MeReal* p1 = points[1];
            p0[0] = x;
            p0[1] = y;
            p1[0] = x;
            p1[1] = y;
            if (dir[2] > 0.0) {
                p0[2] = h;
                p1[2] = -h;
            } else {
                p0[2] = -h;
                p1[2] = h;
            }
        } else {
            *count = 1;
            p0[0] = x;
            p0[1] = y;
            p0[2] = dir[2] < 0.0 ? -h : h;
        }
    } else {
        *count = -1;
        *capRadius = r;
        p0[0] = 0.0;
        p0[1] = 0.0;
        p0[2] = dir[2] > 0.0 ? h : -h;
    }
    return dir[0] * p0[0] + dir[1] * p0[1] + dir[2] * p0[2];
}

// Mac 0x4b4305, Win 0x7b1260: a and b coincide (within 0.15) across the plane of normal n
int PointsClose(const MeReal* a, const MeReal* b, const MeReal* n)
{
    MeReal d = (n[0] * b[0] + n[1] * b[1] + n[2] * b[2]) - (n[0] * a[0] + n[1] * a[1] + n[2] * a[2]);
    MeReal z = (d * n[2] + a[2]) - b[2];
    MeReal y = (d * n[1] + a[1]) - b[1];
    MeReal x = (d * n[0] + a[0]) - b[0];
    return sqrt(x * x + y * y + z * z) <= (MeReal)0.15f;
}

// Mac 0x4b43cd, Win 0x7b1110: p inside the prism over the convex polygon poly (within 0.15)
int PointInPolygon(const MeReal* p, const int* n, const MeReal* poly, const MeReal* /*normal: unused*/)
{
    MeReal nx = (poly[4] - poly[1]) * (poly[8] - poly[5]) - (poly[5] - poly[2]) * (poly[7] - poly[4]);
    MeReal ny = (poly[5] - poly[2]) * (poly[6] - poly[3]) - (poly[3] - poly[0]) * (poly[8] - poly[5]);
    MeReal nz = (poly[3] - poly[0]) * (poly[7] - poly[4]) - (poly[4] - poly[1]) * (poly[6] - poly[3]);
    MeReal len = sqrt(nx * nx + ny * ny + nz * nz);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        nx = nx * inv;
        ny = ny * inv;
        nz = inv * nz;
    }
    const MeReal* cur = poly;
    for (int i = 0; i < *n;) {
        ++i;
        const MeReal* nxt = poly + (i % *n) * 3;
        MeReal ez = nxt[2] - cur[2];
        MeReal ey = nxt[1] - cur[1];
        MeReal ex = nxt[0] - cur[0];
        MeReal mx = nz * ey - ny * ez;
        MeReal my = ez * nx - nz * ex;
        MeReal mz = ex * ny - nx * ey;
        if (!((mx * p[0] + my * p[1] + mz * p[2]) - (cur[0] * mx + cur[1] * my + cur[2] * mz) <= (MeReal)0.15f))
            return 0;
        cur += 3;
    }
    return 1;
}

// world contacts of the generator (Mac 200 x 0x4c, Win 200 x 0x50 bytes of stack)
const int kMaxContacts = 200;

// Mac 0x4b45ab, Win 0x7b1470: contacts of two convexes along the normal n (from B to A), depth < 0.
// dirA = -R_A^T n, dirB = R_B^T n: the support directions in each side's frame
void GenerateContacts(VincIntersectResult* r, const VincConvexDesc* A, const VincConvexDesc* B, const MeReal* dirA,
                      const MeReal* dirB, const MeReal* n, MeReal depth, MeReal* cylBufA, MeReal* cylBufB,
                      MeReal* hintA, MeReal* hintB)
{
    VincContact contacts[kMaxContacts];
    PointPool pool;
    MeReal worldA[100][3], worldB[100][3];
    MeReal bufA[104][3], bufB[104][3];      // circle / polygon points, or (both faces) pointer lists
    MeReal* ptsA[100];
    MeReal* ptsB[100];
    int nA = 0, nB = 0, isFaceA = 0, isFaceB = 0, count = 0;
    MeReal capA = 0.0, capB = 0.0;
    MeReal faceA[3], faceB[3];
    MeReal nAw[3] = {0.0, 0.0, 0.0}, nBw[3] = {0.0, 0.0, 0.0};
    const MeReal* tmA = A->tm;
    const MeReal* tmB = B->tm;
    int i;

    SupportFeature(dirA, 100, &nA, ptsA, A->convex, &capA, cylBufA, hintA, s_featureEps, faceA, &isFaceA);
    SupportFeature(dirB, 100, &nB, ptsB, B->convex, &capB, cylBufB, hintB, s_featureEps, faceB, &isFaceB);
    if (isFaceB) {
        if (tmB == 0)
            Copy3(nBw, faceB);
        else
            VincTMRotate(tmB, faceB, nBw);
        if (((CnvHull*)B->convex)->vertexCount == 3 && nBw[0] * n[0] + nBw[1] * n[1] + nBw[2] * n[2] < (MeReal)0.8f) {
            nBw[0] = -nBw[0];
            nBw[1] = -nBw[1];
            nBw[2] = -nBw[2];
        }
    }
    if (isFaceA) {
        if (tmA == 0)
            Copy3(nAw, faceA);
        else
            VincTMRotate(tmA, faceA, nAw);
        nAw[0] = -nAw[0];
        nAw[1] = -nAw[1];
        nAw[2] = -nAw[2];
        if (((CnvHull*)A->convex)->vertexCount == 3 && nAw[0] * n[0] + nAw[1] * n[1] + nAw[2] * n[2] < (MeReal)0.8f) {
            nAw[0] = -nAw[0];
            nAw[1] = -nAw[1];
            nAw[2] = -nAw[2];
        }
    }
    MeReal mA = A->margin, mB = B->margin;
    for (i = 0; i < nA; ++i)
        ToWorld(tmA, ptsA[i], worldA[i]);
    for (i = 0; i < nB; ++i)
        ToWorld(tmB, ptsB[i], worldB[i]);

    if (nA == -1 && nB != 1) {
        // ---- A's cylinder cap against B's feature, in A's frame ----
        MeReal rel[16];
        VincTMMakeRelative(tmB, tmA, rel);
        int m = 0;
        if (nB == -1) {
            MeReal h = ptsA[0][2];
            MeReal cx = rel[12], cy = rel[13];
            MeReal d2 = cx * cx + cy * cy;
            MeReal rA2 = capA * capA;
            if (d2 < rA2 + capB * capB) {
                MeReal d = sqrt(d2);
                if (fabs(capB - capA) + 1e-08 <= d) {
                    MeReal a = ((d2 + rA2) - capB * capB) / (d2 + d2);
                    MeReal hh = sqrt(rA2 / d2 - a * a);
                    // NOTE: (cy, cx) instead of the perpendicular (-cy, cx): the two crossing
                    // points are mirrored (both binaries)
                    MeReal sy = hh * cy;
                    bufA[0][0] = a * cx + sy;
                    MeReal sx = hh * cx;
                    bufA[0][1] = a * cy + sx;
                    bufA[1][0] = a * cx - sy;
                    bufA[1][1] = a * cy - sx;
                    bufA[2][0] = (capA / d) * cx;
                    bufA[2][1] = (capA / d) * cy;
                    MeReal f = (d - capB) / d;
                    bufA[3][0] = f * cx;
                    bufA[3][1] = f * cy;
                    m = 4;
                } else {
                    MeReal rr = capB <= capA ? capB : capA;
                    bufA[0][0] = 0.0;
                    bufA[0][1] = rr;
                    bufA[1][0] = rr * (MeReal)0.866025388f;
                    bufA[1][1] = rr * -0.5;
                    bufA[2][0] = rr * -(MeReal)0.866025388f;
                    bufA[2][1] = rr * -0.5;
                    m = 3;
                }
                for (i = 0; i < m; ++i)
                    bufA[i][2] = h;
                if (m > kMaxContacts)
                    m = kMaxContacts;
            } else {
                MeReal t = capA / (capB + capA);
                bufA[0][0] = t * cx;
                bufA[0][1] = t * cy;
                bufA[0][2] = h;      // NOTE: not written by the original (stale stack)
                m = 1;
            }
        } else {
            if (nB == 2) {
                MeReal seg[2][3];
                for (i = 0; i < 2; ++i)
                    VincTMTransform(rel, ptsB[i], seg[i]);
                ClipSegmentToCircle(seg[0], seg[1], &bufA[0][0], capA, &m);
            } else {
                for (i = 0; i < nB; ++i)
                    VincTMTransform(rel, ptsB[i], bufB[i]);
                int maxOut = 100;
                VincPolygonCircleIntersect(&bufB[0][0], &nB, &capA, &maxOut, &bufA[0][0], &m);
            }
            if (m > kMaxContacts)
                m = kMaxContacts;
        }
        count = m;
        MeReal off = 0.0 <= ptsA[0][2] ? mA : -mA;
        for (i = 0; i < count; ++i) {
            VincContact* c = &contacts[i];
            Copy3(c->normal, n);
            bufA[i][2] = bufA[i][2] + off;
            VincTMTransform(tmA, bufA[i], c->position);
            c->separation = depth;
        }
    } else if (nB == -1 && nA != 1) {
        // ---- B's cylinder cap against A's feature, in B's frame ----
        MeReal rel[16];
        VincTMMakeRelative(tmA, tmB, rel);
        int m = 0;
        if (nA == 2) {
            MeReal seg[2][3];
            for (i = 0; i < 2; ++i)
                VincTMTransform(rel, ptsA[i], seg[i]);
            ClipSegmentToCircle(seg[0], seg[1], &bufB[0][0], capB, &m);
        } else {
            for (i = 0; i < nA; ++i)
                VincTMTransform(rel, ptsA[i], bufA[i]);
            int maxOut = 100;
            VincPolygonCircleIntersect(&bufA[0][0], &nA, &capB, &maxOut, &bufB[0][0], &m);
        }
        if (m > kMaxContacts)
            m = kMaxContacts;
        count = m;
        MeReal off = 0.0 <= ptsB[0][2] ? mB : -mB;
        for (i = 0; i < count; ++i) {
            VincContact* c = &contacts[i];
            Copy3(c->normal, n);
            bufB[i][2] = bufB[i][2] + off;
            VincTMTransform(tmB, bufB[i], c->position);
            c->separation = depth;
        }
    } else if (!isFaceA && !isFaceB) {
        // ---- vertices and edges ----
        MeReal pt[2][3];
        int m = 1;
        if (nA == 1) {
            if (nB == 1) {
                Copy3(pt[0], worldA[0]);
                if (!PointsClose(worldA[0], worldB[0], n))
                    goto none;
            } else {
                Copy3(pt[0], worldA[0]);
            }
            pt[0][0] = pt[0][0] - mA * n[0];
            pt[0][1] = pt[0][1] - mA * n[1];
            pt[0][2] = pt[0][2] - mA * n[2];
        } else if (nA == 2 && nB == 2) {
            char parallel[4];
            MeReal d = VincSegmentSegmentClosestPoints(worldA[0], worldA[1], worldB[0], worldB[1], pt[0], pt[1],
                                                       parallel, &m);
            pt[0][0] = pt[0][0] - mA * n[0];
            pt[0][1] = pt[0][1] - mA * n[1];
            pt[0][2] = pt[0][2] - mA * n[2];
            if (m > kMaxContacts)
                m = kMaxContacts;
            if (m > 1) {
                pt[1][0] = pt[1][0] - mA * n[0];
                pt[1][1] = pt[1][1] - mA * n[1];
                pt[1][2] = pt[1][2] - mA * n[2];
            }
            if (0.001 - depth < d - (mA + mB))
                goto none;
        } else if (nB == 1 && nA == 2) {
            if (!PointNearSegment(worldB[0], worldA[0], worldA[1], n))
                goto none;
            pt[0][0] = mA * n[0] + worldB[0][0];
            pt[0][1] = mA * n[1] + worldB[0][1];
            pt[0][2] = mA * n[2] + worldB[0][2];
        } else {
            if (nB != 1)
                goto none;
            pt[0][0] = mB * n[0] + worldB[0][0];
            pt[0][1] = worldB[0][1] + mB * n[1];
            pt[0][2] = worldB[0][2] + mB * n[2];
        }
        count = m;
        for (i = 0; i < count; ++i) {
            VincContact* c = &contacts[i];
            Copy3(c->position, pt[i]);
            c->separation = depth;
            Copy3(c->normal, n);
        }
        goto finish;
none:
        count = 0;
    } else {
        // ---- a face: the reference face (B's when it has one) against the incident feature ----
        int nRef;
        MeReal N[3];
        MeReal mRef;
        MeReal (*refPts)[3];
        MeReal (*incPts)[3];
        const MeReal* cNormal;
        if (isFaceB) {
            nRef = nB;
            Copy3(N, nBw);
            mRef = mA;
            refPts = worldB;
            incPts = worldA;
            cNormal = nBw;
        } else {
            nRef = nA;
            N[0] = -nAw[0];
            N[1] = -nAw[1];
            N[2] = -nAw[2];
            mRef = mB;
            refPts = worldA;
            incPts = worldB;
            cNormal = nAw;
        }
        if (!isFaceA || !isFaceB) {
            if (nA < 2 || nB < 2) {
                if (!PointInPolygon(incPts[0], &nRef, &refPts[0][0], cNormal)) {
                    count = 0;
                    goto finish;
                }
                Copy3(contacts[0].position, incPts[0]);
                count = 1;
            } else {
                MeReal t0 = 1.0, t1 = 0.0;
                count = ClipSegmentToPolygon(&t0, &t1, incPts[0], incPts[1], &nRef, &refPts[0][0], N);
                if (count < 1)
                    goto finish;
                for (i = 0; i != 2; ++i) {
                    MeReal* p = contacts[i].position;
                    MeReal t = i == 0 ? t1 : t0;
                    p[0] = (incPts[1][0] - incPts[0][0]) * t;
                    p[1] = (incPts[1][1] - incPts[0][1]) * t;
                    p[2] = (incPts[1][2] - incPts[0][2]) * t;
                    p[0] = p[0] + incPts[0][0];
                    p[1] = p[1] + incPts[0][1];
                    p[2] = p[2] + incPts[0][2];
                }
                count = 2;
            }
        } else {
            // both faces: A's polygon clipped by the side planes of B's
            MeReal** listA = (MeReal**)&bufB[0][0];   // Mac: the 300-double buffers as pointer lists
            MeReal** listB = (MeReal**)&bufA[0][0];
            int countA = nA, countB = 0;
            for (i = 0; i < nA; ++i)
                listA[i] = worldA[i];
            pool.count = 0;
            MeReal** result = listA;
            int resultCount = countA;
            if (nB >= 1) {
                bool flip = false, empty = false;
                for (int j = 0; j < nB;) {
                    const MeReal* cur = worldB[j];
                    ++j;
                    const MeReal* nxt = worldB[j % nB];
                    MeReal ex = cur[0] - nxt[0], ey = cur[1] - nxt[1], ez = cur[2] - nxt[2];
                    MeReal m[3];
                    m[0] = ey * nBw[2] - ez * nBw[1];
                    m[1] = ez * nBw[0] - ex * nBw[2];
                    m[2] = ex * nBw[1] - ey * nBw[0];
                    MeReal d = m[0] * cur[0] + m[1] * cur[1] + m[2] * cur[2];
                    if (flip) {
                        ClipPolygon(listA, &countA, listB, countB, m, d, &pool);
                        if (countA < 1) {
                            empty = true;          // the result is the empty list A
                            flip = false;
                            break;
                        }
                    } else {
                        ClipPolygon(listB, &countB, listA, countA, m, d, &pool);
                        if (countB < 1) {
                            empty = true;          // the result is the empty list B
                            flip = true;
                            break;
                        }
                    }
                    flip = !flip;
                }
                (void)empty;
                if (flip) {
                    result = listB;
                    resultCount = countB;
                } else {
                    result = listA;
                    resultCount = countA;
                }
            }
            count = resultCount;
            if (count > kMaxContacts)
                count = kMaxContacts;
            if (count < 1)
                goto finish;
            for (i = 0; i != count; ++i)
                Copy3(contacts[i].position, result[i]);
        }
        for (i = 0;;) {
            VincContact* c = &contacts[i];
            Copy3(c->normal, cNormal);
            MeReal sep = ((c->position[0] - refPts[0][0]) * N[0] + (c->position[1] - refPts[0][1]) * N[1]
                          + (c->position[2] - refPts[0][2]) * N[2]) - (mA + mB);
            c->separation = depth <= sep ? sep : depth;
            c->position[0] = c->position[0] - mRef * N[0];
            c->position[1] = c->position[1] - mRef * N[1];
            c->position[2] = c->position[2] - mRef * N[2];
            ++i;
            if (count <= i)
                break;
        }
    }
finish:
    VincRequest request;
    VincRequestInit(&request);
    request.faceNormalsFirst = 0;
    request.contactMaxCount = r->contactMaxCount;
    if (count == 0 || r->contactMaxCount == 0)
        ResultClear(r);
    else
        r->contactCount = VincContactSimplifyCore(contacts[0].normal, contacts, count, r->contacts,
                                                  request.contactMaxCount, &request, 0);
    for (i = 0; i < r->contactCount; ++i)
        r->contacts[i].separation = r->contacts[i].separation + (MeReal)1e-4f;
}

// ---- GJK ----

// Mac 0x4b75b4, Win 0x80bb10 (regparm): out = sum lambda_i pts_i
void Combine(MeReal* out, const MeReal* pts, int n, const MeReal* lambda)
{
    out[2] = 0.0;
    out[1] = 0.0;
    out[0] = 0.0;
    for (int c = 0; c < 3; ++c)
        for (int i = 0; i < n; ++i)
            out[c] = pts[i * 3 + c] * lambda[i] + out[c];
}

// Win 0x80b450 (Mac: inlined): dots of the Minkowski points w_i = B_i - A_i and the deltas of
// every subset of the first n
void JohnsonDeltas(Johnson* j, const CnvSimplex* s, int n)
{
    MeReal w[4][3];
    int i, k, set, t;
    for (i = 0; i < n; ++i) {
        w[i][0] = s->pointsB[i * 3 + 0] - s->pointsA[i * 3 + 0];
        w[i][1] = s->pointsB[i * 3 + 1] - s->pointsA[i * 3 + 1];
        w[i][2] = s->pointsB[i * 3 + 2] - s->pointsA[i * 3 + 2];
    }
    for (i = 0; i < n; ++i)
        for (k = i; k < n; ++k) {
            MeReal d = w[i][0] * w[k][0] + w[i][1] * w[k][1] + w[i][2] * w[k][2];
            j->dot[k][i] = d;
            j->dot[i][k] = d;
        }
    for (set = 1; set != 16; ++set) {
        if (n <= kMaxIndex[set])
            break;
        int size = kSize[set];
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
                MeReal sum = 0.0;
                for (k = 0; k < kSize[sub]; ++k) {
                    int u = kMembers[sub][k];
                    sum = sum + (j->dot[u][kMembers[sub][0]] - j->dot[u][m]) * j->delta[sub][u];
                }
                j->delta[set][m] = sum;
            }
        }
    }
}

// Win 0x80b2c0 (Mac: inlined): the simplex reduced to subset `set` (support pointers, world
// points, weights); cylinder points that leave the simplex are freed in their cache
void JohnsonReduce(int set, CnvPairCache* cache, CnvSimplex* s, CnvCylinderCache* cylA, CnvCylinderCache* cylB,
                   const Johnson* j)
{
    if (set >= 16) {
        // NOTE: the original reads one entry past its size table (the next table's -1): the
        // simplex count becomes -1 (see VincGJK.c)
        cache->count = -1;
        return;
    }
    int size = kSize[set], t;
    for (t = 0; t < size; ++t) {
        int m = kMembers[set][t];
        if (m != t) {
            MeReal* a = cache->a[t];
            MeReal* b = cache->b[t];
            cache->a[t] = cache->a[m];
            cache->b[t] = cache->b[m];
            if (cylA != 0)
                cache->a[m] = a;
            if (cylB != 0)
                cache->b[m] = b;
            Copy3(s->pointsA + t * 3, s->pointsA + m * 3);
            Copy3(s->pointsB + t * 3, s->pointsB + m * 3);
        }
        s->lambda[t] = j->delta[set][kMembers[set][t]] / j->total[set];
    }
    for (; t < cache->count; ++t) {
        if (cylA != 0)
            cylA->used[(MeVector3*)cache->a[t] - cylA->points] = 0;
        if (cylB != 0)
            cylB->used[(MeVector3*)cache->b[t] - cylB->points] = 0;
    }
    cache->count = size;
}

// Win 0x80b9d0 (Mac: inlined): the first subset whose deltas are all positive and that no other
// point improves, reduced to; 0 when there is none. NOTE: as in VincGJK.c, a subset that passes
// with a delta total below 1e-20 does not stop the search but stays "found" when the search ends
int JohnsonSelect(CnvPairCache* cache, CnvSimplex* s, CnvCylinderCache* cylA, CnvCylinderCache* cylB, Johnson* j)
{
    int n = cache->count;
    int set = 1, ok = 0, t;
    for (;;) {
        if (!(kMaxIndex[set] < n))
            break;
        int size = kSize[set];
        j->total[set] = 0.0;
        ok = 1;
        for (t = 0; t < size; ++t) {
            MeReal d = j->delta[set][kMembers[set][t]];
            if (d <= 0.0) {
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
            if (ok && !(j->total[set] < 1e-20))
                break;
        }
        ++set;
        if (set == 16)
            break;
    }
    if (!ok)
        return 0;
    JohnsonReduce(set, cache, s, cylA, cylB, j);
    return 1;
}

// Mac 0x4b79c2, Win 0x80c260: a face of the penetration polytope
struct Face {                  // Mac 0x2c, Win 0x30
    MeReal* v[3];              // +0x00
    MeVector3 n;               // +0x0c +0x10 unit normal (v1 - v0) x (v2 - v1)
    MeReal d;                  // +0x24 +0x28 n . v0
};

void FacePlane(Face* f)
{
    const MeReal* a = f->v[0];
    const MeReal* b = f->v[1];
    const MeReal* c = f->v[2];
    MeReal nx = (b[1] - a[1]) * (c[2] - b[2]) - (b[2] - a[2]) * (c[1] - b[1]);
    MeReal ny = (b[2] - a[2]) * (c[0] - b[0]) - (b[0] - a[0]) * (c[2] - b[2]);
    MeReal nz = (b[0] - a[0]) * (c[1] - b[1]) - (b[1] - a[1]) * (c[0] - b[0]);
    MeReal len = sqrt(nx * nx + ny * ny + nz * nz);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        nx = nx * inv;
        ny = ny * inv;
        nz = inv * nz;
    } else {
        nx = 1.0;
        ny = 0.0;
        nz = 0.0;
    }
    f->n[0] = nx;
    f->n[1] = ny;
    f->n[2] = nz;
    f->d = a[0] * nx + a[1] * f->n[1] + a[2] * f->n[2];
}

// Mac 0x4ba276, Win 0x80b180: the world points of A's simplex supports
void SimplexToWorldA(CnvSimplex* s, const CnvPairCache* cache, void* /*convex: unused*/, const MeReal* tm)
{
    for (int i = 0; i < cache->count; ++i)
        ToWorld(tm, cache->a[i], s->pointsA + i * 3);
}

// Mac 0x4ba18a, Win 0x80b220
void SimplexToWorldB(CnvSimplex* s, const CnvPairCache* cache, void* /*convex: unused*/, const MeReal* tm)
{
    for (int i = 0; i < cache->count; ++i)
        ToWorld(tm, cache->b[i], s->pointsB + i * 3);
}

// support values of both sides along n (B along -n), with the transforms' translations
void SupportPair(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache, CnvCylinderCache* cylA,
                 CnvCylinderCache* cylB, const MeReal* n, MeReal* ha, MeReal* hb)
{
    MeReal dA[3], dB[3];
    ToLocal(a->tm, n, dA);
    ToLocal(b->tm, n, dB);
    dB[0] = -dB[0];
    dB[1] = -dB[1];
    dB[2] = -dB[2];
    cache->hintA = cnvSupportPoint(a->convex, cylA != 0, cache->hintA, ha, dA);
    cache->hintB = cnvSupportPoint(b->convex, cylB != 0, cache->hintB, hb, dB);
}

// the world difference of the current hints: out = A(hintA) - B(hintB)
void HintDifference(const VincConvexDesc* a, const VincConvexDesc* b, const CnvPairCache* cache, MeReal* out)
{
    MeReal pa[3], pb[3];
    ToWorld(a->tm, cache->hintA, pa);
    ToWorld(b->tm, cache->hintB, pb);
    out[0] = pa[0] - pb[0];
    out[1] = pa[1] - pb[1];
    out[2] = pa[2] - pb[2];
}

// (computed with a zero translation when tm is NULL, as the original does)
inline MeReal TranslationDot(const MeReal* tm, const MeReal* n)
{
    MeReal x = 0.0, y = 0.0, z = 0.0;
    if (tm != 0) {
        x = tm[12];
        y = tm[13];
        z = tm[14];
    }
    return n[0] * x + n[1] * y + n[2] * z;
}

// Win 0x80d510 (Mac: inlined in cnvDistance, unreachable there): one support step along n
MeReal AxisPenetration(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache, CnvCylinderCache* cylA,
                       CnvCylinderCache* cylB, const MeReal* n)
{
    MeReal nn[3] = {-n[0], -n[1], -n[2]};
    MeReal dA[3], dB[3];
    ToLocal(a->tm, nn, dA);
    ToLocal(b->tm, n, dB);
    MeReal off = 0.0;
    if (a->tm != 0)
        off = (n[0] * a->tm[12] + n[1] * a->tm[13] + n[2] * a->tm[14]) + 0.0;
    if (b->tm != 0)
        off = off - (n[0] * b->tm[12] + n[1] * b->tm[13] + n[2] * b->tm[14]);
    MeReal ha, hb;
    cache->hintA = cnvSupportPoint(a->convex, cylA != 0, cache->hintA, &ha, dA);
    cache->hintB = cnvSupportPoint(b->convex, cylB != 0, cache->hintB, &hb, dB);
    return (off - ha) - hb;
}

} // namespace

extern "C" {

// ---- support ----

// Mac 0x4b3292, Win 0x7b03b0
MeReal cnvPlaneSupport(MeReal** points, MeReal* dots, int* maxCount, const MeReal* dir, int* count,
                             CnvHull* hull, MeReal eps, MeReal* normal, int* isFace)
{
    *count = 0;
    *isFace = 0;
    int best = -1;
    if (hull->faceCount >= 1) {
        MeReal bestDot = -DBL_MAX;
        for (int i = 0; i < hull->faceCount; ++i) {
            const MeReal* fn = hull->faces[i].normal;
            MeReal d = fn[0] * dir[0] + fn[1] * dir[1] + fn[2] * dir[2];
            if (bestDot < d) {
                best = i;
                bestDot = d;
                if (fabs(d - 1.0) < eps)
                    *isFace = 1;
            }
        }
        if (*isFace) {
            CnvFace* f = &hull->faces[best];
            for (int k = 0; k < f->count; ++k)
                if (*count < *maxCount)
                    points[(*count)++] = f->edges[k].vertex->pos;
            *isFace = 1;
            Copy3(normal, f->normal);
            const MeReal* p0 = points[0];
            const MeReal* p1 = points[f->count / 2];
            return ((dir[0] * p0[0] + dir[1] * p0[1] + dir[2] * p0[2]) + (dir[0] * p1[0] + dir[1] * p1[1] + dir[2] * p1[2]))
                   * 0.5;
        }
    }
    if (hull->faceCount == 0) {
        if (hull->vertexCount > 0) {
            MeReal bestDot = -DBL_MAX;
            int bestV = -1;
            for (int i = 0; i < hull->vertexCount; ++i) {
                const MeReal* p = hull->vertices[i].pos;
                MeReal d = dir[0] * p[0] + dir[1] * p[1] + dir[2] * p[2];
                dots[i] = d;
                if (bestDot < d) {
                    bestV = i;
                    bestDot = d;
                }
            }
            if (bestV != -1) {
                *count = 0;
                for (int i = 0; i < hull->vertexCount; ++i)
                    if (fabs(bestDot - dots[i]) < eps)
                        points[(*count)++] = hull->vertices[i].pos;
                return bestDot;
            }
        }
        return 0.0;
    }
    if (best == -1)
        return 0.0;
    CnvFace* f = &hull->faces[best];
    int n = f->count;
    if (n <= 0)
        return 0.0;
    MeReal bestDot = -DBL_MAX;
    int k0 = -1;
    for (int k = 0; k != n; ++k) {
        const MeReal* p = f->edges[k].pos;
        MeReal d = dir[0] * p[0] + dir[1] * p[1] + dir[2] * p[2];
        dots[k] = d;
        if (bestDot < d) {
            bestDot = d;
            k0 = k;
        }
    }
    if (k0 == -1)
        return 0.0;
    MeReal d0 = dots[k0];
    int next = (k0 + 1) % n;
    if (eps <= fabs(d0 - dots[next])) {
        int prev = (k0 != 0 ? k0 : n) - 1;
        if (eps <= fabs(d0 - dots[prev])) {
            *count = 1;
            points[0] = f->edges[k0].vertex->pos;
        } else {
            *count = 2;
            points[0] = f->edges[k0].vertex->pos;
            points[1] = f->edges[prev].vertex->pos;
            bestDot = (bestDot + dots[prev]) * 0.5;
        }
    } else {
        *count = 2;
        points[0] = f->edges[k0].vertex->pos;
        points[1] = f->edges[next].vertex->pos;
        bestDot = (bestDot + dots[next]) * 0.5;
    }
    return bestDot;
}

// Mac 0x4b761f, Win 0x80bc30 (regparm)
CnvVertex* cnvHullSupport(CnvHull* hull, CnvVertex* hint, MeReal* dot, const MeReal* dir)
{
    MeReal dx = dir[0], dy = dir[1], dz = dir[2];
    if (hint == 0) {
        hint = hull->vertices;
        if (hint == 0) {
            *dot = 0.0;      // NOTE: the original dereferences the NULL vertex array first
            return 0;
        }
    }
    MeReal best = dx * hint->pos[0] + dy * hint->pos[1] + dz * hint->pos[2];
    CnvVertex* prev = 0;
    CnvVertex* cur;
    do {
        cur = hint;
        CnvVertex** nb = cur->neighbours;
        CnvVertex** end = nb + cur->neighbourCount;
        if (end <= nb)
            break;
        for (; nb < end; ++nb) {
            CnvVertex* v = *nb;
            if (v != prev) {
                MeReal d = dx * v->pos[0] + dy * v->pos[1] + dz * v->pos[2];
                if (best < d) {
                    hint = v;
                    best = d;
                }
            }
        }
        prev = cur;
    } while (hint != cur);
    *dot = best;
    return cur;
}

// Win 0x80bbd0 (dead code, no callers; Mac: dead-stripped): support by scanning every vertex
CnvVertex* cnvHullSupportScan(CnvHull* hull, const MeReal* dir, MeReal* dot)
{
    CnvVertex* v = hull->vertices;
    CnvVertex* best = v;
    MeReal bestDot = v->pos[0] * dir[0] + v->pos[1] * dir[1] + v->pos[2] * dir[2];
    for (++v; v != hull->vertices + hull->vertexCount; ++v) {
        MeReal d = v->pos[0] * dir[0] + v->pos[1] * dir[1] + v->pos[2] * dir[2];
        if (bestDot < d) {
            bestDot = d;
            best = v;
        }
    }
    *dot = bestDot;
    return best;
}

// Win 0x80bdd0 (dead code; regparm ECX / EDX): the slot of `neighbour` in v's neighbour list (the
// list end when absent, NULL without a vertex)
CnvVertex** cnvFindNeighbour(CnvVertex* v, CnvVertex* neighbour)
{
    if (v == 0)
        return 0;
    CnvVertex** p = v->neighbours;
    CnvVertex** end = v->neighbours + v->neighbourCount;
    while (p < end && *p != neighbour)
        ++p;
    return p;
}

// Win 0x80bdf0 (dead code): the neighbour of v after (step 1) or before (otherwise) `neighbour` in
// v's sorted list, wrapping around; `neighbour` itself without a vertex
CnvVertex* cnvNextNeighbour(CnvHull* /*hull: unused*/, CnvVertex* v, CnvVertex* neighbour, int step)
{
    if (v == 0)
        return neighbour;
    CnvVertex** begin = v->neighbours;
    CnvVertex** end = begin + v->neighbourCount;
    CnvVertex** p = begin;
    while (p < end && neighbour != *p)
        ++p;
    p = step == 1 ? p + 1 : p - 1;
    if (begin <= p && p < end)
        return *p;
    return step == 1 ? *begin : end[-1];
}

} // extern "C"

namespace {
// Mac 0x4b7724, Win 0x80b010 (regparm): a cylinder's support point
void CylinderSupport(void* cylinder, MeReal* out, MeReal* dot, const MeReal* dir)
{
    VincCylinderMaximumPointLocal((VincGeometryID)cylinder, dir, out);
    *dot = out[0] * dir[0] + out[1] * dir[1] + out[2] * dir[2];
}
} // namespace

extern "C" {

// Mac 0x4b777d, Win 0x80bd60
MeReal* cnvSupport(void* convex, CnvCylinderCache* cylinder, MeReal* hint, MeReal* dot, const MeReal* dir)
{
    if (cylinder == 0)
        return cnvHullSupport((CnvHull*)convex, (CnvVertex*)hint, dot, dir)->pos;
    int i = 0;
    while (cylinder->used[i] != 0)
        ++i;
    MeReal* p = cylinder->points[i];
    CylinderSupport(convex, p, dot, dir);
    return p;
}

// Mac 0x4b77da, Win 0x80bd00: NOTE: a cylinder's support point overwrites the point `out` refers to
MeReal* cnvSupportPoint(void* convex, int isCylinder, MeReal* out, MeReal* dot, const MeReal* dir)
{
    if (isCylinder == 0)
        return cnvHullSupport((CnvHull*)convex, (CnvVertex*)out, dot, dir)->pos;
    CylinderSupport(convex, out, dot, dir);
    return out;
}

// Mac 0x4b7813, Win 0x80b040
void cnvPairCacheInit(CnvPairCache* cache, CnvCylinderCache* cylA, CnvCylinderCache* cylB, MeReal* axis,
                            const VincConvexDesc* a, const VincConvexDesc* b)
{
    MeReal zero[3] = {0.0, 0.0, 0.0};
    const MeReal* pa = a->tm != 0 ? a->tm + 12 : zero;
    const MeReal* pb = b->tm != 0 ? b->tm + 12 : zero;
    MeReal z = pa[2] - pb[2], y = pa[1] - pb[1], x = pa[0] - pb[0];
    axis[0] = x;
    axis[1] = y;
    axis[2] = z;
    MeReal len = sqrt(x * x + y * y + z * z);
    if (len == 0.0) {
        axis[0] = 1.0;
        axis[1] = 0.0;
        axis[2] = 0.0;
    } else {
        MeReal inv = 1.0 / len;
        axis[0] = x * inv;
        axis[1] = y * inv;
        axis[2] = inv * z;
    }
    MeReal dot;
    if (cylA == 0) {
        cache->a[0] = ((CnvHull*)a->convex)->vertices->pos;
    } else {
        CylinderSupport(a->convex, cylA->points[0], &dot, axis);
        cache->a[0] = cylA->points[0];
        cylA->used[0] = 1;
        cylA->used[1] = 0;
        cylA->used[2] = 0;
        cylA->used[3] = 0;
    }
    if (cylB == 0) {
        cache->b[0] = ((CnvHull*)b->convex)->vertices->pos;
    } else {
        // NOTE: B's start point is taken along +axis too (both binaries)
        CylinderSupport(b->convex, cylB->points[0], &dot, axis);
        cache->b[0] = cylB->points[0];
        cylB->used[0] = 1;
        cylB->used[1] = 0;
        cylB->used[2] = 0;
        cylB->used[3] = 0;
    }
    cache->count = 1;
    cache->hintA = cache->a[0];
    cache->hintB = cache->b[0];
}

// Mac 0x4b7adc, Win 0x80be50
int cnvGJK(void* convexA, void* convexB, MeReal* tmA, MeReal* tmB, MeReal* closestA, MeReal* closestB,
                 CnvPairCache* cache, CnvSimplex* simplex, CnvCylinderCache* cylA, CnvCylinderCache* cylB,
                 MeReal margins, MeReal* distance2)
{
    Johnson j;
    memset(&j, 0, sizeof j);      // (stack in the original; read only where written)
    *distance2 = 0.0;
    MeReal r2 = margins * margins;
    MeReal prev = 1e30f;          // (a float constant in both binaries)
    int steps = 29;
    for (;;) {
        if (cache->count == 1) {
            simplex->lambda[0] = 1.0;
        } else {
            JohnsonDeltas(&j, simplex, cache->count);
            if (!JohnsonSelect(cache, simplex, cylA, cylB, &j))
                return *distance2 < r2 ? 2 : 3;
            if (3 < cache->count) {
                *distance2 = 0.0;
                return 0;
            }
        }
        Combine(closestA, simplex->pointsA, cache->count, simplex->lambda);
        Combine(closestB, simplex->pointsB, cache->count, simplex->lambda);
        MeReal v[3];
        v[2] = closestB[2] - closestA[2];
        v[1] = closestB[1] - closestA[1];
        v[0] = closestB[0] - closestA[0];
        MeReal d2 = v[0] * v[0] + v[1] * v[1] + v[2] * v[2];
        *distance2 = d2;
        if (d2 < 1e-16)
            return 1;
        MeReal nv[3] = {-v[0], -v[1], -v[2]};
        MeReal dA[3], dB[3];
        ToLocal(tmA, v, dA);
        ToLocal(tmB, nv, dB);
        MeReal ha, hb;
        cache->hintA = cnvSupport(convexA, cylA, cache->hintA, &ha, dA);
        cache->hintB = cnvSupport(convexB, cylB, cache->hintB, &hb, dB);
        MeReal s = ha + hb;
        if (tmA != 0)
            s = s + (tmA[12] * v[0] + tmA[13] * v[1] + tmA[14] * v[2]);
        if (tmB != 0)
            s = s + (tmB[12] * nv[0] + tmB[13] * nv[1] + tmB[14] * nv[2]);
        if (!(0.0 < s) && !(s != s)) {
            if (margins == 0.0 || !(s * s < r2 * *distance2))
                return 3;
        }
        MeReal t = *distance2 + s;
        if (t < 0.0 || t < 1e-08) {
            if (*distance2 < r2)
                return 2;
            return 3;
        }
        if (prev <= *distance2)
            return 4;
        int i = cache->count;
        cache->a[i] = cache->hintA;
        cache->b[i] = cache->hintB;
        simplex->lambda[i] = 0.0;
        if (cylA != 0)
            cylA->used[(MeVector3*)cache->a[cache->count] - cylA->points] = 1;
        if (cylB != 0)
            cylB->used[(MeVector3*)cache->b[cache->count] - cylB->points] = 1;
        ToWorld(tmA, cache->hintA, simplex->pointsA + cache->count * 3);
        ToWorld(tmB, cache->hintB, simplex->pointsB + cache->count * 3);
        ++cache->count;
        prev = *distance2;
        if (steps == 0)
            return 4;
        --steps;
    }
}

// Mac 0x4b888a, Win 0x80c3b0
MeReal cnvPenetrationDepth(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache,
                                 CnvCylinderCache* cylA, CnvCylinderCache* cylB, CnvSimplex* simplex, MeReal* normal)
{
    const int kMaxPoints = 211, kMaxFaces = 420, kMaxEdges = 90;
    MeReal P[kMaxPoints][3];
    Face F[kMaxFaces];
    Face* removed[kMaxFaces];          // (Mac: 60 entries; more visible faces overflow there)
    MeReal* E[kMaxEdges][2];
    MeReal ha, hb;
    int k, i;
    // NOTE: the n == 2 early exit below returns face 0 before it is computed (stack garbage in the
    // original); zeroed here
    memset(&F[0], 0, sizeof(Face));
    if (cache->count == 1)
        return 1.0;                    // NOTE: (as in the original) depth +1, normal untouched
    k = 0;
    for (i = 0; i != 2; ++i) {
        P[k][0] = simplex->pointsA[i * 3 + 0] - simplex->pointsB[i * 3 + 0];
        P[k][1] = simplex->pointsA[i * 3 + 1] - simplex->pointsB[i * 3 + 1];
        P[k][2] = simplex->pointsA[i * 3 + 2] - simplex->pointsB[i * 3 + 2];
        ++k;
        if (k >= kMaxPoints)
            k = 0;
    }
    if (cache->count == 2) {
        MeReal ez = P[0][2] - P[1][2];
        MeReal ey = P[0][1] - P[1][1];
        MeReal ex = P[0][0] - P[1][0];
        MeReal d[3];
        if (fabs(ex) <= fabs(ey)) {
            MeReal inv = 1.0 / sqrt(ez * ez + ey * ey);
            d[1] = -ez * inv;
            d[2] = ey * inv;
            d[0] = 0.0;
        } else {
            MeReal inv = 1.0 / sqrt(ez * ez + ex * ex);
            d[0] = ez * inv;
            d[2] = -ex * inv;
            d[1] = 0.0;
        }
        SupportPair(a, b, cache, cylA, cylB, d, &ha, &hb);
        MeReal v = fabs((((hb + ha) - (P[0][0] * d[0] + P[0][1] * d[1] + P[0][2] * d[2])) + TranslationDot(a->tm, d))
                        - TranslationDot(b->tm, d));
        if (v < 1e-08) {
            normal[0] = -F[0].n[0];
            normal[1] = -F[0].n[1];
            normal[2] = -F[0].n[2];
            return -F[0].d;
        }
        HintDifference(a, b, cache, P[k]);
    } else {
        P[k][0] = simplex->pointsA[6] - simplex->pointsB[6];
        P[k][1] = simplex->pointsA[7] - simplex->pointsB[7];
        P[k][2] = simplex->pointsA[8] - simplex->pointsB[8];
    }
    ++k;
    if (k >= kMaxPoints)
        k = 0;
    F[0].v[0] = P[0];
    F[0].v[1] = P[1];
    F[0].v[2] = P[2];
    int next;
    if (cache->count < 4) {
        FacePlane(&F[0]);
        SupportPair(a, b, cache, cylA, cylB, F[0].n, &ha, &hb);
        MeReal v = fabs((((ha + hb) + TranslationDot(a->tm, F[0].n)) - TranslationDot(b->tm, F[0].n)) - F[0].d);
        if (v < 1e-07) {
            normal[0] = -F[0].n[0];
            normal[1] = -F[0].n[1];
            normal[2] = -F[0].n[2];
            return -F[0].d;
        }
        HintDifference(a, b, cache, P[k]);
    } else {
        P[k][0] = simplex->pointsA[9] - simplex->pointsB[9];
        P[k][1] = simplex->pointsA[10] - simplex->pointsB[10];
        P[k][2] = simplex->pointsA[11] - simplex->pointsB[11];
        FacePlane(&F[0]);
    }
    next = k + 1;
    if (next >= kMaxPoints)
        next = 0;
    // the tetrahedron's other faces
    int nf = 1;
    for (int j = 1; j != 4; ++j) {
        F[nf].v[0] = P[nf];
        if ((j & 1) == 0) {
            F[nf].v[1] = P[(nf + 1) % 4];
            F[nf].v[2] = P[(nf + 2) % 4];
        } else {
            F[nf].v[2] = P[(nf + 1) % 4];
            F[nf].v[1] = P[(nf + 2) % 4];
        }
        FacePlane(&F[nf]);
        ++nf;
        if (nf >= kMaxFaces)
            nf = 0;
    }
    // the face farthest from the origin decides the orientation
    MeReal m = fabs(F[0].d);
    int best = 0;
    if (fabs(F[1].d) > m) {
        m = fabs(F[1].d);
        best = 1;
    }
    if (fabs(F[2].d) > m) {
        m = fabs(F[2].d);
        best = 2;
    }
    if (fabs(F[3].d) > m)
        best = 3;
    if (F[best].d < 0.0) {
        // inside out: turn every face
        for (i = 0; i != 4; ++i) {
            MeReal* t = F[i].v[0];
            F[i].v[0] = F[i].v[1];
            F[i].v[1] = t;
            F[i].d = -F[i].d;
            F[i].n[0] = -F[i].n[0];
            F[i].n[1] = -F[i].n[1];
            F[i].n[2] = -F[i].n[2];
        }
    }
    // expand towards the closest face
    best = 0;
    int iter = 0;
    MeReal prevD = -DBL_MAX;
    for (i = 1;;) {
        for (; i < nf; ++i)
            if (F[i].d < F[best].d)
                best = i;
        MeReal d = F[best].d;
        if (d < prevD)
            break;
        SupportPair(a, b, cache, cylA, cylB, F[best].n, &ha, &hb);
        ha = TranslationDot(a->tm, F[best].n) + ha;
        hb = hb - TranslationDot(b->tm, F[best].n);
        if ((hb + ha) - F[best].d < 1e-07)
            break;
        HintDifference(a, b, cache, P[next]);
        MeReal* np = P[next];
        ++next;
        if (next >= kMaxPoints)
            next = 0;
        // the horizon: edges of the visible faces seen once
        E[0][0] = F[best].v[0];
        E[0][1] = F[best].v[1];
        E[1][0] = F[best].v[1];
        E[1][1] = F[best].v[2];
        E[2][0] = F[best].v[2];
        E[2][1] = F[best].v[0];
        int ne = 3, nr = 1;
        removed[0] = &F[best];
        for (int f = 0; f != nf; ++f) {
            if (f == best)
                continue;
            const MeReal* v0 = F[f].v[0];
            if (!(0.0 < (np[0] - v0[0]) * F[f].n[0] + (np[1] - v0[1]) * F[f].n[1] + (np[2] - v0[2]) * F[f].n[2]))
                continue;
            removed[nr++] = &F[f];
            for (int e = 0; e != 3; ++e) {
                MeReal* p = F[f].v[e];
                MeReal* q = F[f].v[(e + 1) % 3];
                bool unique = true;
                for (int x = 0; x < ne; ++x) {
                    if ((p == E[x][0] && q == E[x][1]) || (p == E[x][1] && q == E[x][0])) {
                        E[x][0] = 0;
                        E[x][1] = 0;
                        unique = false;
                        break;
                    }
                }
                if (unique && ne < kMaxEdges) {
                    E[ne][0] = p;
                    E[ne][1] = q;
                    ++ne;
                }
            }
        }
        if ((58 < nf || 28 < next || 3 < ne) && next < kMaxPoints && nf < kMaxFaces) {
            for (int e = 0; e != ne; ++e) {
                if (E[e][0] == 0)
                    continue;
                Face* t;
                if (nr < 1) {
                    if (58 < nf)
                        continue;
                    t = &F[nf++];
                } else {
                    t = removed[--nr];
                }
                t->v[0] = np;
                t->v[1] = E[e][0];
                t->v[2] = E[e][1];
                FacePlane(t);
            }
            // close the gaps left by unused removed faces with the last faces
            for (int x = nr; x > 0; --x) {
                Face* slot = removed[x - 1];
                Face* last = &F[nf - nr + x - 1];
                if (slot != last)
                    *slot = *last;
            }
            nf -= nr;
        }
        ++iter;
        if (99 < iter)
            break;
        best = 0;
        i = 1;
        prevD = d;
    }
    normal[0] = -F[best].n[0];
    normal[1] = -F[best].n[1];
    normal[2] = -F[best].n[2];
    return -F[best].d;
}

// Mac 0x4ba361, Win 0x80d6e0
MeReal cnvDistance(const VincConvexDesc* a, const VincConvexDesc* b, CnvPairCache* cache,
                         CnvCylinderCache* cylA, CnvCylinderCache* cylB, MeReal* normal)
{
    CnvSimplex s;
    s.pointsA = new MeReal[12];
    s.pointsB = new MeReal[12];
    s.lambda[0] = 1.0;
    SimplexToWorldA(&s, cache, a->convex, a->tm);
    SimplexToWorldB(&s, cache, b->convex, b->tm);
    MeReal margins = a->margin + b->margin;
    MeReal depth = margins;          // (Win: returned for an unknown cnvPenetrationMethod)
    // NOTE: cnvGJK can return 2 from its first Johnson step (a degenerate warm-started simplex)
    // without writing the closest points; the original then normalises stale stack data (often
    // NaN). Zeroed here: the normal comes out (0, 0, 0)
    MeReal ca[3] = {0.0, 0.0, 0.0}, cb[3] = {0.0, 0.0, 0.0}, d2;
    int r = cnvGJK(a->convex, b->convex, a->tm, b->tm, ca, cb, cache, &s, cylA, cylB, margins, &d2);
    if (r == 3) {
        d2 = margins * margins + 1.0;
    } else if (r < 2) {
        if (cnvPenetrationMethod == 0)
            depth = cnvPenetrationDepth(a, b, cache, cylA, cylB, &s, normal);
        else if (cnvPenetrationMethod == 1)
            depth = AxisPenetration(a, b, cache, cylA, cylB, normal);
        delete[] s.pointsA;
        delete[] s.pointsB;
        return depth;
    }
    depth = sqrt(d2);
    MeReal z = ca[2] - cb[2], y = ca[1] - cb[1], x = ca[0] - cb[0];
    normal[0] = x;
    normal[1] = y;
    normal[2] = z;
    MeReal len = sqrt(x * x + y * y + z * z);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        normal[0] = x * inv;
        normal[1] = y * inv;
        normal[2] = inv * z;
    }
    delete[] s.pointsA;
    delete[] s.pointsB;
    return depth;
}

// ---- convex vs convex ----

// Mac 0x4b6426, Win 0x7b2730
int VincConvexConvexIntersect(VincIntersectResult* r, const VincConvexDesc* a, const VincConvexDesc* b,
                                    CnvCylinderCache* cylA, CnvCylinderCache* cylB, const MeReal* /*padding: unused*/,
                                    CnvPairCache* cache, MeReal* axis, int maxCount)
{
    if (cache->count == 0)
        cnvPairCacheInit(cache, cylA, cylB, axis, a, b);
    ResultClear(r);
    MeReal d = cnvDistance(a, b, cache, cylA, cylB, axis);
    if (d == 0.0 && axis[0] * axis[0] + axis[1] * axis[1] + axis[2] * axis[2] == 0.0)
        return 0;
    // NOTE: the models' padding is not applied (both binaries)
    d = (d - a->margin) - b->margin;
    if (0.0 < d)
        return 0;
    int n = r->contactMaxCount;
    if (maxCount <= r->contactMaxCount)
        n = maxCount;
    if (n == 0 || !(d < 0.0)) {
        ResultClear(r);
        return 1;
    }
    MeReal dirA[3], dirB[3];
    ToLocal(a->tm, axis, dirA);
    ToLocal(b->tm, axis, dirB);
    dirA[0] = -dirA[0];
    dirA[1] = -dirA[1];
    dirA[2] = -dirA[2];
    MeReal bufA[6], bufB[6];
    GenerateContacts(r, a, b, dirA, dirB, axis, d, cylA != 0 ? bufA : 0, cylB != 0 ? bufB : 0, cache->hintA,
                     cache->hintB);
    r->normal[0] = axis[0];
    r->normal[1] = axis[1];
    r->normal[2] = axis[2];
    return 1;
}

// Mac 0x4b6755, Win 0x7b28c0
int VincConvexConvexIntersectCached(VincIntersectResult* r, const VincConvexDesc* a, const VincConvexDesc* b,
                                          const MeReal* padding, CnvPairCache* cache, int maxCount)
{
    return VincConvexConvexIntersect(r, a, b, 0, 0, padding, cache, cache->axis, maxCount);
}

// Mac 0x4b38ff, Win 0x7b1400
int MEAPI VincConvexPairGoodbye(VincModelPair* pair)
{
    return VincConvexPairDataRelease(pair->model1, pair->model2, pair->pairData);
}

// Mac 0x4b3922, Win 0x7b13e0
int MEAPI VincConvexPairHello(VincModelPair* pair)
{
    return VincConvexPairDataAcquire(pair->model1, pair->model2, &pair->pairData);
}

// Mac 0x4b3945, Win 0x7b1420
int MEAPI VincConvexMeshRegisterSelfInteraction(void)
{
    VincInteractions i;
    i.helloFn = VincConvexPairHello;
    i.goodbyeFn = VincConvexPairGoodbye;
    i.intersectFn = VincConvexConvexPairIntersect;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;           // NOTE: left uninitialised by the original
    i.distanceBoundedFn = 0;
    int t1 = VincConvexGetTypeId();
    int t2 = VincConvexGetTypeId();
    VincGeometryPairRegisterInteractions(t2, t1, &i, 0);
    return 1;
}

// ---- the geometry type ----

// Mac 0x4b67a8, Win 0x7b5f10
MeI16 MEAPI VincConvexMeshGetTypeId(void)
{
    return s_convexTypeId;
}

// Win 0x7b66f0
int MEAPI VincConvexMeshGetType(void)
{
    return s_convexTypeId;
}

// Mac 0x4b67ba, Win 0x7b6480
void MEAPI VincConvexMeshGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    Copy3(center, Convex(g)->center);
    *radius = Convex(g)->radius;
}

// Mac 0x4b67ee, Win 0x7b61b0
void MEAPI VincConvexMeshDestroy(VincGeometryID g)
{
    if (g != 0) {
        cnvHullDestroy(Convex(g)->hull);
        MeMemoryAPI.destroy(g);
    }
}

} // extern "C"

namespace {

// the support value of the hull along the world direction dw (one row of the AABB)
MeReal AxisExtent(VincConvexGeometry* g, const MeReal* tm, const MeReal* dw, CnvVertex* hint)
{
    MeReal d[3];
    VincTMInverseRotate(tm, dw, d);
    MeReal h;
    cnvSupport(g->hull, 0, hint->pos, &h, d);
    return h;
}

// Mac 0x4b7182, Win 0x7b61d0: aabb6 = {min x, max x, min y, max y, min z, max z} (hints not updated)
void ConvexAABB(VincGeometryID geometry, const MeReal* tm, MeReal* aabb6)
{
    static const MeReal kDirs[6][3] = {{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, -1.0, 0.0},
                                       {0.0, 1.0, 0.0},  {0.0, 0.0, -1.0}, {0.0, 0.0, 1.0}};
    VincConvexGeometry* g = Convex(geometry);
    for (int k = 0; k < 6; ++k)
        aabb6[k] = AxisExtent(g, tm, kDirs[k], g->hint[k]);
    for (int k = 0; k < 3; ++k) {
        aabb6[2 * k] = (tm[12 + k] - aabb6[2 * k]) - g->margin;
        aabb6[2 * k + 1] = tm[12 + k] + g->margin + aabb6[2 * k + 1];
    }
}

// Win 0x7b5f20 (Mac: dead-stripped): centre = mean of the vertices, radius = farthest + margin
void ComputeBSphere(VincConvexGeometry* g)
{
    int n = g->hull->vertexCount;
    CnvVertex* v = g->hull->vertices;
    g->center[0] = 0.0;
    g->center[1] = 0.0;
    g->center[2] = 0.0;
    int i;
    for (i = 0; i < n; ++i) {
        g->center[0] = v[i].pos[0] + g->center[0];
        g->center[1] = v[i].pos[1] + g->center[1];
        g->center[2] = v[i].pos[2] + g->center[2];
    }
    MeReal inv = 1.0 / (MeReal)n;
    g->center[0] = g->center[0] * inv;
    g->center[1] = g->center[1] * inv;
    g->center[2] = inv * g->center[2];
    MeReal maxd2 = 0.0;
    for (i = 0; i < n; ++i) {
        MeReal dx = v[i].pos[0] - g->center[0];
        MeReal dy = v[i].pos[1] - g->center[1];
        MeReal dz = v[i].pos[2] - g->center[2];
        MeReal d2 = dx * dx + dy * dy + dz * dz;
        if (maxd2 < d2)
            maxd2 = d2;
    }
    g->radius = sqrt(maxd2) + g->margin;
}

} // namespace

extern "C" {

// Mac 0x4b7578, Win 0x7b6450
void MEAPI VincConvexMeshUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                                    MeReal* aabb6)
{
    ConvexAABB(g, &tm[0][0], aabb6);
    VincAABBGetMinMax(aabb6, minCorner, maxCorner);
}

// Mac 0x4b6d86, Win 0x7b6540: {min x, max x, min y, max y}
void MEAPI VincConvexMeshGetXYAABB(VincGeometryID geometry, MeMatrix4 tm4, MeReal* out4)
{
    static const MeReal kDirs[4][3] = {{-1.0, 0.0, 0.0}, {1.0, 0.0, 0.0}, {0.0, -1.0, 0.0}, {0.0, 1.0, 0.0}};
    VincConvexGeometry* g = Convex(geometry);
    const MeReal* tm = &tm4[0][0];
    for (int k = 0; k < 4; ++k)
        out4[k] = AxisExtent(g, tm, kDirs[k], g->hint[k]);
    for (int k = 0; k < 2; ++k) {
        out4[2 * k] = (tm[12 + k] - out4[2 * k]) - g->margin;
        out4[2 * k + 1] = tm[12 + k] + g->margin + out4[2 * k + 1];
    }
}

// Mac 0x4b7034, Win 0x7b64b0
void MEAPI VincConvexMeshMaximumPoint(VincGeometryID g, MeMatrix4 tm4, const MeVector3 dir, MeVector3 out)
{
    const MeReal* tm = &tm4[0][0];
    MeReal d[3];
    VincTMInverseRotate(tm, dir, d);
    MeReal h;
    MeReal* p = cnvSupport(Convex(g)->hull, 0, 0, &h, d);
    VincTMTransform(tm, p, out);
}

// Mac 0x4b6892, Win 0x7b6760
int MEAPI VincConvexMeshGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    int nf = VincConvexGetFaceCount(g);
    int tris = 0;
    int f;
    for (f = 0; f < nf; ++f) {
        int c = VincConvexGetFaceVertexCount(g, f);
        if (2 < c)
            tris = tris + -2 + c;
    }
    MeReal* tetra = (MeReal*)MeMemoryAPI.create(tris * 13 * sizeof(MeReal));
    MeVector3 center;
    MeReal radius;
    VincConvexMeshGetBSphere(g, center, &radius);
    int k = 0;
    for (f = 0; f < nf; ++f) {
        int c = VincConvexGetFaceVertexCount(g, f);
        if (2 < c) {
            // NOTE: a strip (j, j+1, j+2), not a fan: polygons with more than four corners are
            // covered wrongly (both binaries)
            for (int j = 0; j < c - 2; ++j) {
                MeReal a[3], b[3], cc[3];
                const MeReal* p = VincConvexGetFaceVertexPtr(g, f, j);
                a[0] = p[0];
                a[1] = p[1];
                a[2] = p[2];
                p = VincConvexGetFaceVertexPtr(g, f, j + 1);
                b[0] = p[0];
                b[1] = p[1];
                b[2] = p[2];
                p = VincConvexGetFaceVertexPtr(g, f, j + 2);
                a[0] = a[0] - center[0];
                a[1] = a[1] - center[1];
                a[2] = a[2] - center[2];
                b[0] = b[0] - center[0];
                b[1] = b[1] - center[1];
                b[2] = b[2] - center[2];
                cc[0] = p[0] - center[0];
                cc[1] = p[1] - center[1];
                cc[2] = p[2] - center[2];
                VincTetraMassProperties(tetra + k * 13, a, cc, b);
                ++k;
            }
        }
    }
    MeReal sum[13];
    VincSumMassProperties(tetra, k, sum);
    *volume = sum[0];
    MeReal* tm = &relTM[0][0];
    tm[0] = 1.0;
    tm[1] = 0.0;
    tm[2] = 0.0;
    tm[3] = 0.0;
    tm[4] = 0.0;
    tm[5] = 1.0;
    tm[6] = 0.0;
    tm[7] = 0.0;
    tm[8] = 0.0;
    tm[9] = 0.0;
    tm[10] = 1.0;
    tm[11] = 0.0;
    tm[15] = 1.0;
    tm[12] = sum[1] + center[0];
    tm[13] = sum[2] + center[1];
    tm[14] = sum[3] + center[2];
    MeReal* in = &inertia[0][0];
    for (int x = 0; x < 9; ++x)
        in[x] = sum[4 + x];
    MeMemoryAPI.destroy(tetra);
    return 1;
}

// Mac 0x4b6826, Win 0x7b6a20
void MEAPI VincConvexMeshRegisterType(void)
{
    s_convexTypeId = (MeI16)VincGeometryRegisterType(9, s_convexTypeName, VincConvexMeshDestroy, VincConvexMeshUpdateAABB,
                                                     VincConvexMeshGetXYAABB, VincConvexMeshGetBSphere,
                                                     VincConvexMeshMaximumPoint, VincConvexMeshGetMassProperties);
}

// Win 0x7b6700
int MEAPI VincConvexMeshInit(VincConvexGeometry* g, CnvHull* hull, MeReal margin)
{
    g->margin = margin;
    g->hull = hull;
    ComputeBSphere(g);
    for (int k = 0; k < 6; ++k)
        g->hint[k] = g->hull->vertices;
    return 1;
}

// Win 0x7b6a60
VincGeometryID MEAPI VincConvexMeshCreate(CnvHull* hull, MeReal margin)
{
    VincConvexGeometry* g = (VincConvexGeometry*)MeMemoryAPI.create(sizeof(VincConvexGeometry));
    VincGeometryInit(&g->base, (unsigned short)s_convexTypeId);
    VincConvexMeshInit(g, hull, margin);
    return &g->base;
}

// Win 0x7b6aa0
VincGeometryID MEAPI VincConvexMeshCreateSegment(MeReal radius, MeReal halfLength)
{
    CnvHull* hull = cnvCreateSegment(halfLength);
    VincConvexGeometry* g = (VincConvexGeometry*)MeMemoryAPI.create(sizeof(VincConvexGeometry));
    VincGeometryInit(&g->base, (unsigned short)s_convexTypeId);
    VincConvexMeshInit(g, hull, radius);
    return &g->base;
}

} // extern "C"
