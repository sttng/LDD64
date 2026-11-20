/* Vortex 5.0.0 build 800 "vortexldd41" - mass properties of closed triangle meshes as sums of
 * tetrahedra (origin, a, b, c) (restored; Mac 558e 0x5492bc-0x549786, Win
 * 0x7ade20-0x7ae770). Used by the triangle mesh (VincTriangleMesh.cpp) and the convex mesh
 * (VincConvexMesh.cpp). All names inferred. cdecl in the Win original, MEAPI here
 * (VincInternal.h). */
#include "Vinc/VincIntersect.h"
#include "VincInternal.h"

/* Mac 0x5492bc */
static MeReal Trace(const MeReal* m)
{
    return m[0] + m[4] + m[8];
}

/* Mac 0x5492dd, Win 0x7adf60: |r|^2 I - r r^T */
static void ParallelAxis(const MeReal* r, MeReal* out)
{
    MeReal x = r[0], y = r[1], z = r[2];
    MeReal d = x * x + y * y + z * z;
    MeReal o = d * 0.0;
    out[0] = d - x * x;
    out[1] = o - x * y;
    out[2] = o - x * z;
    out[3] = out[1];
    out[4] = d - y * y;
    out[5] = o - y * z;
    out[6] = out[2];
    out[7] = out[5];
    out[8] = d - z * z;
}

/* Win 0x7ade20 (inlined on Mac): ab = a * b (3x3) */
static void Multiply3(MeReal* ab, const MeReal* a, const MeReal* b)
{
    int i, j;
    for (i = 0; i < 3; ++i)
        for (j = 0; j < 3; ++j)
            ab[i * 3 + j] = a[i * 3 + 0] * b[0 * 3 + j] + a[i * 3 + 1] * b[1 * 3 + j] + a[i * 3 + 2] * b[2 * 3 + j];
}

/* Mac 0x549786, Win 0x7ae770: out = {volume, centroid[3], inertia[9]} of the tetrahedron (0, a, b, c);
 * inertia = tr(V^T C V) I - V^T C V + ParallelAxis(centroid) with V = rows a, b, c and C the unit
 * tetrahedron's covariance (3 / 80 on, -1 / 80 off the diagonal) */
void MEAPI VincTetraMassProperties(MeReal* out, const MeVector3 a, const MeVector3 b, const MeVector3 c)
{
    static const MeReal C[9] = {0.0375, -0.0125, -0.0125, -0.0125, 0.0375, -0.0125, -0.0125, -0.0125, 0.0375};
    MeReal V[9], Vt[9], CV[9], M[9], pa[9], centroid[3], t, o;
    int k;
    V[0] = a[0]; V[1] = a[1]; V[2] = a[2];
    V[3] = b[0]; V[4] = b[1]; V[5] = b[2];
    V[6] = c[0]; V[7] = c[1]; V[8] = c[2];
    Vt[0] = a[0]; Vt[1] = b[0]; Vt[2] = c[0];
    Vt[3] = a[1]; Vt[4] = b[1]; Vt[5] = c[1];
    Vt[6] = a[2]; Vt[7] = b[2]; Vt[8] = c[2];
    out[0] = ((b[1] * c[2] - b[2] * c[1]) * a[0] + (b[2] * c[0] - c[2] * b[0]) * a[1] +
              (c[1] * b[0] - b[1] * c[0]) * a[2]) / 6.0;
    centroid[2] = (a[2] + b[2] + c[2]) * 0.25;
    centroid[1] = (a[1] + b[1] + c[1]) * 0.25;
    centroid[0] = (a[0] + b[0] + c[0]) * 0.25;
    out[1] = centroid[0];
    out[2] = centroid[1];
    out[3] = centroid[2];
    Multiply3(CV, C, V);
    Multiply3(M, Vt, CV);
    t = Trace(M);
    o = t * 0.0;
    M[0] = t - M[0];
    M[1] = o - M[1];
    M[2] = o - M[2];
    M[3] = o - M[3];
    M[4] = t - M[4];
    M[5] = o - M[5];
    M[6] = o - M[6];
    M[7] = o - M[7];
    M[8] = t - M[8];
    ParallelAxis(centroid, pa);
    for (k = 0; k < 9; ++k)
        out[4 + k] = M[k] + pa[k];
}

/* Mac 0x54939f, Win 0x7ae040: volume, volume-weighted centroid and inertia of `count` tetrahedra
 * (13 doubles each), the inertia moved to the centroid and divided by the volume */
void MEAPI VincSumMassProperties(const MeReal* t, int count, MeReal* out)
{
    int i, k;
    const MeReal* p;
    out[0] = 0;
    out[1] = 0;
    out[2] = 0;
    out[3] = 0;
    for (i = 0, p = t; i < count; ++i, p += 13) {
        out[0] = out[0] + p[0];
        out[1] = p[0] * p[1] + out[1];
        out[2] = out[2] + p[0] * p[2];
        out[3] = out[3] + p[0] * p[3];
    }
    if (out[0] > 1e-05) {
        MeReal inv = 1.0 / out[0];
        out[1] = inv * out[1];
        out[2] = inv * out[2];
        out[3] = inv * out[3];
    }
    for (k = 4; k < 13; ++k)
        out[k] = 0;
    for (i = 0, p = t; i < count; ++i, p += 13)
        for (k = 4; k < 13; ++k)
            out[k] = p[k] * p[0] + out[k];
    if (out[0] > 0) {
        MeReal pa[9], m = out[0], inv;
        ParallelAxis(out + 1, pa);
        for (k = 0; k < 9; ++k)
            out[4 + k] = out[4 + k] - pa[k] * m;
        inv = 1.0 / out[0];
        for (k = 0; k < 9; ++k)
            out[4 + k] = out[4 + k] * inv;
    }
}
