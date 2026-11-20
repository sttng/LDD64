// LEGO Digital Designer — restored source
// LEGO::BIStrategyConnectivity — the pass score when the scene finds a direction to remove / insert
//   the brick (BGScene::GetRemoveDirection / GetInsertDirection with m_flag), which is then stored
//   in the brick (BIBrick::SetDirection, normalised x 10), else the fail score.
//
// Home TU 230 "BIStrategyConnectivity.cpp" (Mac 0x26bc40-0x26bfbf).
// RTTI: class LEGO::BIStrategyConnectivity : BIStrategy. Mac sizeof 0x20 + Object (m_flag +0x1c;
// Win +0x20). Mac vtable 0x737b4c (GetName 0x26be80, D1 0x26beb0, D0 0x26bed0, ScoreRemove(brick)
// 0x26bc40, ScoreAdd(brick) 0x26bd60), Win vtable 0xa2bcd4 (GetName 0x564470, ScoreRemove(brick)
// 0x590200, ScoreAdd(brick) 0x590310), sdd 0x564540. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyConnectivity : public BIStrategy {
public:
    // (name inferred)
    explicit BIStrategyConnectivity(bool flag) : m_flag(flag) {}
    virtual ~BIStrategyConnectivity();                                  // Mac 0x26beb0 / 0x26bed0, Win sdd 0x564540

    virtual String GetName() const;                    // Mac 0x26be80, Win 0x564470 ("Connectivity")
    virtual Real32 ScoreRemove(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);   // idx 7: Mac 0x26bc40, Win 0x590200
    virtual Real32 ScoreAdd(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);      // idx 8: Mac 0x26bd60, Win 0x590310

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;

    bool m_flag;   // +0x1c passed to the BGScene direction queries (name inferred)
};

} // namespace LEGO
