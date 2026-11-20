// Vortex 5.0.0 build 800 "vortexldd41" - the TriangleMesh geometry (restored;
// Mac 557c 0x4c41bc-0x4c4975, Win 0x7b2e70-0x7b4330). The Mac keeps the type functions and the
// accessors the pair tests use; creation and editing (VincTriangleMeshCreate / AddTriangle / ...)
// are Win only. Building (OBB tree): VincTriangleMeshBuild.cpp.
#include "Vinc/VincTriangleMesh.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"

#include <math.h>

#include "VincInternal.h"

namespace {

MeI16 s_triangleMeshTypeId = -1;               // Mac 0x779764, Win 0xac7eb4
const char* s_triangleMeshTypeName = "TriangleMesh"; // Mac 0x779768

inline VincTriangleMesh* Mesh(VincGeometryID g)
{
    return (VincTriangleMesh*)g;
}

// (name inferred; inlined on the Mac at 0x4c4718; the Win build calls an out-of-line copy at
// 0x7ae700 in the 558e helper block, not claimed here): |(b - a) x (c - b)| / 2
MeReal TriangleArea(const MeReal* a, const MeReal* b, const MeReal* c)
{
    MeReal ux = c[0] - b[0], uy = c[1] - b[1], uz = c[2] - b[2];
    MeReal wx = b[0] - a[0], wy = b[1] - a[1], wz = b[2] - a[2];
    MeReal x = uz * wy - uy * wz;
    MeReal y = ux * wz - uz * wx;
    MeReal z = uy * wx - ux * wy;
    return sqrt(x * x + y * y + z * z) * 0.5;
}

} // namespace

extern "C" {

// Mac 0x4c41bc, Win 0x7b2e70 (name inferred)
MeI16 MEAPI VincTriangleMeshGetTypeId(void)
{
    return s_triangleMeshTypeId;
}

// Mac 0x4c41ce, Win 0x7b2f40 (name inferred)
int MEAPI VincTriangleMeshGetTriangleCount(VincGeometryID g)
{
    return Mesh(g)->triangleCount;
}

// Mac 0x4c41d9, Win 0x7b2f80 (name inferred)
void MEAPI VincTriangleMeshGetTrianglePtrs(VincGeometryID g, int i, MeReal** v0, MeReal** v1, MeReal** v2)
{
    VincTriangleMeshCorner* c = Mesh(g)->corners;
    *v0 = c[i * 3].position;
    *v1 = c[i * 3 + 1].position;
    *v2 = c[i * 3 + 2].position;
}

// Mac 0x4c421b, Win 0x7b3130 (name inferred)
void MEAPI VincTriangleMeshGetNormalPtr(VincGeometryID g, int i, MeReal** n)
{
    *n = Mesh(g)->normals + i * 3;
}

// Mac 0x4c4234, Win 0x7b3480 (name inferred)
int MEAPI VincTriangleMeshIsBuilt(VincGeometryID g)
{
    VincTriangleMesh* m = Mesh(g);
    if (m->normals == 0 || m->tree == 0)
        return 0;
    return m->tree->nodes != 0;
}

// Mac 0x4c425a, Win 0x7b3540 (name inferred)
void MEAPI VincTriangleMeshGetAABB(VincGeometryID g, MeMatrix4 tm4, MeReal* aabb6)
{
    VincTriangleMesh* m = Mesh(g);
    const MeReal* tm = &tm4[0][0];
    MeReal c0 = m->center[0], c1 = m->center[1], c2 = m->center[2];
    MeReal wx = c0 * tm[0] + c1 * tm[4] + c2 * tm[8] + tm[12];
    MeReal wy = c0 * tm[1] + c1 * tm[5] + c2 * tm[9] + tm[13];
    MeReal wz = c0 * tm[2] + c1 * tm[6] + c2 * tm[10] + tm[14];
    MeReal h0 = m->halfExtents[0], h1 = m->halfExtents[1], h2 = m->halfExtents[2];
    MeReal ex = h0 * fabs(tm[0]) + fabs(tm[4]) * h1 + fabs(tm[8]) * h2 + 1e-14;
    MeReal ey = h0 * fabs(tm[1]) + fabs(tm[5]) * h1 + fabs(tm[9]) * h2 + 1e-14;
    MeReal ez = h0 * fabs(tm[2]) + h1 * fabs(tm[6]) + fabs(tm[10]) * h2 + 1e-14;
    aabb6[0] = wx - ex;
    aabb6[1] = wx + ex;
    aabb6[2] = wy - ey;
    aabb6[3] = wy + ey;
    aabb6[4] = wz - ez;
    aabb6[5] = wz + ez;
}

// Mac 0x4c4419, Win 0x7b36b0 (name inferred)
void MEAPI VincTriangleMeshGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    VincTriangleMesh* m = Mesh(g);
    center[0] = m->center[0];
    center[1] = m->center[1];
    center[2] = m->center[2];
    *radius = m->radius;
}

// Mac 0x4c444d, Win 0x7b3660 (name inferred)
void MEAPI VincTriangleMeshGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4)
{
    MeReal aabb[6];
    MeVector3 mn, mx;
    VincTriangleMeshGetAABB(g, tm, aabb);
    VincAABBGetMinMax(aabb, mn, mx);
    out4[0] = mn[0];
    out4[1] = mx[0];
    out4[2] = mn[1];
    out4[3] = mx[1];
}

// Mac 0x4c44b5, Win 0x7b3630 (name inferred)
void MEAPI VincTriangleMeshUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                                MeReal* aabb6)
{
    VincTriangleMeshGetAABB(g, tm, aabb6);
    VincAABBGetMinMax(aabb6, minCorner, maxCorner);
}

// Mac 0x4c44f1, Win 0x7b34d0 (name inferred)
void MEAPI VincTriangleMeshDestroy(VincGeometryID g)
{
    VincTriangleMesh* m = Mesh(g);
    if (m == 0)
        return;
    if (m->areas != 0)
        MeMemoryAPI.destroy(m->areas);
    if (m->tree != 0) {
        VincOBBTreeDestroy(m->tree);
        MeMemoryAPI.destroy(m->tree);
    }
    if (m->m08 != 0)
        MeMemoryAPI.destroy(m->m08);
    if (m->m28 != 0)
        MeMemoryAPI.destroy(m->m28);
    if (m->destroyCallback != 0)
        m->destroyCallback(m->destroyCallbackData);
    MeMemoryAPI.destroy(m);
}

// Mac 0x4c4581, Win 0x7b4330 (name inferred)
void MEAPI VincTriangleMeshRegisterType(void)
{
    s_triangleMeshTypeId = (MeI16)VincGeometryRegisterType(
        10, s_triangleMeshTypeName, VincTriangleMeshDestroy, VincTriangleMeshUpdateAABB,
        VincTriangleMeshGetXYAABB, VincTriangleMeshGetBSphere, VincTriangleMeshMaximumPoint,
        VincTriangleMeshGetMassProperties);
}

// Mac 0x4c45ed, Win 0x7b3150 (name inferred)
void MEAPI VincTriangleMeshGetCornerData(VincGeometryID g, int i, MeVector3 data0, MeVector3 data1)
{
    VincTriangleMeshCorner* c = Mesh(g)->corners + i * 3;
    data0[0] = (MeReal)c[0].m08;
    data1[0] = (MeReal)c[0].m0c;
    data0[1] = (MeReal)c[1].m08;
    data1[1] = (MeReal)c[1].m0c;
    data0[2] = (MeReal)c[2].m08;
    data1[2] = (MeReal)c[2].m0c;
}

// Mac 0x4c4695, Win 0x7b36e0 (name inferred). NOTE: the local centre is added without rotation.
void MEAPI VincTriangleMeshMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal r = m->radius;
    out[0] = r * dir[0] + (tm[3][0] + m->center[0]);
    out[1] = r * dir[1] + (tm[3][1] + m->center[1]);
    out[2] = r * dir[2] + (tm[3][2] + m->center[2]);
}

// Mac 0x4c4718, Win 0x7b3de0 (name inferred)
MeReal MEAPI VincTriangleMeshGetTriangleArea(VincGeometryID g, int i)
{
    if (VincTriangleMeshIsBuilt(g) == 0) {
        MeReal *v0, *v1, *v2;
        VincTriangleMeshGetTrianglePtrs(g, i, &v0, &v1, &v2);
        return TriangleArea(v0, v1, v2);
    }
    return Mesh(g)->areas[i];
}

// Mac 0x4c4810, Win 0x7b3cd0 (name inferred)
void MEAPI VincTriangleMeshGetTriangleNormal(VincGeometryID g, int i, MeVector3 n)
{
    if (VincTriangleMeshIsBuilt(g) == 0) {
        MeReal *v0, *v1, *v2;
        VincTriangleMeshGetTrianglePtrs(g, i, &v0, &v1, &v2);
        MeReal x = (v2[2] - v0[2]) * (v1[1] - v0[1]) - (v2[1] - v0[1]) * (v1[2] - v0[2]);
        n[0] = x;
        MeReal y = (v2[0] - v0[0]) * (v1[2] - v0[2]) - (v2[2] - v0[2]) * (v1[0] - v0[0]);
        n[1] = y;
        MeReal z = (v2[1] - v0[1]) * (v1[0] - v0[0]) - (v2[0] - v0[0]) * (v1[1] - v0[1]);
        n[2] = z;
        MeReal len = sqrt(x * x + y * y + z * z);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            n[0] = x * inv;
            n[1] = y * inv;
            n[2] = inv * z;
        }
    } else {
        MeReal* p = Mesh(g)->normals + i * 3;
        n[0] = p[0];
        n[1] = p[1];
        n[2] = p[2];
    }
}

// Mac 0x4c4975, Win 0x7b3fe0 (name inferred): sum of signed tetrahedra (centroid of the vertices,
// triangle) through the 558e mass helpers; relTM = identity at the centre of mass.
int MEAPI VincTriangleMeshGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia,
                                      MeReal* volume)
{
    MeReal area = 0.0, cz = 0.0, cy = 0.0, cx = 0.0, sz = 0.0, sy = 0.0, sx = 0.0;
    for (int i = 0; i < Mesh(g)->triangleCount; ++i) {
        MeReal *v0, *v1, *v2;
        VincTriangleMeshGetTrianglePtrs(g, i, &v0, &v1, &v2);
        cx = cx + v0[0] + v1[0] + v2[0];
        cy = cy + v0[1] + v1[1] + v2[1];
        cz = cz + v0[2] + v1[2] + v2[2];
        MeVector3 nrm;
        VincTriangleMeshGetTriangleNormal(g, i, nrm);
        MeReal a = VincTriangleMeshGetTriangleArea(g, i);
        area = area + a;
        nrm[0] = nrm[0] * a;
        nrm[1] = nrm[1] * a;
        nrm[2] = nrm[2] * a;
        sx = sx + nrm[0];
        sy = sy + nrm[1];
        sz = sz + nrm[2];
    }
    int n = Mesh(g)->triangleCount;
    MeReal inv3n = 1.0 / (MeReal)(n * 3);
    MeReal ccx = cx * inv3n;
    MeReal ccy = cy * inv3n;
    MeReal ccz = inv3n * cz;
    MeReal ia = 1.0 / area;
    if (1e-7 < sqrt(sx * ia * sx * ia + sy * ia * sy * ia + ia * sz * ia * sz))
        MeInfo(1, "TriangleMesh: mesh volume seems not to be closed. Mass properties might be inacurate.\n");
    MeReal* tetra = (MeReal*)MeMemoryAPI.create(Mesh(g)->triangleCount * 13 * sizeof(MeReal));
    n = Mesh(g)->triangleCount;
    for (int i = 0; i < n; ++i, n = Mesh(g)->triangleCount) {
        MeReal *v0, *v1, *v2;
        VincTriangleMeshGetTrianglePtrs(g, i, &v0, &v1, &v2);
        MeVector3 a, b, c;
        a[2] = v0[2] - ccz;
        a[1] = v0[1] - ccy;
        a[0] = v0[0] - ccx;
        b[2] = v1[2] - ccz;
        b[1] = v1[1] - ccy;
        b[0] = v1[0] - ccx;
        c[2] = v2[2] - ccz;
        c[1] = v2[1] - ccy;
        c[0] = v2[0] - ccx;
        VincTetraMassProperties(tetra + i * 13, a, b, c);
    }
    MeReal sum[13];
    VincSumMassProperties(tetra, n, sum);
    *volume = sum[0];
    relTM[0][0] = 1.0;
    relTM[0][1] = 0.0;
    relTM[0][2] = 0.0;
    relTM[0][3] = 0.0;
    relTM[1][0] = 0.0;
    relTM[1][1] = 1.0;
    relTM[1][2] = 0.0;
    relTM[1][3] = 0.0;
    relTM[2][0] = 0.0;
    relTM[2][1] = 0.0;
    relTM[2][2] = 1.0;
    relTM[2][3] = 0.0;
    relTM[3][3] = 1.0;
    relTM[3][0] = ccx + sum[1];
    relTM[3][1] = ccy + sum[2];
    relTM[3][2] = ccz + sum[3];
    for (int k = 0; k < 9; ++k)
        (&inertia[0][0])[k] = sum[4 + k];
    MeMemoryAPI.destroy(tetra);
    return 1;
}

// ---- Win-only API (dead-stripped on the Mac) ----

// Win 0x7b2e80 (name inferred)
VincGeometryID MEAPI VincTriangleMeshCreate(int triangleMaxCount, int flags)
{
    VincTriangleMesh* m = (VincTriangleMesh*)MeMemoryAPI.create(sizeof(VincTriangleMesh));
    VincGeometryInit(&m->base, s_triangleMeshTypeId);
    if (triangleMaxCount < 1)
        triangleMaxCount = 1;
    m->triangleMaxCount = triangleMaxCount;
    m->degenerateTolerance = 1e-14;
    m->triangleCount = 0;
    m->normals = 0;
    m->areas = 0;
    m->tree = 0;
    m->m08 = 0;
    m->m28 = 0;
    m->halfExtents[0] = 0.0;
    m->halfExtents[1] = 0.0;
    m->halfExtents[2] = 0.0;
    m->center[0] = 0.0;
    m->center[1] = 0.0;
    m->center[2] = 0.0;
    m->radius = 0.0;
    m->corners = (VincTriangleMeshCorner*)MeMemoryAPI.create(m->triangleMaxCount * 3 * sizeof(VincTriangleMeshCorner));
    m->destroyCallback = 0;
    m->destroyCallbackData = 0;
    m->flags = flags;
    m->m80 = -1;
    return &m->base;
}

// Win 0x7b2f20 (name inferred)
void MEAPI VincTriangleMeshSetTriangleData(VincGeometryID g, VincTriangleMeshCorner* corners, MeReal* normals,
                                     int count)
{
    Mesh(g)->corners = corners;
    Mesh(g)->normals = normals;
    Mesh(g)->triangleCount = count;
}

// Win 0x7b2f50 (name inferred)
void MEAPI VincTriangleMeshSetDegenerateTolerance(VincGeometryID g, MeReal tol)
{
    Mesh(g)->degenerateTolerance = tol;
}

// Win 0x7b2f60 (name inferred)
void MEAPI VincTriangleMeshGetM08(VincGeometryID g, int** p)
{
    *p = Mesh(g)->m08;
}

// Win 0x7b2fc0 (name inferred)
void MEAPI VincTriangleMeshGetVertex0Ptr(VincGeometryID g, int i, MeReal** v0)
{
    *v0 = Mesh(g)->corners[i * 3].position;
}

// Win 0x7b2fe0 (name inferred)
void MEAPI VincTriangleMeshGetTriangle(VincGeometryID g, int i, MeVector3 v0, MeVector3 v1, MeVector3 v2)
{
    VincTriangleMeshCorner* c = Mesh(g)->corners + i * 3;
    MeReal* p = c[0].position;
    v0[0] = p[0];
    v0[1] = p[1];
    v0[2] = p[2];
    p = c[1].position;
    v1[0] = p[0];
    v1[1] = p[1];
    v1[2] = p[2];
    p = c[2].position;
    v2[0] = p[0];
    v2[1] = p[1];
    v2[2] = p[2];
}

// Win 0x7b3050 (name inferred)
void MEAPI VincTriangleMeshComputeNormals(VincGeometryID g)
{
    VincTriangleMesh* m = Mesh(g);
    for (int i = 0; i < m->triangleCount; ++i) {
        VincTriangleMeshCorner* c = m->corners + i * 3;
        MeReal* v0 = c[0].position;
        MeReal* v1 = c[1].position;
        MeReal* v2 = c[2].position;
        MeReal* n = m->normals + i * 3;
        n[0] = (v1[1] - v0[1]) * (v2[2] - v0[2]) - (v1[2] - v0[2]) * (v2[1] - v0[1]);
        n[1] = (v1[2] - v0[2]) * (v2[0] - v0[0]) - (v1[0] - v0[0]) * (v2[2] - v0[2]);
        n[2] = (v1[0] - v0[0]) * (v2[1] - v0[1]) - (v1[1] - v0[1]) * (v2[0] - v0[0]);
        MeReal len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
        if (len != 0.0) {
            MeReal inv = 1.0 / len;
            n[0] = n[0] * inv;
            n[1] = n[1] * inv;
            n[2] = inv * n[2];
        }
    }
}

// Win 0x7b31b0 (name inferred)
VincOBBTree* MEAPI VincTriangleMeshGetTree(VincGeometryID g)
{
    return Mesh(g)->tree;
}

// Win 0x7b31c0 (name inferred)
void MEAPI VincTriangleMeshSetNormal(VincGeometryID g, int i, const MeVector3 n)
{
    MeReal* p = Mesh(g)->normals + i * 3;
    p[0] = n[0];
    p[1] = n[1];
    p[2] = n[2];
}

// Win 0x7b31f0 (name inferred). NOTE: triangleMaxCount is not updated.
void MEAPI VincTriangleMeshSetTriangleMaxCount(VincGeometryID g, int n)
{
    Mesh(g)->corners = (VincTriangleMeshCorner*)MeMemoryAPI.resize(
        Mesh(g)->corners, n * 3 * sizeof(VincTriangleMeshCorner));
}

// Win 0x7b3220 ("VincTriangleMeshAddTriangle()" string). The vertices are referenced, not copied.
int MEAPI VincTriangleMeshAddTriangle(VincGeometryID g, MeReal* v0, MeReal* v1, MeReal* v2)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal area = TriangleArea(v0, v1, v2);
    if (area < m->degenerateTolerance) {
        MeInfo(1, "VincTriangleMeshAddTriangle(): degenerate triangle ignored: \n [(%g, %g, %g), (%g, %g, %g), (%g, %g, %g)]\n",
               v0[0], v0[1], v0[2], v1[0], v1[1], v1[2], v2[0], v2[1], v2[2], (double)(float)area);
        return 0;
    }
    int max = m->triangleMaxCount;
    if (m->triangleCount == max) {
        m->triangleMaxCount = max * 2;
        m->corners = (VincTriangleMeshCorner*)MeMemoryAPI.resize(
            m->corners, max * 2 * 3 * sizeof(VincTriangleMeshCorner));
    }
    VincTriangleMeshCorner* c = m->corners + m->triangleCount * 3;
    c[0].position = v0;
    c[1].position = v1;
    c[2].position = v2;
    m->triangleCount = m->triangleCount + 1;
    return 1;
}

// Win 0x7b3310 (name inferred)
void MEAPI VincTriangleMeshComputeBounds(VincGeometryID g)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal maxX = -1e+20, minZ = 1e+20, minX = 1e+20, minY = 1e+20, maxZ = -1e+20, maxY = -1e+20;
    for (int i = 0; i < m->triangleCount; ++i) {
        for (int k = 0; k < 3; ++k) {
            MeReal* p = m->corners[i * 3 + k].position;
            if (p[0] < minX)
                minX = p[0];
            if (maxX < p[0])
                maxX = p[0];
            if (p[1] < minY)
                minY = p[1];
            if (maxY < p[1])
                maxY = p[1];
            if (p[2] < minZ)
                minZ = p[2];
            if (maxZ < p[2])
                maxZ = p[2];
        }
    }
    m->halfExtents[0] = (maxX - minX) * 0.5;
    m->center[0] = (maxX + minX) * 0.5;
    m->halfExtents[1] = (maxY - minY) * 0.5;
    m->center[1] = (maxY + minY) * 0.5;
    m->halfExtents[2] = (maxZ - minZ) * 0.5;
    m->center[2] = (maxZ + minZ) * 0.5;
    m->radius = sqrt(m->halfExtents[0] * m->halfExtents[0] + m->halfExtents[1] * m->halfExtents[1]
                     + m->halfExtents[2] * m->halfExtents[2]);
}

// Win 0x7b3450 (name inferred)
int MEAPI VincTriangleMeshGetCornerVertexIndex(VincGeometryID g, int i, int corner)
{
    return Mesh(g)->corners[i * 3 + corner].m04 / 3;
}

// Win 0x7b34b0 (name inferred)
void MEAPI VincTriangleMeshSetDestroyCallback(VincGeometryID g, VincTriangleMeshDestroyCallback fn)
{
    Mesh(g)->destroyCallback = fn;
}

// Win 0x7b34c0 (name inferred)
void MEAPI VincTriangleMeshSetDestroyCallbackData(VincGeometryID g, void* data)
{
    Mesh(g)->destroyCallbackData = data;
}

// Win 0x7b3c30 (name inferred)
void MEAPI VincTriangleMeshComputeAreas(VincGeometryID g)
{
    VincTriangleMesh* m = Mesh(g);
    for (int i = 0; i < m->triangleCount; ++i) {
        VincTriangleMeshCorner* c = m->corners + i * 3;
        m->areas[i] = TriangleArea(c[0].position, c[1].position, c[2].position);
    }
}

namespace {
inline void Extreme(MeReal* p, const MeReal* dir, MeReal** maxV, MeReal** minV, MeReal* maxD, MeReal* minD)
{
    MeReal d = (p[1] * dir[1] + dir[0] * p[0]) + p[2] * dir[2]; // the original's (x + y) + z
    if (d < *minD) {
        *minD = d;
        *minV = p;
    }
    if (*maxD < d) {
        *maxD = d;
        *maxV = p;
    }
}
} // namespace

// Win 0x7b3730 (name inferred)
void MEAPI VincTriangleMeshExtremeVerticesOf(VincGeometryID g, const int* tris, int n, const MeReal* dir,
                                       MeReal** maxV, MeReal** minV, MeReal* maxD, MeReal* minD)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal* p = m->corners[tris[0] * 3].position;
    *maxV = p;
    *minV = p;
    MeReal d = (p[1] * dir[1] + dir[0] * p[0]) + p[2] * dir[2]; // the original's (x + y) + z
    *minD = d;
    *maxD = d;
    for (int i = 0; i < n; ++i) {
        VincTriangleMeshCorner* c = m->corners + tris[i] * 3;
        Extreme(c[0].position, dir, maxV, minV, maxD, minD);
        Extreme(c[1].position, dir, maxV, minV, maxD, minD);
        Extreme(c[2].position, dir, maxV, minV, maxD, minD);
    }
}

// Win 0x7b3880 (name inferred)
void MEAPI VincTriangleMeshExtremeVertices(VincGeometryID g, const MeReal* dir, MeReal** maxV, MeReal** minV,
                                     MeReal* maxD, MeReal* minD)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal* p = m->corners[0].position;
    *maxV = p;
    *minV = p;
    MeReal d = (p[1] * dir[1] + dir[0] * p[0]) + p[2] * dir[2]; // the original's (x + y) + z
    *minD = d;
    *maxD = d;
    for (int i = 0; i < m->triangleCount; ++i) {
        VincTriangleMeshCorner* c = m->corners + i * 3;
        Extreme(c[0].position, dir, maxV, minV, maxD, minD);
        Extreme(c[1].position, dir, maxV, minV, maxD, minD);
        Extreme(c[2].position, dir, maxV, minV, maxD, minD);
    }
}

// Win 0x7b39b0 (name inferred): one corner of every other triangle, cycling corners 0, 1, 2
void MEAPI VincTriangleMeshExtremeVerticesSparse(VincGeometryID g, const MeReal* dir, MeReal** maxV,
                                           MeReal** minV, MeReal* maxD, MeReal* minD)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal* p = m->corners[0].position;
    *maxV = p;
    *minV = p;
    MeReal d = (p[1] * dir[1] + dir[0] * p[0]) + p[2] * dir[2]; // the original's (x + y) + z
    *minD = d;
    *maxD = d;
    int corner = 0;
    if (0 < m->triangleCount) {
        int count = ((unsigned int)(m->triangleCount - 1) >> 1) + 1;
        for (int i = 0; count != 0; i += 2, --count) {
            Extreme(m->corners[i * 3 + corner].position, dir, maxV, minV, maxD, minD);
            if (2 < ++corner)
                corner = 0;
        }
    }
}

// Win 0x7b3a90 (name inferred): like ExtremeVerticesOf, each vertex once (marks[vertex] = mark)
void MEAPI VincTriangleMeshExtremeVerticesMarked(VincGeometryID g, const int* tris, int n, const MeReal* dir,
                                           MeReal** maxV, MeReal** minV, MeReal* maxD, MeReal* minD,
                                           const int* vertexIndices3, int* marks, int mark)
{
    VincTriangleMesh* m = Mesh(g);
    MeReal* p = m->corners[tris[0] * 3].position;
    *maxV = p;
    *minV = p;
    MeReal d = (p[1] * dir[1] + p[0] * dir[0]) + p[2] * dir[2]; // the original's (x + y) + z
    *minD = d;
    *maxD = d;
    for (int i = 0; i < n; ++i) {
        VincTriangleMeshCorner* c = m->corners + tris[i] * 3;
        const int* vi = vertexIndices3 + i * 3;
        for (int k = 0; k < 3; ++k) {
            if (marks[vi[k]] != mark) {
                marks[vi[k]] = mark;
                Extreme(c[k].position, dir, maxV, minV, maxD, minD);
            }
        }
    }
}

} // extern "C"
