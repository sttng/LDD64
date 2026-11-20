/* Vortex 5.0.0 build 800 "vortexldd41" - VincModel: a collision model = geometry + transform
 * (restored; src/Vinc/VincModel.cpp, VincAggregate.cpp).
 *
 * Models come from the framework's VinPool (VincInit(geoTypeMaxCount, modelCount)). The struct is
 * laid out naturally: Mac i386 aligns doubles to 4, MSVC to 8, which gives exactly the two measured
 * layouts (Mac 0xac bytes, Win 0xb0; VincTypes.h McdModelOffsets). Names inferred unless a
 * string names them (VincModelSetTransform, VincModelDestroy, VincModelID, VincModelPair).
 */
#ifndef VINC_VINCMODEL_H
#define VINC_VINCMODEL_H

#include "Vinc/VincGeometry.h"
#include <stddef.h>

struct VinPool;   /* Me/VinPool.h */

#ifdef __cplusplus
extern "C" {
#endif

typedef struct McdModel VincModel;
typedef struct McdModel* VincModelID;

/* called before the AABB update: recompute transform from relTM/bodyTM (VincModelSetRelativeTransformPtrs) */
typedef void (MEAPI *VincModelUpdateTransformFn)(VincModelID model);
/* VincModelSetVelocityPtrs implementation (aggregates propagate to their elements) */
typedef void (MEAPI *VincModelSetVelocityPtrsFn)(VincModelID model, MeReal* linear, MeReal* angular);

/*                                       Mac    Win */
struct McdModel {
    VincGeometryID geometry;          /* +0x00 +0x00 */
    MeReal aabb[6];                   /* +0x04 +0x08 {min x, max x, min y, max y, min z, max z} */
    MeReal* transform;                /* +0x34 +0x38 MeMatrix4 (row-major TM) */
    MeReal* linearVelocity;           /* +0x38 +0x3c default: zero vector */
    MeReal* angularVelocity;          /* +0x3c +0x40 default: zero vector */
    VincModelID parent;               /* +0x40 +0x44 aggregate model owning this element */
    MeReal padding;                   /* +0x44 +0x48 contact padding (framework default 0.005) */
    void* userData;                   /* +0x4c +0x50 the owning Vx::VxCollisionGeometry */
    int m50;                          /* +0x50 +0x54 (0) */
    void* space;                      /* +0x54 +0x58 CxSpace (C++ object) while in a space */
    int spaceHandle;                  /* +0x58 +0x5c handle in the space (-1) */
    VincModelUpdateTransformFn updateTransformFn; /* +0x5c +0x60 */
    VincModelSetVelocityPtrsFn setVelocityPtrsFn; /* +0x60 +0x64 */
    MeReal* relTransform;             /* +0x64 +0x68 model relative to its body */
    MeReal* bodyTransform;            /* +0x68 +0x6c body transform (relTransform != 0) */
    ptrdiff_t m6c;                    /* +0x6c +0x70 (0) a part's MdtBody (VxCollisionGeometry::setEntity);
                                         pointer-sized for 64-bit hosts */
    int m70;                          /* +0x70 +0x74 (0) */
    MeReal density;                   /* +0x74 +0x78 (1.0) mass = volume * density */
    void* m7c;                        /* +0x7c +0x80 (0; VincModelSetM7C) */
    void* m80;                        /* +0x80 +0x84 (0; VincModelSetM80) */
    MeVector3 negCOM;                 /* +0x84 +0x88 minus the centre of rotation in model frame */
    int sweep;                        /* +0x9c +0xa0 swept (continuous) collision on */
    int state;                        /* +0xa0 +0xa4 2: special state (space notified) */
    ptrdiff_t ma4;                    /* +0xa4 +0xa8 (0) the geometry's Vx::VxIntersectFilter (VxBridge, V4.5);
                                         pointer-sized for 64-bit hosts */
    const char* ma8;                  /* +0xa8 +0xac (0) the owning sensor's name (VxCollisionSensor::setName);
                                         an int in both binaries, PORT64: the pointer itself */
};

/* framework default padding: Mac 0x4c189a, Win 0x74f720 */
void MEAPI VincSetDefaultPadding(const MeReal* padding);
int MEAPI VincModelGetState(VincModelID m);                          /* Mac 0x4c18b9, Win 0x74f7c0 */
void MEAPI VincModelSetState(VincModelID m, int state);              /* Mac 0x4c1d03, Win 0x74f780 */
MeReal MEAPI VincModelGetPadding(VincModelID m);                     /* Mac 0x4c18c7, Win 0x74f810 */
void MEAPI VincModelSetPadding(VincModelID m, MeReal padding);       /* Win 0x74f800 */
void MEAPI VincModelUpdateTransform(VincModelID m);                  /* Mac 0x4c18d2, Win 0x74f890 */
MeReal* MEAPI VincModelGetTransformPtr(VincModelID m);               /* Mac 0x4c18e9, Win 0x74faa0 */
void MEAPI VincModelSetTransformPtrRaw(VincModelID m, MeReal* tm);   /* Mac 0x4c18f4, Win 0x74fab0 */
void MEAPI VincModelSetTransformPtr(VincModelID m, MeReal* tm);      /* Mac 0x4c1902, Win 0x74fac0 */
MeReal* MEAPI VincModelGetEffectiveTransformPtr(VincModelID m);      /* Win 0x74fae0 */
void MEAPI VincModelSetLinearVelocityPtr(VincModelID m, MeReal* v);  /* Mac 0x4c191b, Win 0x74fb00 */
MeReal* MEAPI VincModelGetLinearVelocityPtr(VincModelID m);          /* Mac 0x4c1929, Win 0x74fb10 */
void MEAPI VincModelSetAngularVelocityPtr(VincModelID m, MeReal* w); /* Mac 0x4c1934, Win 0x74fb20 */
void MEAPI VincModelSetVelocityPtrs(VincModelID m, MeReal* v, MeReal* w); /* Mac 0x4c1942 */
MeReal* MEAPI VincModelGetAngularVelocityPtr(VincModelID m);         /* Mac 0x4c1970, Win 0x74fb30 */
void MEAPI VincModelSetUserData(VincModelID m, void* data);          /* Mac 0x4c197b, Win 0x74fb40 */
void* MEAPI VincModelGetUserData(VincModelID m);                     /* Mac 0x4c1989, Win 0x74fb50 */
void MEAPI VincModelSetGeometry(VincModelID m, VincGeometryID g);    /* Mac 0x4c1994 */
VincGeometryID MEAPI VincModelGetGeometry(VincModelID m);            /* Mac 0x4c19a1 */
void MEAPI VincModelSetRelativeTransformPtrs(VincModelID m, MeReal* relTM, MeReal* bodyTM,
                                       MeReal* tm);            /* Mac 0x4c19ab */
void MEAPI VincModelGetRelativeTransformPtrs(VincModelID m, MeReal** relTM,
                                       MeReal** bodyTM);       /* Mac 0x4c19e3, Win 0x74fc30 */
void MEAPI VincModelSetM7C(VincModelID m, void* p);                  /* Mac 0x4c19fb, Win 0x74fc70 */
void* MEAPI VincModelGetM7C(VincModelID m);                          /* Mac 0x4c1a09, Win 0x74fc90 */
void MEAPI VincModelSetM80(VincModelID m, void* p);                  /* Mac 0x4c1a14, Win 0x74fca0 */
void* MEAPI VincModelGetM80(VincModelID m);                          /* Mac 0x4c1a25, Win 0x74fcc0 */
void MEAPI VincModelSetDensity(VincModelID m, MeReal density);       /* Mac 0x4c1a33, Win 0x74fcf0 */
MeReal MEAPI VincModelGetDensity(VincModelID m);                     /* Mac 0x4c1a45, Win 0x74fd00 */
void MEAPI VincModelSetSweep(VincModelID m, int on);                 /* Win 0x74f820 */
int MEAPI VincModelGetSweep(VincModelID m);                          /* Win 0x74f840 */
void MEAPI VincModelSetUpdateTransformFn(VincModelID m, VincModelUpdateTransformFn fn); /* Win 0x74fcd0 */
VincModelUpdateTransformFn MEAPI VincModelGetUpdateTransformFn(VincModelID m);          /* Win 0x74fce0 */
/* the default updateTransformFn: transform = relTM * bodyTM (Mac 0x4c1a50, Win 0x74fc50) */
void MEAPI VincModelComposeRelativeTransform(VincModelID m);
/* AABB at the current transform, extended by the swept position after dt when the model sweeps
 * (Mac 0x4c1a74, Win 0x74f8f0) */
void MEAPI VincModelUpdate(VincModelID m, MeVector3 minCorner, MeVector3 maxCorner, MeReal dt);
/* AABB at the current transform (Win 0x74f8b0) */
void MEAPI VincModelUpdateAABB(VincModelID m, MeVector3 minCorner, MeVector3 maxCorner);
/* bounding sphere centre in world coordinates (Mac 0x4c1c27) */
void MEAPI VincModelGetBSphere(VincModelID m, MeVector3 center, MeReal* radius);
/* Mac 0x4c1d52: from a VinPool */
VincModelID MEAPI VincModelCreateFromPool(VincGeometryID g, struct VinPool* pool);
VincModelID MEAPI VincModelCreate(VincGeometryID g);                 /* Mac 0x4c1e58 */
void MEAPI VincModelDestroy(VincModelID m);                          /* Mac 0x4c1e85, Win 0x74f750 */
void MEAPI VincModelDestroyToPool(VincModelID m, struct VinPool* pool); /* Win 0x74f7d0 */
void MEAPI VincModelPoolDestroy(void);                               /* Mac 0x4c1ecf */
int MEAPI VincModelPoolInit(int modelCount);                         /* Mac 0x4c1ef5, Win 0x74f6e0 */
/* centre of rotation (world point when the model has a relTM: body frame), Mac 0x4c1f37, Win 0x74f680 */
void MEAPI VincModelSetCenterOfRotation(VincModelID m, const MeVector3 p);
/* mass properties: volume, mass = volume * density (aggregates: their mass * density), inertia
 * scaled by the mass (aggregates: by density) and rotated into the body frame (relTransform)
 * (Mac 0x4c2042, Win 0x74fd10); returns the geometry's status */
MeI16 MEAPI VincModelGetMassProperties(VincModelID m, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume,
                               MeReal* mass);
/* Mac 0x54d24d (TU 558e; name inferred): mass
 * properties of `count` models together: relTM (COM frame), inertia (per unit mass), the total
 * volume and mass (volume * density per model). VxPart Mac 0x46a0a4 / 0x46a7cc. */
MeI16 MEAPI VincModelsGetMassProperties(VincModelID* models, int count, MeMatrix4 relTM,
                                        MeMatrix3 inertia, MeReal* volume, MeReal* mass);

/* ---- aggregates (VincAggregate.cpp; the aggregate geometry itself lives in the space item) ---- */
int MEAPI VincModelIsAggregate(VincModelID m);                          /* Mac 0x4bfbdd, Win 0x7747c0 */
VincModelID MEAPI VincAggregateGetElementModel(VincModelID m, int i);   /* Mac 0x4bfc00, Win 0x774730 */
void MEAPI VincAggregateSetVelocityPtrs(VincModelID m, MeReal* v, MeReal* w); /* Mac 0x4bfc1c */
void MEAPI VincAggregateModelUpdate(VincModelID m);                     /* Mac 0x4bfc71, Win 0x7746e0 */
int MEAPI VincAggregateRemoveElement(VincModelID m, int i);             /* Mac 0x4bfc86 */
int MEAPI VincAggregateAddElement(VincModelID m, VincModelID element, MeReal* relTM); /* Mac 0x4bfcea, Win 0x7746a0 */
void MEAPI VincAggregateModelDestroy(VincModelID m);                    /* Mac 0x4bfd44, Win 0x774650 */
void MEAPI VincAggregateModelDestroyToPool(VincModelID m, struct VinPool* pool); /* Win 0x774680 */
VincModelID MEAPI VincModelGetParent(VincModelID m);                    /* Win 0x7747e0 */
VincModelID MEAPI VincAggregateModelCreate(int maxElements);            /* Mac 0x4bfd82 */
void MEAPI VincAggregateModelUpdateTransforms(VincModelID m);              /* Win 0x774700 */
MeReal* MEAPI VincAggregateGetElementTransform(VincModelID m, int i);      /* Win 0x774750 */
int MEAPI VincAggregateModelGetElementCount(VincModelID m);              /* Win 0x774780 */
int MEAPI VincAggregateModelGetMaxElementCount(VincModelID m);           /* Win 0x7747a0 */
VincModelID* MEAPI VincAggregateModelReleaseElements(VincModelID m);     /* Win 0x7747f0 */

#ifdef __cplusplus
}
#endif

#endif
