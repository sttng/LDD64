// Vortex 5.0.0 build 800 "vortexldd41" - construction of the triangle mesh's OBB tree (restored in
// the fixes). Win-only: Win 0x80dc60-0x80e360 (tree), 0x825000-0x826510 (box
// fitter), 0x8022e0 + its std::map helpers 0x7ec950-0x802990 (vertex index), 0x854740-0x855c40
// (box refinement), 0x7ae690 (triangle area); dead-stripped on the Mac (LDD never builds a triangle
// mesh). The original's C++ members (thiscall, /LTCG register conventions) are C-callable functions
// here. Every name is inferred. A node box (VincOBBNode +0x10 on Win) is 15 doubles: OBBBox below.
// x87 at 53 bits (= SSE2 doubles), original sum association (the restoration's diff tests).
#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincTriangleMeshContacts.h"
#include "Me/MeMemory.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <map>
#include "Me/MeCrtMath.h"

namespace {

struct OBBBox {
    MeReal he[3];       // half extents
    MeReal axis[3][3];  // box axes (rows)
    MeReal center[3];
};

inline OBBBox* BoxOf(VincOBBNode* node)
{
    return (OBBBox*)node->halfExtents;
}

inline MeReal Dot(const MeReal* a, const MeReal* b)
{
    return a[0] * b[0] + a[1] * b[1] + a[2] * b[2];
}

// surface measure of a box: (h0 h1 + h1 h2) + h0 h2 (Win 0x8552c0, inlined many times)
inline MeReal Surface(const MeReal* h)
{
    return (h[0] * h[1] + h[1] * h[2]) + h[0] * h[2];
}

inline void SwapAxes(OBBBox* b, int i, int j)
{
    MeReal t = b->he[i];
    b->he[i] = b->he[j];
    b->he[j] = t;
    for (int k = 0; k < 3; ++k) {
        t = b->axis[i][k];
        b->axis[i][k] = b->axis[j][k];
        b->axis[j][k] = t;
    }
}

// Win 0x825640 (cdecl): half extents in decreasing order (the axes follow)
void SortExtents(OBBBox* b)
{
    if (b->he[1] < b->he[2])
        SwapAxes(b, 1, 2);
    if (b->he[0] < b->he[1])
        SwapAxes(b, 0, 1);
    if (b->he[1] < b->he[2])
        SwapAxes(b, 1, 2);
}

// Win 0x7ae690 (cdecl): area of the triangle v[0..2]
MeReal TriangleArea(MeReal* const* v)
{
    const MeReal* a = v[0];
    const MeReal* b = v[1];
    const MeReal* c = v[2];
    const MeReal u0 = c[0] - b[0], u1 = c[1] - b[1], u2 = c[2] - b[2];
    const MeReal w0 = b[0] - a[0], w1 = b[1] - a[1], w2 = b[2] - a[2];
    const MeReal x = w1 * u2 - w2 * u1;
    const MeReal y = w2 * u0 - w0 * u2;
    const MeReal z = u1 * w0 - u0 * w1;
    return sqrt((x * x + y * y) + z * z) * 0.5;
}

// Win 0x8257a0 (cdecl): grow [lo, hi] by p (per component; a value below lo never raises hi)
void ExpandMinMax(MeReal* lo, MeReal* hi, const MeReal* p)
{
    for (int k = 0; k < 3; ++k) {
        if (lo[k] > p[k])
            lo[k] = p[k];
        else if (hi[k] < p[k])
            hi[k] = p[k];
    }
}

// Win 0x825a00 (cdecl): axis-aligned box of the triangles (the deep levels of the tree)
void FitAABB(VincGeometryID mesh, OBBBox* box, const int* tris, int n)
{
    box->axis[2][2] = 1.0;
    box->axis[1][1] = 1.0;
    box->axis[0][0] = 1.0;
    box->center[2] = 0.0;
    box->center[1] = 0.0;
    box->center[0] = 0.0;
    box->axis[2][1] = 0.0;
    box->axis[2][0] = 0.0;
    box->axis[1][2] = 0.0;
    box->axis[1][0] = 0.0;
    box->axis[0][2] = 0.0;
    box->axis[0][1] = 0.0;
    MeReal lo[3] = { DBL_MAX, DBL_MAX, DBL_MAX };
    MeReal hi[3] = { -DBL_MAX, -DBL_MAX, -DBL_MAX };
    for (int i = 0; i < n; ++i) {
        MeReal* v[3];
        VincTriangleMeshGetTrianglePtrs(mesh, tris[i], &v[0], &v[1], &v[2]);
        for (int j = 0; j < 3; ++j)
            for (int k = 0; k < 3; ++k) {
                if (lo[k] > v[j][k])
                    lo[k] = v[j][k];
                if (hi[k] < v[j][k])
                    hi[k] = v[j][k];
            }
    }
    for (int k = 0; k < 3; ++k) {
        box->he[k] = fabs((hi[k] - lo[k]) * 0.5);
        box->center[k] = box->he[k] + lo[k];
    }
    SortExtents(box);
}

// Win 0x854840 (EAX = dir, EBX = minV, EDI = minD; cdecl rest): the extreme vertices of the
// triangles along dir (the first vertex of the first triangle seeds both)
void ExtremeVertices(VincGeometryID mesh, const int* tris, int n, const MeReal* dir, MeReal** minV,
                     MeReal** maxV, MeReal* minD, MeReal* maxD)
{
    MeReal* v[3];
    VincTriangleMeshGetTrianglePtrs(mesh, tris[0], &v[0], &v[1], &v[2]);
    const MeReal d0 = dir[0], d1 = dir[1], d2 = dir[2];
    *minV = v[0];
    *maxV = v[0];
    const MeReal s = d0 * v[0][0] + d1 * v[0][1] + d2 * v[0][2];
    *minD = s;
    *maxD = s;
    for (int i = 0; i < n; ++i) {
        VincTriangleMeshGetTrianglePtrs(mesh, tris[i], &v[0], &v[1], &v[2]);
        for (int j = 0; j < 3; ++j) {
            const MeReal d = d0 * v[j][0] + d1 * v[j][1] + d2 * v[j][2];
            if (d < *minD) {
                *minD = d;
                *minV = v[j];
            }
            if (*maxD < d) {
                *maxD = d;
                *maxV = v[j];
            }
        }
    }
}

// extents and centre of a box with given axes: the extreme vertices along each axis
// (Win 0x854ba0 / 0x8549b0 / 0x8262d0 share this loop)
void FitAlongAxes(VincGeometryID mesh, const int* tris, int n, OBBBox* box, MeReal** minV, MeReal** maxV)
{
    MeReal c[3] = { 0.0, 0.0, 0.0 };
    for (int k = 0; k < 3; ++k) {
        MeReal lo, hi;
        ExtremeVertices(mesh, tris, n, box->axis[k], &minV[k], &maxV[k], &lo, &hi);
        box->he[k] = fabs((hi - lo) * 0.5);
        const MeReal t = box->he[k] + lo;
        c[0] = box->axis[k][0] * t + c[0];
        c[1] = box->axis[k][1] * t + c[1];
        c[2] = box->axis[k][2] * t + c[2];
    }
    box->center[0] = c[0];
    box->center[1] = c[1];
    box->center[2] = c[2];
}

// Win 0x854740 (EAX = q, ECX = m): rotation matrix of the quaternion (w, x, y, z), row-major
void QuaternionToMatrix(MeReal* m, const MeReal* q)
{
    const MeReal ww = q[0] * q[0], xx = q[1] * q[1], yy = q[2] * q[2], zz = q[3] * q[3];
    const MeReal w2 = q[0] * 2.0, x2 = q[1] * 2.0;
    const MeReal wx = q[1] * w2, wy = q[2] * w2, wz = w2 * q[3];
    const MeReal xy = q[2] * x2, xz = x2 * q[3];
    const MeReal yz = (2.0 * q[2]) * q[3];
    const MeReal d = ww - xx;
    m[0] = ((xx + ww) - yy) - zz;
    m[3] = xy - wz;
    m[6] = xz + wy;
    m[1] = xy + wz;
    m[4] = (d + yy) - zz;
    m[7] = yz - wx;
    m[2] = xz - wy;
    m[5] = yz + wx;
    m[8] = (d - yy) + zz;
}

// Win 0x8549b0 (ECX = rotation axis, EAX = maxV slots; cdecl rest): `in` rotated by angle about
// axis, refitted to the triangles -> out
void RotateAndFit(VincGeometryID mesh, const int* tris, int n, MeReal** minV, MeReal** maxV, const MeReal* axis,
                  MeReal angle, const OBBBox* in, OBBBox* out)
{
    const MeReal half = angle * 0.5;
    MeReal q[4], m[9];
    const MeReal s = sin(half);
    q[0] = cos(half);
    q[1] = axis[0] * s;
    q[2] = axis[1] * s;
    q[3] = s * axis[2];
    QuaternionToMatrix(m, q);
    MeReal c[3] = { 0.0, 0.0, 0.0 };
    for (int k = 0; k < 3; ++k) {
        MeReal a[3];
        for (int j = 0; j < 3; ++j)
            a[j] = ((m[j] * in->axis[k][0] + 0.0) + in->axis[k][1] * m[3 + j]) + m[6 + j] * in->axis[k][2];
        out->axis[k][0] = a[0];
        out->axis[k][1] = a[1];
        out->axis[k][2] = a[2];
        MeReal lo, hi;
        ExtremeVertices(mesh, tris, n, out->axis[k], &minV[k], &maxV[k], &lo, &hi);
        out->he[k] = fabs((hi - lo) * 0.5);
        const MeReal t = out->he[k] + lo;
        c[0] = out->axis[k][0] * t + c[0];
        c[1] = out->axis[k][1] * t + c[1];
        c[2] = out->axis[k][2] * t + c[2];
    }
    out->center[0] = c[0];
    out->center[1] = c[1];
    out->center[2] = c[2];
}

// Win 0x854ba0 (cdecl): a box aligned with two perpendicular triangle normals (the first
// triangle's and the first one perpendicular to it within 1e-14) and their cross product; 0 when
// no triangle is perpendicular to the first
int PerpendicularNormalsBox(VincGeometryID mesh, OBBBox* box, const int* tris, int n)
{
    MeReal* n0;
    MeReal* ni;
    VincTriangleMeshGetNormalPtr(mesh, tris[0], &n0);
    for (int i = 1; i < n; ++i) {
        VincTriangleMeshGetNormalPtr(mesh, tris[i], &ni);
        if (!(fabs((ni[1] * n0[1] + n0[0] * ni[0]) + ni[2] * n0[2]) < 1e-14))
            continue;
        MeReal c0 = n0[1] * ni[2] - ni[1] * n0[2];
        MeReal c1 = n0[2] * ni[0] - n0[0] * ni[2];
        MeReal c2 = ni[1] * n0[0] - n0[1] * ni[0];
        const MeReal len = sqrt((c0 * c0 + c1 * c1) + c2 * c2);
        if (len != 0.0) {
            const MeReal inv = 1.0 / len;
            c0 = inv * c0;
            c1 = c1 * inv;
            c2 = c2 * inv;
        }
        box->axis[0][0] = n0[0];
        box->axis[0][1] = n0[1];
        box->axis[0][2] = n0[2];
        box->axis[1][0] = ni[0];
        box->axis[1][1] = ni[1];
        box->axis[1][2] = ni[2];
        box->axis[2][0] = c0;
        box->axis[2][1] = c1;
        box->axis[2][2] = c2;
        MeReal* minV[3];
        MeReal* maxV[3];
        FitAlongAxes(mesh, tris, n, box, minV, maxV);
        return 1;
    }
    return 0;
}

// one face pair of the surface gradient: (w a) x (v - c) (Win 0x854ed0)
inline void AddTorque(MeReal* g, const MeReal* a, MeReal w, const MeReal* v, const MeReal* c)
{
    const MeReal dx = v[0] - c[0], dy = v[1] - c[1], dz = v[2] - c[2];
    const MeReal ax = a[0] * w, ay = a[1] * w, az = a[2] * w;
    g[0] = (dz * ay - dy * az) + g[0];
    g[1] = (az * dx - dz * ax) + g[1];
    g[2] = (ax * dy - ay * dx) + g[2];
}

// Win 0x854ed0 (EAX = box, ECX = centre, EDI = maxV, EBX = minV; cdecl rest): the rotation
// direction that shrinks the box surface fastest (normalised when longer than 1e-14), *len its
// length
MeReal* SurfaceGradient(MeReal* g, MeReal* len, const OBBBox* box, const MeReal* c, MeReal* const* minV,
                        MeReal* const* maxV)
{
    const MeReal w[3] = { box->he[1] + box->he[2], box->he[0] + box->he[2], box->he[0] + box->he[1] };
    g[0] = 0.0;
    g[1] = 0.0;
    g[2] = 0.0;
    for (int k = 0; k < 3; ++k) {
        AddTorque(g, box->axis[k], w[k], maxV[k], c);
        AddTorque(g, box->axis[k], -w[k], minV[k], c);
    }
    const MeReal l = sqrt((g[0] * g[0] + g[1] * g[1]) + g[2] * g[2]);
    *len = l;
    if (1e-14 < l) {
        const MeReal inv = 1.0 / l;
        g[0] = g[0] * inv;
        g[1] = inv * g[1];
        g[2] = g[2] * inv;
    }
    return g;
}

// Win 0x855220 (ESI = d): a random unit direction. NOTE: the C library's rand() (the original
// shares the VS2008 CRT's rand state with the rest of the program).
MeReal* RandomDirection(MeReal* d)
{
    for (int k = 0; k < 3; ++k) {
        const MeReal r = (MeReal)rand();
        d[k] = (r + r) / 32767.0 - 1.0;
    }
    const MeReal l = sqrt((d[0] * d[0] + d[1] * d[1]) + d[2] * d[2]);
    if (l != 0.0) {
        const MeReal inv = 1.0 / l;
        d[0] = d[0] * inv;
        d[1] = d[1] * inv;
        d[2] = inv * d[2];
    }
    return d;
}

// Win 0x854d80 (ECX = box): axis2 = axis0 x axis1 within 1e-13 per component
bool RightHanded(const OBBBox* b)
{
    const MeReal(*a)[3] = b->axis;
    if (!(fabs((a[1][2] * a[0][1] - a[1][1] * a[0][2]) - a[2][0]) <= 1e-13))
        return false;
    if (!(fabs((a[1][0] * a[0][2] - a[1][2] * a[0][0]) - a[2][1]) <= 1e-13))
        return false;
    return fabs((a[1][1] * a[0][0] - a[0][1] * a[1][0]) - a[2][2]) <= 1e-13;
}

// Win 0x854df0 (ECX = box): sorted extents, right-handed axes
void Normalize(OBBBox* b)
{
    SortExtents(b);
    if (!RightHanded(b)) {
        b->axis[2][0] = -b->axis[2][0];
        b->axis[2][1] = -b->axis[2][1];
        b->axis[2][2] = -b->axis[2][2];
    }
}

const MeReal kStep = 0.3141592653589793;    // pi / 10
const MeReal kGolden = 0.10053096491487339; // 0.32 kStep

// Win 0x8552c0 (cdecl): minimise the box surface over rotations: the perpendicular-normals box,
// 15 gradient steps (step -0.1, x 2/3 when the surface grew), then per axis a coarse search in
// pi/10 steps and a golden-section search (0.32 / 0.68) to 0.05
void RefineBox(VincGeometryID mesh, const int* tris, int n, OBBBox* box)
{
    if ((box->he[0] * box->he[1]) * box->he[2] < 1e-14)
        return;
    MeReal best = Surface(box->he);
    MeReal cur = best;
    OBBBox tmp, bestBox;
    if (PerpendicularNormalsBox(mesh, &tmp, tris, n)) {
        cur = Surface(tmp.he);
        if (cur < best) {
            *box = tmp;
            best = cur;
        }
    }
    MeReal step = -0.1;
    bestBox = *box;
    MeReal* minV[3];
    MeReal* maxV[3];
    for (int k = 0; k < 3; ++k) {
        MeReal lo, hi;
        ExtremeVertices(mesh, tris, n, box->axis[k], &minV[k], &maxV[k], &lo, &hi);
    }
    MeReal dir[3];
    for (int it = 15; it != 0; --it) {
        const MeReal prev = cur;
        MeReal c[3] = { box->center[0], box->center[1], box->center[2] };
        MeReal g[3], len;
        MeReal* p = SurfaceGradient(g, &len, box, c, minV, maxV);
        dir[0] = p[0];
        dir[1] = p[1];
        dir[2] = p[2];
        if (1e-14 > len) {
            step = -0.1;
            MeReal r[3];
            p = RandomDirection(r);
            dir[0] = p[0];
            dir[1] = p[1];
            dir[2] = p[2];
        }
        RotateAndFit(mesh, tris, n, minV, maxV, dir, step, box, &tmp);
        *box = tmp;
        cur = Surface(box->he);
        if (best > cur) {
            bestBox = tmp;
            best = cur;
        }
        if (prev < cur)
            step = step * 0.6666666666666666;
    }
    *box = bestBox;
    best = Surface(box->he);
    for (int k = 0; k < 3; ++k) {
        dir[0] = box->axis[k][0];
        dir[1] = box->axis[k][1];
        dir[2] = box->axis[k][2];
        int bestJ = 0;
        for (int j = 0; j < 5; ++j) {
            RotateAndFit(mesh, tris, n, minV, maxV, dir, (MeReal)j * kStep, box, &tmp);
            const MeReal s = Surface(tmp.he);
            if (best > s) {
                bestBox = tmp;
                bestJ = j;
                best = s;
            }
        }
        // NOTE: the original fits at both bracket ends without using the results
        MeReal a = (MeReal)(bestJ - 1) * kStep;
        RotateAndFit(mesh, tris, n, minV, maxV, dir, a, box, &tmp);
        MeReal b = (MeReal)(bestJ + 1) * kStep;
        RotateAndFit(mesh, tris, n, minV, maxV, dir, b, box, &tmp);
        MeReal x1 = (MeReal)bestJ * kStep;
        MeReal f1 = best;
        MeReal x2 = x1 + kGolden;
        RotateAndFit(mesh, tris, n, minV, maxV, dir, x2, box, &tmp);
        MeReal f2 = Surface(tmp.he);
        for (int iter = 0; fabs(b - a) > 0.05000000074505806 && iter < 15; ++iter) {
            if (f1 > f2) {
                a = x1;
                x1 = x2;
                f1 = f2;
                x2 = x2 * 0.68 + b * 0.32;
                RotateAndFit(mesh, tris, n, minV, maxV, dir, x2, box, &tmp);
                f2 = Surface(tmp.he);
            } else {
                b = x2;
                x2 = x1;
                f2 = f1;
                x1 = x1 * 0.68 + a * 0.32;
                RotateAndFit(mesh, tris, n, minV, maxV, dir, x1, box, &tmp);
                f1 = Surface(tmp.he);
            }
        }
        RotateAndFit(mesh, tris, n, minV, maxV, dir, f1 < f2 ? x1 : x2, box, &tmp);
        const MeReal s = Surface(tmp.he);
        if (s < best) {
            *box = tmp;
            bestBox = tmp;
            best = s;
        } else {
            *box = bestBox;
        }
    }
    Normalize(box);
}

// the box fitter (Win: a 0x10-byte object, ctor 0x825000 / dtor 0x825010 empty)
struct Fitter {
    VincGeometryID mesh;
    OBBBox* box;
    const int* tris;
    int n;
};

// Win 0x825300 (thiscall): area-weighted covariance of the triangles about their centroid
// NOTE: the centroid sums the vertex sums divided by the area (1 / area weights) and divides by
// 6 n; kept as in the original.
void Covariance(const Fitter* f, MeReal* C)
{
    MeReal s[3] = { 0.0, 0.0, 0.0 };
    MeReal* area = (MeReal*)MeMemoryAPI.create(f->n * 8);
    for (int i = 0; i < f->n; ++i) {
        MeReal* v[3];
        VincTriangleMeshGetTrianglePtrs(f->mesh, f->tris[i], &v[0], &v[1], &v[2]);
        MeReal m[3];
        for (int k = 0; k < 3; ++k)
            m[k] = ((0.0 + v[0][k]) + v[1][k]) + v[2][k];
        const MeReal a = TriangleArea(v);
        area[i] = a;
        const MeReal aa = (0.0 > a) ? -a : a;
        if (aa > 1e-14) {
            const MeReal inv = 1.0 / a;
            s[0] = m[0] * inv + s[0];
            s[1] = m[1] * inv + s[1];
            s[2] = m[2] * inv + s[2];
        }
    }
    const MeReal invN = 1.0 / ((MeReal)f->n * 6.0);
    s[0] = s[0] * invN;
    s[1] = s[1] * invN;
    s[2] = s[2] * invN;
    for (int i = 0; i < f->n; ++i) {
        const MeReal aa = (0.0 > area[i]) ? -area[i] : area[i];
        if (!(aa > 1e-14))
            continue;
        MeReal* v[3];
        VincTriangleMeshGetTrianglePtrs(f->mesh, f->tris[i], &v[0], &v[1], &v[2]);
        MeReal M[3][3] = { { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 }, { 0.0, 0.0, 0.0 } };
        for (int j = 0; j < 3; ++j) {
            const MeReal d[3] = { v[j][0] - s[0], v[j][1] - s[1], v[j][2] - s[2] };
            for (int r = 0; r < 3; ++r)
                for (int c = 0; c < 3; ++c)
                    M[r][c] = d[r] * d[c] + M[r][c];
        }
        const MeReal inv = 1.0 / area[i];
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                M[r][c] = M[r][c] * inv;
        for (int r = 0; r < 3; ++r)
            for (int c = 0; c < 3; ++c)
                C[r * 3 + c] = M[r][c] + C[r * 3 + c];
    }
    MeMemoryAPI.destroy(area);
    const MeReal h = invN * 0.5;
    for (int i = 0; i < 9; ++i)
        C[i] = C[i] * h;
}

// Win 0x825830 (thiscall, this unused): eigenvectors of C (columns of V) by Jacobi, columns in
// decreasing eigenvalue order; returns the sweep count (> 35: not converged, V unsorted)
int Eigenvectors(MeReal* V, MeReal* C)
{
    MeReal v[9] = { 1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0 };
    const int sweeps = VincJacobi3(C, v, 0x65);
    for (int i = 0; i < 9; ++i)
        V[i] = v[i];
    if (sweeps > 0x23)
        return sweeps;
    MeReal e0 = C[0], e1 = C[4], e2 = C[8];
    if (e2 > e1) {
        for (int r = 0; r < 3; ++r) {
            const MeReal t = V[r * 3 + 2];
            V[r * 3 + 2] = V[r * 3 + 1];
            V[r * 3 + 1] = t;
        }
        const MeReal t = e1;
        e1 = e2;
        e2 = t;
    }
    if (e1 > e0) {
        for (int r = 0; r < 3; ++r) {
            const MeReal t = V[r * 3 + 1];
            V[r * 3 + 1] = V[r * 3];
            V[r * 3] = t;
        }
        e1 = e0;
    }
    if (e2 > e1) {
        for (int r = 0; r < 3; ++r) {
            const MeReal t = V[r * 3 + 2];
            V[r * 3 + 2] = V[r * 3 + 1];
            V[r * 3 + 1] = t;
        }
    }
    return sweeps;
}

// Win 0x825dd0 (thiscall): box of one triangle: axis 0 along the longest edge, axis 2 the
// normal, axis 1 = axis 2 x axis 0
int FitTriangle(Fitter* f)
{
    MeReal* v[3];
    VincTriangleMeshGetTrianglePtrs(f->mesh, f->tris[0], &v[0], &v[1], &v[2]);
    MeReal e[3][3];
    for (int k = 0; k < 3; ++k) {
        e[0][k] = v[0][k] - v[1][k];
        e[1][k] = v[1][k] - v[2][k];
        e[2][k] = v[2][k] - v[0][k];
    }
    MeReal L[3];
    for (int i = 0; i < 3; ++i)
        L[i] = (e[i][0] * e[i][0] + e[i][1] * e[i][1]) + e[i][2] * e[i][2];
    int i = 0;
    if (L[1] > L[0])
        i = 1;
    if (L[2] > L[i])
        i = 2;
    OBBBox* b = f->box;
    MeReal* E = e[i];
    const MeReal len = sqrt((E[0] * E[0] + E[1] * E[1]) + E[2] * E[2]);
    if (len != 0.0) {
        const MeReal inv = 1.0 / len;
        E[0] = E[0] * inv;
        E[1] = inv * E[1];
        E[2] = inv * E[2];
    }
    b->axis[0][0] = E[0];
    b->axis[0][1] = E[1];
    b->axis[0][2] = E[2];
    const MeReal nx = e[0][1] * e[1][2] - e[1][1] * e[0][2];
    const MeReal ny = e[0][2] * e[1][0] - e[1][2] * e[0][0];
    const MeReal nz = e[0][0] * e[1][1] - e[0][1] * e[1][0];
    b->axis[2][2] = nz;
    b->axis[2][0] = nx;
    b->axis[2][1] = ny;
    const MeReal nl = sqrt((nx * nx + ny * ny) + nz * nz);
    if (nl != 0.0) {
        const MeReal inv = 1.0 / nl;
        b->axis[2][0] = nx * inv;
        b->axis[2][1] = ny * inv;
        b->axis[2][2] = inv * nz;
    }
    b->axis[1][0] = b->axis[2][1] * b->axis[0][2] - b->axis[2][2] * b->axis[0][1];
    b->axis[1][1] = b->axis[2][2] * b->axis[0][0] - b->axis[0][2] * b->axis[2][0];
    b->axis[1][2] = b->axis[0][1] * b->axis[2][0] - b->axis[2][1] * b->axis[0][0];
    MeReal lo[3], hi[3], p[3];
    for (int k = 0; k < 3; ++k)
        lo[k] = hi[k] = Dot(v[0], b->axis[k]);
    for (int j = 1; j < 3; ++j) {
        for (int k = 0; k < 3; ++k)
            p[k] = Dot(v[j], b->axis[k]);
        ExpandMinMax(lo, hi, p);
    }
    const MeReal m0 = (hi[0] + lo[0]) * 0.5, m1 = (hi[1] + lo[1]) * 0.5, m2 = (hi[2] + lo[2]) * 0.5;
    for (int k = 0; k < 3; ++k)
        b->center[k] = (m0 * b->axis[0][k] + m1 * b->axis[1][k]) + b->axis[2][k] * m2;
    b->he[0] = (hi[0] - lo[0]) * 0.5;
    b->he[1] = (hi[1] - lo[1]) * 0.5;
    b->he[2] = (hi[2] - lo[2]) * 0.5;
    return 1;
}

// Win 0x8262d0 (thiscall, stdcall args): the box of the triangles: principal axes of the
// covariance (identity when Jacobi does not converge in 35 sweeps), extents along them, sorted,
// then refined (when `refine`; the original also checks a global switch, Win 0xad1070, which is
// always 1)
void Fit(Fitter* f, VincGeometryID mesh, OBBBox* box, const int* tris, int n, int refine)
{
    f->mesh = mesh;
    f->box = box;
    f->tris = tris;
    f->n = n;
    if (n == 1) {
        FitTriangle(f);
        return;
    }
    MeReal C[9] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
    Covariance(f, C);
    MeReal V[9];
    if (Eigenvectors(V, C) > 0x23) {
        box->axis[0][0] = 1.0;
        box->axis[0][1] = 0.0;
        box->axis[0][2] = 0.0;
        box->axis[1][0] = 0.0;
        box->axis[1][1] = 1.0;
        box->axis[1][2] = 0.0;
        box->axis[2][0] = 0.0;
        box->axis[2][1] = 0.0;
        box->axis[2][2] = 1.0;
    } else {
        for (int k = 0; k < 3; ++k) {
            box->axis[k][0] = V[k];
            box->axis[k][1] = V[3 + k];
            box->axis[k][2] = V[6 + k];
        }
    }
    MeReal c[3] = { 0.0, 0.0, 0.0 };
    for (int k = 0; k < 3; ++k) {
        const MeReal a[3] = { box->axis[k][0], box->axis[k][1], box->axis[k][2] };
        MeReal* minV;
        MeReal* maxV;
        MeReal lo, hi;
        VincTriangleMeshExtremeVerticesOf(mesh, tris, n, a, &maxV, &minV, &hi, &lo);
        box->he[k] = fabs((hi - lo) * 0.5);
        const MeReal t = box->he[k] + lo;
        c[0] = a[0] * t + c[0];
        c[1] = a[1] * t + c[1];
        c[2] = a[2] * t + c[2];
    }
    box->center[0] = c[0];
    box->center[1] = c[1];
    box->center[2] = c[2];
    SortExtents(box);
    if (refine)
        RefineBox(mesh, tris, n, box);
}

// Win 0x80dc80 (cdecl): the middle of the triangle's extent along axis
MeReal MidProjection(MeReal* const* v, const MeReal* axis)
{
    const MeReal p0 = Dot(v[0], axis);
    const MeReal p1 = Dot(v[1], axis);
    MeReal lo, hi;
    if (p1 > p0) {
        lo = p0;
        hi = p1;
    } else if (p1 < p0) {
        lo = p1;
        hi = p0;
    } else {
        lo = p0;
        hi = p0;
    }
    const MeReal p2 = Dot(v[2], axis);
    if (p2 > hi)
        return (lo + p2) * 0.5;
    if (p2 < lo)
        return (p2 + hi) * 0.5;
    return (lo + hi) * 0.5;
}

inline void SwapInt(int* a, int* b)
{
    const int t = *a;
    *a = *b;
    *b = t;
}

// Win 0x80dd30 (cdecl): partitions tris[0..n) by the box centre plane of axis 0, 1, 2 (the first
// axis that splits them; the plane moves by 0.1 extent when more than 90 % or less than 10 % fall
// on the lower side), else alternately; lower part first
void Split(VincGeometryID mesh, const OBBBox* box, int* tris, int n, int* nLow, int* nHigh)
{
    const MeReal dn = (MeReal)n;
    for (int i = 0;; ++i) {
        const MeReal* axis = box->axis[i];
        MeReal split = Dot(box->center, axis);
        *nHigh = 0;
        *nLow = 0;
        for (int k = 0; k < n; ++k) {
            MeReal* v[3];
            VincTriangleMeshGetTrianglePtrs(mesh, tris[*nLow], &v[0], &v[1], &v[2]);
            if (split > MidProjection(v, axis))
                ++*nLow;
            else {
                SwapInt(&tris[*nLow], &tris[n - *nHigh - 1]);
                ++*nHigh;
            }
        }
        const MeReal ratio = (MeReal)*nLow / dn;
        const bool tooLow = 0.9 < ratio;
        if (tooLow)
            split = split - box->he[i] * 0.1;
        bool again = tooLow;
        if (!(0.1 <= ratio)) {
            split = box->he[i] * 0.1 + split;
            again = true;
        }
        if (again) {
            *nHigh = 0;
            *nLow = 0;
            for (int k = 0; k < n; ++k) {
                MeReal* v[3];
                VincTriangleMeshGetTrianglePtrs(mesh, tris[*nLow], &v[0], &v[1], &v[2]);
                if (MidProjection(v, axis) < split)
                    ++*nLow;
                else {
                    SwapInt(&tris[*nLow], &tris[n - *nHigh - 1]);
                    ++*nHigh;
                }
            }
        }
        if (*nHigh > 0 && *nLow > 0)
            return;
        if (i >= 2)
            break;
    }
    if (*nHigh != 0 && *nLow != 0)
        return;
    *nHigh = 0;
    *nLow = 0;
    for (int k = 0; k < n; ++k) {
        if ((k & 1) == 0)
            ++*nLow;
        else {
            SwapInt(&tris[*nLow], &tris[n - *nHigh - 1]);
            ++*nHigh;
        }
    }
}

// Win 0x8473f0 (cdecl, empty): per-node hook of meshes with flag 1
void NodeHook(VincGeometryID /*mesh*/, VincOBBNode* /*node*/, int* /*tris*/, int /*n*/)
{
}

// Win 0x80dc70 (inlined in 0x80df60): floor(log2(n)), -1 for 0
int Log2(unsigned int n)
{
    int r = -1;
    while (n != 0) {
        n >>= 1;
        ++r;
    }
    return r;
}

// Win node +0x8c (no such field in the Mac layout)
inline void SetFitKind(VincOBBNode* node, int kind)
{
#if defined(_WIN32)
    node->fitKind = kind;
#else
    (void)node;
    (void)kind;
#endif
}

// the triangle subset of a node (VincOBBTree::m28 entries, mesh flag 2)
struct NodeRange {
    int* tris;
    int count;
};

// Win 0x80df60 (thiscall): fills node m08 for tris[0..n) under parent; returns 0 for a leaf
int BuildNode(VincOBBTree* t, int* tris, int n, VincOBBNode* parent)
{
    VincOBBNode* node = &t->nodes[t->m08];
    node->m08 = (int)(size_t)parent;
    if (t->m10 != 0) {
        NodeRange* r = (NodeRange*)t->m28 + t->m08;
        r->tris = tris;
        r->count = n;
    }
    const int depth = Log2((unsigned int)n);
    VincGeometryID mesh = &t->mesh->base;
    if (depth < t->m14) {
        Fitter f;
        Fit(&f, mesh, BoxOf(node), tris, n, 1);
        SetFitKind(node, 0);
    } else {
        FitAABB(mesh, BoxOf(node), tris, n);
        SetFitKind(node, (depth == t->m14) + 1);
    }
    if (t->mesh->flags & 1)
        NodeHook(mesh, node, tris, n);
    if (n == 1) {
        node->triangle = tris[0];
        return 0;
    }
    int nLow, nHigh;
    Split(mesh, BoxOf(node), tris, n, &nLow, &nHigh);
    if (nLow > 0) {
        ++t->m08;
        node->child[0] = &t->nodes[t->m08];
        BuildNode(t, tris, nLow, node);
    }
    if (nHigh > 0) {
        ++t->m08;
        node->child[1] = &t->nodes[t->m08];
        BuildNode(t, tris + nLow, nHigh, node);
    }
    return 1;
}

// Win 0x80e100 (thiscall): adds the box volumes of node's subtree
void AddVolume(const VincOBBNode* node, MeReal* sum)
{
    for (;;) {
        *sum = (node->halfExtents[1] * node->halfExtents[0]) * node->halfExtents[2] + *sum;
        if (node->child[0] == 0 && node->child[1] == 0)
            return;
        AddVolume(node->child[0], sum);
        node = node->child[1];
    }
}

} // namespace

extern "C" {

// Win 0x80e270 (thiscall; the inline initialisation before it is in VincTriangleMeshBuildTree):
// the node array (2 n - 1 nodes: children 0, triangle -1) and, with mesh flag 2, the node ranges
void MEAPI VincOBBTreeInit(VincOBBTree* t, VincGeometryID mesh)
{
    t->m08 = 0;
    t->mesh = (VincTriangleMesh*)mesh;
    const int n = VincTriangleMeshGetTriangleCount(mesh);
    const int count = n + n - 1;
    VincOBBNode* nodes = (VincOBBNode*)MeMemoryAPI.create(count * (int)sizeof(VincOBBNode));
    for (int i = count - 1; i >= 0; --i) {
        nodes[i].child[0] = 0;
        nodes[i].child[1] = 0;
        nodes[i].m08 = 0;
        nodes[i].triangle = -1;
    }
    t->nodes = nodes;
    if (t->m10 != 0) {
        NodeRange* r = (NodeRange*)MeMemoryAPI.create(count * (int)sizeof(NodeRange));   // 8 on 32-bit (PORT64)
        t->m28 = r;
        for (int i = count - 1; i >= 0; --i) {
            r[i].tris = 0;
            r[i].count = 0;
        }
    }
}

// Win 0x80dc60 (thiscall): the depth from which nodes get axis-aligned boxes (VincTriangleMesh
// m80; < 0: 999999999, or 1 with mesh flag 2)
void MEAPI VincOBBTreeSetMaxDepth(VincOBBTree* t, int depth)
{
    t->m14 = depth;
}

// Win 0x8022e0 (thiscall): m1c = per triangle corner the index of its vertex (vertices told apart
// by their position pointers, numbered in order of first use)
void MEAPI VincOBBTreeBuildVertexIndex(VincOBBTree* t)
{
    VincTriangleMesh* m = t->mesh;
    const int n = m->triangleCount;
    int* index = new int[n * 3];
    t->m1c = index;
    std::map<MeReal*, int> ids;
    int next = 0;
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < 3; ++k) {
            MeReal* p = m->corners[i * 3 + k].position;
            if (ids.find(p) == ids.end())
                ids[p] = next++;
        }
    for (int i = 0; i < n; ++i)
        for (int k = 0; k < 3; ++k)
            index[i * 3 + k] = ids[m->corners[i * 3 + k].position];
}

// Win 0x80e150 (thiscall): builds the tree over all triangles; with mesh flag 2 the triangle
// order (m18) and the vertex index (m1c) are kept
int MEAPI VincOBBTreeBuild(VincOBBTree* t)
{
    const int n = VincTriangleMeshGetTriangleCount(&t->mesh->base);
    int* tris = (int*)MeMemoryAPI.create(n * 4);
    if (t->m14 < 0)
        t->m14 = t->m10 != 0 ? 1 : 999999999;
    for (int i = 0; i < n; ++i)
        tris[i] = i;
    BuildNode(t, tris, n, 0);
    if (t->m10 == 0) {
        if (tris != 0)
            MeMemoryAPI.destroy(tris);
        return 1;
    }
    t->m18 = tris;
    VincOBBTreeBuildVertexIndex(t);
    return 1;
}

// Win 0x80e240 (thiscall): the summed box volume of all nodes
MeReal MEAPI VincOBBTreeGetVolume(VincOBBTree* t)
{
    MeReal sum = 0.0;
    AddVolume(t->nodes, &sum);
    return sum;
}

// entry points of the box fitter for the tests (not in the original)
void MEAPI VincOBBFitBoxForTests(VincGeometryID mesh, MeReal* box15, int* tris, int n, int kind)
{
    Fitter f;
    if (kind == 0)
        Fit(&f, mesh, (OBBBox*)box15, tris, n, 1);
    else if (kind == 1)
        Fit(&f, mesh, (OBBBox*)box15, tris, n, 0);
    else if (kind == 2)
        FitAABB(mesh, (OBBBox*)box15, tris, n);
    else if (kind == 3)
        RefineBox(mesh, tris, n, (OBBBox*)box15);
    else if (kind == 4) {
        f.mesh = mesh;
        f.box = (OBBBox*)box15;
        f.tris = tris;
        f.n = n;
        Covariance(&f, box15);
    } else if (kind == 5)
        PerpendicularNormalsBox(mesh, (OBBBox*)box15, tris, n);
}

void MEAPI VincOBBSplitForTests(VincGeometryID mesh, const MeReal* box15, int* tris, int n, int* nLow, int* nHigh)
{
    Split(mesh, (const OBBBox*)box15, tris, n, nLow, nHigh);
}

void MEAPI VincOBBRotateAndFitForTests(VincGeometryID mesh, int* tris, int n, const MeReal* axis, MeReal angle,
                                       const MeReal* in15, MeReal* out15)
{
    MeReal* minV[3];
    MeReal* maxV[3];
    RotateAndFit(mesh, tris, n, minV, maxV, axis, angle, (const OBBBox*)in15, (OBBBox*)out15);
}

} // extern "C"
