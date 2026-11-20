// LEGO Digital Designer — restored source
// LEGO::BGAnalyzerSkeleton — BGAnalyzerMinifig for skeleton / robot figures: heads by main group
//   245 or design ID, torsos, legs and skeleton arms by design ID only. GetName not overridden
//   ("Minifig"). All names inferred.
//
// Home TU 365 "BGAnalyzerSkeleton.cpp" (Mac 0x34c7d0-0x34ce4f, Win 0x5a0970-0x5a0daf).
// RTTI: class LEGO::BGAnalyzerSkeleton : BGAnalyzerMinifig. Mac vtable 0x74d68c {0x2230a0,
// 0x3436e0, D1 0x34cc60, D0 0x34cd20, 0x343710, 0x223110, 0x344590, idx 7-11: 0x34c860,
// 0x34ca20, 0x34cb10, 0x34cb60, 0x34cbe0}; complete constructor 0x34c7d0. Win vtable 0xa2c8d4
// {.., 0x5a09e0, 0x5a0bb0, 0x5a0c60, 0x5a0cb0, 0x5a0d30}, sdd 0x5a0970.
#pragma once
#include "BGAnalyzerMinifig.h"

namespace LEGO {

class BGAnalyzerSkeleton : public BGAnalyzerMinifig {
public:
    // Mac 0x34c7d0, inlined on Win
    BGAnalyzerSkeleton(BIProducer* producer, Real32 priority, Real32 weight);
    virtual ~BGAnalyzerSkeleton();                                         // Mac 0x34cc60 / 0x34cd20, Win sdd 0x5a0970

    // As BGAnalyzerMinifig, with the skeleton arms. Mac 0x34c860, Win 0x5a09e0
    virtual void CollectPart(BIGraph* graph, BIBrickPrimitive* torso, BIBrickPrimitive* part,
                             PrimitiveSet& result);                        // idx 7
    // Mac 0x34ca20, Win 0x5a0bb0
    virtual bool IsMinifigPart(BIBrickPrimitive* primitive);               // idx 8
    // Main group 245 or designs 85946 / 30529. Mac 0x34cb10, Win 0x5a0c60
    virtual bool IsHead(BIBrickPrimitive* primitive);                      // idx 9
    // Designs 30375, 6260, 60115, 54275, 53988. Mac 0x34cb60, Win 0x5a0cb0
    virtual bool IsTorso(BIBrickPrimitive* primitive);                     // idx 10
    // Designs 6266, 30530, 30376, 53984, 54276. Mac 0x34cbe0, Win 0x5a0d30
    virtual bool IsLegs(BIBrickPrimitive* primitive);                      // idx 11

    // Designs 6265, 30377, 59230, 53989. Inlined on Mac, Win 0x5a0db0
    bool IsSkeletonArm(BIBrickPrimitive* primitive);
    // The arm and, per neighbour, CollectHeldItem (held items) or CollectAttached. Mac 0x34c970,
    // Win 0x5a0af0
    void CollectSkeletonArm(BIGraph* graph, BIBrickPrimitive* arm, PrimitiveSet& result);
};

} // namespace LEGO
