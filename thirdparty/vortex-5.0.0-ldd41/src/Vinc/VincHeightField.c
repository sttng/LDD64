/* Vortex 5.0.0 build 800 "vortexldd41" - regular-grid height field geometry and its pair tests
 * (restored). Mac sub-TU 558c 0x532a4f-0x53b3de (the CxSmallSort methods that
 * follow it belong to CxSmallSort.cpp, V3.8), Win 0x770910-0x7738d0, 0x75c400-0x75c6a0,
 * 0x7b43b0-0x7b5e62. See Vinc/VincHeightField.h. Names (name inferred) except "RGHeightField".
 * The pair tests build the two triangles of each cell under the other geometry, hand them to the
 * triangle contact generators (VincTriangleContacts.h) and reduce with VincContactSimplify. */
#include "Vinc/VincHeightField.h"
#include "Vinc/VincPrimitives.h"
#include "Vinc/VincTriangleContacts.h"
#include "Vinc/VincContactSimplify.h"
#include "Vinc/VincMath.h"
#include "Me/MeMemory.h"
#include "Me/MeMessage.h"

#include <float.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>

/* Mac 0x779d20 / Win 0xac7ec0 */
static MeI16 sHeightFieldTypeId = -1;
/* Mac 0x779d24 */
static const char* sHeightFieldName = "RGHeightField";

int VincBoxHeightFieldMaxContacts = 128;       /* Mac 0x779d28, Win 0xabd93c */
int VincCapsuleHeightFieldMaxContacts = 128;   /* Mac 0x779d2c, Win 0xabd8c4 */
int VincCylinderHeightFieldMaxContacts = 400;  /* Mac 0x779d30, Win 0xabd8ec */
int VincSphereHeightFieldMaxContacts = 128;    /* Mac 0x779d34, Win 0xabd914 */

#define HF(g) ((VincHeightField*)(g))
#define VINC_MAX(a, b) ((a) > (b) ? (a) : (b))

/* sum of squares from 0 (the original's loop order: ((0 + x x) + y y) + z z) */
static MeReal VincSumSq3Fn(MeReal x, MeReal y, MeReal z)
{
    MeReal s = 0.0;
    s = x * x + s;
    s = y * y + s;
    s = z * z + s;
    return s;
}

/* ======================================================================================== */
/* geometry                                                                                 */
/* ======================================================================================== */

/* Mac 0x532a4f, Win 0x7b43b0 (the public 0x53b3d0 / 0x75c400 returns it as int) */
MeI16 MEAPI VincHeightFieldGetTypeId(void)
{
    return sHeightFieldTypeId;
}

/* Mac 0x532a61, Win 0x7b43c0 (also Win 0x75c6a0) */
void MEAPI VincHeightFieldDestroy(VincGeometryID g)
{
    if (g != 0)
        MeMemoryAPI.destroy(g);
}

/* Mac 0x532a87 (public 0x53b350), Win 0x7b43e0 (public 0x75c4a0) */
void MEAPI VincRGHeightFieldUpdate(VincGeometryID g)
{
    VincHeightField* hf = HF(g);
    int i;
    hf->zMin = 1e30;
    hf->zMax = -1e30;
    for (i = 0; i < hf->xCount * hf->yCount; ++i) {
        if (hf->heights[i] < hf->zMin)
            hf->zMin = hf->heights[i];
        if (hf->heights[i] > hf->zMax)
            hf->zMax = hf->heights[i];
    }
}

/* Mac 0x532afc, Win 0x7b4fe0 */
int MEAPI VincHeightFieldInit(VincHeightField* hf, MeReal* heights, const int* xCount, const int* yCount,
                              const MeReal* dx, const MeReal* dy, const MeReal* x0, const MeReal* y0)
{
    hf->heights = heights;
    hf->xCount = *xCount;
    hf->yCount = *yCount;
    hf->dx = *dx;
    hf->dy = *dy;
    hf->x0 = *x0;
    hf->y0 = *y0;
    hf->xCells = *xCount - 1;
    hf->yCells = *yCount - 1;
    hf->xSize = (MeReal)hf->xCells * *dx;
    hf->ySize = (MeReal)hf->yCells * *dy;
    hf->invDx = 1.0 / hf->dx;
    hf->invDy = 1.0 / hf->dy;
    VincRGHeightFieldUpdate((VincGeometryID)hf);
    return 1;
}

/* the bottom of the solid: zMin - depth (infinite depth: zMin - the larger half extent) */
static MeReal VincHeightFieldBottom(const VincHeightField* hf, MeReal hx, MeReal hy)
{
    if (hf->infiniteDepth)
        return hf->zMin - VINC_MAX(hy, hx);
    return hf->zMin - hf->depth;
}

/* Mac 0x532bbc, Win 0x7b4570 */
void MEAPI VincHeightFieldGetAABB(VincGeometryID g, MeMatrix4 tm4, MeReal box[6])
{
    const VincHeightField* hf = HF(g);
    const MeReal* tm = (const MeReal*)tm4;
    MeReal hx = hf->xSize * 0.5, hy = 0.5 * hf->ySize;
    MeReal zlow = VincHeightFieldBottom(hf, hx, hy);
    MeReal hz = (hf->zMax - zlow) * 0.5;
    MeVector3 c, w;
    MeReal ex, ey, ez;
    c[2] = (zlow + hf->zMax) * 0.5;
    c[1] = (hf->y0 + (hf->y0 + hf->ySize)) * 0.5;
    c[0] = (hf->x0 + (hf->x0 + hf->xSize)) * 0.5;
    VincTMTransform(tm, c, w);
    ex = fabs(tm[0]) * hx + fabs(tm[4]) * hy + fabs(tm[8]) * hz;
    ey = fabs(tm[1]) * hx + fabs(tm[5]) * hy + fabs(tm[9]) * hz;
    ez = fabs(tm[2]) * hx + fabs(tm[6]) * hy + fabs(tm[10]) * hz;
    box[0] = w[0] - ex;
    box[1] = w[0] + ex;
    box[2] = w[1] - ey;
    box[3] = w[1] + ey;
    box[4] = w[2] - ez;
    box[5] = w[2] + ez;
}

/* MeSign: 0, -1 or 1 (NaN: 1) (Mac 0x532db3 regparm EAX, Win 0x7b47d0 ECX) */
static MeReal VincHFSign(const MeReal* x)
{
    if (*x == 0.0)
        return 0.0;
    return *x < 0.0 ? -1.0 : 1.0;
}

/* Mac 0x5332b9, Win 0x7b4f10: unit mass properties (the height field is static) */
int MEAPI VincHeightFieldGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume)
{
    (void)g;
    inertia[0][0] = 1.0;
    inertia[1][1] = 1.0;
    inertia[2][2] = 1.0;
    inertia[0][1] = 0.0;
    inertia[0][2] = 0.0;
    inertia[1][0] = 0.0;
    inertia[1][2] = 0.0;
    inertia[2][0] = 0.0;
    inertia[2][1] = 0.0;
    *volume = 1.0;
    relTM[0][0] = 1.0; relTM[0][1] = 0.0; relTM[0][2] = 0.0; relTM[0][3] = 0.0;
    relTM[1][0] = 0.0; relTM[1][1] = 1.0; relTM[1][2] = 0.0; relTM[1][3] = 0.0;
    relTM[2][0] = 0.0; relTM[2][1] = 0.0; relTM[2][2] = 1.0; relTM[2][3] = 0.0;
    relTM[3][0] = 0.0; relTM[3][1] = 0.0; relTM[3][2] = 0.0; relTM[3][3] = 1.0;
    return 1;
}

/* Mac 0x533370, Win 0x7b4730 */
void MEAPI VincHeightFieldGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius)
{
    const VincHeightField* hf = HF(g);
    MeReal hx = hf->xSize * 0.5, hy = hf->ySize * 0.5;
    MeReal zlow = VincHeightFieldBottom(hf, hx, hy);
    MeReal hz = (hf->zMax - zlow) * 0.5;
    center[0] = (hf->x0 + hf->xSize + hf->x0) * 0.5;
    center[1] = (hf->y0 + hf->ySize + hf->y0) * 0.5;
    center[2] = (zlow + hf->zMax) * 0.5;
    *radius = sqrt(hx * hx + hy * hy + hz * hz);
}

/* Mac 0x53345b, Win 0x7b46e0 */
void MEAPI VincHeightFieldGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal out[4])
{
    MeReal box[6];
    MeVector3 lo, hi;
    VincHeightFieldGetAABB(g, tm, box);
    VincAABBGetMinMax(box, lo, hi);
    out[0] = lo[0];
    out[1] = hi[0];
    out[2] = lo[1];
    out[3] = hi[1];
}

/* Mac 0x5334c3, Win 0x7b46b0 */
void MEAPI VincHeightFieldUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6])
{
    VincHeightFieldGetAABB(g, tm, box);
    VincAABBGetMinMax(box, min, max);
}

/* Mac 0x5334ff, Win 0x7b5080 */
VincGeometryID MEAPI VincHeightFieldCreateFromPtrs(MeReal* heights, const int* xCount, const int* yCount,
                                                   const MeReal* dx, const MeReal* dy, const MeReal* x0,
                                                   const MeReal* y0)
{
    VincHeightField* hf = (VincHeightField*)MeMemoryAPI.create(sizeof(VincHeightField));
    VincGeometryInit((VincGeometryID)hf, VincHeightFieldGetTypeId());
    VincHeightFieldInit(hf, heights, xCount, yCount, dx, dy, x0, y0);
    hf->m68 = 0.0;
    hf->infiniteDepth = 1;
    hf->depth = 0.0;
    return (VincGeometryID)hf;
}

/* Mac 0x533719, Win 0x7b47f0. NOTE: the support point of the box around the origin of the
 * geometry frame (not around the grid's centre as in GetAABB; infinite depth counts as 100), as in
 * the original */
void MEAPI VincHeightFieldMaximumPoint(VincGeometryID g, MeMatrix4 tm4, const MeVector3 dir, MeVector3 out)
{
    const VincHeightField* hf = HF(g);
    const MeReal* tm = (const MeReal*)tm4;
    MeReal xSize = hf->xSize, ySize = hf->ySize;
    MeReal hz = (hf->zMax - (hf->infiniteDepth ? hf->zMin - 100.0 : hf->zMin - hf->depth)) * 0.5;
    MeVector3 d, p;
    VincTMInverseRotate(tm, dir, d);
    p[2] = VincHFSign(&d[2]) * hz;
    p[1] = VincHFSign(&d[1]) * (0.5 * ySize);
    p[0] = VincHFSign(&d[0]) * (0.5 * xSize);
    VincTMTransform(tm, p, out);
}

/* Mac 0x533584 (public 0x53b343), Win 0x7b4fa0 (public 0x75c670) */
void MEAPI VincHeightFieldRegisterType(void)
{
    sHeightFieldTypeId = (MeI16)VincGeometryRegisterType(
        8, sHeightFieldName, VincHeightFieldDestroy, VincHeightFieldUpdateAABB, VincHeightFieldGetXYAABB,
        VincHeightFieldGetBSphere, VincHeightFieldMaximumPoint, VincHeightFieldGetMassProperties);
}

/* Mac 0x53b363, Win 0x75c410 */
VincGeometryID MEAPI VincRGHeightFieldCreate(MeReal* heights, int xCount, int yCount, MeReal dx, MeReal dy, MeReal x0,
                                           MeReal y0)
{
    return VincHeightFieldCreateFromPtrs(heights, &xCount, &yCount, &dx, &dy, &x0, &y0);
}

/* ======================================================================================== */
/* grid queries                                                                             */
/* ======================================================================================== */

/* Mac 0x532df4, Win 0x7b4940 (public 0x75c550) */
int MEAPI VincHeightFieldGetCellRange(VincGeometryID g, const MeReal* xy, int* x0, int* x1, int* y0, int* y1)
{
    const VincHeightField* hf = HF(g);
    if (xy[1] < hf->x0 || hf->x0 + hf->xSize < xy[0] || xy[3] < hf->y0 || hf->y0 + hf->ySize < xy[2])
        return 0;
    *x0 = (int)((xy[0] - hf->x0) * hf->invDx);
    *x0 = *x0 < 0 ? 0 : *x0;
    *x1 = (int)((xy[1] - hf->x0) * hf->invDx) + 1;
    *x1 = hf->xCells < *x1 ? hf->xCells : *x1;
    *y0 = (int)((xy[2] - hf->y0) * hf->invDy);
    *y0 = *y0 < 0 ? 0 : *y0;
    *y1 = (int)((xy[3] - hf->y0) * hf->invDy) + 1;
    *y1 = hf->yCells < *y1 ? hf->yCells : *y1;
    return 1;
}

/* Win 0x7b4a20 */
int MEAPI VincHeightFieldGetCellRangeClamped(VincGeometryID g, const MeReal* xy, int* x0, int* x1, int* y0, int* y1)
{
    const VincHeightField* hf = HF(g);
    if (xy[1] <= hf->x0) {
        *x0 = 0;
        *x1 = 1;
    } else if (hf->x0 + hf->xSize < xy[0]) {
        *x1 = hf->xCells;
        *x0 = hf->xCells - 1;
    } else {
        *x0 = (int)((xy[0] - hf->x0) * hf->invDx);
        *x0 = *x0 < 0 ? 0 : *x0;
        *x1 = (int)((xy[1] - hf->x0) * hf->invDx) + 1;
        *x1 = hf->xCells < *x1 ? hf->xCells : *x1;
    }
    if (xy[3] <= hf->y0) {
        *y0 = 0;
        *y1 = 1;
    } else if (hf->y0 + hf->ySize < xy[2]) {
        *y1 = hf->yCells;
        *y0 = hf->yCells - 1;
    } else {
        *y0 = (int)((xy[2] - hf->y0) * hf->invDy);
        *y0 = *y0 < 0 ? 0 : *y0;
        *y1 = (int)((xy[3] - hf->y0) * hf->invDy) + 1;
        *y1 = hf->yCells < *y1 ? hf->yCells : *y1;
    }
    return 1;
}

/* Mac 0x5335f0, Win 0x7b4b50 */
int MEAPI VincHeightFieldGetSphereCellRange(VincGeometryID g, const MeReal* c, const MeReal* radius, int* x0, int* x1,
                                            int* y0, int* y1)
{
    const VincHeightField* hf = HF(g);
    MeReal r = *radius, loX, hiX, loY, hiY;
    if (!(c[2] - r <= hf->zMax))
        return 0;
    hiX = r + c[0];
    if (!(hf->x0 <= hiX))
        return 0;
    loX = c[0] - r;
    if (!(loX <= hf->x0 + hf->xSize))
        return 0;
    hiY = r + c[1];
    if (!(hf->y0 <= hiY))
        return 0;
    loY = c[1] - r;
    if (!(loY <= hf->y0 + hf->ySize))
        return 0;
    *x0 = (int)((loX - hf->x0) * hf->invDx);
    *x0 = *x0 < 0 ? 0 : *x0;
    *x1 = (int)((hiX - hf->x0) * hf->invDx) + 1;
    *x1 = hf->xCells < *x1 ? hf->xCells : *x1;
    *y0 = (int)((loY - hf->y0) * hf->invDy);
    *y0 = *y0 < 0 ? 0 : *y0;
    *y1 = (int)((hiY - hf->y0) * hf->invDy) + 1;
    *y1 = hf->yCells < *y1 ? hf->yCells : *y1;
    return 1;
}

/* Mac 0x532ef0, Win 0x7b4c50 */
void MEAPI VincHeightFieldGetVertex(VincGeometryID g, MeVector3 out, int x, int y)
{
    const VincHeightField* hf = HF(g);
    out[0] = (MeReal)x * hf->dx + hf->x0;
    out[1] = (MeReal)y * hf->dy + hf->y0;
    out[2] = hf->heights[y * hf->xCount + x];
}

/* the cell under (x, y) (grid frame, relative to the origin) and the fractions; 0 outside.
 * NOTE: x == xSize (y == ySize) is inside: the lookup then reads one column (row) past the grid. */
static int VincHeightFieldLocate(const VincHeightField* hf, MeReal* x, MeReal* y, int* ix, int* iy)
{
    *x = *x - hf->x0;
    if (*x < 0.0 || *x > hf->xSize)
        return 0;
    *y = *y - hf->y0;
    if (*y < 0.0 || *y > hf->ySize)
        return 0;
    *x = *x * hf->invDx;
    *ix = (int)*x;
    *x = *x - (MeReal)*ix;
    *y = *y * hf->invDy;
    *iy = (int)*y;
    *y = *y - (MeReal)*iy;
    return 1;
}

/* Mac 0x532f3b, Win 0x7b4c90 (public 0x75c590) */
int MEAPI VincHeightFieldGetHeight(VincGeometryID g, MeReal x, MeReal y, MeReal* z)
{
    const VincHeightField* hf = HF(g);
    MeReal h00, h01, h10, h11;
    int ix, iy, r0, r1;
    if (!VincHeightFieldLocate(hf, &x, &y, &ix, &iy))
        return 0;
    r0 = hf->xCount * iy;
    r1 = (iy + 1) * hf->xCount;
    h00 = hf->heights[r0 + ix];
    h01 = hf->heights[r1 + ix];
    h10 = hf->heights[r0 + ix + 1];
    h11 = hf->heights[r1 + ix + 1];
    if (x + y <= 1.0)
        *z = (h10 - h00) * x + h00 + (h01 - h00) * y;
    else
        *z = h10 + h01 - h11 + (h11 - h01) * x + (h11 - h10) * y;
    return 1;
}

/* Mac 0x533096, Win 0x7b4da0 (public 0x75c5c0) */
int MEAPI VincHeightFieldGetHeightAndTriangle(VincGeometryID g, MeReal x, MeReal y, MeReal* z, MeVector3 v0,
                                              MeVector3 v1, MeVector3 v2)
{
    const VincHeightField* hf = HF(g);
    MeReal h00, h01, h10, h11;
    int ix, iy, r0, r1;
    if (!VincHeightFieldLocate(hf, &x, &y, &ix, &iy))
        return 0;
    r0 = iy * hf->xCount;
    r1 = hf->xCount * (iy + 1);
    h00 = hf->heights[ix + r0];
    h01 = hf->heights[ix + r1];
    h10 = hf->heights[r0 + ix + 1];
    h11 = hf->heights[r1 + ix + 1];
    if (x + y <= 1.0) {
        *z = (h10 - h00) * x + h00 + (h01 - h00) * y;
        VincHeightFieldGetVertex(g, v0, ix, iy);
        VincHeightFieldGetVertex(g, v1, ix + 1, iy);
        VincHeightFieldGetVertex(g, v2, ix, iy + 1);
    } else {
        *z = h10 + h01 - h11 + (h11 - h01) * x + (h11 - h10) * y;
        VincHeightFieldGetVertex(g, v0, ix + 1, iy);
        VincHeightFieldGetVertex(g, v1, ix + 1, iy + 1);
        VincHeightFieldGetVertex(g, v2, ix, iy + 1);
    }
    return 1;
}

/* ---- public accessors (Win only) ---- */

/* Win 0x75c440 */
void MEAPI VincHeightFieldGetParameters(VincGeometryID g, MeReal** heights, int* xCount, int* yCount, MeReal* dx,
                                        MeReal* dy, MeReal* x0, MeReal* y0)
{
    const VincHeightField* hf = HF(g);
    *heights = hf->heights;
    *xCount = hf->xCount;
    *yCount = hf->yCount;
    *dx = hf->dx;
    *dy = hf->dy;
    *x0 = hf->x0;
    *y0 = hf->y0;
}

/* Win 0x7b4510 */
void MEAPI VincHeightFieldGetGrid(VincGeometryID g, MeReal** heights, int* xCount, int* yCount, MeReal* dx, MeReal* dy,
                                  MeReal* x0, MeReal* y0, int* xCells, int* yCells, MeReal* xSize, MeReal* ySize)
{
    const VincHeightField* hf = HF(g);
    (void)heights;
    *xCount = hf->xCount;
    *yCount = hf->yCount;
    *dx = hf->dx;
    *dy = hf->dy;
    *x0 = hf->x0;
    *y0 = hf->y0;
    *xCells = hf->xCells;
    *yCells = hf->yCells;
    *xSize = hf->xSize;
    *ySize = hf->ySize;
}

/* Win 0x75c490 */
void MEAPI VincHeightFieldSetHeights(VincGeometryID g, MeReal* heights)
{
    HF(g)->heights = heights;
}

/* Win 0x75c580 */
MeReal* MEAPI VincHeightFieldGetHeights(VincGeometryID g)
{
    return HF(g)->heights;
}

/* Win 0x75c4b0 (-> 0x7b4450) */
void MEAPI VincHeightFieldSetInfiniteDepth(VincGeometryID g, int on)
{
    HF(g)->infiniteDepth = (char)(on != 0);
}

/* Win 0x75c4d0 (-> 0x7b4460) */
int MEAPI VincHeightFieldGetInfiniteDepth(VincGeometryID g)
{
    return HF(g)->infiniteDepth != 0;
}

/* Win 0x75c4f0 (-> 0x7b4470) */
void MEAPI VincHeightFieldSetDepth(VincGeometryID g, MeReal depth)
{
    HF(g)->depth = depth;
}

/* Win 0x75c510 (-> 0x7b4480) */
MeReal MEAPI VincHeightFieldGetDepth(VincGeometryID g)
{
    return HF(g)->depth;
}

/* Win 0x75c520 (-> 0x7b4f80) */
void MEAPI VincHeightFieldSetOrigin(VincGeometryID g, MeReal x0, MeReal y0)
{
    HF(g)->x0 = x0;
    HF(g)->y0 = y0;
}

/* Win 0x75c540 (-> 0x7b4490) */
void MEAPI VincHeightFieldGetZRangeOfCells(VincGeometryID g, int x0, int x1, int y0, int y1, MeReal* zMin, MeReal* zMax)
{
    const VincHeightField* hf = HF(g);
    int x, y;
    (void)x0;
    (void)x1;
    (void)y0;
    (void)y1;
    *zMin = 1e30;
    *zMax = -1e30;
    for (y = 0; y < hf->yCount; ++y)
        for (x = 0; x < hf->xCount; ++x) {
            if (hf->heights[hf->xCount * y + x] < *zMin)
                *zMin = hf->heights[hf->xCount * y + x];
            if (hf->heights[hf->xCount * y + x] > *zMax)
                *zMax = hf->heights[hf->xCount * y + x];
        }
}

/* Win 0x75c600 */
void MEAPI VincHeightFieldSetM68(VincGeometryID g, MeReal v)
{
    if (!(v < 0.0) && !(1.0 < v))
        HF(g)->m68 = v;
}

/* Win 0x75c630 */
void MEAPI VincHeightFieldSetZRange(VincGeometryID g, MeReal zMin, MeReal zMax)
{
    HF(g)->zMin = zMin;
    HF(g)->zMax = zMax;
}

/* Win 0x75c650 */
void MEAPI VincHeightFieldGetZRange(VincGeometryID g, MeReal* zMin, MeReal* zMax)
{
    *zMin = HF(g)->zMin;
    *zMax = HF(g)->zMax;
}

/* Win 0x7b5060 */
void MEAPI VincHeightFieldGetBSphere4(VincGeometryID g, int unused, MeReal* sphere4)
{
    (void)unused;
    VincHeightFieldGetBSphere(g, sphere4, sphere4 + 3);
}

/* ======================================================================================== */
/* pair tests                                                                               */
/* ======================================================================================== */

/* Mac 0x5338df / 0x534fc7 / 0x536853 / 0x539f18 (regparm EAX; one copy per test), Win 0x7724a0 */
static void ResultClear(VincIntersectResult* result)
{
    result->contactGroupCount = 0;
    result->contactCount = 0;
}

static void VincHeightFieldTriangleInit(VincTriangle* tri, MeReal* a, MeReal* b, MeReal* c)
{
    tri->index = -1;
    tri->edgeData0[0] = -2.0;
    tri->edgeData0[1] = -2.0;
    tri->edgeData0[2] = -2.0;
    tri->edgeData1[0] = 0.0;
    tri->edgeData1[1] = 0.0;
    tri->edgeData1[2] = 0.0;
    tri->vertices[0] = a;
    tri->vertices[1] = b;
    tri->vertices[2] = c;
}

static void VincTriangleUpdate(VincTriangle* tri)
{
    VincTriangleUpdateEdges(tri);
    VincTriangleUpdateNormal(tri);
}

static void NormalizeResultNormal(VincIntersectResult* result)
{
    MeReal x = result->normal[0];
    MeReal len = sqrt(x * x + result->normal[1] * result->normal[1] + result->normal[2] * result->normal[2]);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        result->normal[0] = x * inv;
        result->normal[1] = result->normal[1] * inv;
        result->normal[2] = inv * result->normal[2];
    }
}

/* the cell index of a triangle: 2 per cell, row-major over the cells */
static int CellTriangleIndex(const VincHeightField* hf, int x, int y)
{
    return (x + (hf->xCount - 1) * y) * 2;
}

/* the wrapper of the Box / Capsule / Sphere tests (Mac 0x534ed3 / 0x53675f / 0x53b22e): the core
 * writes raw contacts into a stack buffer of *maxContacts (swapped into the result), then
 * VincContactSimplify reduces them into the caller's array */
typedef int (*VincHeightFieldCoreFn)(VincIntersectResult* result, VincModelID m1, VincModelID m2, const MeReal* pad,
                                     void* pairData);
static int VincHeightFieldIntersectBuffered(VincModelPair* pair, VincIntersectResult* result, const int* maxContacts,
                                            VincHeightFieldCoreFn core)
{
    /* the original allocates the buffer on the stack (alloca / MSVC _alloca_probe_16) */
    int n = *maxContacts;
    VincContact* buffer = (VincContact*)malloc((size_t)(n + 16) * sizeof(VincContact)); /* (slack: see VincTriangleCapsuleContacts) */
    int saveMax = result->contactMaxCount;
    VincContact* saveContacts = result->contacts;
    MeReal pad;
    int touch;
    result->contactMaxCount = n;
    result->contacts = buffer;
    pad = VincModelGetPadding(pair->model1);
    pad = pad + VincModelGetPadding(pair->model2);
    touch = core(result, pair->model1, pair->model2, &pad, pair->pairData);
    result->contacts = saveContacts;
    result->contactMaxCount = saveMax;
    if (touch)
        result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    free(buffer);
    return touch != 0;
}

/* ---- Box ---- */

/* Mac 0x533967, Win 0x772e10: the box (half extents radii, origin of the triangle's frame) against
 * one triangle; contacts within the padding are appended in world space (tm = the box TM), all of
 * them with the triangle's normal; returns 1 when contacts were added */
static int VincBoxHeightFieldTriangle(VincTriangle* tri, MeReal* radii, const MeReal* pad, const MeReal* tm,
                                      VincIntersectResult* result)
{
    MeReal points[17 * 3];
    MeReal seps[18];
    MeReal* pointCursor = points;
    MeReal* sepCursor = seps;
    MeReal out1[3], normal[3], out2[2];
    MeVector3 n;
    MeI16 dims;
    int start;
    if (!VincBoxTriangleContact(out1, normal, out2, &pointCursor, &sepCursor, &dims, pad, radii, tri))
        return 0;
    VincTMRotate(tm, tri->normal, n);
    start = result->contactCount;
    sepCursor = seps;
    if (points != pointCursor && start < result->contactMaxCount) {
        VincContact* c = &result->contacts[start];
        MeReal* p = points;
        for (;;) {
            VincTMTransform(tm, p, c->position);
            c->dims = dims;
            c->separation = *sepCursor++;
            c->normal[0] = n[0];
            c->normal[1] = n[1];
            c->normal[2] = n[2];
            if (c->separation < *pad) {
                ++c;
                ++result->contactCount;
            }
            p += 3;
            if (pointCursor == p || result->contactCount >= result->contactMaxCount)
                break;
        }
    }
    result->normal[0] = result->normal[0] + n[0];
    result->normal[1] = result->normal[1] + n[1];
    result->normal[2] = result->normal[2] + n[2];
    return result->contactCount != start;
}

/* Mac 0x533bac, Win 0x772fc0 */
static int VincBoxHeightFieldCore(VincIntersectResult* result, VincModelID m1, VincModelID m2, const MeReal* pad,
                                  void* pairData)
{
    MeReal* tm1 = VincModelGetTransformPtr(m1);
    MeReal* tm2 = VincModelGetTransformPtr(m2);
    VincGeometryID box = VincModelGetGeometry(m1);
    VincHeightField* hf = HF(VincModelGetGeometry(m2));
    MeReal rel1[16], rel2[16], xy[4], dir[3], deep[3], p[3], v[3], radii[3];
    MeReal a[3], b[3], c[3];
    MeReal* r = VincBoxGetRadii(box);
    VincTriangle tri;
    VincRequest* rq;
    int maxC, x, y, x0, x1, y0, y1;
    MeReal h;
    (void)pairData;
    radii[0] = r[0];
    radii[1] = r[1];
    radii[2] = r[2];
    VincTMMakeRelative(tm1, tm2, rel1);
    VincBoxGetXYAABB(box, (MeReal(*)[4])rel1, xy);
    VincTMMakeRelative(tm2, tm1, rel2);
    ResultClear(result);
    if (!VincHeightFieldGetCellRange((VincGeometryID)hf, xy, &x0, &x1, &y0, &y1)) {
        ResultClear(result);
        return 0;
    }
    rq = VincModelPairGetRequestPtr(result->pair);
    maxC = rq->contactMaxCount;
    if (result->contactMaxCount <= maxC)
        maxC = result->contactMaxCount;

    /* the box's deepest point straight down onto the surface */
    dir[0] = (rel2[0] * 0.0 + rel2[4] * 0.0) - rel2[8];
    dir[1] = (rel2[1] * 0.0 + rel2[5] * 0.0) - rel2[9];
    dir[2] = (rel2[2] * 0.0 + rel2[6] * 0.0) - rel2[10];
    VincBoxMaximumPointLocal(box, dir, deep);
    p[0] = deep[0] * rel1[0] + deep[1] * rel1[4] + deep[2] * rel1[8] + rel1[12];
    p[1] = deep[0] * rel1[1] + deep[1] * rel1[5] + deep[2] * rel1[9] + rel1[13];
    if (VincHeightFieldGetHeight((VincGeometryID)hf, p[0], p[1], &h)) {
        MeReal sep = deep[0] * rel1[2] + deep[1] * rel1[6] + deep[2] * rel1[10] + rel1[14] - h;
        if (sep < 0.0 && (hf->infiniteDepth || -hf->depth < sep)) {
            VincContact* k;
            if (maxC == 0)
                return 1;
            k = result->contacts;
            p[2] = h;
            VincTMTransform(tm2, p, k->position);
            k->normal[0] = tm2[8];
            k->normal[1] = tm2[9];
            k->normal[2] = tm2[10];
            k->separation = sep;
            k->dims = 0x201;
            result->contactCount = 1;
            result->normal[0] = tm2[8];
            result->normal[1] = tm2[9];
            result->normal[2] = tm2[10];
        }
    }

    /* the cells' triangles, in the box frame */
    VincHeightFieldTriangleInit(&tri, a, b, c);
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1 && result->contactCount < result->contactMaxCount; ++x) {
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y);
            VincTMTransform(rel2, v, a);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y);
            VincTMTransform(rel2, v, b);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y + 1);
            VincTMTransform(rel2, v, c);
            VincTriangleUpdate(&tri);
            if (VincBoxHeightFieldTriangle(&tri, radii, pad, tm1, result)) {
                if (maxC == 0) {
                    ResultClear(result);
                    return 1;
                }
                result->contacts[result->contactCount - 1].element2 = CellTriangleIndex(hf, x, y);
            }
            if (result->contactMaxCount <= result->contactCount)
                break;
            a[0] = b[0];
            a[1] = b[1];
            a[2] = b[2];
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y + 1);
            VincTMTransform(rel2, v, b);
            VincTriangleUpdate(&tri);
            if (VincBoxHeightFieldTriangle(&tri, radii, pad, tm1, result)) {
                if (maxC == 0) {
                    ResultClear(result);
                    return 1;
                }
                result->contacts[result->contactCount - 1].element2 = CellTriangleIndex(hf, x, y) + 1;
            }
        }
    }
    if (result->contactCount == result->contactMaxCount) {
        VincBoxHeightFieldMaxContacts = VincBoxHeightFieldMaxContacts * 2;
        MeWarning(1, "Contact buffer full in BoxHeightField. Buffer size was increased to %d\n",
                  VincBoxHeightFieldMaxContacts);
    }
    if (result->contactCount == 0)
        return 0;
    NormalizeResultNormal(result);
    return 1;
}

/* Mac 0x534ed3, Win 0x7737c0 */
int MEAPI VincBoxHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    return VincHeightFieldIntersectBuffered(pair, result, &VincBoxHeightFieldMaxContacts, VincBoxHeightFieldCore);
}

/* the interaction {0, 0, intersect, 0...} of (type, height field) */
static void RegisterWithHeightField(int type, VincIntersectFn fn)
{
    VincInteractions i;
    i.helloFn = 0;
    i.goodbyeFn = 0;
    i.intersectFn = fn;
    i.sweptIntersectFn = 0;
    i.safeTimeFn = 0;
    i.distanceFn = 0;
    i.distanceBoundedFn = 0;
    VincGeometryPairRegisterInteractions(type, VincHeightFieldGetTypeId(), &i, 0);
}

/* Mac 0x5338f2, Win 0x773880 */
int MEAPI VincBoxHeightFieldRegisterInteraction(void)
{
    RegisterWithHeightField(VincBoxGetTypeId(), VincBoxHeightFieldIntersect);
    return 1;
}

/* ---- Capsule ---- */

/* Mac 0x53504f, Win 0x770910 */
static int VincCapsuleHeightFieldCore(VincIntersectResult* result, VincModelID m1, VincModelID m2, const MeReal* pad,
                                      void* pairData)
{
    MeReal* tm1 = VincModelGetTransformPtr(m1);
    MeReal* tm2 = VincModelGetTransformPtr(m2);
    VincGeometryID cap = VincModelGetGeometry(m1);
    VincHeightField* hf = HF(VincModelGetGeometry(m2));
    MeReal radius = VincCapsuleGetRadius(cap);
    MeReal hh = VincCapsuleGetHalfHeight(cap);
    MeReal rel1[16], rel2[16], xy[4], ends[6], v[3], a[3], b[3], c[3], local[3], n[3];
    MeReal minSep, h, sep, bound;
    VincTriangle tri;
    VincRequest* rq;
    VincContact* out;
    int k, x, y, x0, x1, y0, y1, nEnds;
    (void)pairData;
    VincTMMakeRelative(tm1, tm2, rel1);
    VincCapsuleGetXYAABB(cap, (MeReal(*)[4])rel1, xy);
    VincTMMakeRelative(tm2, tm1, rel2);
    ResultClear(result);
    result->normal[2] = 0.0;
    result->normal[1] = 0.0;
    result->normal[0] = 0.0;
    if (!VincHeightFieldGetCellRange((VincGeometryID)hf, xy, &x0, &x1, &y0, &y1)) {
        ResultClear(result);
        return 0;
    }
    VincHeightFieldTriangleInit(&tri, a, b, c);
    rq = VincModelPairGetRequestPtr(result->pair);

    /* the axis' end points (lowest points of the end spheres) straight onto the surface */
    VincCapsuleGetEndpoints(cap, (MeReal(*)[4])rel1, ends, ends + 3);
    ends[0] = hh * rel1[8] + rel1[12];
    ends[1] = hh * rel1[9] + rel1[13];
    ends[3] = rel1[8] * -hh + rel1[12];
    ends[4] = rel1[9] * -hh + rel1[13];
    ends[2] = hh * rel1[10] + rel1[14] - radius;
    ends[5] = rel1[10] * -hh + rel1[14] - radius;
    out = result->contacts;
    nEnds = 0;
    minSep = 0.0;
    for (k = 0; k < 2; ++k) {
        MeReal* e = &ends[k * 3];
        if (VincHeightFieldGetHeightAndTriangle((VincGeometryID)hf, e[0], e[1], &h, a, b, c) && (sep = e[2] - h) < 0.0 &&
            (hf->infiniteDepth || -hf->depth < sep)) {
            if (result->contactMaxCount == 0)
                return 1;
            if (rq->contactMaxCount == 0) {
                ResultClear(result);
                return 1;
            }
            out->separation = sep;
            if (minSep <= sep)
                sep = minSep;
            VincTMTransform(tm2, e, out->position);
            VincTriangleUpdate(&tri);
            VincTMRotate(tm2, tri.normal, out->normal);
            out->dims = 0x201;
            ++result->contactCount;
            out->element2 = -1;
            ++out;
            ++nEnds;
            minSep = sep;
        }
    }

    /* the cells' triangles, in world space */
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1 && result->contactCount < result->contactMaxCount; ++x) {
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y);
            VincTMTransform(tm2, v, a);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y);
            VincTMTransform(tm2, v, b);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y + 1);
            VincTMTransform(tm2, v, c);
            VincTriangleUpdate(&tri);
            VincTMInverseTransform(tm1, b, local);
            n[0] = tri.normal[0];
            n[1] = tri.normal[1];
            n[2] = tri.normal[2];
            bound = fabs(hh * (n[0] * tm1[8] + n[1] * tm1[9] + n[2] * tm1[10])) +
                    fabs((n[0] * tm1[0] + n[1] * tm1[1] + n[2] * tm1[2]) * radius) +
                    fabs(radius * (n[0] * tm1[4] + n[1] * tm1[5] + n[2] * tm1[6]));
            if (fabs(local[0] * n[0] + local[1] * n[1] + local[2] * n[2]) < bound) {
                tri.index = CellTriangleIndex(hf, x, y);
                VincTriangleCapsuleContacts(result, &tri, radius, hh, tm1, *pad);
                if (result->contactMaxCount <= result->contactCount)
                    break;
            }
            a[0] = b[0];
            a[1] = b[1];
            a[2] = b[2];
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y + 1);
            VincTMTransform(tm2, v, b);
            VincTriangleUpdate(&tri);
            n[0] = tri.normal[0];
            n[1] = tri.normal[1];
            n[2] = tri.normal[2];
            bound = fabs(hh * (n[0] * tm1[8] + n[1] * tm1[9] + n[2] * tm1[10])) +
                    fabs((n[0] * tm1[0] + n[1] * tm1[1] + n[2] * tm1[2]) * radius) +
                    fabs(radius * (n[0] * tm1[4] + n[1] * tm1[5] + n[2] * tm1[6]));
            /* NOTE: the plane distance of the old b (now a vertex of this triangle too) */
            if (fabs(local[0] * n[0] + local[1] * n[1] + local[2] * n[2]) < bound) {
                tri.index = CellTriangleIndex(hf, x, y) + 1;
                VincTriangleCapsuleContacts(result, &tri, radius, hh, tm1, *pad);
            }
        }
    }
    if (result->contactMaxCount <= result->contactCount) {
        VincCapsuleHeightFieldMaxContacts = VincCapsuleHeightFieldMaxContacts * 2;
        MeWarning(1, "Contact buffer full in CapsuleHeightField. Buffer size was increased to %d\n",
                  VincCapsuleHeightFieldMaxContacts);
    }
    if (result->contactCount == 0)
        return 0;
    /* no contact deeper than the deepest end point */
    if (nEnds != 0)
        for (k = 0; k < result->contactCount; ++k)
            if (result->contacts[k].separation < minSep)
                result->contacts[k].separation = minSep;
    NormalizeResultNormal(result);
    return 1;
}

/* Mac 0x53675f, Win 0x7712c0 */
int MEAPI VincCapsuleHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    return VincHeightFieldIntersectBuffered(pair, result, &VincCapsuleHeightFieldMaxContacts,
                                            VincCapsuleHeightFieldCore);
}

/* Mac 0x534fda, Win 0x771380 */
int MEAPI VincCapsuleHeightFieldRegisterInteraction(void)
{
    RegisterWithHeightField(VincCapsuleGetTypeId(), VincCapsuleHeightFieldIntersect);
    return 1;
}

/* ---- Cylinder ---- */

/* append the generator's points (cylinder frame) as contacts in world space (tm) */
static void VincCylinderHeightFieldAddContacts(VincIntersectResult* result, VincContact** out, const MeReal* tm,
                                               const MeReal* points, const MeReal* pointEnd, const MeReal* seps,
                                               const MeReal* normal, MeI16 dims, int index)
{
    MeVector3 n;
    VincTMRotate(tm, normal, n);
    if (points != pointEnd && result->contactCount < VincCylinderHeightFieldMaxContacts) {
        const MeReal* p = points;
        for (;;) {
            VincContact* c = *out;
            VincTMTransform(tm, p, c->position);
            c->dims = dims;
            c->separation = *seps++;
            c->element2 = index;
            c->normal[0] = n[0];
            c->normal[1] = n[1];
            c->normal[2] = n[2];
            ++*out;
            ++result->contactCount;
            p += 3;
            if (pointEnd == p || result->contactCount >= VincCylinderHeightFieldMaxContacts)
                break;
        }
    }
    result->normal[0] = result->normal[0] + n[0];
    result->normal[1] = result->normal[1] + n[1];
    result->normal[2] = result->normal[2] + n[2];
}

/* Mac 0x5368db, Win 0x7714e0 (registered directly: no wrapper) */
int MEAPI VincCylinderHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    VincGeometryID cyl = VincModelGetGeometry(pair->model1);
    VincHeightField* hf = HF(VincModelGetGeometry(pair->model2));
    MeReal pad, radius, hh, cosTol, tol, h, sep;
    MeReal *tm1, *tm2;
    MeReal rel1[16], rel2[16], xy[4], ends[6], off[3], v[3], a[3], b[3], c[3];
    MeReal points[17 * 3], seps[18], out1[2], out2[2], normal[3];
    MeReal *pointCursor, *sepCursor;
    VincContact endContacts[2];
    VincContact *buffer, *out;
    VincTriangle tri;
    VincRequest* rq;
    MeI16 dims;
    int k, x, y, x0, x1, y0, y1, nEnds, flag, maxC;

    pad = VincModelGetPadding(pair->model1);
    pad = pad + VincModelGetPadding(pair->model2);
    tm1 = VincModelGetTransformPtr(pair->model1);
    tm2 = VincModelGetTransformPtr(pair->model2);
    radius = VincCylinderGetRadius(cyl);
    hh = VincCylinderGetHalfHeight(cyl);
    ResultClear(result);
    VincTMMakeRelative(tm2, tm1, rel1);
    VincTMMakeRelative(tm1, tm2, rel2);
    VincCylinderGetXYAABB(cyl, (MeReal(*)[4])rel2, xy);
    if (!VincHeightFieldGetCellRange((VincGeometryID)hf, xy, &x0, &x1, &y0, &y1))
        return 0;
    /* the original allocates the buffer on the stack (alloca / MSVC _alloca_probe_16) */
    buffer = (VincContact*)malloc((size_t)VincCylinderHeightFieldMaxContacts * sizeof(VincContact));
    result->normal[0] = 0.0;
    result->normal[1] = 0.0;
    result->normal[2] = 0.0;
    rq = VincModelPairGetRequestPtr(pair);
    VincHeightFieldTriangleInit(&tri, a, b, c);

    /* the lowest rim points of the two caps straight onto the surface */
    VincCylinderGetEndpoints(cyl, (MeReal(*)[4])rel2, ends, ends + 3);
    if (fabs(rel2[10]) < 0.99) {
        MeReal ax = rel2[8], ay = rel2[9], az = rel2[10];
        MeReal ux = ay, uy = -ax, uz = 0.0;
        MeReal wx = uy * az - uz * ay, wy = uz * ax - ux * az, wz = ux * ay - uy * ax;
        MeReal s = radius / sqrt(VincSumSq3Fn(wx, wy, wz));
        s = wz < 0.0 ? s : -s;
        off[0] = wx * s;
        off[1] = wy * s;
        off[2] = s * wz;
    } else {
        off[0] = 0.0;
        off[1] = 0.0;
        off[2] = 0.0;
    }
    nEnds = 0;
    for (k = 0; k < 2; ++k) {
        MeReal* e = &ends[k * 3];
        e[0] = off[0] + e[0];
        e[1] = off[1] + e[1];
        e[2] = off[2] + e[2];
        if (VincHeightFieldGetHeightAndTriangle((VincGeometryID)hf, e[0], e[1], &h, a, b, c) && (sep = e[2] - h) < 0.0 &&
            (hf->infiniteDepth || -hf->depth < sep)) {
            VincContact* ec = &endContacts[nEnds];
            if (result->contactMaxCount == 0) {
                free(buffer);
                return 1;
            }
            if (rq->contactMaxCount == 0) {
                ResultClear(result);
                free(buffer);
                return 1;
            }
            ec->separation = sep;
            VincTMTransform(tm2, e, ec->position);
            VincTriangleUpdate(&tri);
            VincTMRotate(tm2, tri.normal, ec->normal);
            ec->dims = 0x201;
            ec->element2 = -1;
            ++nEnds;
        }
    }

    /* the cells' triangles, in the cylinder frame */
    out = buffer + result->contactCount;
    flag = ((VincCylinder*)cyl)->flag;
    cosTol = VincCylinderGetCosTolerance(cyl);
    tol = pad / 1000.0;
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y);
            VincTMTransform(rel1, v, a);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y);
            VincTMTransform(rel1, v, b);
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x, y + 1);
            VincTMTransform(rel1, v, c);
            VincTriangleUpdate(&tri);
            if (fabs(tri.normal[0] * a[0] + tri.normal[1] * a[1] + tri.normal[2] * a[2]) <
                fabs(tri.normal[2] * hh) + fabs(radius * tri.normal[0]) + fabs(radius * tri.normal[1])) {
                /* edge data: -2 / 1 on the grid's border edges (bottom row, left column) */
                tri.edgeData0[0] = y == 0 ? -2.0 : 1.0;
                tri.edgeData0[1] = 1.0;
                tri.edgeData0[2] = x + 1 == 1 ? -2.0 : 1.0;
                tri.edgeData1[0] = y == 0 ? 1.0 : 0.0;
                tri.edgeData1[1] = 0.0;
                tri.edgeData1[2] = x + 1 == 1 ? 1.0 : 0.0;
                tri.index = CellTriangleIndex(hf, x, y);
                pointCursor = points;
                sepCursor = seps;
                if (VincCylinderTriangleContacts(out1, normal, out2, &pointCursor, &dims, &tol, &radius, &hh, &tri,
                                                 cosTol, flag, &sepCursor))
                    VincCylinderHeightFieldAddContacts(result, &out, tm1, points, pointCursor, seps, normal, dims,
                                                       tri.index);
            }
            a[0] = b[0];
            a[1] = b[1];
            a[2] = b[2];
            VincHeightFieldGetVertex((VincGeometryID)hf, v, x + 1, y + 1);
            VincTMTransform(rel1, v, b);
            VincTriangleUpdate(&tri);
            if (fabs(tri.normal[0] * a[0] + tri.normal[1] * a[1] + tri.normal[2] * a[2]) <
                fabs(tri.normal[2] * hh) + fabs(radius * tri.normal[0]) + fabs(radius * tri.normal[1])) {
                /* NOTE: the border tests of the upper triangle compare with the last vertex row /
                 * column (never true for a cell): its edges always count as interior */
                tri.edgeData0[2] = 1.0;
                tri.edgeData1[0] = 0.0;
                tri.edgeData1[2] = 0.0;
                tri.edgeData0[0] = hf->yCount - 1 == y ? -2.0 : 1.0;
                tri.edgeData0[1] = hf->xCount - 1 == x ? -2.0 : 1.0;
                tri.edgeData1[1] = 0.0;
                tri.index = CellTriangleIndex(hf, x, y) + 1;
                pointCursor = points;
                sepCursor = seps;
                if (VincCylinderTriangleContacts(out1, normal, out2, &pointCursor, &dims, &tol, &radius, &hh, &tri,
                                                 cosTol, flag, &sepCursor))
                    VincCylinderHeightFieldAddContacts(result, &out, tm1, points, pointCursor, seps, normal, dims,
                                                       tri.index);
            }
        }
    }
    if (nEnds >= 1 && result->contactCount == 0) {
        /* only end-point contacts: they are used (last one first) */
        result->normal[0] = endContacts[0].normal[0];
        result->normal[1] = endContacts[0].normal[1];
        result->normal[2] = endContacts[0].normal[2];
        for (k = nEnds - 1; k >= 0; --k) {
            memcpy(out, &endContacts[k], sizeof *out);
            ++out;
            ++result->contactCount;
        }
    }
    if (result->contactCount == 0) {
        free(buffer);
        return 0;
    }
    /* NOTE: compares with the caller's capacity, not the buffer's */
    if (result->contactCount == result->contactMaxCount) {
        VincCylinderHeightFieldMaxContacts = VincCylinderHeightFieldMaxContacts * 2;
        MeWarning(1, "Contact buffer full in CylinderHeightField. Buffer size was increased to %d\n",
                  VincCylinderHeightFieldMaxContacts);
    }
    maxC = rq->contactMaxCount;
    if (result->contactMaxCount <= maxC)
        maxC = result->contactMaxCount;
    if (maxC == 0) {
        ResultClear(result);
        free(buffer);
        return 1;
    }
    result->contactCount = VincContactSimplify(result->normal, buffer, result->contactCount, result);
    NormalizeResultNormal(result);
    free(buffer);
    return result->contactCount > 0;
}

/* Mac 0x536866, Win 0x7722f0 */
int MEAPI VincCylinderHeightFieldRegisterInteraction(void)
{
    RegisterWithHeightField(VincCylinderGetTypeId(), VincCylinderHeightFieldIntersect);
    return 1;
}

/* ---- Sphere ---- */

/* Win 0x772d80 (Mac: inlined): c lies inside the triangle's prism */
static int VincTriangleContainsPoint(const VincTriangle* tri, const MeReal* c)
{
    int i;
    for (i = 0; i < 3; ++i) {
        const MeReal* e = tri->edges[i];
        const MeReal* v = tri->vertices[i];
        MeReal mx = tri->normal[1] * e[2] - tri->normal[2] * e[1];
        MeReal my = tri->normal[2] * e[0] - tri->normal[0] * e[2];
        MeReal mz = tri->normal[0] * e[1] - tri->normal[1] * e[0];
        if (c[0] * mx + c[1] * my + c[2] * mz < mx * v[0] + my * v[1] + mz * v[2])
            return 0;
    }
    return 1;
}

/* Win 0x772340 (Mac: inlined): the point of edge i nearest to c (out), returns its distance */
static MeReal VincTriangleEdgeClosest(const MeReal* c, const VincTriangle* tri, const int* i, MeReal* out)
{
    const MeReal* e = tri->edges[*i];
    const MeReal* q = tri->vertices[(*i + 1) % 3];
    MeReal wx = c[0] - q[0], wy = c[1] - q[1], wz = c[2] - q[2];
    MeReal s;
    if (e[0] * wx + wy * e[1] + e[2] * wz < 0.0) {
        q = tri->vertices[*i];
        wx = c[0] - q[0];
        wy = c[1] - q[1];
        wz = c[2] - q[2];
        s = e[0] * wx + e[1] * wy + e[2] * wz;
        if (s > 0.0) {
            MeReal dx, dy, dz;
            s = s / (e[0] * e[0] + e[1] * e[1] + e[2] * e[2]);
            out[0] = e[0] * s + q[0];
            out[1] = e[1] * s + q[1];
            out[2] = e[2] * s + q[2];
            dx = out[0] - c[0];
            dy = out[1] - c[1];
            dz = out[2] - c[2];
            return sqrt(dx * dx + dy * dy + dz * dz);
        }
    }
    out[0] = q[0];
    out[1] = q[1];
    out[2] = q[2];
    return sqrt(wx * wx + wy * wy + wz * wz);
}

/* Mac 0x539fa0, Win 0x7724b0: the sphere (c, *radius; triangle frame) against one triangle: a face
 * contact (dims 0x203) when the centre projects into it, else the nearest edge point (0x103);
 * the contact is written to *out in tm's frame (tm == NULL: unchanged) */
static int VincSphereHeightFieldTriangle(const MeReal* c, const MeReal* radius, const VincTriangle* tri,
                                         const MeReal* tm, VincContact* out)
{
    const MeReal* n = tri->normal;
    const MeReal* v0 = tri->vertices[0];
    MeReal d = n[0] * c[0] + c[1] * n[1] + c[2] * n[2] - (n[0] * v0[0] + n[1] * v0[1] + n[2] * v0[2]);
    MeReal depth;
    MeVector3 p;
    MeI16 dims;
    if (!(d <= *radius))
        return 0;
    if (VincTriangleContainsPoint(tri, c) && (depth = *radius - d) > 0.0) {
        p[2] = c[2] - d * n[2];
        p[1] = c[1] - d * n[1];
        p[0] = c[0] - d * n[0];
        dims = 0x203;
    } else {
        MeReal best = DBL_MAX, dist;
        MeVector3 q;
        int i;
        p[0] = p[1] = p[2] = 0.0; /* (only NaN distances leave it: the original's is stale stack) */
        for (i = 0; i < 3; ++i) {
            dist = VincTriangleEdgeClosest(c, tri, &i, q);
            if (dist < best) {
                best = dist;
                p[0] = q[0];
                p[1] = q[1];
                p[2] = q[2];
            }
        }
        depth = *radius - best;
        if (!(depth > 0.0))
            return 0;
        dims = 0x103;
    }
    if (tm == 0) {
        out->position[0] = p[0];
        out->position[1] = p[1];
        out->position[2] = p[2];
        out->normal[0] = n[0];
        out->normal[1] = n[1];
        out->normal[2] = n[2];
    } else {
        VincTMTransform(tm, p, out->position);
        VincTMRotate(tm, n, out->normal);
    }
    out->dims = dims;
    out->separation = -depth;
    return 1;
}

/* Mac 0x53a587, Win 0x772680 */
static int VincSphereHeightFieldCore(VincIntersectResult* result, VincModelID m1, VincModelID m2, const MeReal* pad,
                                     void* pairData)
{
    MeReal* tm1 = VincModelGetTransformPtr(m1);
    MeReal* tm2 = VincModelGetTransformPtr(m2);
    VincGeometryID sph = VincModelGetGeometry(m1);
    VincHeightField* hf = HF(VincModelGetGeometry(m2));
    MeReal radius = VincSphereGetRadius(sph);
    MeReal twoR = radius + radius;
    MeReal rel[16], c[3], a[3], b[3], v2[3], h;
    VincTriangle tri;
    VincRequest* rq;
    VincContact* out;
    int maxC, resMax, x, y, x0, x1, y0, y1;
    (void)pad;
    (void)pairData;
    VincTMMakeRelative(tm1, tm2, rel);
    c[0] = rel[12];
    c[1] = rel[13];
    c[2] = rel[14];
    ResultClear(result);
    result->normal[2] = 0.0;
    result->normal[1] = 0.0;
    result->normal[0] = 0.0;
    rq = VincModelPairGetRequestPtr(result->pair);
    maxC = rq->contactMaxCount;
    resMax = result->contactMaxCount;
    if (!VincHeightFieldGetSphereCellRange((VincGeometryID)hf, c, &radius, &x0, &x1, &y0, &y1))
        return 0;
    VincHeightFieldTriangleInit(&tri, a, b, v2);
    if (resMax <= maxC)
        maxC = resMax;
    for (y = y0; y < y1; ++y) {
        for (x = x0; x < x1; ++x) {
            if (result->contactMaxCount <= result->contactCount) {
                y = y1;
                break;
            }
            VincHeightFieldGetVertex((VincGeometryID)hf, a, x, y);
            VincHeightFieldGetVertex((VincGeometryID)hf, b, x + 1, y);
            VincHeightFieldGetVertex((VincGeometryID)hf, v2, x, y + 1);
            VincTriangleUpdate(&tri);
            out = &result->contacts[result->contactCount];
            if (VincSphereHeightFieldTriangle(c, &radius, &tri, tm2, out)) {
                if (maxC == 0) {
                    ResultClear(result);
                    return 1;
                }
                if (hf->infiniteDepth || -hf->depth < twoR + out->separation) {
                    result->normal[0] = result->normal[0] + out->normal[0];
                    result->normal[1] = result->normal[1] + out->normal[1];
                    result->normal[2] = result->normal[2] + out->normal[2];
                    ++result->contactCount;
                    out->element2 = CellTriangleIndex(hf, x, y);
                    ++out;
                }
            }
            if (result->contactMaxCount <= result->contactCount)
                break; /* NOTE: on to the next row, which stops at once */
            a[0] = b[0];
            a[1] = b[1];
            a[2] = b[2];
            VincHeightFieldGetVertex((VincGeometryID)hf, b, x + 1, y + 1);
            VincTriangleUpdate(&tri);
            if (VincSphereHeightFieldTriangle(c, &radius, &tri, tm2, out)) {
                if (maxC == 0) {
                    ResultClear(result);
                    return 1;
                }
                if (hf->infiniteDepth || -hf->depth < twoR + out->separation) {
                    out->element2 = CellTriangleIndex(hf, x, y) + 1;
                    ++result->contactCount;
                    result->normal[0] = result->normal[0] + out->normal[0];
                    result->normal[1] = result->normal[1] + out->normal[1];
                    result->normal[2] = result->normal[2] + out->normal[2];
                }
            }
        }
    }
    if (result->contactCount == result->contactMaxCount) {
        VincSphereHeightFieldMaxContacts = VincSphereHeightFieldMaxContacts * 2;
        MeWarning(1, "Contact buffer full in SphereHeightField. Buffer size was increased to %d\n",
                  VincSphereHeightFieldMaxContacts);
    }
    if (result->contactCount == 0) {
        /* no triangle contact: the point straight under the centre */
        if (VincHeightFieldGetHeight((VincGeometryID)hf, c[0], c[1], &h)) {
            MeReal sep = c[2] - radius - h;
            if (sep < 0.0 && (hf->infiniteDepth || -hf->depth < sep)) {
                VincContact* k;
                MeVector3 p;
                if (maxC == 0)
                    return 1;
                k = result->contacts;
                p[0] = c[0];
                p[1] = c[1];
                p[2] = h;
                VincTMTransform(tm2, p, k->position);
                k->normal[0] = tm2[8];
                k->normal[1] = tm2[9];
                k->normal[2] = tm2[10];
                k->separation = sep;
                k->dims = 0x201;
                result->contactCount = 1;
                result->normal[0] = tm2[8];
                result->normal[1] = tm2[9];
                result->normal[2] = tm2[10];
                return 1;
            }
        }
        if (result->contactCount == 0)
            return 0;
    }
    NormalizeResultNormal(result);
    return 1;
}

/* Mac 0x53b22e, Win 0x772c80 */
int MEAPI VincSphereHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result)
{
    return VincHeightFieldIntersectBuffered(pair, result, &VincSphereHeightFieldMaxContacts,
                                            VincSphereHeightFieldCore);
}

/* Mac 0x539f2b, Win 0x772d30 */
int MEAPI VincSphereHeightFieldRegisterInteraction(void)
{
    RegisterWithHeightField(VincSphereGetTypeId(), VincSphereHeightFieldIntersect);
    return 1;
}

/* ======================================================================================== */
/* line segment                                                                             */
/* ======================================================================================== */

/* Mac 0x538693, Win 0x7b50e0: segment p1-p2 against segment q1-q2 (xy); *t = parameter on q */
static int VincSegmentsCross2D(const MeReal* p1x, const MeReal* p1y, const MeReal* p2x, const MeReal* p2y,
                               const MeReal* q1x, const MeReal* q1y, const MeReal* q2x, const MeReal* q2y, MeReal* t)
{
    MeReal dqy = *q2y - *q1y, dqx = *q2x - *q1x;
    MeReal den = (*p2x - *p1x) * dqy - (*p2y - *p1y) * dqx;
    MeReal u;
    if (!(1e-14 <= fabs(den)))
        return 0;
    *t = ((*p1y - *q1y) * dqx - (*p1x - *q1x) * dqy) / den;
    if (!(0.0 <= *t && *t <= 1.0))
        return 0;
    u = ((*p1y - *q1y) * (*p2x - *p1x) - (*p1x - *q1x) * (*p2y - *p1y)) / den;
    if (!(0.0 <= u))
        return 0;
    return u <= 1.0;
}

static int VincPointInRect(const MeReal* x0, const MeReal* x1, const MeReal* y0, const MeReal* y1, const MeReal* p)
{
    if (p[0] < *x0 || *x1 < p[0] || p[1] < *y0)
        return 0;
    return p[1] <= *y1;
}

/* Mac 0x5387d0, Win 0x7b51c0: does the segment a-b (xy) touch the rectangle; *t = 0 when a is
 * inside, else the first crossing of its border (1 if none) */
static int VincSegmentClipRect(const MeReal* x0, const MeReal* x1, const MeReal* y0, const MeReal* y1, const MeReal* a,
                               const MeReal* b, MeReal* t)
{
    MeReal s;
    int inA, inB;
    *t = 0.0;
    inA = VincPointInRect(x0, x1, y0, y1, a);
    inB = VincPointInRect(x0, x1, y0, y1, b);
    if (inA) {
        if (!inB) {
            if (!VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y0, x1, y0, &s) &&
                !VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y0, x1, y1, &s) &&
                !VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y1, x0, y1, &s))
                return VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y1, x0, y0, &s);
        }
        return 1;
    }
    *t = 1.0;
    if (VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y0, x1, y0, &s) && s < *t)
        *t = s;
    if (VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y0, x1, y1, &s) && s < *t)
        *t = s;
    if (VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y1, x0, y1, &s) && s < *t)
        *t = s;
    if (VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y1, x0, y0, &s) && s < *t)
        *t = s;
    return *t < 1.0;
}

/* Mac 0x538b41, Win 0x7b5410: the segment a-b (xy) touches the rectangle */
static int VincSegmentTouchesRect(const MeReal* x0, const MeReal* x1, const MeReal* y0, const MeReal* y1,
                                  const MeReal* a, const MeReal* b)
{
    MeReal s;
    if (*x0 <= a[0] && a[0] <= *x1 && *y0 <= a[1] && a[1] <= *y1 && *x0 <= b[0] && b[0] <= *x1 && *y0 <= b[1] &&
        b[1] <= *y1)
        return 1;
    if (VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y0, x1, y0, &s) ||
        VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y0, x1, y1, &s) ||
        VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x1, y1, x0, y1, &s))
        return 1;
    return VincSegmentsCross2D(&a[0], &a[1], &b[0], &b[1], x0, y1, x0, y0, &s);
}

/* Mac 0x538ce4, Win 0x7b5530: the cell under (x, y) */
static int VincHeightFieldCellOf(const VincHeightField* hf, const MeReal* x, const MeReal* y, int* ix, int* iy)
{
    *ix = (int)((*x - hf->x0) * hf->invDx);
    *iy = (int)((*y - hf->y0) * hf->invDy);
    if (*ix < 0 || hf->xCells - 1 < *ix || *iy < 0)
        return 0;
    return *iy <= hf->yCells - 1;
}

/* Mac 0x538d47, Win 0x7b5590: the cell's rectangle and its corner heights (x0 y0, x1 y0, x1 y1,
 * x0 y1) */
static int VincHeightFieldCellBounds(const VincHeightField* hf, const int* ix, const int* iy, MeReal* x0, MeReal* x1,
                                     MeReal* y0, MeReal* y1, MeReal* h4)
{
    int row;
    if (*ix < 0 || hf->xCells - 1 < *ix || *iy < 0 || hf->yCells - 1 < *iy)
        return 0;
    *x0 = (MeReal)*ix * hf->dx + hf->x0;
    *x1 = *x0 + hf->dx;
    *y0 = (MeReal)*iy * hf->dy + hf->y0;
    *y1 = *y0 + hf->dy;
    row = *iy * hf->xCount + *ix;
    h4[0] = hf->heights[row];
    h4[1] = hf->heights[row + 1];
    row = (*iy + 1) * hf->xCount + *ix;
    h4[2] = hf->heights[row + 1];
    h4[3] = hf->heights[row];
    return 1;
}

/* Mac 0x538e40, Win 0x7b5640: p (xy) inside the triangle a b c (barycentric, 1e-14 slack) */
static int VincPointInTriangle2D(const MeReal* a, const MeReal* b, const MeReal* c, const MeReal* p)
{
    MeReal ax = a[0], ay = a[1];
    MeReal det = (c[0] - ax) * (b[1] - ay) - (c[1] - ay) * (b[0] - ax);
    MeReal inv, s, u;
    if (!(1e-14 <= fabs(det)))
        return 0;
    inv = 1.0 / det;
    s = ((p[0] - ax) * (b[1] - ay) - (b[0] - ax) * (p[1] - ay)) * inv;
    if (!(-1e-14 <= s))
        return 0;
    u = ((c[0] - ax) * (p[1] - ay) - (c[1] - ay) * (p[0] - ax)) * inv;
    if (!(-1e-14 <= u))
        return 0;
    return -1e-14 <= 1.0 - s - u;
}

/* the hit of a-b on the plane of the triangle (point o, normal n) inside the triangle */
static int VincHeightFieldHitTriangle(const MeReal* o, MeReal* n, const MeReal* t0, const MeReal* t1,
                                      const MeReal* t2, const MeReal* tm, const MeReal* a, const MeReal* b,
                                      VincLineSegResult* result, int element)
{
    MeReal dz = b[2] - a[2], dy = b[1] - a[1], dx = b[0] - a[0];
    MeReal den = n[0] * dx + n[1] * dy + n[2] * dz;
    MeReal t, len;
    MeVector3 p;
    if (!(1e-14 < fabs(den)))
        return 0;
    t = -((n[0] * a[0] + n[1] * a[1] + n[2] * a[2]) - (o[0] * n[0] + o[1] * n[1] + o[2] * n[2])) / den;
    if (!(0.0 <= t && t <= 1.0))
        return 0;
    p[2] = dz * t + a[2];
    p[1] = dy * t + a[1];
    p[0] = dx * t + a[0];
    if (!VincPointInTriangle2D(t0, t1, t2, p))
        return 0;
    VincTMTransform(tm, p, result->position);
    len = sqrt(n[0] * n[0] + n[1] * n[1] + n[2] * n[2]);
    if (len != 0.0) {
        MeReal inv = 1.0 / len;
        n[0] = n[0] * inv;
        n[1] = n[1] * inv;
        n[2] = inv * n[2];
    }
    VincTMRotate(tm, n, result->normal);
    result->distance = sqrt((a[0] - p[0]) * (a[0] - p[0]) + (a[1] - p[1]) * (a[1] - p[1]) + (a[2] - p[2]) * (a[2] - p[2]));
    result->element = element;
    return 1;
}

/* Mac 0x538f84, Win 0x7b5710: the segment a-b (geometry frame) against the cell's two triangles;
 * result->element = 0 / 1 (the triangle) */
static int VincHeightFieldCellHit(const MeReal* x0p, const MeReal* x1p, const MeReal* y0p, const MeReal* y1p,
                                  const MeReal* h4, const MeReal* tm, const MeReal* a, const MeReal* b,
                                  VincLineSegResult* result)
{
    MeReal x0 = *x0p, y0 = *y0p, h0 = h4[0], x1 = *x1p, h1 = h4[1], y1 = *y1p, h2 = h4[2], h3 = h4[3];
    MeReal A[3], B[3], C[3], D[3], n[3];
    A[0] = x0; A[1] = y0; A[2] = h0;
    B[0] = x1; B[1] = y0; B[2] = h1;
    C[0] = x0; C[1] = y1; C[2] = h3;
    D[0] = x1; D[1] = y1; D[2] = h2;
    /* (B - A) x (C - B) */
    n[0] = (h3 - h1) * (y0 - y0) - (y1 - y0) * (h1 - h0);
    n[1] = (x0 - x1) * (h1 - h0) - (h3 - h1) * (x1 - x0);
    n[2] = (y1 - y0) * (x1 - x0) - (x0 - x1) * (y0 - y0);
    if (VincHeightFieldHitTriangle(A, n, A, B, C, tm, a, b, result, 0))
        return 1;
    /* (D - C) x (C - B) */
    n[0] = (h3 - h1) * (D[1] - C[1]) - (y1 - y0) * (h2 - h3);
    n[1] = (x0 - B[0]) * (h2 - h3) - (h3 - h1) * (D[0] - x0);
    n[2] = (y1 - y0) * (D[0] - x0) - (x0 - B[0]) * (D[1] - C[1]);
    return VincHeightFieldHitTriangle(B, n, B, D, C, tm, a, b, result, 1);
}

/* Mac 0x53992a, Win 0x7b5b30: walks the cells along the segment's xy projection from where it
 * enters the grid; result->element = 2 * cell + triangle */
int MEAPI VincHeightFieldLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1, VincLineSegResult* result)
{
    MeReal* tm = VincModelGetTransformPtr(model);
    VincHeightField* hf = HF(VincModelGetGeometry(model));
    MeReal a[3], b[3], x0, x1, y0, y1, t, px, py, h4[4];
    int sx, sy, ix, iy, nx, ny;
    result->model = model;
    VincTMInverseTransform(tm, p0, a);
    VincTMInverseTransform(tm, p1, b);
    if (!(hf->zMin <= (b[2] <= a[2] ? a[2] : b[2])))
        return 0;
    if (!((b[2] <= a[2] ? b[2] : a[2]) <= hf->zMax))
        return 0;
    x0 = hf->x0;
    x1 = x0 + hf->xSize;
    y0 = hf->y0;
    y1 = y0 + hf->ySize;
    if (!VincSegmentClipRect(&x0, &x1, &y0, &y1, a, b, &t))
        return 0;
    t = t + 1e-14;
    sx = (0.0 <= b[0] - a[0]) * 2 - 1;
    sy = (0.0 <= b[1] - a[1]) * 2 - 1;
    px = a[0] + t * (b[0] - a[0]);
    py = a[1] + (b[1] - a[1]) * t;
    if (!VincHeightFieldCellOf(hf, &px, &py, &ix, &iy))
        return 0;
    if (!VincHeightFieldCellBounds(hf, &ix, &iy, &x0, &x1, &y0, &y1, h4))
        return 0;
    VincSegmentTouchesRect(&x0, &x1, &y0, &y1, a, b);
    for (;;) {
        if (VincHeightFieldCellHit(&x0, &x1, &y0, &y1, h4, tm, a, b, result)) {
            result->element = result->element + ((hf->xCount - 1) * iy + ix) * 2;
            return 1;
        }
        nx = sx + ix;
        ny = iy;
        if (VincHeightFieldCellBounds(hf, &nx, &ny, &x0, &x1, &y0, &y1, h4) &&
            VincSegmentTouchesRect(&x0, &x1, &y0, &y1, a, b)) {
            ix = nx;
            iy = ny;
            continue;
        }
        nx = ix;
        ny = sy + iy;
        if (VincHeightFieldCellBounds(hf, &nx, &ny, &x0, &x1, &y0, &y1, h4) &&
            VincSegmentTouchesRect(&x0, &x1, &y0, &y1, a, b)) {
            ix = nx;
            iy = ny;
            continue;
        }
        nx = sx + ix;
        ny = sy + iy;
        if (VincHeightFieldCellBounds(hf, &nx, &ny, &x0, &x1, &y0, &y1, h4) &&
            VincSegmentTouchesRect(&x0, &x1, &y0, &y1, a, b)) {
            ix = nx;
            iy = ny;
            continue;
        }
        return 0;
    }
}

/* Mac 0x538f59, Win 0x7b5e50 */
void MEAPI VincHeightFieldRegisterLineSegment(void)
{
    VincGeometryRegisterLineSegmentFn(VincHeightFieldGetTypeId(), VincHeightFieldLineSegment);
}

/* Mac 0x53b322, Win 0x75c680 */
void MEAPI VincHeightFieldRegisterInteractions(void)
{
    VincSphereHeightFieldRegisterInteraction();
    VincBoxHeightFieldRegisterInteraction();
    VincCylinderHeightFieldRegisterInteraction();
    VincCapsuleHeightFieldRegisterInteraction();
    VincHeightFieldRegisterLineSegment();
}
