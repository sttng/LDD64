// LEGO Digital Designer — restored source
// LEGO::BIAnalyzerArticulation — proposes, for every articulation point of the graph, the
//   largest part it holds that has at most half the primitives (BIGraph::GetSmallSide), when
//   that part has at least m_minSize primitives.
//
// Home TU 163 "BIAnalyzerArticulation.cpp" (Mac 0x222f70-0x2232cf).
// RTTI: class LEGO::BIAnalyzerArticulation : BIAnalyzer. Mac sizeof 0x14 + Object (m_minSize +0x10), vtable
// 0x72d7cc {GetFacadeType 0x2230a0, GetName 0x223190, D1 0x2231c0, D0 0x2231e0, Analyze 0x222f70,
// GetWeight 0x223110, GetAttributes 0x226760}; Win m_minSize +0x14, vtable 0xa2c5b4
// {0x5913d0, 0x591950, 0x59c6e0, 0x5913e0, 0x559e90}, destructor 0x5919b0, sdd 0x591a20. The constructor
// is inlined into BIProducer::BIProducer (Mac 0x236b40).
#pragma once
#include "BIAnalyzer.h"

namespace LEGO {

class BIAnalyzerArticulation : public BIAnalyzer {
public:
    // (names inferred)
    BIAnalyzerArticulation(BIProducer* producer, Real32 priority, Real32 weight, UInt32 minSize)
        : BIAnalyzer(producer, priority, weight), m_minSize(minSize) {}
    virtual ~BIAnalyzerArticulation();                                  // Mac 0x2231c0 / 0x2231e0, Win 0x5919b0 / sdd 0x591a20

    virtual String GetName() const;                    // Mac 0x223190, Win 0x591950 ("Articulation")
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // Mac 0x222f70, Win 0x59c6e0

    UInt32 m_minSize;   // Mac +0x10 / Win +0x14 (name inferred)
};

} // namespace LEGO
