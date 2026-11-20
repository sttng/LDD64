// LEGO Digital Designer — restored source
// LEGO::BIStrategyBalance — removing (once something is placed and there is more than one
//   candidate): the bricks farthest from the volume-weighted centre of the placed primitives first.
//
// Home TU 190 "BIStrategyBalance.cpp" (Mac 0x245710-0x245f6f).
// RTTI: class LEGO::BIStrategyBalance : BIStrategy. Mac sizeof 0x1c + Object. Mac vtable 0x731f9c
// (GetName 0x245e30, D1 0x245e60, D0 0x245e80, ScoreRemove(set) 0x245710), Win vtable 0xa2be54
// (GetName 0x564fb0, ScoreRemove(set) 0x58f8d0), sdd 0x565080. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyBalance : public BIStrategy {
public:
    BIStrategyBalance() {}
    virtual ~BIStrategyBalance();                                  // Mac 0x245e60 / 0x245e80, Win sdd 0x565080

    virtual String GetName() const;                    // Mac 0x245e30, Win 0x564fb0 ("Balance")
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);   // idx 5: Mac 0x245710, Win 0x58f8d0

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;
};

} // namespace LEGO
