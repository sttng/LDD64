// LEGO Digital Designer — restored source
// LEGO::BIAnalyzerRigid — proposes the primitives of the rigid bodies of the physics scene
//   (RigidElement): one substep per PrimitiveRigidElement and one per rigid body.
//
// Home TU 193 "BIAnalyzerRigid.cpp" (Mac 0x246ed0-0x2475df).
// RTTI: class LEGO::BIAnalyzerRigid : BIAnalyzer. Mac sizeof 0x14 + Object (m_minSize +0x10), vtable
// 0x732a1c {GetFacadeType 0x2230a0, GetName 0x247330, D1 0x247360, D0 0x247380, Analyze 0x246ed0,
// GetWeight 0x223110, GetAttributes 0x226760}; Win m_minSize +0x14, vtable 0xa2c60c
// {0x5913d0, 0x591d50, 0x5a0160, 0x5913e0, 0x559e90}, destructor 0x591db0, sdd 0x591e20. The constructor
// is inlined into BIProducer::BIProducer (Mac 0x236b40).
#pragma once
#include "BIAnalyzer.h"

namespace LEGO {

class PrimitiveRigidElement;

class BIAnalyzerRigid : public BIAnalyzer {
public:
    // (names inferred)
    BIAnalyzerRigid(BIProducer* producer, Real32 priority, Real32 weight, UInt32 minSize)
        : BIAnalyzer(producer, priority, weight), m_minSize(minSize) {}
    virtual ~BIAnalyzerRigid();                                  // Mac 0x247360 / 0x247380, Win 0x591db0 / sdd 0x591e20

    virtual String GetName() const;                    // Mac 0x247330, Win 0x591d50 ("Rigid")
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // Mac 0x246ed0, Win 0x5a0160

    // The primitive of the graph that holds each element of `rigid` (the first that
    // BIBrick::ContainsElement); false when an element has none. Inlined on Mac, Win 0x5a00e0
    // (name inferred)
    static bool CollectPrimitives(const PrimitiveRigidElement* rigid, BIGraph* graph,
                                  BasicSet<BIBrickPrimitive*>& primitives);

    UInt32 m_minSize;   // Mac +0x10 / Win +0x14 (name inferred)
};

} // namespace LEGO
