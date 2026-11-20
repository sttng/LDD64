// LEGO Digital Designer — restored source
// LEGO::BIStrategyIntegration — removing: 1 / the number of primitives of the brick's graph (bricks
//   of small graphs first).
//
// Home TU 158 "BIStrategyIntegration.cpp" (Mac 0x221810-0x2219af).
// RTTI: class LEGO::BIStrategyIntegration : BIStrategy. Mac sizeof 0x1c + Object. Mac vtable
// 0x72d66c (GetName 0x221870, D1 0x2218a0, D0 0x2218c0, ScoreRemove(brick) 0x221810), Win vtable
// 0xa2bd54 (GetName 0x5647f0, ScoreRemove(brick) 0x58cc80), sdd 0x5648c0. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyIntegration : public BIStrategy {
public:
    BIStrategyIntegration() {}
    virtual ~BIStrategyIntegration();                                  // Mac 0x2218a0 / 0x2218c0, Win sdd 0x5648c0

    virtual String GetName() const;                    // Mac 0x221870, Win 0x5647f0 ("Integration")
    virtual Real32 ScoreRemove(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);   // idx 7: Mac 0x221810, Win 0x58cc80

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;
};

} // namespace LEGO
