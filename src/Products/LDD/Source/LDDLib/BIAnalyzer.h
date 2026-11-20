// LEGO Digital Designer — restored source
// LEGO::BIAnalyzer — base of the building-guide analyzers: each looks at one BIGraph (a
//   connected group of primitives) and proposes sub-assemblies (BIBrickSubstep) that the guide
//   may show as one step. A proposed substep remembers its analyzer (BIBrick::m_group), whose
//   weight scales the substep's score in the strategies (BIStrategy::Accumulate).
//   Subclasses (created by BIProducer): BIAnalyzerArticulation, BIAnalyzerDirection,
//   BIAnalyzerGroups, BIAnalyzerRigid, BIAnalyzerSeparation, BGAnalyzerMinifig, BGAnalyzerSkeleton.
//
// Home TU 167 "BIAnalyzer.cpp" (Mac 0x2262f0-0x22686f); the inline virtuals are emitted in TU
// 163 (Mac 0x2230a0-0x223140).
// RTTI: class LEGO::BIAnalyzer : BIFacade (: virtual Object).
//   Mac sizeof 0x10 + Object: vptr +0, m_parent (the BIProducer) +4, m_weight +8,
//       m_priority +0xc, Object +0x10. Vtable 0x72d84c {GetFacadeType 0x2230a0, pure,
//       D1 0x2230b0, D0 0x2230d0, pure, GetWeight 0x223110, GetAttributes 0x226760};
//       base-object constructor 0x2262f0 (VTT).
//   Win: vfptr +0, vbptr +4, m_parent +8, m_weight +0xc, m_priority +0x10. Vtable 0xa2c534
//       {0x5913d0, pure, pure, 0x5913e0, 0x559e90}, sdd 0x591470.
// The subclasses add m_minSize (Mac +0x10, Win +0x14): the smallest substep they propose.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BIFacade.h"
#include "BIBrickSubstep.h"

namespace LEGO {

class BIGraph;
class BIBrickPrimitive;
class BIProducer;

class BIAnalyzer : public BIFacade {
public:
    // `priority`: Mac +0xc, meaning unknown (the producer passes 10, 2, 1, 11). (names
    // inferred) Mac 0x2262f0 (base-object constructor), inlined on Win
    BIAnalyzer(BIProducer* producer, Real32 priority, Real32 weight);
    virtual ~BIAnalyzer();                                                 // Mac 0x2230b0 / 0x2230d0, Win sdd 0x591470

    virtual UInt32 GetFacadeType() const;                                  // idx 0: Mac 0x2230a0, Win 0x5913d0 (2)
    // Proposes the substeps of `graph`: appended to `substeps` (an equal substep already in
    // the list only gets this analyzer's attributes). Returns the number of new substeps.
    // (name inferred)
    virtual UInt32 Analyze(BIGraph* graph, BasicArray<BIBrickSubstep*>& substeps) = 0;   // idx 4
    // Weight of the substeps of this analyzer. Mac 0x223110, Win 0x5913e0 (name inferred)
    virtual Real32 GetWeight() const;                                      // idx 5
    // The attributes given to the substeps of this analyzer; none here. Mac 0x226760,
    // Win 0x559e90 (name inferred)
    virtual void GetAttributes(BIBrickSubstep::AttributeMap& attributes) const;   // idx 6

    // CreateSubstep(primitives, substeps, minSize, maxSize, this). Mac 0x226730 (name inferred)
    bool AddSubstep(const BasicSet<BIBrickPrimitive*>& primitives, BasicArray<BIBrickSubstep*>& substeps,
                    UInt32 minSize, UInt32 maxSize) const;
    // A substep of `primitives` for `substeps` when minSize <= size and (maxSize == 0 or
    // size <= maxSize): an equal substep of the list gets the analyzer's attributes (false);
    // else a new BIBrickSubstep (group `analyzer`, weight 1) is registered in the
    // BIBrickRepository, gets the attributes and is appended (true). Mac 0x226350 (name inferred)
    static bool CreateSubstep(const BasicSet<BIBrickPrimitive*>& primitives, BasicArray<BIBrickSubstep*>& substeps,
                              UInt32 minSize, UInt32 maxSize, const BIAnalyzer* analyzer);
    // Always true (BIFilter::Select asks it for the analyzer of a substep). Mac 0x226340
    // (name inferred)
    bool IsActive() const;
    // The producer's application was cancelled. Mac 0x226770 (name inferred)
    bool IsCancelled() const;

    BIProducer* GetProducer() const;
    Real32 GetPriority() const { return m_priority; }

    Real32 m_weight;     // +0x08 (name inferred)
    Real32 m_priority;   // +0x0c (name inferred; meaning unknown)
};

} // namespace LEGO
