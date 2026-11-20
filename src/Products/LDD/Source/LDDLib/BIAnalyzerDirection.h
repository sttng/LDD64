// LEGO Digital Designer — restored source
// LEGO::BIAnalyzerDirection — proposes the groups of primitives that are connected to each
//   other along one insertion direction (BIBrick::Connection directions, equal or opposite
//   within 0.1 per component): for every connection direction of every primitive, the
//   primitives reachable through connections with that direction, when there are at least
//   m_minSize and at most half the graph.
//
// Home TU 164 "BIAnalyzerDirection.cpp" (Mac 0x2232d0-0x2245f9).
// RTTI: class LEGO::BIAnalyzerDirection : BIAnalyzer. Mac sizeof 0x14 + Object (m_minSize
// +0x10), vtable 0x72d90c {GetFacadeType 0x2230a0, GetName 0x223aa0, D1 0x223ad0, D0 0x223af0,
// Analyze 0x2236a0, GetWeight 0x223110, GetAttributes 0x226760}; Win m_minSize +0x14, vtable
// 0xa2c5e0 {0x5913d0, 0x591b50, 0x59c990, 0x5913e0, 0x559e90}, destructor 0x591bb0,
// sdd 0x591c20. The constructor is inlined into BIProducer::BIProducer (Mac 0x236b40).
// Statics: the direction table (Mac BasicMap<UInt32, Vector3> 0x77aec8, constructed by the TU
// initialiser 0x224560; Win 0xb42760 area) and its counter (Mac 0x77aee4, Win 0xb42778).
#pragma once
#include "BIAnalyzer.h"
#include "Geometry.h"

namespace LEGO {

class BIBrickPrimitive;

class BIAnalyzerDirection : public BIAnalyzer {
public:
    // The directions already seen per primitive.
    typedef BasicMap<const BIBrickPrimitive*, BasicSet<UInt32> > VisitedMap;

    // (names inferred)
    BIAnalyzerDirection(BIProducer* producer, Real32 priority, Real32 weight, UInt32 minSize)
        : BIAnalyzer(producer, priority, weight), m_minSize(minSize) {}
    virtual ~BIAnalyzerDirection();                    // Mac 0x223ad0 / 0x223af0, Win 0x591bb0 / sdd 0x591c20

    virtual String GetName() const;                    // Mac 0x223aa0, Win 0x591b50 ("Direction")
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps);   // Mac 0x2236a0, Win 0x59c990

    // The number of the direction: the first registered direction equal or opposite to
    // `direction` within 0.1 per component, else `direction` registered under the next number
    // (numbers are global and never reset). Mac 0x2234f0, Win 0x59cc40 (name inferred)
    static UInt32 GetDirectionID(const Vector3& direction);
    // Adds `primitive` to `component` and, recursively, the neighbours whose connection from
    // `primitive` has a direction with the number `direction` — unless the primitive has
    // been visited with that direction. Mac 0x2232d0, Win 0x59c820 (name inferred)
    void Collect(BIBrickPrimitive* primitive, BIGraph* graph, BasicSet<BIBrickPrimitive*>& component,
                 UInt32 direction, VisitedMap& visited);

    UInt32 m_minSize;   // Mac +0x10 / Win +0x14 (name inferred)

    static BasicMap<UInt32, Vector3> s_directions;   // Mac 0x77aec8 (name inferred)
    static UInt32                    s_nextDirection; // Mac 0x77aee4, Win 0xb42778 (name inferred)
};

} // namespace LEGO
