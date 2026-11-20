// LEGO Digital Designer — restored source
// LEGO::BIAnalyzerSeparation — proposes nothing itself; the BIProducer uses it as the analyzer
//   (group) of the substeps it makes of whole graphs when the scene has several graphs.
//
// Home TU 165 "BIAnalyzerSeparation.cpp" (Mac 0x224600-0x22474f).
// RTTI: class LEGO::BIAnalyzerSeparation : BIAnalyzer. Mac sizeof 0x14 + Object (m_minSize +0x10), vtable
// 0x72da4c {GetFacadeType 0x2230a0, GetName 0x224610, D1 0x224640, D0 0x224660, Analyze 0x224600,
// GetWeight 0x223110, GetAttributes 0x226760}; Win m_minSize +0x14, vtable 0xa2c638
// {0x5913d0, 0x591f50, 0x59c690, 0x5913e0, 0x559e90}, destructor 0x591fb0, sdd 0x592020. The constructor
// is inlined into BIProducer::BIProducer (Mac 0x236b40).
#pragma once
#include "BIAnalyzer.h"

namespace LEGO {

class BIAnalyzerSeparation : public BIAnalyzer {
public:
    // (names inferred)
    BIAnalyzerSeparation(BIProducer* producer, Real32 priority, Real32 weight, UInt32 minSize)
        : BIAnalyzer(producer, priority, weight), m_minSize(minSize) {}
    virtual ~BIAnalyzerSeparation();                                  // Mac 0x224640 / 0x224660, Win 0x591fb0 / sdd 0x592020

    virtual String GetName() const;                    // Mac 0x224610, Win 0x591f50 ("Separation")
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // Mac 0x224600, Win 0x59c690

    UInt32 m_minSize;   // Mac +0x10 / Win +0x14 (name inferred)
};

} // namespace LEGO
