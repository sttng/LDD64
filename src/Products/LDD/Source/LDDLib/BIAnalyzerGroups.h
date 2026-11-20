// LEGO Digital Designer — restored source
// LEGO::BIAnalyzerGroups — proposes the user's brick groups (BrickGroup, with their
//   sub-groups) whose parts are all in the graph and connected.
//
// Home TU 222 "BIAnalyzerGroups.cpp" (Mac 0x2672a0-0x267cef).
// RTTI: class LEGO::BIAnalyzerGroups : BIAnalyzer. Mac sizeof 0x14 + Object (m_minSize +0x10), vtable
// 0x73707c {GetFacadeType 0x2230a0, GetName 0x267a40, D1 0x267a70, D0 0x267a90, Analyze 0x2674a0,
// GetWeight 0x223110, GetAttributes 0x226760}; Win m_minSize +0x14, vtable 0xa2c588
// {0x5913d0, 0x591750, 0x5a10b0, 0x5913e0, 0x559e90}, destructor 0x5917b0, sdd 0x591820. The constructor
// is inlined into BIProducer::BIProducer (Mac 0x236b40).
#pragma once
#include "BIAnalyzer.h"

namespace LEGO {

class BrickGroup;
class PrimitiveElement;

class BIAnalyzerGroups : public BIAnalyzer {
public:
    // (names inferred)
    BIAnalyzerGroups(BIProducer* producer, Real32 priority, Real32 weight, UInt32 minSize)
        : BIAnalyzer(producer, priority, weight), m_minSize(minSize) {}
    virtual ~BIAnalyzerGroups();                                  // Mac 0x267a70 / 0x267a90, Win 0x5917b0 / sdd 0x591820

    virtual String GetName() const;                    // Mac 0x267a40, Win 0x591750 ("Groups")
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // Mac 0x2674a0, Win 0x5a10b0

    // Adds `group` and all its sub-groups. Mac 0x2672a0, Win 0x5a0eb0 (name inferred)
    static void CollectGroups(BrickGroup* group, BasicSet<BrickGroup*>& groups);
    // The primitives of every part of `group` and its sub-groups; false when the group (or a
    // sub-group) has no parts or a part element is not in `elements`. Mac 0x2672f0,
    // Win 0x5a0f20 (name inferred)
    static bool CollectPrimitives(BrickGroup* group, const BasicArray<PrimitiveElement*>& elements,
                                  BasicSet<BIBrickPrimitive*>& primitives);

    UInt32 m_minSize;   // Mac +0x10 / Win +0x14 (name inferred)
};

} // namespace LEGO
