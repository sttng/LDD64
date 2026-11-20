// LEGO Digital Designer — restored source
// LEGO::BISubstepFilter — a rule that rejects proposed substeps (BISubstepFilterProgram).
//   Subclasses: BISubstepFilterBaseplate, BGSubstepFilterMinifig.
//
// Home TU 362 "BISubstepFilter.cpp" (Mac 0x344d60-0x344e8f).
// RTTI: class LEGO::BISubstepFilter : virtual Object. Mac sizeof 4 + Object (vptr, Object +4),
// vtable 0x74446c {pure, Begin 0x344d60, D1 0x344d80, D0 0x344dc0}, base-object destructor
// 0x344d70 (empty); Win vtable 0xa2c91c {pure, 0x56b1c0}, sdd 0x5a1820.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"

namespace LEGO {

class BIBrick;

class BISubstepFilter : public virtual Object {
public:
    BISubstepFilter() {}
    // The substep may be used. (name inferred)
    virtual bool Accept(const BIBrick* substep) = 0;                       // slot 0
    // Called before a pass over the substeps; empty here. Mac 0x344d60, Win 0x56b1c0
    // (name inferred)
    virtual void Begin();                                                  // slot 1
    virtual ~BISubstepFilter();                                            // Mac 0x344d80 / 0x344dc0, Win sdd 0x5a1820
};

} // namespace LEGO
