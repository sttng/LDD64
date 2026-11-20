// LEGO Digital Designer — restored source
// LEGO::BIStrategySweeping — removing: the bricks with the largest minimum x and z first (the mean
//   of the two normalised coordinates); adding has no score of its own.
//
// Home TU 183 "BIStrategySweeping.cpp" (Mac 0x237970-0x237d9f).
// RTTI: class LEGO::BIStrategySweeping : BIStrategy. Mac sizeof 0x1c + Object. Mac vtable 0x72f30c
// (GetName 0x237c60, D1 0x237c90, D0 0x237cb0, ScoreRemove(set) 0x237970), Win vtable 0xa2bdd4
// (GetName 0x564bd0, ScoreRemove(set) 0x58ffd0), sdd 0x564ca0. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategySweeping : public BIStrategy {
public:
    BIStrategySweeping() {}
    virtual ~BIStrategySweeping();                                  // Mac 0x237c90 / 0x237cb0, Win sdd 0x564ca0

    virtual String GetName() const;                    // Mac 0x237c60, Win 0x564bd0 ("Sweeping")
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);   // idx 5: Mac 0x237970, Win 0x58ffd0

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;
};

} // namespace LEGO
