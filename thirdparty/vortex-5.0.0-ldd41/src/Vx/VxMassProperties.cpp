// Vortex 5.0.0 build 800 "vortexldd41" - VxMassProperties (restored).
// Mac TU 503 0x45e31a-0x45ff0c (static initialiser 0x45fe59: <iostream> and a file-static
// identity transform built from Euler angles, Mac 0x77ff00, unused); Win 0x761c40-0x762fe0.
#include "Vx/VxPart.h"
#include "Vinc/VincTriangleMeshContacts.h" // VincJacobi3
#include <string.h>
#include <iostream>

namespace Vx {

namespace {
// Mac 0x77ff00: VxTransform(0, 0, order *0x702540) (C1 Mac 0x47fa1a); never read
const VxTransform sIdentity(VxVector3(0, 0, 0), VxVector3(0, 0, 0), 0);
const VxReal kMinimumMomentRatio = 0.01; // Mac 0x779388

// Mac 0x45e414 (eax = a, edx = v, ecx = max sweeps): v = eigenvectors (rows) of a, a diagonalised
int Diagonalize(VxReal a[9], VxReal v[9], int maxSweeps)
{
    v[0] = 1.0; v[1] = 0; v[2] = 0;
    v[3] = 0; v[4] = 1.0; v[5] = 0;
    v[6] = 0; v[7] = 0; v[8] = 1.0;
    int sweeps = VincJacobi3(a, v, maxSweeps);
    VxReal t[9];
    memcpy(t, v, sizeof(t));
    v[1] = t[3];
    v[2] = t[6];
    v[3] = t[1];
    v[5] = t[7];
    v[6] = t[2];
    v[7] = t[5];
    return sweeps;
}

// Mac 0x45eb0a: clamps the principal moments of I (row-major 3x3, in place); false if unchanged
bool ClampInertia(VxReal* I)
{
    VxReal a[9];
    for (int k = 0; k < 9; ++k)
        a[k] = I[k];
    VxReal V[9];
    Diagonalize(a, V, 10);
    const VxReal e0 = a[0], e1 = a[4], e2 = a[8];
    VxReal m = e1 <= e0 ? e0 : e1;
    m = (m < e2 ? e2 : m) * kMinimumMomentRatio;
    if (a[0] < m)
        a[0] = m;
    if (a[4] < m)
        a[4] = m;
    if (a[8] < m)
        a[8] = m;
    bool changed = e2 < m || e1 < m || e0 < m;
    if (a[4] + a[8] < a[0]) {
        a[0] = a[8] <= a[4] ? a[4] : a[8];
        changed = true;
    }
    if (a[0] + a[8] < a[4]) {
        a[4] = a[8] <= a[0] ? a[0] : a[8];
        changed = true;
    }
    if (a[8] <= a[4] + a[0]) {
        if (!changed)
            return false;
    } else {
        a[8] = a[0] <= a[4] ? a[4] : a[0];
    }
    // V^T (a V) with the corrected diagonal (the off-diagonal residues of the sweeps kept).
#if defined(_WIN32)
    // Win 0x7624f0: two calls of the 3x3 product Win 0x761a50 (every sum in index order)
    VxReal T[9];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            T[i * 3 + j] = a[i * 3 + 0] * V[0 * 3 + j] + a[i * 3 + 1] * V[1 * 3 + j] + a[i * 3 + 2] * V[2 * 3 + j];
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            I[i * 3 + j] = V[0 * 3 + i] * T[0 * 3 + j] + V[1 * 3 + i] * T[1 * 3 + j] + V[2 * 3 + i] * T[2 * 3 + j];
#else
    // Mac 0x45eb0a: the products expanded (some sums in another order)
    const VxReal t0 = a[0] * V[0] + V[3] * a[1] + V[6] * a[2];
    const VxReal t1 = a[0] * V[1] + a[1] * V[4] + a[2] * V[7];
    const VxReal t2 = a[0] * V[2] + a[1] * V[5] + a[2] * V[8];
    const VxReal t3 = V[6] * a[5] + V[3] * a[4] + V[0] * a[3];
    const VxReal t4 = V[7] * a[5] + V[4] * a[4] + V[1] * a[3];
    const VxReal t5 = a[3] * V[2] + V[5] * a[4] + V[8] * a[5];
    const VxReal t6 = V[6] * a[8] + V[3] * a[7] + V[0] * a[6];
    const VxReal t7 = V[1] * a[6] + V[4] * a[7] + V[7] * a[8];
    const VxReal t8 = a[6] * V[2] + V[5] * a[7] + V[8] * a[8];
    I[0] = t0 * V[0] + t3 * V[3] + t6 * V[6];
    I[1] = t1 * V[0] + t4 * V[3] + t7 * V[6];
    I[2] = t2 * V[0] + t5 * V[3] + t8 * V[6];
    I[3] = t0 * V[1] + t3 * V[4] + t6 * V[7];
    I[4] = t1 * V[1] + t4 * V[4] + t7 * V[7];
    I[5] = t2 * V[1] + t5 * V[4] + t8 * V[7];
    I[6] = t6 * V[8] + t0 * V[2] + t3 * V[5];
    I[7] = t7 * V[8] + t1 * V[2] + t4 * V[5];
    I[8] = t8 * V[8] + t2 * V[2] + t5 * V[5];
#endif
    VxReal s = (I[3] + I[1]) * 0.5;
    I[1] = s;
    I[3] = s;
    s = (I[6] + I[2]) * 0.5;
    I[2] = s;
    I[6] = s;
    s = (I[5] + I[7]) * 0.5;
    I[7] = s;
    I[5] = s;
    return true;
}

// Mac 0x45f227 (eax = v, edx = out): |v|^2 I - v v^T
void ParallelAxis(const VxReal* v, VxReal* out)
{
    const VxReal x = v[0], y = v[1], z = v[2];
    const VxReal n = y * y + x * x + z * z;
    const VxReal z0 = n * 0.0;
    out[0] = n - x * x;
    out[1] = z0 - x * y;
    out[2] = z0 - x * z;
    out[3] = out[1];
    out[4] = n - y * y;
    out[5] = z0 - y * z;
    out[6] = out[2];
    out[7] = out[5];
    out[8] = n - z * z;
}
} // namespace

// C2 Mac 0x45e732 (C1 0x45e7b8)
VxMassProperties::VxMassProperties(VxEntity* entity)
    : mTransformPtr(entity->mTransformPtr), mMass(0)
{
    memset(mInertia, 0, sizeof(mInertia));
    reset();
}

// D2 Mac 0x45e604 (D1 0x45e62e, D0 0x45e5cc / Win 0x762fe0)
VxMassProperties::~VxMassProperties()
{
}

// Mac 0x45fe48, Win 0x762fd0
const char* VxMassProperties::getClassName() const
{
    return "VxMassProperties";
}

// Mac 0x45e674
void VxMassProperties::reset()
{
    mCOMTransform = VxTransform();
    memset(mInertia, 0, sizeof(mInertia));
}

// Mac 0x45e872, Win 0x761fd0
void VxMassProperties::setInertiaTensor(const VxReal inertia[3][3])
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            mInertia[i][j] = inertia[i][j];
}

// Mac 0x45e8f8, Win 0x761f70
void VxMassProperties::getInertiaTensor(VxReal inertia[3][3]) const
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            inertia[i][j] = mInertia[i][j];
}

// Mac 0x45f166, Win 0x762030
void VxMassProperties::scaleInertiaTensor(VxReal s)
{
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            mInertia[i][j] = mInertia[i][j] * s;
}

// Mac 0x45f14a, Win 0x762f60
bool VxMassProperties::makeInertiaTensorValid()
{
    return ClampInertia(&mInertia[0][0]);
}

// Mac 0x45e4e4, Win 0x762180
void VxMassProperties::setCOMTransform(const VxTransform& tm)
{
    VxTransform t;
    t.assign(tm);
    mCOMTransform = t;
}

// Mac 0x45e7d2, Win 0x7620c0
void VxMassProperties::setCOMPosition(const VxVector3& p)
{
    mCOMTransform = VxTransform();
    mCOMTransform.m[3][0] = p[0];
    mCOMTransform.m[3][1] = p[1];
    mCOMTransform.m[3][2] = p[2];
}

// Mac 0x45e642, Win 0x762130
void VxMassProperties::getCOMPosition(VxVector3& p) const
{
    p[0] = mCOMTransform.m[3][0];
    p[1] = mCOMTransform.m[3][1];
    p[2] = mCOMTransform.m[3][2];
}

// Mac 0x45e32c, Win 0x761c40
void VxMassProperties::getCOMWorldPosition(VxVector3& p) const
{
    const VxReal(*R)[4] = mTransformPtr->m;
    const VxReal* c = mCOMTransform.m[3];
    const VxReal x = R[0][0] * c[0] + R[1][0] * c[1] + R[2][0] * c[2];
    p[0] = x;
    const VxReal y = R[0][1] * c[0] + R[1][1] * c[1] + R[2][1] * c[2];
    p[1] = y;
    const VxReal z = R[0][2] * c[0] + R[1][2] * c[1] + R[2][2] * c[2];
    p[2] = z;
    p[0] = x + R[3][0];
    p[1] = y + R[3][1];
    p[2] = z + R[3][2];
}

// Mac 0x45e97e, Win 0x761e10
void VxMassProperties::copy(const VxMassProperties& m)
{
    if (this == &m) {
        VxWarning(0, "VxMassProperties::copy: can't copy from self.\n");
        return;
    }
    mMass = m.mMass;
    setInertiaTensor(m.mInertia);
    mCOMTransform = m.mCOMTransform;
}

// Mac 0x45fdf8, Win 0x762f70
void VxMassProperties::addMassProperties(const VxMassProperties& m, const VxTransform& tm)
{
    if (this == &m)
        VxWarning(0, "VxMassProperties::addMassProperties: can't add self.\n");
    else
        combine(m, false, tm);
}

// Mac 0x45fda8, Win 0x762fa0
void VxMassProperties::subtractMassProperties(const VxMassProperties& m, const VxTransform& tm)
{
    if (this == &m)
        VxWarning(0, "VxMassProperties::subtractMassProperties: can't subtract self.\n");
    else
        combine(m, true, tm);
}

// Mac 0x45f2e4, Win 0x762790. NOTE: the result's COM frame is reset to a pure translation.
void VxMassProperties::combine(const VxMassProperties& m, bool subtract, const VxTransform& tm)
{
    const VxReal massA = mMass;
    VxReal massB = m.mMass;
    if (massB == 0.0)
        return;
    if (subtract)
        massB = -massB;
    const VxReal mass = massB + massA;
    const VxReal inv = 1.0 / mass;
    const VxReal* cb = m.mCOMTransform.m[3];
    const VxReal(*R)[4] = tm.m;
    VxReal p[3];
    p[0] = cb[0] * R[0][0] + cb[1] * R[1][0] + cb[2] * R[2][0] + R[3][0];
    p[1] = cb[0] * R[0][1] + cb[1] * R[1][1] + cb[2] * R[2][1] + R[3][1];
    p[2] = cb[0] * R[0][2] + cb[1] * R[1][2] + cb[2] * R[2][2] + R[3][2];
    VxReal ca[3] = { mCOMTransform.m[3][0], mCOMTransform.m[3][1], mCOMTransform.m[3][2] };
    VxVector3 com;
    com[2] = inv * (p[2] * massB + massA * ca[2]);
    com[1] = inv * (p[1] * massB + massA * ca[1]);
    com[0] = (p[0] * massB + massA * ca[0]) * inv;
    setCOMPosition(com);
    setMass(mass);

    VxReal S[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    VxReal IB[3][3];
    m.getInertiaTensor(IB);
    const VxReal s = (subtract ? -1.0 : 1.0) / massB;
    VxReal T[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            IB[i][j] = IB[i][j] * s;
    for (int i = 0; i < 3; ++i)
        for (int j = 0; j < 3; ++j)
            S[i * 3 + j] = S[i * 3 + j] + mInertia[i][j];
    // T = R^T IB R (B's inertia per unit mass in this frame)
    for (int r = 0; r < 3; ++r) {
        for (int c = 0; c < 3; ++c) {
            VxReal acc = T[r * 3 + c];
            for (int k = 0; k < 3; ++k) {
                const VxReal rk = R[k][r];
                acc = acc + rk * IB[k][0] * R[0][c];
                acc = acc + rk * IB[k][1] * R[1][c];
                acc = acc + rk * IB[k][2] * R[2][c];
            }
            T[r * 3 + c] = acc;
        }
    }
    VxReal d[3] = { p[0] - ca[0], p[1] - ca[1], p[2] - ca[2] };
    VxReal P[9];
    ParallelAxis(d, P);
    for (int k = 0; k < 9; ++k)
        T[k] = (T[k] + P[k]) * massB;
    VxReal q[3];
    q[0] = (massB * d[0] + 0.0) * inv;
    q[1] = (massB * d[1] + 0.0) * inv;
    q[2] = (massB * d[2] + 0.0) * inv;
    for (int k = 0; k < 9; ++k)
        S[k] = (S[k] + T[k]) * inv;
    ParallelAxis(q, P);
    for (int k = 0; k < 9; ++k)
        P[k] = P[k] * -1.0;
    for (int k = 0; k < 9; ++k)
        S[k] = S[k] + P[k];
    VxReal I[3][3];
    I[0][0] = S[0] * mass;
    I[0][1] = I[1][0] = (S[3] + S[1]) * 0.5 * mass;
    I[0][2] = I[2][0] = (S[6] + S[2]) * 0.5 * mass;
    I[1][1] = S[4] * mass;
    I[1][2] = I[2][1] = (S[5] + S[7]) * 0.5 * mass;
    I[2][2] = S[8] * mass;
    setInertiaTensor(I);
}

} // namespace Vx
