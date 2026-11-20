/* Vortex 5.0.0 build 800 "vortexldd41" - Me math helpers (restored).
 * Mac __text 0x487e9f-0x489c67 (TU 530 Me block); Win pairs in the comments. Names inferred.
 * FP operation order follows the Mac disassembly (not Ghidra's re-associated sums); checked bit for
 * bit against Win by the restoration's diff tests. sin / cos / asin / atan2 go through
 * Me/MeCrtMath.h (MeSinMul / MeCosMul where Win multiplies the unrounded x87 _CIsin / _CIcos
 * result). */
#include "Me/MeMath.h"
#include <math.h>
#include "Me/MeCrtMath.h"

/* Mac 0x644570 */
static const int sNext[3] = { 1, 2, 0 };

/* Mac 0x487e9f */
void MEAPI MeQuaternionMakeGMatrix(MeReal G[3][4], const MeVector4 q)
{
    G[0][0] = -q[1];
    G[1][0] = -q[2];
    G[2][0] = -q[3];
    G[2][3] = G[1][2] = G[0][1] = q[0];
    G[2][2] = q[1];
    G[1][3] = -q[1];
    G[0][3] = q[2];
    G[2][1] = -q[2];
    G[1][1] = q[3];
    G[0][2] = -q[3];
}

/* Mac 0x487f37, Win 0x74e140 */
int MEAPI MeMatrix3Invert(MeMatrix3 m)
{
    MeReal a00 = m[0][0], a01 = m[0][1], a02 = m[0][2];
    MeReal a10 = m[1][0], a11 = m[1][1], a12 = m[1][2];
    MeReal a20 = m[2][0], a21 = m[2][1], a22 = m[2][2];
    MeReal t = -a02 * a11;
    MeReal det = t * a20 + a01 * a12 * a20 + a02 * a10 * a21 - a00 * a12 * a21 - a01 * a10 * a22
                 + a11 * a00 * a22;
    MeReal r[9];
    int i;
    if (det == 0.0)
        return 0;
    det = 1.0 / det;
    r[0] = a11 * a22 - a12 * a21;
    r[1] = a02 * a21 - a01 * a22;
    r[2] = t + a01 * a12;
    r[3] = a20 * a12 - a10 * a22;
    r[4] = -a02 * a20 + a00 * a22;
    r[5] = a02 * a10 - a00 * a12;
    r[6] = a10 * a21 - a11 * a20;
    r[7] = a01 * a20 - a00 * a21;
    r[8] = a11 * a00 - a01 * a10;
    for (i = 0; i < 3; ++i) {
        m[i][0] = r[i * 3 + 0] * det;
        m[i][1] = r[i * 3 + 1] * det;
        m[i][2] = r[i * 3 + 2] * det;
    }
    return 1;
}

/* Mac 0x488192, Win 0x74e2b0: m symmetric (reads m[1][0], m[2][0], m[2][1] and m[1][2] once) */
void MEAPI MeMatrix3InvertSymmetric(const MeMatrix3 m, MeVector3 row0, MeVector3 row1, MeVector3 row2)
{
    MeReal a00 = m[0][0], a11 = m[1][1], a22 = m[2][2];
    MeReal a21 = m[2][1], a10 = m[1][0], a20 = m[2][0];
    MeReal c01 = a21 * a20 - a22 * a10;
    MeReal c02 = a21 * a10 - a11 * a20;
    MeReal inv = 1.0 / ((a11 * a22 - a21 * a21) * a00 + a10 * c01 + a20 * c02);
    row0[0] = (a11 * a22 - a21 * m[1][2]) * inv;
    row0[1] = c01 * inv;
    row0[2] = c02 * inv;
    row1[1] = (a22 * a00 - a20 * a20) * inv;
    row1[2] = (a20 * a10 - a21 * a00) * inv;
    row2[2] = (a00 * a11 - a10 * a10) * inv;
    row1[0] = row0[1];
    row2[0] = row0[2];
    row2[1] = row1[2];
}

/* Mac 0x4882fb */
void MEAPI MeMatrix4TMCompound(MeMatrix4 ab, const MeMatrix4 a, const MeMatrix4 b)
{
    int i;
    for (i = 0; i < 3; ++i) {
        ab[i][0] = a[i][0] * b[0][0] + a[i][1] * b[1][0] + a[i][2] * b[2][0];
        ab[i][1] = a[i][0] * b[0][1] + a[i][1] * b[1][1] + a[i][2] * b[2][1];
        ab[i][2] = a[i][0] * b[0][2] + a[i][1] * b[1][2] + a[i][2] * b[2][2];
        ab[i][3] = 0.0;
    }
    ab[3][0] = a[3][0] * b[0][0] + a[3][1] * b[1][0] + a[3][2] * b[2][0] + b[3][0];
    ab[3][1] = a[3][0] * b[0][1] + a[3][1] * b[1][1] + a[3][2] * b[2][1] + b[3][1];
    ab[3][2] = a[3][0] * b[0][2] + a[3][1] * b[1][2] + a[3][2] * b[2][2] + b[3][2];
    ab[3][3] = 1.0;
}

/* Mac 0x488595 */
void MEAPI MeMatrix4TMCompoundRotation(MeMatrix4 ab, const MeMatrix4 a, const MeMatrix4 b)
{
    int i;
    for (i = 0; i < 3; ++i) {
        ab[i][0] = a[i][0] * b[0][0] + a[i][1] * b[1][0] + a[i][2] * b[2][0];
        ab[i][1] = a[i][0] * b[0][1] + a[i][1] * b[1][1] + a[i][2] * b[2][1];
        ab[i][2] = a[i][0] * b[0][2] + a[i][1] * b[1][2] + a[i][2] * b[2][2];
        ab[i][3] = 0.0;
    }
    ab[3][3] = 1.0;
}

/* Mac 0x48878b */
void MEAPI MeMatrix4TMMakeRelative(MeMatrix4 rel, const MeMatrix4 a, const MeMatrix4 b)
{
    int i;
    MeReal d0, d1, d2;
    for (i = 0; i < 3; ++i) {
        rel[i][0] = a[i][0] * b[0][0] + a[i][1] * b[0][1] + a[i][2] * b[0][2];
        rel[i][1] = a[i][0] * b[1][0] + a[i][1] * b[1][1] + a[i][2] * b[1][2];
        rel[i][2] = a[i][0] * b[2][0] + a[i][1] * b[2][1] + a[i][2] * b[2][2];
    }
    d0 = a[3][0] - b[3][0];
    d1 = a[3][1] - b[3][1];
    d2 = a[3][2] - b[3][2];
    rel[3][0] = d0 * b[0][0] + d1 * b[0][1] + d2 * b[0][2];
    rel[3][1] = d0 * b[1][0] + d1 * b[1][1] + d2 * b[1][2];
    rel[3][2] = d0 * b[2][0] + d1 * b[2][1] + d2 * b[2][2];
}

/* Mac 0x4889e0 (Win 0x792320 is a different function) */
void MEAPI MeQuaternionFromTM(MeVector4 q, const MeMatrix4 tm)
{
    MeReal trace = tm[0][0] + tm[1][1] + tm[2][2];
    if (trace > 0.0) {
        MeReal s = sqrt(trace + 1.0);
        q[0] = s * 0.5;
        s = 0.5 / s;
        q[1] = (tm[1][2] - tm[2][1]) * s;
        q[2] = (tm[2][0] - tm[0][2]) * s;
        q[3] = (tm[0][1] - tm[1][0]) * s;
    } else {
        MeReal v[4];
        int i = (tm[0][0] < tm[1][1]) ? 1 : 0;
        int j, k;
        MeReal s;
        if (tm[2][2] > tm[i][i])
            i = 2;
        j = sNext[i];
        k = sNext[j];
        s = sqrt(tm[i][i] - tm[j][j] - tm[k][k] + 1.0);
        v[i] = s * 0.5;
        if (s != 0.0)
            s = 0.5 / s;
        v[3] = (tm[j][k] - tm[k][j]) * s;
        v[j] = (tm[i][j] + tm[j][i]) * s;
        v[k] = (tm[i][k] + tm[k][i]) * s;
        q[1] = v[0];
        q[2] = v[1];
        q[3] = v[2];
        q[0] = v[3];
    }
}

/* Mac 0x488baf, Win 0x74e590. Win keeps sin(z) in st(0): its products use MeSinMul. */
void MEAPI MeMatrix3FromEulerAngles(MeMatrix3 R, MeReal x, MeReal y, MeReal z)
{
    MeReal cx = cos(x), sx = sin(x);
    MeReal cy = cos(y), sy = sin(y);
    MeReal cz = cos(z);
    R[0][0] = cy * cz;
    R[0][1] = -MeSinMul(z, cy);
    R[0][2] = -sy;
    R[1][0] = sy * -sx * cz + MeSinMul(z, cx);
    R[1][1] = MeSinMul(z, sx * sy) + cx * cz;
    R[1][2] = cy * -sx;
    R[2][0] = cx * sy * cz + MeSinMul(z, sx);
    R[2][1] = -MeSinMul(z, cx * sy) + sx * cz;
    R[2][2] = cx * cy;
}

/* Mac 0x488d21, Win 0x74d780. Win keeps cos(half) in st(0): its products use MeCosMul. */
void MEAPI MeQuaternionFiniteRotation(MeVector4 q, const MeVector3 w, MeReal h)
{
    MeReal u[3], c[3];
    MeReal len = sqrt(w[0] * w[0] + w[1] * w[1] + w[2] * w[2]);
    MeReal inv = 1.0 / (len + 1e-14);
    MeReal half = len * h * 0.5;
    MeReal s, q0, q1, q2, q3;
    int i;
    u[0] = w[0] * inv;
    u[1] = w[1] * inv;
    u[2] = w[2] * inv;
    s = sin(half);
    q0 = q[0];
    q1 = q[1];
    q2 = q[2];
    q3 = q[3];
    c[0] = u[1] * q3 - q2 * u[2];
    c[1] = q1 * u[2] - u[0] * q3;
    c[2] = u[0] * q2 - q1 * u[1];
    for (i = 0; i < 3; ++i)
        q[i + 1] = s * q[0] * u[i] + MeCosMul(half, q[i + 1]) + s * c[i];
    q[0] = MeCosMul(half, q0) - (q1 * u[0] + q2 * u[1] + q3 * u[2]) * s;
}

/* Mac 0x488ee8, Win 0x74ea80 (the rotation-matrix part is Win 0x74d280, not inlined): epsilon
 * is unused */
void MEAPI MeMatrix4TMUpdateFromVelocities(MeMatrix4 tm, MeReal epsilon, MeReal h, const MeVector3 vel,
                                     const MeVector3 angvel, const MeMatrix4 tm0, const MeReal* q0)
{
    MeVector4 q;
    MeReal yy, zz, w2, xy2, xz2, yz2, ww_xx;
    int i;
    (void)epsilon;
    if (q0 == 0) {
        MeQuaternionFromTM(q, tm0);
    } else {
        q[0] = q0[0];
        q[1] = q0[1];
        q[2] = q0[2];
        q[3] = q0[3];
    }
    MeQuaternionFiniteRotation(q, angvel, h);
    yy = q[2] * q[2];
    zz = q[3] * q[3];
    w2 = q[0] + q[0];
    xy2 = q[2] * (q[1] + q[1]);
    xz2 = (q[1] + q[1]) * q[3];
    yz2 = q[3] * (q[2] + q[2]);
    tm[0][0] = q[1] * q[1] + q[0] * q[0] - yy - zz;
    tm[1][0] = xy2 - w2 * q[3];
    tm[2][0] = q[2] * w2 + xz2;
    tm[0][1] = xy2 + w2 * q[3];
    ww_xx = q[0] * q[0] - q[1] * q[1];
    tm[1][1] = yy + ww_xx - zz;
    tm[2][1] = yz2 - q[1] * w2;
    tm[0][2] = xz2 - q[2] * w2;
    tm[1][2] = q[1] * w2 + yz2;
    tm[2][2] = ww_xx - yy + zz;
    for (i = 0; i < 3; ++i)
        tm[3][i] = h * vel[i] + tm0[3][i];
}

/* Mac 0x48968f, Win 0x74eb10 */
void MEAPI MeMatrix4TMUpdateFromVelocitiesCOM(MeMatrix4 tm, MeReal epsilon, MeReal h, const MeVector3 vel,
                                        const MeVector3 angvel, const MeMatrix4 tm0, const MeReal* q0,
                                        const MeVector3 com)
{
    MeMatrix4TMUpdateFromVelocities(tm, epsilon, h, vel, angvel, tm0, q0);
    tm[3][0] -= com[0] * tm0[0][0] + com[1] * tm0[1][0] + com[2] * tm0[2][0];
    tm[3][1] -= com[0] * tm0[0][1] + com[1] * tm0[1][1] + com[2] * tm0[2][1];
    tm[3][2] -= com[0] * tm0[0][2] + com[1] * tm0[1][2] + com[2] * tm0[2][2];
    tm[3][0] += com[0] * tm[0][0] + com[1] * tm[1][0] + com[2] * tm[2][0];
    tm[3][1] += com[0] * tm[0][1] + com[1] * tm[1][1] + com[2] * tm[2][1];
    tm[3][2] += com[0] * tm[0][2] + com[1] * tm[1][2] + com[2] * tm[2][2];
}

/* relative difference used by the identity tests: (expected - v) / v when |v| >= tol */
static MeReal MEAPI MeRelDiff(MeReal expected, MeReal v, MeReal tol)
{
    if (fabs(v) >= tol)
        return (expected - v) / v;
    return expected;
}

/* Mac 0x489088.
 * NOTE: for |m_ij| < tol the original compares the expected value itself (0 or 1) with tol, so
 * a tiny diagonal entry is "not identity" and a tiny off-diagonal one passes. */
int MEAPI MeMatrix3IsIdentity(const MeMatrix3 m, MeReal tol)
{
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            if (fabs(MeRelDiff(i == j ? 1.0 : 0.0, m[i][j], tol)) >= tol)
                return 0;
    return 1;
}

/* Mac 0x4891ec, Win 0x74e850 */
int MEAPI MeMatrix4IsIdentity(const MeMatrix4 m, MeReal tol)
{
    int i, j;
    for (i = 0; i < 4; ++i)
        for (j = 0; j < 4; ++j)
            if (fabs(MeRelDiff(i == j ? 1.0 : 0.0, m[i][j], tol)) >= tol)
                return 0;
    return 1;
}

/* Mac 0x48938e, Win 0x74e7c0 (Win 0x74e730 is the same on a MeMatrix3, not in the Mac build) */
void MEAPI MeMatrix4TMGetEulerAngles(const MeMatrix4 tm, MeVector3 angles)
{
    MeReal a = asin(tm[0][2]);
    MeReal c = cos(-a);
    MeReal x, z;
    if (fabs(c) <= 0.005) {
        z = atan2(tm[1][0], tm[1][1]);
        x = 0.0;
    } else {
        MeReal ic = 1.0 / c;
        x = atan2(-tm[1][2] * ic, ic * tm[2][2]);
        z = atan2(-tm[0][1] * ic, ic * tm[0][0]);
    }
    angles[0] = x;
    angles[1] = -a;
    angles[2] = z;
}

/* Mac 0x4894a4 */
void MEAPI MeMatrix4TMInvert(MeMatrix4 tm)
{
    MeReal t0 = tm[3][0], t1 = tm[3][1], t2 = tm[3][2], s;
    tm[3][0] = -(t0 * tm[0][0] + t1 * tm[0][1] + t2 * tm[0][2]);
    tm[3][1] = -(t0 * tm[1][0] + t1 * tm[1][1] + t2 * tm[1][2]);
    tm[3][2] = -(t0 * tm[2][0] + t1 * tm[2][1] + t2 * tm[2][2]);
    s = tm[0][1];
    tm[0][1] = tm[1][0];
    tm[1][0] = s;
    s = tm[0][2];
    tm[0][2] = tm[2][0];
    tm[2][0] = s;
    s = tm[1][2];
    tm[1][2] = tm[2][1];
    tm[2][1] = s;
}

/* Mac 0x48958d */
void MEAPI MeQuaternionDifferenceToAngularVelocity(MeVector3 w, const MeVector4 q0, const MeVector4 q1)
{
    MeReal G[3][4];
    MeReal dq[4];
    int i, j;
    dq[0] = q1[0] - q0[0];
    dq[1] = q1[1] - q0[1];
    dq[2] = q1[2] - q0[2];
    dq[3] = q1[3] - q0[3];
    MeQuaternionMakeGMatrix(G, q0);
    w[2] = 0.0;
    w[1] = 0.0;
    w[0] = 0.0;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 4; ++j)
            w[i] = G[i][j] * dq[j] + w[i];
    w[0] = w[0] + w[0];
    w[1] = w[1] + w[1];
    w[2] = w[2] + w[2];
}

/* Mac 0x489807, Win 0x74e490 */
int MEAPI MeMatrix3IsValidOrientation(const MeMatrix3 R, MeReal tol)
{
    MeMatrix3 RRt;
    int i;
    for (i = 0; i < 3; ++i) {
        RRt[0][i] = R[0][0] * R[i][0] + R[0][1] * R[i][1] + R[0][2] * R[i][2];
        RRt[1][i] = R[1][0] * R[i][0] + R[1][1] * R[i][1] + R[1][2] * R[i][2];
        RRt[2][i] = R[2][0] * R[i][0] + R[2][1] * R[i][1] + R[2][2] * R[i][2];
    }
    if (!MeMatrix3IsIdentity((const MeReal(*)[3])RRt, tol))
        return 0;
    return fabs((R[1][1] * R[2][2] - R[2][1] * R[1][2]) * R[0][0]
                + (R[2][1] * R[0][2] - R[2][2] * R[0][1]) * R[1][0]
                + (R[1][2] * R[0][1] - R[1][1] * R[0][2]) * R[2][0] - 1.0)
           < tol;
}

/* Mac 0x489b59, Win 0x74e940: the last column must be (0,0,0,1) within tol (for tol > 1 the
 * [3][3] entry is compared with 0), then the rotation must be a valid orientation. */
int MEAPI MeMatrix4IsTM(const MeMatrix4 tm, MeReal tol)
{
    MeReal d = (tol <= 1.0) ? tm[3][3] - 1.0 : tm[3][3];
    MeMatrix3 R;
    int i;
    if (!(fabs(d) < tol))
        return 0;
    for (i = 0; i < 3; ++i)
        if (!(tol > fabs(tm[i][3])))
            return 0;
    for (i = 0; i < 3; ++i) {
        R[i][0] = tm[i][0];
        R[i][1] = tm[i][1];
        R[i][2] = tm[i][2];
    }
    return MeMatrix3IsValidOrientation((const MeReal(*)[3])R, tol);
}
