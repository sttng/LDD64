/* Vortex 5.0.0 build 800 "vortexldd41" - box / box pair test and the swept-box safe time shared by
 * the primitive pair tests (restored). Mac 0x4db81f-0x4dcf2d (557e), Win
 * 0x81c740-0x81d380. Names (name inferred) after the Mcd pair-test naming.
 *
 * Bit-exact contract (Win x87, 53-bit precision): every sum below keeps the association order of
 * the Win disassembly; it is not always the natural one (e.g. VincBoxBoxSeparation's projected
 * radii). */
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincPairTests.h"
#include "Vinc/VincPairTestsB.h"
#include "Vinc/VincPairSupport.h"
#include "Vinc/VincBoxSweep.h"
#include "Vinc/VincMath.h"

#include <float.h>
#include <math.h>
#include <string.h>

/* Mac 0x4db81f (regparm EAX = result; Win: inlined) */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

/* next axis: 0 -> 1 -> 2 -> 0 (the original's ((i >> 1) + 1 + i) & 3) */
#define NEXT(i) ((((i) >> 1) + 1 + (i)) & 3)

/* Mac 0x4db832, Win 0x81ca90 (cdecl): separating-axis test of two boxes; rel = box 2 in box 1's
 * frame (rows: box 2's axes, row 3: its position), r1 / r2 half extents. The 15 axes are scored
 * by separation (edge axes normalised); returns 0 as soon as one separates by more than *pad.
 * Otherwise *sep = the largest separation, n = its axis (box 1 frame, pointing from box 2 to box 1),
 * *d = the offset of box 1's support plane along n, dims: 0x?02 face of box 1, 0x20? face of box
 * 2 (the low digits count the near-zero direction cosines), 0x101 edge / edge. padValue: unused. */
int VincBoxBoxSeparation(MeReal* sep, MeReal* n, MeReal* d, MeI16* dims, const MeReal* pad,
                         const MeReal* r1, const MeReal* r2, const MeReal* rel, MeReal padValue)
{
    MeReal absR[12];          /* |rel| rows 0..2 (stride 4) */
    MeReal eps = *pad * *pad * 10.0;
    MeReal best = -DBL_MAX;
    MeReal inv = 1.0, axis = 1.0, dBest = 0.0;
    MeVector3 v;
    unsigned char code = 0xf;
    int i, j, k, flag;
    (void)padValue;
    for (i = 0; i < 3; ++i) {
        absR[i * 4 + 0] = fabs(rel[i * 4 + 0]);
        absR[i * 4 + 1] = fabs(rel[i * 4 + 1]);
        absR[i * 4 + 2] = fabs(rel[i * 4 + 2]);
    }
    /* box 1's face normals */
    for (i = 0; i < 3; ++i) {
        MeReal t = rel[12 + i];
        MeReal s = fabs(t) - (((absR[i] * r2[0] + r1[i]) + absR[4 + i] * r2[1]) + absR[8 + i] * r2[2]);
        if (best < s) {
            best = s;
            code = (unsigned char)(i | 0xc);
            dBest = -r1[i] - s;
            axis = t;
            if (*pad < s)
                return 0;
        }
    }
    /* box 2's face normals */
    for (j = 0; j < 3; ++j) {
        const MeReal* R = rel + j * 4;
        const MeReal* A = absR + j * 4;
        MeReal p = (R[0] * rel[12] + R[1] * rel[13]) + rel[14] * R[2];
        MeReal ap = fabs(p);
        MeReal s = ap - (((A[0] * r1[0] + r2[j]) + r1[1] * A[1]) + A[2] * r1[2]);
        if (best < s) {
            best = s;
            code = (unsigned char)((j << 2) | 3);
            dBest = r2[j] - ap;
            axis = p;
            if (*pad < s)
                return 0;
        }
    }
    /* edge / edge: box 2's axis i x box 1's axis k */
    flag = best < 0.0;
    for (i = 0; i < 3; ++i) {
        int u = NEXT(i), w = NEXT(u);
        const MeReal* R = rel + i * 4;
        const MeReal* A = absR + i * 4;
        for (k = 0; k < 3; ++k) {
            int k1 = NEXT(k), k2 = NEXT(k1);
            MeReal q = absR[w * 4 + k] * r2[u] + r2[w] * absR[u * 4 + k];
            MeReal c = R[k1] * rel[12 + k2] - R[k2] * rel[12 + k1];
            MeReal ac = fabs(c);
            MeReal s = (ac - (A[k1] * r1[k2] + A[k2] * r1[k1])) - q;
            MeReal l = 1.0 - R[k] * R[k];
            if (!(eps < l))
                continue; /* (nearly) parallel axes */
            if (s < 0.0) {
                if (!flag || best * best * l <= s * s)
                    continue;
                flag = 1;
            } else {
                if (eps * l < s * s)
                    return 0;
                if (!flag && best * best * l >= s * s)
                    continue;
                flag = 0;
            }
            inv = 1.0 / sqrt(l);
            code = (unsigned char)((i << 2) | k);
            best = s * inv;
            dBest = (q - ac) * inv;
            axis = c;
        }
    }
    *sep = best;
    *d = dBest;
    if ((code & 0xc) == 0xc) {
        int a = code & 3;
        int cnt = (absR[a] < 0.0001) + (absR[4 + a] < 0.0001) + (absR[8 + a] < 0.0001);
        n[0] = 0.0;
        n[1] = 0.0;
        n[2] = 0.0;
        n[a] = (axis < 0.0) ? 1.0 : -1.0;
        *dims = (MeI16)((cnt << 8) | 2);
        return 1;
    }
    if ((code & 3) == 3) {
        int b = (code >> 2) & 3;
        int cnt = (absR[b * 4] < 0.0001) + (absR[b * 4 + 1] < 0.0001) + (absR[b * 4 + 2] < 0.0001);
        n[0] = rel[b * 4];
        n[1] = rel[b * 4 + 1];
        n[2] = rel[b * 4 + 2];
        if (0.0 < axis) {
            n[0] = n[0] * -1.0;
            n[1] = n[1] * -1.0;
            n[2] = n[2] * -1.0;
        }
        *dims = (MeI16)(cnt | 0x200);
        return 1;
    } else {
        int a = code & 3, b = (code >> 2) & 3;
        int u = NEXT(a), w = NEXT(u);
        v[a] = 0.0;
        if (!(axis < 0.0)) {
            v[u] = rel[b * 4 + w];
            v[w] = -rel[b * 4 + u];
        } else {
            v[u] = -rel[b * 4 + w];
            v[w] = rel[b * 4 + u];
        }
        n[0] = inv * v[0];
        n[1] = v[1] * inv;
        n[2] = inv * v[2];
        *dims = 0x101;
        return 1;
    }
}

/* inverse of a rigid TM (Win 0x75bfd0, a matrix method there; Mac: inlined) */
static void TMInverse(const MeReal* m, MeReal* out)
{
    MeReal x, y, z;
    out[0] = m[0];
    out[1] = m[4];
    out[2] = m[8];
    out[4] = m[1];
    out[5] = m[5];
    out[6] = m[9];
    out[8] = m[2];
    out[9] = m[6];
    out[10] = m[10];
    x = -m[12];
    y = -m[13];
    z = -m[14];
    out[12] = (out[4] * y + out[0] * x) + out[8] * z;
    out[13] = (out[5] * y + x * out[1]) + out[9] * z;
    out[14] = (x * out[2] + out[6] * y) + out[10] * z;
}

/* Mac 0x4dc122, Win 0x81c9a0: the contact point candidates of two boxes in box 1's frame: box 2's
 * features against box 1 (VincBoxClipBoxPoints), then box 1's against box 2, transformed back with
 * rel; appended at *cursor */
void MEAPI VincBoxBoxContactPoints(MeReal** cursor, const MeReal* rel, const MeReal* r1, const MeReal* r2, MeReal pad)
{
    MeReal inv[16], local[72 * 3];
    MeReal *end = local, *p;
    VincBoxClipBoxPoints(cursor, rel, r1, r2, pad);
    TMInverse(rel, inv);
    VincBoxClipBoxPoints(&end, inv, r2, r1, pad);
    for (p = local; p != end; p += 3) {
        MeReal* o = *cursor;
        *cursor = o + 3;
        VincTMTransform(rel, p, o);
    }
}

/* Mac 0x4dc3d5, Win 0x81d0a0 (model1, model2 = boxes) */
int MEAPI VincBoxBoxIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    MeReal* tm1 = VincModelGetTransformPtr(pair->model1);
    MeReal* tm2 = VincModelGetTransformPtr(pair->model2);
    MeReal pad = VincModelGetPadding(pair->model1) + VincModelGetPadding(pair->model2);
    MeReal* r1 = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    MeReal* r2 = VincBoxGetRadii(VincModelGetGeometry(pair->model2));
    MeReal rel[16], points[48 * 3], n[3], sep, d;
    MeReal *cursor = points, *p;
    VincContact contacts[48];
    VincRequest* rq;
    MeI16 dims;
    int cnt;
    VincTMMakeRelative(tm2, tm1, rel);
    if (!VincBoxBoxSeparation(&sep, n, &d, &dims, &pad, r1, r2, rel, pad)) {
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
    VincBoxBoxContactPoints(&cursor, rel, r1, r2, pad);
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
    if (result->contactCount == 0) {
        result->touch = 0;
        return 0;
    }
    result->contactCount = VincContactSimplify(result->normal, contacts, result->contactCount, result);
    result->touch = 1;
    return 1;
}

/* Mac 0x4dc35b, Win 0x81d330 */
int MEAPI VincBoxBoxRegisterInteraction(void)
{
    VincInteractions i;
    memset(&i, 0, sizeof i);
    i.intersectFn = VincBoxBoxIntersect;
    i.sweptIntersectFn = VincIntersectSafeTime;
    i.safeTimeFn = VincBoxBoxSafeTime;
    VincGeometryPairRegisterInteractions(VincBoxGetTypeId(), VincBoxGetTypeId(), &i, 0);
    return 1;
}

/* Mac 0x4dc9ee, Win 0x81c740: conservative safe time of two moving boxes (half extents radii1 /
 * radii2 along the models' axes; the callers bound spheres / capsules / cylinders by boxes; the
 * V3.6 name "ellipsoid" is kept). maxTime is cut into n + 1 steps, n = (|w1| + |w2|) maxTime / 0.2
 * (angular speeds); in each step the boxes sweep linearly (VincBoxSweepTime) from the poses
 * integrated to the start of the step. result->time = the first possible contact time (maxTime
 * when none); returns 1 when one was found. */
int MEAPI VincEllipsoidSafeTime(VincModelPair* pair, MeReal maxTime, const MeReal* radii1, const MeReal* radii2,
                                VincSafeTimeResult* result)
{
    MeReal *v1, *v2, *w1, *w2, *tm1, *tm2;
    MeReal q1[4], q2[4], c1[16], c2[16], dt, t;
    int n, i;
    result->time = maxTime;
    result->pair = pair;
    v1 = VincModelGetLinearVelocityPtr(pair->model1);
    v2 = VincModelGetLinearVelocityPtr(pair->model2);
    w1 = VincModelGetAngularVelocityPtr(pair->model1);
    w2 = VincModelGetAngularVelocityPtr(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    MeQuaternionFromTM(q1, (const MeReal(*)[4])tm1);
    MeQuaternionFromTM(q2, (const MeReal(*)[4])tm2);
    {
        MeReal s2 = sqrt((w2[0] * w2[0] + w2[1] * w2[1]) + w2[2] * w2[2]);
        MeReal s1 = sqrt((w1[0] * w1[0] + w1[1] * w1[1]) + w1[2] * w1[2]);
        n = (int)(((s1 + s2) * maxTime) / 0.2);
    }
    dt = maxTime / (MeReal)(n + 1);
    memcpy(c1, tm1, sizeof c1);
    memcpy(c2, tm2, sizeof c2);
    for (i = 0; i < n + 1; ++i) {
        if (VincBoxSweepTime(radii1, c1, v1, radii2, c2, v2, dt, &t)) {
            result->time = (MeReal)i * dt + t;
            return 1;
        }
        if (i < n) {
            MeReal h = (MeReal)(i + 1) * dt;
            MeMatrix4TMUpdateFromVelocitiesCOM((MeReal(*)[4])c1, 1e-14, h, v1, w1, (const MeReal(*)[4])tm1, q1,
                                               pair->model1->negCOM);
            MeMatrix4TMUpdateFromVelocitiesCOM((MeReal(*)[4])c2, 1e-14, h, v2, w2, (const MeReal(*)[4])tm2, q2,
                                               pair->model2->negCOM);
        }
    }
    return 0;
}

/* Mac 0x4dceb9, Win 0x81c950 */
int MEAPI VincBoxBoxSafeTime(VincModelPair* pair, MeReal maxTime, VincSafeTimeResult* result)
{
    MeReal* r1 = VincBoxGetRadii(VincModelGetGeometry(pair->model1));
    MeReal* r2 = VincBoxGetRadii(VincModelGetGeometry(pair->model2));
    return VincEllipsoidSafeTime(pair, maxTime, r1, r2, result);
}
