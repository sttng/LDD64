// LEGO Digital Designer — restored source
// LEGO::BISubstepFilterProgram — the substep filters of the automatic building guide
//   (BGAutomaticApplication +0xc0): Baseplate and BGSubstepFilterMinifig.
//
// Home TU 302 "BISubstepFilterProgram.cpp" (Mac 0x2af8c0-0x2aff3f).
// RTTI: class LEGO::BISubstepFilterProgram : virtual Object. Mac sizeof 0x14 + Object (vptr,
// m_filters +4, Object +0x14), constructor 0x2afa70, vtable 0x7444cc {D1 0x2afd30, D0 0x2afe00};
// Win: no own slots (vftable of the Object part only), destructor 0x592b90, sdd 0x592b50.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BISubstepFilter.h"

namespace LEGO {

class BISubstepFilterProgram : public virtual Object {
public:
    // Mac 0x2afa70 (inlined on Win)
    BISubstepFilterProgram();
    virtual ~BISubstepFilterProgram();                                     // Mac 0x2afd30 / 0x2afe00, Win 0x592b90 / sdd 0x592b50

    // BISubstepFilter::Begin of every filter, then removes from `bricks` the substeps that a
    // filter does not accept; returns the number removed. Mac 0x2afc40, Win 0x592bc0
    // (name inferred)
    UInt32 Apply(BasicSet<BIBrick*>& bricks);

    BasicArray<RefPointer<BISubstepFilter> > m_filters;   // +0x04 (name inferred)
};

} // namespace LEGO
