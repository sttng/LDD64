/* Vortex 5.0.0 build 800 "vortexldd41" - Vinc primitive geometry types (restored;
 * sources src/Vinc/VincBox.c, VincSphere.c, VincPlane.c, VincCylinder.c, VincCapsule.c,
 * VincCone.c, VincTriangleList.c, VincPrimitives.c).
 *
 * Vinc is the C collision library Vortex inherited from the MathEngine Collision Toolkit (Mcd).
 * The primitive types keep the Mcd design: a geometry is a small heap block that starts with the
 * 16-bit type id (VincGeometryInit), followed by the type's own data; each type registers six
 * callbacks with VincGeometryRegisterType (the type-name strings "Box", "Sphere", ... and the
 * function "VincGeometryRegisterType" are in both binaries). The C API names follow the Mcd names
 * (McdBoxCreate -> VincBoxCreate, ...) and are all inferred unless stated otherwise.
 *
 * Doubles are 4-aligned inside structs on Mac i386 and 8-aligned on Win, so every field moves:
 * "Mac +0x04 / Win +0x08" etc. The struct definitions below give both layouts by themselves.
 *
 * Type ids (fixed by the registration calls, both builds):
 *   Plane 0, Sphere 1, Box 2, Cylinder 3, Cone 4, TriangleList 5, Capsule 6
 *   (8 RGHeightField, 9 Convex, 10 TriangleMesh, 11 Composite belong to other items).
 * Transforms ("TM") are MeMatrix4 = double[4][4], rows 0..2 = axes, row 3 = position. */
#ifndef VINC_VINCPRIMITIVES_H
#define VINC_VINCPRIMITIVES_H

#include "Me/MeMath.h"
#include "Me/MeCall.h"
#include "Vinc/VincGeometry.h"   /* VincGeometry (type id head), registry */

#ifdef __cplusplus
extern "C" {
#endif

/* Box: Mac new(0x24) 0x4f63bf, Win new(0x28) 0x770830 */
typedef struct VincBox {
    VincGeometry base;
    MeReal radii[3];            /* Mac +0x04 / Win +0x08: half extents */
    MeReal boundingRadius;      /* Mac +0x1c / Win +0x20: |radii| */
} VincBox;

/* Sphere: Mac new(0x0c) 0x4f87ee, Win new(0x10) 0x774280 */
typedef struct VincSphere {
    VincGeometry base;
    MeReal radius;              /* Mac +0x04 / Win +0x08 */
} VincSphere;

/* Plane: header only (the plane is z = 0 of its transform). */
typedef struct VincPlane {
    VincGeometry base;
} VincPlane;

/* Cylinder (axis = z): Mac new(0x28) 0x4f7ac5, Win new(0x30) 0x770210 */
typedef struct VincCylinder {
    VincGeometry base;
    MeReal radius;              /* Mac +0x04 / Win +0x08 */
    MeReal halfHeight;          /* Mac +0x0c / Win +0x10 */
    MeReal boundingRadius;      /* Mac +0x14 / Win +0x18: sqrt(r^2 + h^2) */
    MeReal cosTolerance;        /* Mac +0x1c / Win +0x20: 0.9999999 at creation; used by the
                                   triangle-list test (name inferred) */
    int flag;                   /* Mac +0x24 / Win +0x28: 1 at creation, not read in Vinc */
} VincCylinder;

/* Capsule (axis = z, segment of half length halfHeight + spheres of radius): Mac new(0x1c)
 * 0x4f6a90, Win new(0x20) 0x76a870 */
typedef struct VincCapsule {
    VincGeometry base;
    MeReal radius;              /* Mac +0x04 / Win +0x08 */
    MeReal halfHeight;          /* Mac +0x0c / Win +0x10 */
    MeReal reserved;            /* Mac +0x14 / Win +0x18: allocated, never written */
} VincCapsule;

/* Cone (axis = z, apex up): Win new(0x30) 0x7b7a50 / 0x7b7aa0, init Win 0x7b7570. The centre of
 * mass is the origin: apex at +top (3/4 h), base at -bottom (1/4 h). */
typedef struct VincCone {
    VincGeometry base;
    MeReal top;                 /* Mac +0x04 / Win +0x08: 0.75 h */
    MeReal bottom;              /* Mac +0x0c / Win +0x10: 0.25 h */
    MeReal radius;              /* Mac +0x14 / Win +0x18: base radius */
    MeReal user;                /* Mac +0x1c / Win +0x20: 4th create argument (name inferred) */
    MeReal sinHalfAngle;        /* Mac +0x24 / Win +0x28: r / sqrt(h^2 + r^2) (1 if h <= 0); the
                                   support point is the apex when dir . axis > this */
} VincCone;

/* Triangle list: a box-bounded geometry whose triangles come from a user callback, called by the
 * pair tests with the query box (in the triangle list's frame) and filling `triangles` /
 * `triangleCount`. Win new(0x88) 0x7adb60 (a call is in Mac 0x4bc96d). */
struct McdModelPair;
struct VincTriangleList;
typedef struct VincTriangleListTri VincTriangleListTri; /* stride 0x2c (Mac): v0*, v1*, v2*,
                                                           normal*, ..., +0x28 int user id */
/* Callbacks (stdcall on Win: no stack cleanup after the calls in Win 0x759060). */
typedef int (MEAPI *VincTriangleListGenerator)(struct McdModelPair* pair, MeReal* min, MeReal* max,
                                               int flag);
/* the triangles within `radius` of `center` (sphere tests, Mac 0x4f4800 / Win 0x7dc730) */
typedef int (MEAPI *VincTriangleListSphereGenerator)(struct McdModelPair* pair, const MeReal* center,
                                                     MeReal radius, int flag);
/* fetch one triangle by index into `triangles` (Win 0x759060: (pair, 1, &index, geometry)) */
typedef int (MEAPI *VincTriangleListGetTriangleFn)(struct McdModelPair* pair, int count, int* indices,
                                                   struct VincTriangleList* g);
typedef struct VincTriangleList {
    VincGeometry base;
    MeReal radii[3];            /* Mac +0x04 / Win +0x08: half extents of the bounding box */
    MeReal min[3];              /* Mac +0x1c / Win +0x20: local box min */
    MeReal max[3];              /* Mac +0x34 / Win +0x38: local box max */
    MeReal boundingRadius;      /* Mac +0x4c / Win +0x50 */
    MeReal stepLength;          /* Mac +0x54 / Win +0x58: 0 at creation; line segment queries
                                   walk the segment in steps of this length (> 0) */
    VincTriangleListTri* triangles; /* Mac +0x5c / Win +0x60 (filled by the generators) */
    int triangleCount;          /* Mac +0x60 / Win +0x64 */
    int triangleMaxCount;       /* Mac +0x64 / Win +0x68: capacity of `triangles` */
    int m68;                    /* Mac +0x68 / Win +0x6c */
    void* m6c;                  /* Mac +0x6c / Win +0x70 (get/set Win 0x7ada40 / 0x7ada50) */
    int m70;                    /* Mac +0x70 / Win +0x74: -1 */
    VincTriangleListSphereGenerator sphereGenerator; /* Mac +0x74 / Win +0x78: create argument
                                   (required), used by the sphere / cylinder tests */
    VincTriangleListGenerator generator; /* Mac +0x78 / Win +0x7c: box query, used by the other
                                   pair tests (e.g. Mac 0x4bc96d) */
    VincTriangleListGenerator lineSegmentGenerator; /* Mac +0x7c / Win +0x80: used by the line
                                   segment query (Mac 0x4f52ad) */
    VincTriangleListGetTriangleFn getTriangle; /* Mac +0x80 / Win +0x84 */
} VincTriangleList;

/* ---- type ids (Mac globals 0x7797c0.., Win 0xabd8b8..) and registration ---- */
MeI16 MEAPI VincBoxGetTypeId(void);          /* Mac 0x4f5f9d, Win 0x7702b0 */
MeI16 MEAPI VincSphereGetTypeId(void);       /* Mac 0x4f8664, Win 0x774270 */
MeI16 MEAPI VincPlaneGetTypeId(void);        /* Mac 0x4f8042, Win 0x803040 */
MeI16 MEAPI VincCylinderGetTypeId(void);     /* Mac 0x4f76df, Win 0x76fbd0 */
MeI16 MEAPI VincCapsuleGetTypeId(void);      /* Mac 0x4f66f2, Win 0x76a280 */
MeI16 MEAPI VincConeGetTypeId(void);         /* Mac 0x4f6ea7, Win 0x7b7560 */
MeI16 MEAPI VincTriangleListGetTypeId(void); /* Mac 0x4f8a18, Win 0x7ad530 */

void MEAPI VincBoxRegisterType(void);          /* Mac 0x4f6447, Win 0x7707f0 */
void MEAPI VincSphereRegisterType(void);       /* Mac 0x4f8843, Win 0x774510 */
void MEAPI VincPlaneRegisterType(void);        /* Mac 0x4f8475, Win 0x803440 */
void MEAPI VincCylinderRegisterType(void);     /* Mac 0x4f7b4d, Win 0x7701d0 */
void MEAPI VincCapsuleRegisterType(void);      /* Mac 0x4f6b03, Win 0x76a830 */
void MEAPI VincConeRegisterType(void);         /* Mac 0x4f70ac, Win 0x7b7a10 */
void MEAPI VincTriangleListRegisterType(void); /* Mac 0x4f8edb, Win 0x7adb20 */

void MEAPI VincSphereBoxPlaneRegisterTypes(void);        /* Mac 0x4f8a01, Win 0x7b7b10 */
void MEAPI VincSphereBoxPlaneRegisterInteractions(void); /* Mac 0x4f89d1 */
void MEAPI VincPrimitivesRegisterTypes(void);            /* Mac 0x4f8643, Win 0x75cd00 */
int MEAPI VincPrimitivesRegisterInteractions(void);      /* Mac 0x4f85eb */
void MEAPI VincCapsuleRegisterTypeAndInteractions(void); /* Mac 0x4f6b6f, Win 0x76a8b0 */

/* ---- pair-test registrations (each registers one VincIntersectFn pair with
 * VincGeometryPairRegisterInteractions(A, B, ...); the name is Vinc<A><B>; return 1). 557g. */
int MEAPI VincCylinderPlaneRegisterInteraction(void);        /* Mac 0x4f0da2, Win 0x7e0ac0 */
int MEAPI VincCylinderSphereRegisterInteraction(void);       /* Mac 0x4f1815, Win 0x7e04b0 */
int MEAPI VincCylinderTriangleListRegisterInteraction(void); /* Mac 0x4f1eb8, Win 0x7dbff0 */
int MEAPI VincSpherePlaneRegisterInteraction(void);          /* Mac 0x4f3b75, Win 0x81f8b0 */
int MEAPI VincSphereSphereRegisterInteraction(void);         /* Mac 0x4f411e, Win 0x8204b0 */
int MEAPI VincSphereTriangleListRegisterInteraction(void);   /* Mac 0x4f478a, Win 0x7dcd20 */
/* line segment queries (VincGeometryRegisterLineSegmentFn) */
void MEAPI VincCylinderRegisterLineSegment(void);            /* Mac 0x4efd81, Win 0x7dda40 */
void MEAPI VincPlaneRegisterLineSegment(void);               /* Mac 0x4f3069, Win 0x7ad2d0 */
void MEAPI VincSphereRegisterLineSegment(void);              /* Mac 0x4f3094, Win 0x7ad2b0 */
void MEAPI VincBoxRegisterLineSegment(void);                 /* Mac 0x4f30bf, Win 0x7ad470 */
int MEAPI VincTriangleListRegisterLineSegment(void);         /* Mac 0x4f527d, Win 0x759890 */
/* implemented by the other pair-test items (557d-f); declared here because
 * VincPrimitivesRegisterInteractions / VincCapsuleRegisterTypeAndInteractions call them: */
int MEAPI VincBoxBoxRegisterInteraction(void);               /* Mac 0x4dc35b, Win 0x81d330 */
int MEAPI VincBoxCapsuleRegisterInteraction(void);           /* Mac 0x4dd24a, Win 0x7dac10 */
int MEAPI VincBoxCylinderRegisterInteraction(void);          /* Mac 0x4ddcb9, Win 0x7e1eb0 */
int MEAPI VincBoxPlaneRegisterInteraction(void);             /* Mac 0x4e0639, Win 0x820020 */
int MEAPI VincBoxSphereRegisterInteraction(void);            /* Mac 0x4e1141, Win 0x81e790 */
int MEAPI VincBoxTriangleListRegisterInteraction(void);      /* Mac 0x4e17c9, Win 0x7dc6d0 */
int MEAPI VincCapsuleCapsuleRegisterInteraction(void);       /* Mac 0x4e282f, Win 0x7d8330 */
void MEAPI VincCapsuleRegisterLineSegment(void);             /* Mac 0x4e3b23, Win 0x7db8e0 */
int MEAPI VincCapsulePlaneRegisterInteraction(void);         /* Mac 0x4e429f, Win 0x7db660 */
int MEAPI VincCapsuleSphereRegisterInteraction(void);        /* Mac 0x4e4a1f, Win 0x7db120 */
int MEAPI VincCapsuleTriangleListRegisterInteraction(void);  /* Mac 0x4e5387, Win 0x7d78d0 */
int MEAPI VincConeBoxRegisterInteraction(void);              /* Mac 0x4e5da3, Win 0x7e2440 */
int MEAPI VincConeConeRegisterInteraction(void);             /* Mac 0x4e5e61, Win 0x7e23b0 */
int MEAPI VincConeCylinderRegisterInteraction(void);         /* Mac 0x4e5f1f, Win 0x7dd420 */
void MEAPI VincConeRegisterLineSegment(void);                /* Mac 0x4e5fdd, Win 0x7e2350 */
int MEAPI VincConePlaneRegisterInteraction(void);            /* Mac 0x4e674d, Win 0x7e3010 */
int MEAPI VincConeSphereRegisterInteraction(void);           /* Mac 0x4e729b, Win 0x7e2ae0 */
int MEAPI VincCylinderCapsuleRegisterInteraction(void);      /* Mac 0x4e8002, Win 0x7da460 */
int MEAPI VincCylinderCylinderRegisterInteraction(void);     /* Mac 0x4ebcd5, Win 0x7dff70 */

/* ---- Box ---- */
VincGeometryID MEAPI VincBoxCreate(MeReal dx, MeReal dy, MeReal dz);               /* Mac 0x4f63bf, Win 0x770830 */
void MEAPI VincBoxSetDimensions(VincGeometryID g, MeReal dx, MeReal dy, MeReal dz); /* Mac 0x4f6366, Win 0x7702c0 */
void MEAPI VincBoxGetDimensions(VincGeometryID g, MeReal* dx, MeReal* dy, MeReal* dz); /* Mac 0x4f5faf, Win 0x770310 */
MeReal* MEAPI VincBoxGetRadii(VincGeometryID g);                                    /* Mac 0x4f5fe7, Win 0x770350 */
MeReal MEAPI VincBoxGetBoundingRadius(VincGeometryID g);                            /* Win 0x770340 */
void MEAPI VincBoxMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out); /* Mac 0x4f62b4 */
/* registered callbacks */
void MEAPI VincBoxDestroy(VincGeometryID g);                                          /* Mac 0x4f5ff2, Win 0x770360 */
void MEAPI VincBoxGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);             /* Mac 0x4f6018, Win 0x770380 */
void MEAPI VincBoxUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f632a, Win 0x770410 */
void MEAPI VincBoxGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);           /* Mac 0x4f662c, Win 0x770780 */
void MEAPI VincBoxGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius);     /* Mac 0x4f612b, Win 0x7704b0 */
void MEAPI VincBoxMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f64b3, Win 0x770500 */
int MEAPI VincBoxGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f6199, Win 0x770610 */

/* ---- Sphere ---- */
VincGeometryID MEAPI VincSphereCreate(MeReal radius);         /* Mac 0x4f87ee, Win 0x774280 */
void MEAPI VincSphereSetRadius(VincGeometryID g, MeReal r);   /* Mac 0x4f8676 */
MeReal MEAPI VincSphereGetRadius(VincGeometryID g);           /* Mac 0x4f8688, Win 0x7742d0 */
void MEAPI VincSphereDestroy(VincGeometryID g);                                     /* Mac 0x4f8693, Win 0x7742e0 */
void MEAPI VincSphereGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);        /* Win 0x774300 */
void MEAPI VincSphereUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f8945, Win 0x774350 */
void MEAPI VincSphereGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);      /* Mac 0x4f88fd, Win 0x7743b0 */
void MEAPI VincSphereGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4f86b9, Win 0x7743f0 */
void MEAPI VincSphereMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f88af, Win 0x774410 */
int MEAPI VincSphereGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f86e2, Win 0x774450 */

/* ---- Plane ---- */
void MEAPI VincPlaneDestroy(VincGeometryID g);                                      /* Mac 0x4f8054, Win 0x803080 */
void MEAPI VincPlaneGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);         /* Mac 0x4f8076, Win 0x803090 */
void MEAPI VincPlaneUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f8439, Win 0x803220 */
void MEAPI VincPlaneGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);       /* Mac 0x4f83d1, Win 0x803250 */
void MEAPI VincPlaneGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4f82a2, Win 0x8032a0 */
void MEAPI VincPlaneMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f8553, Win 0x8032c0 */
int MEAPI VincPlaneGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f82cc, Win 0x803330 */
void MEAPI VincPlaneGetNormal(VincGeometryID g, MeMatrix4 tm, MeVector3 normal);    /* Mac 0x4f8383, Win 0x8033a0 */
MeReal MEAPI VincPlaneGetDistance(VincGeometryID g, MeMatrix4 tm, const MeVector3 p); /* Mac 0x4f84e1, Win 0x8033e0 */

/* ---- Cylinder ---- */
VincGeometryID MEAPI VincCylinderCreate(MeReal radius, MeReal height);              /* Mac 0x4f7ac5, Win 0x770210 */
void MEAPI VincCylinderSetDimensions(VincGeometryID g, MeReal radius, MeReal height); /* Mac 0x4f79e0, Win 0x76fc70 */
void MEAPI VincCylinderSetHeight(VincGeometryID g, MeReal height);                  /* Mac 0x4f7a1f, Win 0x76fc40 */
void MEAPI VincCylinderSetRadius(VincGeometryID g, MeReal radius);                  /* Mac 0x4f7a5d, Win 0x76fc10 */
MeReal MEAPI VincCylinderGetRadius(VincGeometryID g);                               /* Mac 0x4f76f1, Win 0x76fbe0 */
MeReal MEAPI VincCylinderGetHeight(VincGeometryID g);                               /* Mac 0x4f76fc, Win 0x76fbf0 */
MeReal MEAPI VincCylinderGetHalfHeight(VincGeometryID g);                           /* Mac 0x4f7718, Win 0x76fc00 */
MeReal MEAPI VincCylinderGetBoundingRadius(VincGeometryID g);                       /* Win 0x76feb0 */
void MEAPI VincCylinderSetCosTolerance(VincGeometryID g, MeReal c);                 /* Mac 0x4f7888, Win 0x7701b0 */
MeReal MEAPI VincCylinderGetCosTolerance(VincGeometryID g);                         /* Mac 0x4f789a, Win 0x7701c0 */
void MEAPI VincCylinderMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out); /* Mac 0x4f7c8f, Win 0x770070 */
void MEAPI VincCylinderGetEndpoints(VincGeometryID g, MeMatrix4 tm, MeVector3 top, MeVector3 bottom); /* Mac 0x4f7f55, Win 0x76fec0 */
void MEAPI VincCylinderDestroy(VincGeometryID g);                                   /* Mac 0x4f7723, Win 0x76fca0 */
void MEAPI VincCylinderGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);      /* Mac 0x4f78a5, Win 0x76fcc0 */
void MEAPI VincCylinderUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f7a89, Win 0x76fdb0 */
void MEAPI VincCylinderGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);    /* Mac 0x4f7bb9, Win 0x76fde0 */
void MEAPI VincCylinderGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4f7749, Win 0x76fe90 */
void MEAPI VincCylinderMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f7d65, Win 0x76ff60 */
int MEAPI VincCylinderGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f7772, Win 0x770100 */

/* ---- Capsule ---- */
VincGeometryID MEAPI VincCapsuleCreate(MeReal radius, MeReal height);               /* Mac 0x4f6a90, Win 0x76a870 */
void MEAPI VincCapsuleSetDimensions(VincGeometryID g, MeReal radius, MeReal height); /* Mac 0x4f674c, Win 0x76a2f0 */
void MEAPI VincCapsuleSetRadius(VincGeometryID g, MeReal radius);                   /* Mac 0x4f671a, Win 0x76a2c0 */
void MEAPI VincCapsuleSetHeight(VincGeometryID g, MeReal height);                   /* Mac 0x4f672c, Win 0x76a2d0 */
MeReal MEAPI VincCapsuleGetRadius(VincGeometryID g);                                /* Mac 0x4f6704, Win 0x76a290 */
MeReal MEAPI VincCapsuleGetHalfHeight(VincGeometryID g);                            /* Mac 0x4f670f, Win 0x76a2b0 */
MeReal MEAPI VincCapsuleGetHeight(VincGeometryID g);                                /* Win 0x76a2a0 */
MeReal MEAPI VincCapsuleGetBoundingRadius(VincGeometryID g);                        /* Mac 0x4f67ca, Win 0x76a540 */
void MEAPI VincCapsuleGetEndpoints(VincGeometryID g, MeMatrix4 tm, MeVector3 top, MeVector3 bottom); /* Mac 0x4f6c79, Win 0x76a550 */
void MEAPI VincCapsuleMaximumPointLocal(VincGeometryID g, const MeVector3 dir, MeVector3 out); /* Win 0x76a6c0 */
void MEAPI VincCapsuleDestroy(VincGeometryID g);                                    /* Mac 0x4f6776, Win 0x76a310 */
void MEAPI VincCapsuleGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);       /* Mac 0x4f6915, Win 0x76a330 */
void MEAPI VincCapsuleUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f6a54, Win 0x76a430 */
void MEAPI VincCapsuleGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);     /* Mac 0x4f6b9f, Win 0x76a460 */
void MEAPI VincCapsuleGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4f679c, Win 0x76a510 */
void MEAPI VincCapsuleMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f6d70, Win 0x76a600 */
int MEAPI VincCapsuleGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f67e7, Win 0x76a770 */

/* ---- Cone ---- */
VincGeometryID MEAPI VincConeCreate(MeReal height, MeReal radius, MeReal user);     /* Win 0x7b7a50 */
VincGeometryID MEAPI VincConeCreateSimple(MeReal height, MeReal radius);            /* Win 0x7b7aa0 */
void MEAPI VincConeInit(VincGeometryID g, MeReal height, MeReal radius, MeReal user); /* Win 0x7b7570 */
MeReal MEAPI VincConeGetBottom(VincGeometryID g);                                   /* Mac 0x4f6eb9 */
MeReal MEAPI VincConeGetTop(VincGeometryID g);                                      /* Mac 0x4f6ec4 */
MeReal MEAPI VincConeGetHeight(VincGeometryID g);                                   /* Mac 0x4f6ecf */
MeReal MEAPI VincConeGetRadius(VincGeometryID g);                                   /* Mac 0x4f6eec */
void MEAPI VincConeDestroy(VincGeometryID g);                                       /* Mac 0x4f6ef7, Win 0x7b7610 */
void MEAPI VincConeGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]);          /* Mac 0x4f743f, Win 0x7b7780 */
void MEAPI VincConeUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f76a3, Win 0x7b7880 */
void MEAPI VincConeGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]);        /* Mac 0x4f763b, Win 0x7b78b0 */
void MEAPI VincConeGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius);  /* Mac 0x4f6f1d, Win 0x7b7900 */
void MEAPI VincConeMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f7118, Win 0x7b7630 */
int MEAPI VincConeGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f6f80, Win 0x7b7950 */

/* ---- TriangleList ---- */
VincGeometryID MEAPI VincTriangleListCreate(MeReal dx, MeReal dy, MeReal dz, VincTriangleListSphereGenerator gen); /* Win 0x7adb60 */
VincGeometryID MEAPI VincTriangleListCreateWithGenerator(MeReal dx, MeReal dy, MeReal dz, VincTriangleListSphereGenerator sphereGen,
                                                   VincTriangleListGenerator gen,
                                                   VincTriangleListGenerator lineSegmentGen); /* Win 0x7adbf0 */
void MEAPI VincTriangleListSetDimensions(VincGeometryID g, MeReal dx, MeReal dy, MeReal dz); /* Win 0x7ad540 */
void MEAPI VincTriangleListSetBounds(VincGeometryID g, const MeVector3 min, const MeVector3 max); /* Win 0x7ad5c0 */
void MEAPI VincTriangleListGetDimensions(VincGeometryID g, MeReal* dx, MeReal* dy, MeReal* dz); /* Win 0x7ad640 */
MeReal MEAPI VincTriangleListGetBoundingRadius(VincGeometryID g);                  /* Win 0x7ad670 */
MeReal* MEAPI VincTriangleListGetRadii(VincGeometryID g);                          /* Win 0x7ad680 */
void* MEAPI VincTriangleListGetM6c(VincGeometryID g);                              /* Win 0x7ada40 */
void MEAPI VincTriangleListSetM6c(VincGeometryID g, void* p);                      /* Win 0x7ada50 */
void MEAPI VincTriangleListSetTriangles(VincGeometryID g, VincTriangleListTri* t, int count, int maxCount); /* Win 0x7ada60 */
void MEAPI VincTriangleListGetTriangles(VincGeometryID g, VincTriangleListTri** t, int* count, int* maxCount); /* Win 0x7ada80 */
void MEAPI VincTriangleListSetSphereGenerator(VincGeometryID g, VincTriangleListSphereGenerator gen); /* Win 0x7adae0 */
void MEAPI VincTriangleListSetGenerator(VincGeometryID g, VincTriangleListGenerator gen); /* Win 0x7adaf0 */
void MEAPI VincTriangleListSetGetTriangleFn(VincGeometryID g, VincTriangleListGetTriangleFn fn); /* Win 0x7adb00 */
void MEAPI VincTriangleListDestroy(VincGeometryID g);                              /* Mac 0x4f8a2a, Win 0x7ad690 */
void MEAPI VincTriangleListGetAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[6]); /* Mac 0x4f8a50, Win 0x7ad6b0 */
void MEAPI VincTriangleListUpdateAABB(VincGeometryID g, MeMatrix4 tm, MeVector3 min, MeVector3 max, MeReal box[6]); /* Mac 0x4f8e9f, Win 0x7ad7a0 */
void MEAPI VincTriangleListGetXYAABB(VincGeometryID g, MeMatrix4 tm, MeReal box[4]); /* Mac 0x4f8bee, Win 0x7ad7d0 */
void MEAPI VincTriangleListGetBSphere(VincGeometryID g, MeVector3 center, MeReal* radius); /* Mac 0x4f8d2f, Win 0x7ad870 */
void MEAPI VincTriangleListMaximumPoint(VincGeometryID g, MeMatrix4 tm, const MeVector3 dir, MeVector3 out); /* Mac 0x4f8f47, Win 0x7ad8b0 */
int MEAPI VincTriangleListGetMassProperties(VincGeometryID g, MeMatrix4 relTM, MeMatrix3 inertia, MeReal* volume); /* Mac 0x4f8d8c, Win 0x7ad990 */

/* ---- shared helper ---- */
MeReal MEAPI VincSign(const MeReal* x); /* Mac 0x4f6154, Win 0x7704d0: -1 / 0 / +1 (name inferred) */

#ifdef __cplusplus
}
#endif

#endif
