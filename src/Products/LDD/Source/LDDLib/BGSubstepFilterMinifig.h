// LEGO Digital Designer — restored source
// LEGO::BGSubstepFilterMinifig — the minifigure substep filter of the automatic building guide
//   (BISubstepFilterProgram): a substep that only partly overlaps a minifigure substep (one
//   with attribute 1, proposed by BGAnalyzerMinifig) is rejected, so a figure is either built
//   as a whole or inside a larger substep. Names inferred.
//
// Home TU 361 "BGSubstepFilterMinifig.cpp" (Mac 0x344740-0x344d5f, Win 0x5a1ab0-0x5a1c9f).
// RTTI: class LEGO::BGSubstepFilterMinifig : BISubstepFilter. Mac sizeof 0x14 + Object
// (m_minifigs +0x04, Object +0x14), constructor 0x344740, vtable 0x74d28c {Accept 0x3447a0,
// Begin 0x344810, D1 0x344b00, D0 0x344c20, OnFinalRelease 0x344cd0}, Object-in vtable
// 0x74d2b8 {0x344b80, 0x344cb0, 0x344cf0, ..}. Win vtable 0xa2c95c {0x5a1b20, 0x5a1bc0},
// Object-in 0xa2c968 {sdd 0x5a1d70, 0x5a1d60, ..}, scalar deleting destructor of the
// primary 0x5a1ab0.
#pragma once
#include "BISubstepFilter.h"
#include "BasicArray.h"

namespace LEGO {

class BIBrickSubstep;

class BGSubstepFilterMinifig : public BISubstepFilter {
public:
    BGSubstepFilterMinifig();                                              // Mac 0x344740 (inlined on Win)
    virtual ~BGSubstepFilterMinifig();                                     // Mac 0x344b00 / 0x344c20, Win sdd 0x5a1ab0

    // A minifigure substep, or a substep intersecting none of the collected minifigure
    // substeps. Mac 0x3447a0, Win 0x5a1b20
    virtual bool Accept(const BIBrick* substep);
    // Appends the free minifigure substeps of the repository to m_minifigs. NOTE: the list is
    // never cleared (the original keeps appending on every pass). Mac 0x344810, Win 0x5a1bc0
    virtual void Begin();

protected:
    // Only calls Object's. Mac 0x344cd0 (thunk 0x344cf0), Win 0x5a1d60
    virtual void OnFinalRelease();

public:
    BasicArray<BIBrickSubstep*> m_minifigs;   // +0x04
};

} // namespace LEGO
