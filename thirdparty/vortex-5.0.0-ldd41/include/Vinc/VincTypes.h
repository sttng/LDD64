// Vortex 5.0.0 build 800 "vortexldd41" - Vinc (collision) handles and known struct offsets
// (restored header).
//
// Vinc is the C collision library (successor of MathEngine Mcd). The Vx layer keeps opaque
// pointers; the structs are in the other Vinc headers (V3.x). "Mcd" type names follow the
// MathEngine Collision Toolkit API (names inferred).
#ifndef VINC_VINCTYPES_H
#define VINC_VINCTYPES_H

struct McdModel;    // collision model (one per VxCollisionGeometry)
struct McdGeometry; // shape (one per VxGeometry)
struct McdSpace;    // broad phase space (CxSmallSort), VxUniverse +0x60
typedef struct McdModel* McdModelID;      /* ("struct" so that C sources can include this) */
typedef struct McdGeometry* McdGeometryID;
typedef struct McdSpace* McdSpaceID;

#ifdef __cplusplus
namespace Vinc {
#endif

// McdModel offsets (Mac / Win) as first located; -1 = not located then (struct McdModel in
// Vinc/VincModel.h has the full layout: Win sweep +0xa0, ma4 +0xa8).
enum McdModelOffsets {
#if defined(_WIN32)
    kMcdModelParent = 0x44,     // Win VxCollisionGeometry::update 0x710430
    kMcdModelUserCG = 0x50,     // the owning VxCollisionGeometry
    kMcdModelSpace = 0x58,      // != 0 when in a space
    kMcdModelSweep = -1,
    kMcdModelA4 = -1
#else
    kMcdModelParent = 0x40,     // Mac 0x437da2 (parent aggregate model)
    kMcdModelUserCG = 0x4c,     // Mac 0x437da2 (owning VxCollisionGeometry)
    kMcdModelSpace = 0x54,      // Mac 0x437f5c (!= 0 when in a space)
    kMcdModelSweep = 0x9c,      // Mac 0x437efc (int bool)
    kMcdModelA4 = 0xa4          // Mac 0x437f26
#endif
};

// VxCollisionPairRequest record (VxUniverse::getCollisionPairRequest, Mac 0x434626): the
// request object is a pointer to it (VxCollisionPairRequest.h).
enum McdPairRequestOffsets {
    kPairRequestContactMaxCount = 0x00, // <= 0x400 (Mac 0x476b14 / Win 0x722f90)
    kPairRequestGroupMaxCount = 0x18    // <= 6 (Mac 0x476ace / Win 0x723070)
};

#ifdef __cplusplus
} // namespace Vinc
#endif

#endif
