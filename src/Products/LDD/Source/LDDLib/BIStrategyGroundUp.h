// LEGO Digital Designer — restored source
// LEGO::BIStrategyGroundUp — scores the bricks by the height of one of their points
//   (BIBrick::GetPoint m_point): adding prefers the lowest bricks (1 at the bottom, 0 at the top of
//   the candidates), removing the highest.
//
// Home TU 151 "BIStrategyGroundUp.cpp" (Mac 0x213a30-0x213dff).
// RTTI: class LEGO::BIStrategyGroundUp : BIStrategy. Mac sizeof 0x20 + Object (m_point +0x1c; Win
// +0x20). Mac vtable 0x72ba9c (GetName 0x213cc0, D1 0x213cf0, D0 0x213d10, ScoreRemove(set)
// 0x213b80, ScoreAdd(set) 0x213a30), Win vtable 0xa2bd94 (GetName 0x5649e0, ScoreRemove(set)
// 0x58f750, ScoreAdd(set) 0x58f5d0), sdd 0x564ab0. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyGroundUp : public BIStrategy {
public:
    // (name inferred)
    explicit BIStrategyGroundUp(UInt32 point) : m_point(point) {}
    virtual ~BIStrategyGroundUp();                                  // Mac 0x213cf0 / 0x213d10, Win sdd 0x564ab0

    virtual String GetName() const;                    // Mac 0x213cc0, Win 0x5649e0 ("GroundUp")
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);   // idx 5: Mac 0x213b80, Win 0x58f750
    virtual void ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                          ScoreMap& scores);      // idx 6: Mac 0x213a30, Win 0x58f5d0

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;

    UInt32 m_point;   // +0x1c BIBrick::Point (0: the minimum; name inferred)
};

} // namespace LEGO
