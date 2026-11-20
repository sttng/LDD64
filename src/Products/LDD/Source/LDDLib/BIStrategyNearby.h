// LEGO Digital Designer — restored source
// LEGO::BIStrategyNearby — both directions: the bricks near the last m_count placed bricks first
//   (closeness of the centres of mass, normalised per placed brick; the older of those
//   bricks weigh more).
//
// Home TU 191 "BIStrategyNearby.cpp" (Mac 0x245f70-0x2469ef).
// RTTI: class LEGO::BIStrategyNearby : BIStrategy. Mac sizeof 0x20 + Object (m_count +0x1c; Win
// +0x20). Mac vtable 0x7320fc (GetName 0x2468b0, D1 0x2468e0, D0 0x246900, ScoreRemove(set)
// 0x246660, ScoreAdd(set) 0x2468a0), Win vtable 0xa2be14 (GetName 0x564dc0, ScoreRemove(set)
// 0x58c740, ScoreAdd(set) 0x58c900), sdd 0x564e90. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyNearby : public BIStrategy {
public:
    // (name inferred)
    explicit BIStrategyNearby(UInt32 count) : m_count(count) {}
    virtual ~BIStrategyNearby();                                  // Mac 0x2468e0 / 0x246900, Win sdd 0x564e90

    virtual String GetName() const;                    // Mac 0x2468b0, Win 0x564dc0 ("Nearby")
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);   // idx 5: Mac 0x246660, Win 0x58c740
    virtual void ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                          ScoreMap& scores);      // idx 6: Mac 0x2468a0, Win 0x58c900

    // closeness[brick] for every brick: the negated squared distance between the centres of
    // mass of `reference` and the brick, normalised to 0 (farthest) .. 1 (nearest); all 1 when
    // the distances are equal (within 1e-5). Mac 0x245f70 (`this` unused) (name inferred)
    void ComputeCloseness(const BasicSet<BIBrick*>& bricks, BIBrick* reference, ScoreMap& closeness);

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;

    UInt32 m_count;   // +0x1c the number of placed bricks looked at (name inferred)
};

} // namespace LEGO
