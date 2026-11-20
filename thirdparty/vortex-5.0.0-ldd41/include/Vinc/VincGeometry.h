/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc geometry type registry and the pair-interaction
 * table (restored; src/Vinc/VincFramework.cpp, VincInteractions.cpp).
 *
 * Vinc is the CM Labs successor of the MathEngine collision toolkit (Mcd). The names follow the
 * Vinc strings of the binaries ("VincGeometryRegisterType", "VincGeometryGetMassProperties",
 * "VincInit", "VincTerm", "VincSafeTime", the VincErrorTable); the remaining names are inferred
 * from the MathEngine Mcd API that Vinc continues (marked "(name inferred)").
 *
 * API for the other Vinc items (convex mesh, pair tests and primitives, height
 * field, space):
 *
 *  - Geometry types. Every geometry struct starts with VincGeometry (MeI16 type id). A geometry
 *    kind registers once, at VincInit time (VincInit -> registration functions of the kinds):
 *        id = VincGeometryRegisterType(id, "Box", destroy, updateAABB, getXYAABB, getBSphere,
 *                                      maximumPoint, getMassProperties);
 *    and sets the id of each new instance with VincGeometryInit(g, id). The built-in ids are 0..12
 *    (VincInit allocates geoTypeMaxCount + 13 slots); the per-type functions are reached through
 *    the dispatchers VincGeometryDestroy/UpdateAABB/GetXYAABB/GetBSphere/MaximumPoint/
 *    GetMassProperties (Mac 0x4c00a3-0x4c01ad). The registry is a set of parallel tables (one
 *    function pointer per type each) and VincGeometryGetTypeCount() is their size.
 *
 *  - Pair interactions. The pair table is a typeCount x typeCount matrix of VincPairTableCell
 *    (0x24 bytes): cell[type2][type1] = {swap flag, VincInteractions (7 slots), custom getter}.
 *    A pair test registers with
 *        VincInteractions i = {hello, goodbye, intersect, 0, 0, 0, 0};  (C aggregate init)
 *        VincGeometryPairRegisterInteractions(typeA, typeB, &i, 0);
 *    which fills cell(A,B) with swap = 0 and (if A != B) cell(B,A) with swap = 1: an intersect
 *    function always receives the pair with model1 of geometry typeA. VincHello (VincIntersect.h)
 *    installs the cell's functions into the VincModelPair and swaps the models when needed.
 *
 *  - Line segment queries: VincGeometryRegisterLineSegmentFn(type, fn) per type,
 *    VincLineSegIntersect(model, p0, p1, result) dispatches on the model's geometry type.
 */
#ifndef VINC_VINCGEOMETRY_H
#define VINC_VINCGEOMETRY_H

#include "Me/MeMath.h"
#include "Me/MeCall.h"
#include "Vinc/VincTypes.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef short MeI16;

struct McdModelPair;       /* VincModelPair (VincIntersect.h) */
struct McdIntersectResult; /* VincIntersectResult (VincIntersect.h) */
struct McdSafeTimeResult;  /* VincSafeTimeResult (VincIntersect.h) */
struct McdLineSegResult;   /* VincLineSegResult (VincIntersect.h) */

/* Common head of every geometry (Mac/Win +0x00). The kind-specific data follows. */
typedef struct McdGeometry {
    MeI16 type;   /* +0x00 registered type id (VincGeometryInit) */
} VincGeometry;
typedef struct McdGeometry* VincGeometryID;

/* Built-in type count reserved by VincInit (ids 0..12; Mac 0x4bfe9e: geoTypeMaxCount + 0xd). */
#define VINC_BUILTIN_GEOMETRY_TYPES 13

/* ---- per-type functions (registered with VincGeometryRegisterType) ---- */
/* destroy the geometry (Mac table 0x77ce00) */
typedef void (MEAPI *VincGeometryDestroyFn)(VincGeometryID g);
/* world AABB under tm: minCorner/maxCorner, and aabb6 = {min x, max x, min y, max y, min z, max z}
 * (the model's cached box, VincModel +0x04) (Mac table 0x77ce04) */
typedef void (MEAPI *VincGeometryUpdateAABBFn)(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner,
                                         MeVector3 maxCorner, MeReal* aabb6);
/* xy bounds under tm: out = {min x, max x, min y, max y} (Mac table 0x77ce08) (name inferred) */
typedef void (MEAPI *VincGeometryGetXYAABBFn)(VincGeometryID g, MeMatrix4 tm, MeReal* out4);
/* bounding sphere in the geometry frame (Mac table 0x77ce0c) */
typedef void (MEAPI *VincGeometryGetBSphereFn)(VincGeometryID g, MeVector3 center, MeReal* radius);
/* support point in direction dir under tm (Mac table 0x77ce10) */
typedef void (MEAPI *VincGeometryMaximumPointFn)(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir,
                                           MeVector3 out);
/* unit-density mass properties: relTM (output: centre of mass frame), inertia, volume
 * (Mac table 0x77ce14); the return value is a status (the dispatcher returns it as MeI16) */
typedef int (MEAPI *VincGeometryGetMassPropertiesFn)(VincGeometryID g, MeMatrix4 relTM,
                                               MeMatrix3 inertia, MeReal* volume);

/* ---- pair interaction functions ---- */
typedef int (MEAPI *VincHelloFn)(struct McdModelPair* pair);
typedef int (MEAPI *VincGoodbyeFn)(struct McdModelPair* pair);
/* returns "touch" (also stored in result->touch by VincIntersect) */
typedef int (MEAPI *VincIntersectFn)(struct McdModelPair* pair, struct McdIntersectResult* result);
/* continuous ("swept") intersection over dt (used by VincIntersectWithTime) (name inferred) */
typedef int (MEAPI *VincSweptIntersectFn)(struct McdModelPair* pair, struct McdIntersectResult* result,
                                    MeReal dt);
typedef int (MEAPI *VincSafeTimeFn)(struct McdModelPair* pair, MeReal maxTime,
                              struct McdSafeTimeResult* result);
struct McdDistanceResult;  /* VincDistanceResult (VincIntersect.h) */
/* distance queries, interaction slots 5 / 6 (Win 0x754e50 / 0x754df0) (names inferred) */
typedef int (MEAPI *VincDistanceFn)(struct McdModelPair* pair, struct McdDistanceResult* result);
typedef int (MEAPI *VincDistanceBoundedFn)(struct McdModelPair* pair, const MeReal* bounds3,
                                     struct McdDistanceResult* result);
/* line segment p0-p1 against one model (Mac 0x4c165a dispatches) */
typedef int (MEAPI *VincLineSegFn)(struct McdModel* model, const MeVector3 p0, const MeVector3 p1,
                             struct McdLineSegResult* result);

/* One pair-table entry (7 slots, 0x1c bytes on both platforms). Slots 5 and 6 (distance queries)
 * are only read by the Win-only VincDistance / VincDistanceBounded (dead-stripped on the Mac); the
 * Mac registrations that set five slots leave them uninitialised (VincConvexMesh zeroes them). */
typedef struct VincInteractions {
    VincHelloFn helloFn;                   /* +0x00 -> pair  (called by VincHello) */
    VincGoodbyeFn goodbyeFn;               /* +0x04 -> pair->goodbyeFn (+0x20) */
    VincIntersectFn intersectFn;           /* +0x08 -> pair->intersectFn (+0x14) */
    VincSweptIntersectFn sweptIntersectFn; /* +0x0c -> pair->sweptIntersectFn (+0x18) (name inferred) */
    VincSafeTimeFn safeTimeFn;             /* +0x10 -> pair->safeTimeFn (+0x1c) */
    VincDistanceFn distanceFn;             /* +0x14 (name inferred) */
    VincDistanceBoundedFn distanceBoundedFn; /* +0x18 (name inferred) */
} VincInteractions;

/* Optional per-cell getter: returns the interactions for this pair instead of the cell's own. */
typedef VincInteractions* (MEAPI *VincInteractionsGetFn)(struct McdModelPair* pair);

/* Pair table cell (0x24 bytes). */
typedef struct VincPairTableCell {
    int swap;                         /* +0x00 1: models are swapped before use */
    VincInteractions interactions;    /* +0x04 */
    VincInteractionsGetFn getFn;      /* +0x20 (name inferred) */
} VincPairTableCell;

/* ---- framework / registry (VincFramework.cpp) ---- */
/* Mac 0x4bff42, Win 0x7abca0 (name inferred) */
void MEAPI VincGeometryInit(VincGeometryID g, int typeId);
/* Mac 0x4c0091, Win 0x7abe10 (name inferred): number of type slots */
int MEAPI VincGeometryGetTypeCount(void);
/* Mac 0x4c00a3 (name inferred): NULL-safe */
void MEAPI VincGeometryDestroy(VincGeometryID g);
/* Mac 0x4c00cc, Win 0x7abe40 (name inferred) */
void MEAPI VincGeometryUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 minCorner, MeVector3 maxCorner,
                            MeReal* aabb6);
/* Mac 0x4c010d, Win 0x7abe70 (name inferred) */
void MEAPI VincGeometryGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal* out4);
/* Mac 0x4c0140, Win 0x7abe90 (name inferred) */
void MEAPI VincGeometryGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius);
/* Mac 0x4c0173 (name inferred) */
void MEAPI VincGeometryMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out);
/* Mac 0x4c01ad, Win 0x7abee0: calls the type function, then symmetrises the inertia
 * ((I_ij + I_ji) / 2) */
MeI16 MEAPI VincGeometryGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia,
                                    MeReal* volume);
/* Mac 0x4c0240, Win 0x7ac0e0 (name inferred): aabb6 {min x, max x, min y, ...} -> min, max */
void MEAPI VincAABBGetMinMax(const MeReal* aabb6, MeVector3 minCorner, MeVector3 maxCorner);
/* Mac 0x4c0287, Win 0x7ac030: returns typeId; error 21 when typeId >= the slot count */
int MEAPI VincGeometryRegisterType(int typeId, const char* typeName, VincGeometryDestroyFn destroy,
                             VincGeometryUpdateAABBFn updateAABB, VincGeometryGetXYAABBFn getXYAABB,
                             VincGeometryGetBSphereFn getBSphere,
                             VincGeometryMaximumPointFn maximumPoint,
                             VincGeometryGetMassPropertiesFn getMassProperties);
/* Win 0x7abf40 (name inferred): 0 <= typeId < count and a destroy function registered */
int MEAPI VincGeometryTypeIsRegistered(int typeId);
/* Win 0x7abf70 (name inferred): the registered name or "UnknownGeometryType" */
const char* MEAPI VincGeometryGetTypeName(int typeId);
/* Win 0x7abfb0 (name inferred): name of g's type, or "UnknownGeometryType" */
const char* MEAPI VincGeometryGetTypeNameOf(VincGeometryID g);
/* Win 0x7abff0 (name inferred): g != NULL and of a registered type */
int MEAPI VincGeometryIsValid(VincGeometryID g);
/* (not in the binaries; tests) the 7 registry entries of typeId: destroy, updateAABB, getXYAABB,
 * getBSphere, maximumPoint, getMassProperties, name */
void MEAPI VincGeometryGetTypeFunctions(int typeId, void** fns7);

/* ---- pair table (VincInteractions.cpp) ---- */
/* Mac 0x4c13c3, Win 0x7ac860 (name inferred): all 7 slots = 0 */
void MEAPI VincInteractionsReset(VincInteractions* i);
/* Mac 0x4c13fb (name inferred) */
void MEAPI VincInteractionsCopy(const VincInteractions* src, VincInteractions* dst);
/* Mac 0x4c142e, Win 0x7ac8c0 (name inferred): typeCount x typeCount cells + line segment table */
void MEAPI VincPairTableCreate(int typeCount);
/* Mac 0x4c14f9 (name inferred) */
void MEAPI VincPairTableDestroy(void);
/* Mac 0x4c1534, Win 0x7ac9c0 (name inferred) */
void MEAPI VincGeometryPairRegisterInteractions(int typeA, int typeB, const VincInteractions* i,
                                          VincInteractionsGetFn getFn);
/* Mac 0x4c15ce, Win 0x7acb10 (name inferred): cell of (pair->model2 type, pair->model1 type) */
VincInteractions* MEAPI VincPairTableGetInteractions(struct McdModelPair* pair, int* swap);
/* (name inferred) the raw cell, for tests and the space: NULL when out of range */
const VincPairTableCell* MEAPI VincPairTableGetCell(int type1, int type2);
/* Mac 0x4c1627 (name inferred) */
void MEAPI VincGeometryRegisterLineSegmentFn(int typeId, VincLineSegFn fn);
/* Mac 0x4c1643, Win 0x7acb80 (name inferred) */
VincLineSegFn MEAPI VincGeometryGetLineSegmentFn(int typeId);

#ifdef __cplusplus
}
#endif

#endif
