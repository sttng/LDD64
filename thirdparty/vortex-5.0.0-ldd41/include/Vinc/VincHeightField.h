/* Vortex 5.0.0 build 800 "vortexldd41" - the regular-grid height field geometry ("RGHeightField",
 * Vinc type 8) and its pair tests (restored; src/Vinc/VincHeightField.c, Mac
 * sub-TU 558c 0x532a4f-0x53b3de, Win 0x770910-0x7738d0, 0x75c400-0x75c6a0, 0x7b43b0-0x7b5e62).
 *
 * This is the MathEngine McdRGHeightField continued: a grid of xCount x yCount heights (row-major,
 * heights[y * xCount + x]) with spacing dx / dy and origin x0 / y0 in the geometry frame; every cell
 * is split into the triangles (x, y)-(x+1, y)-(x, y+1) and (x+1, y)-(x+1, y+1)-(x, y+1). The solid
 * extends below the surface: without limit (infiniteDepth) or down to zMin - depth. LDD makes one
 * for the terrain (Vx::VxHeightField, VortexPhysicsGeometrySampledHeightField).
 *
 * Pair tests: Box, Capsule, Cylinder and Sphere against the height field (the triangle-mesh and
 * convex ones belong to V3.3 / V3.1), and the line-segment query. Names follow the strings
 * ("RGHeightField", "Contact buffer full in <X>HeightField") and the Mcd API; all other names are
 * inferred. The Win build exports the full public API (stdcall wrappers 0x75c410-0x75c650
 * around the internal functions); the Mac build dead-stripped most of it. */
#ifndef VINC_VINCHEIGHTFIELD_H
#define VINC_VINCHEIGHTFIELD_H

#include "Vinc/VincIntersect.h"

#ifdef __cplusplus
extern "C" {
#endif

/* Mac new(0x7c) 0x5334ff, Win new(0x80) 0x7b5080 */
typedef struct VincHeightField {     /*  Mac   Win */
    VincGeometry base;               /* +0x00 +0x00 */
    MeReal* heights;                 /* +0x04 +0x04 xCount * yCount, not owned */
    int xCount;                      /* +0x08 +0x08 */
    int yCount;                      /* +0x0c +0x0c */
    MeReal dx;                       /* +0x10 +0x10 */
    MeReal dy;                       /* +0x18 +0x18 */
    MeReal x0;                       /* +0x20 +0x20 */
    MeReal y0;                       /* +0x28 +0x28 */
    int xCells;                      /* +0x30 +0x30 xCount - 1 */
    int yCells;                      /* +0x34 +0x34 yCount - 1 */
    MeReal xSize;                    /* +0x38 +0x38 xCells * dx */
    MeReal ySize;                    /* +0x40 +0x40 yCells * dy */
    MeReal invDx;                    /* +0x48 +0x48 */
    MeReal invDy;                    /* +0x50 +0x50 */
    MeReal zMin;                     /* +0x58 +0x58 (VincHeightFieldUpdateZRange) */
    MeReal zMax;                     /* +0x60 +0x60 */
    MeReal m68;                      /* +0x68 +0x68 0; set (0..1) by Win 0x75c600, never read */
    char infiniteDepth;              /* +0x70 +0x70 1 at creation */
    MeReal depth;                    /* +0x74 +0x78 0 at creation: solid below zMin - depth */
} VincHeightField;

/* ---- type ---- */
MeI16 MEAPI VincHeightFieldGetTypeId(void);       /* Mac 0x53b3d0 (Mac 0x532a4f inner), Win 0x75c400 (0x7b43b0) */
void MEAPI VincHeightFieldRegisterType(void);     /* Mac 0x53b343 (0x533584), Win 0x75c670 (0x7b4fa0) */
/* registers the Sphere / Box / Cylinder / Capsule pair tests and the line segment query */
void MEAPI VincHeightFieldRegisterInteractions(void); /* Mac 0x53b322, Win 0x75c680 */

/* ---- creation (the arrays are not copied) ---- */
VincGeometryID MEAPI VincRGHeightFieldCreate(MeReal* heights, int xCount, int yCount, MeReal dx, MeReal dy, MeReal x0,
                                           MeReal y0);                                      /* Mac 0x53b363, Win 0x75c410 */
VincGeometryID MEAPI VincHeightFieldCreateFromPtrs(MeReal* heights, const int* xCount, const int* yCount,
                                                   const MeReal* dx, const MeReal* dy, const MeReal* x0,
                                                   const MeReal* y0);                       /* Mac 0x5334ff, Win 0x7b5080 */
int MEAPI VincHeightFieldInit(VincHeightField* hf, MeReal* heights, const int* xCount, const int* yCount,
                              const MeReal* dx, const MeReal* dy, const MeReal* x0, const MeReal* y0); /* Mac 0x532afc, Win 0x7b4fe0 */
/* zMin / zMax from the heights */
void MEAPI VincRGHeightFieldUpdate(VincGeometryID hf);                 /* Mac 0x53b350 (0x532a87), Win 0x75c4a0 (0x7b43e0) */

/* ---- registered callbacks ---- */
void MEAPI VincHeightFieldDestroy(VincGeometryID g);                                         /* Mac 0x532a61, Win 0x7b43c0 */
void MEAPI VincHeightFieldGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);            /* Mac 0x532bbc, Win 0x7b4570 */
void MEAPI VincHeightFieldUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x5334c3, Win 0x7b46b0 */
void MEAPI VincHeightFieldGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);          /* Mac 0x53345b, Win 0x7b46e0 */
void MEAPI VincHeightFieldGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius);     /* Mac 0x533370, Win 0x7b4730 */
void MEAPI VincHeightFieldMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x533719, Win 0x7b47f0 */
int MEAPI VincHeightFieldGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x5332b9, Win 0x7b4f10 */

/* ---- grid queries ---- */
/* cells overlapping the xy box {min x, max x, min y, max y}: [x0, x1) x [y0, y1); 0 if outside */
int MEAPI VincHeightFieldGetCellRange(VincGeometryID hf, const MeReal* xyBounds4, int* x0, int* x1, int* y0,
                                      int* y1);                     /* Mac 0x532df4, Win 0x7b4940 (API wrapper 0x75c550) */
/* the same, but a box outside the grid on one side gives the border cells (Win only; called by the
 * Win-only 0x81a750) */
int MEAPI VincHeightFieldGetCellRangeClamped(VincGeometryID hf, const MeReal* xyBounds4, int* x0, int* x1, int* y0,
                                             int* y1);                                       /* Win 0x7b4a20 */
/* cells under the sphere (centre, *radius) if it reaches below zMax */
int MEAPI VincHeightFieldGetSphereCellRange(VincGeometryID hf, const MeReal* center, const MeReal* radius, int* x0,
                                            int* x1, int* y0, int* y1);                      /* Mac 0x5335f0, Win 0x7b4b50 */
/* grid vertex (x, y) in the geometry frame */
void MEAPI VincHeightFieldGetVertex(VincGeometryID hf, MeVector3 out, int x, int y);          /* Mac 0x532ef0, Win 0x7b4c50 */
/* the surface height at (x, y) (0 outside the grid) */
int MEAPI VincHeightFieldGetHeight(VincGeometryID hf, MeReal x, MeReal y, MeReal* z); /* Mac 0x532f3b, Win 0x7b4c90 (API 0x75c590) */
/* the same, and the vertices of the triangle under (x, y) */
int MEAPI VincHeightFieldGetHeightAndTriangle(VincGeometryID hf, MeReal x, MeReal y, MeReal* z, MeVector3 v0,
                                              MeVector3 v1, MeVector3 v2); /* Mac 0x533096, Win 0x7b4da0 (API 0x75c5c0) */

/* ---- public accessors (Win only; Mac: dead-stripped) ---- */
void MEAPI VincHeightFieldGetParameters(VincGeometryID hf, MeReal** heights, int* xCount, int* yCount, MeReal* dx,
                                        MeReal* dy, MeReal* x0, MeReal* y0);                 /* Win 0x75c440 */
/* NOTE: `heights` is not written */
void MEAPI VincHeightFieldGetGrid(VincGeometryID hf, MeReal** heights, int* xCount, int* yCount, MeReal* dx,
                                  MeReal* dy, MeReal* x0, MeReal* y0, int* xCells, int* yCells, MeReal* xSize,
                                  MeReal* ySize);                                            /* Win 0x7b4510 */
void MEAPI VincHeightFieldSetHeights(VincGeometryID hf, MeReal* heights);                    /* Win 0x75c490 */
MeReal* MEAPI VincHeightFieldGetHeights(VincGeometryID hf);                                  /* Win 0x75c580 */
void MEAPI VincHeightFieldSetInfiniteDepth(VincGeometryID hf, int on);                       /* Win 0x75c4b0 (0x7b4450) */
int MEAPI VincHeightFieldGetInfiniteDepth(VincGeometryID hf);                                /* Win 0x75c4d0 (0x7b4460) */
void MEAPI VincHeightFieldSetDepth(VincGeometryID hf, MeReal depth);                         /* Win 0x75c4f0 (0x7b4470) */
MeReal MEAPI VincHeightFieldGetDepth(VincGeometryID hf);                                     /* Win 0x75c510 (0x7b4480) */
void MEAPI VincHeightFieldSetOrigin(VincGeometryID hf, MeReal x0, MeReal y0);                /* Win 0x75c520 (0x7b4f80) */
/* NOTE: the cell range arguments are ignored: the range of all heights (Win 0x75c540 -> 0x7b4490) */
void MEAPI VincHeightFieldGetZRangeOfCells(VincGeometryID hf, int x0, int x1, int y0, int y1, MeReal* zMin,
                                           MeReal* zMax);
void MEAPI VincHeightFieldSetM68(VincGeometryID hf, MeReal v);                               /* Win 0x75c600: 0 <= v <= 1 */
void MEAPI VincHeightFieldSetZRange(VincGeometryID hf, MeReal zMin, MeReal zMax);            /* Win 0x75c630 */
void MEAPI VincHeightFieldGetZRange(VincGeometryID hf, MeReal* zMin, MeReal* zMax);          /* Win 0x75c650 */
/* bounding sphere into {centre[3], radius} (Win 0x7b5060) */
void MEAPI VincHeightFieldGetBSphere4(VincGeometryID hf, int unused, MeReal* sphere4);

/* ---- pair tests (model1 = the other geometry, model2 = the height field) ---- */
int MEAPI VincBoxHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result);      /* Mac 0x534ed3, Win 0x7737c0 */
int MEAPI VincCapsuleHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result);  /* Mac 0x53675f, Win 0x7712c0 */
int MEAPI VincCylinderHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result); /* Mac 0x5368db, Win 0x7714e0 */
int MEAPI VincSphereHeightFieldIntersect(VincModelPair* pair, VincIntersectResult* result);   /* Mac 0x53b22e, Win 0x772c80 */
int MEAPI VincBoxHeightFieldRegisterInteraction(void);       /* Mac 0x5338f2, Win 0x773880 */
int MEAPI VincCapsuleHeightFieldRegisterInteraction(void);   /* Mac 0x534fda, Win 0x771380 */
int MEAPI VincCylinderHeightFieldRegisterInteraction(void);  /* Mac 0x536866, Win 0x7722f0 */
int MEAPI VincSphereHeightFieldRegisterInteraction(void);    /* Mac 0x539f2b, Win 0x772d30 */
/* line segment p0-p1 (world) against the height field model */
int MEAPI VincHeightFieldLineSegment(VincModelID model, const MeVector3 p0, const MeVector3 p1,
                                     VincLineSegResult* result);                             /* Mac 0x53992a, Win 0x7b5b30 */
void MEAPI VincHeightFieldRegisterLineSegment(void);         /* Mac 0x538f59, Win 0x7b5e50 */

/* raw contact buffer sizes of the pair tests (doubled with a warning when a test fills one)
 * Box Mac 0x779d28 / Win 0xabd93c, Capsule 0x779d2c / 0xabd8c4, Cylinder 0x779d30 / 0xabd8ec,
 * Sphere 0x779d34 / 0xabd914 */
extern int VincBoxHeightFieldMaxContacts;
extern int VincCapsuleHeightFieldMaxContacts;
extern int VincCylinderHeightFieldMaxContacts;
extern int VincSphereHeightFieldMaxContacts;

#ifdef __cplusplus
}
#endif

#endif
