// LEGO Digital Designer — restored source
// LEGO::BIStrategySubsteps — removing, while every placed brick is a substep: the substeps by their
//   primitive count (big substeps first).
// LEGO::BIPredicate / BIPredicateIsSubstep — a one-slot brick predicate (no destructor slot).
//
// Home TU 218 "BIStrategySubsteps.cpp" (Mac 0x266290-0x26651f).
// RTTI: class LEGO::BIStrategySubsteps : BIStrategy. Mac sizeof 0x1c + Object. Mac vtable 0x736f1c
// (GetName 0x2663d0, D1 0x266400, D0 0x266420, ScoreRemove(set) 0x266290), Win vtable 0xa2be9c
// (GetName 0x5651a0, ScoreRemove(set) 0x58fd20), sdd 0x565270. Constructed (inline) by
// BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

// Mac vtable: none (abstract, inline); Win vtable 0xa2c508 {pure}. (names inferred)
class BIPredicate {
public:
    virtual bool operator()(const BIBrick* brick) const = 0;
};

// The brick is a substep. Mac vtable 0x771710 {0x2664d0}, Win 0xa2c510 {0x58fd00}
class BIPredicateIsSubstep : public BIPredicate {
public:
    virtual bool operator()(const BIBrick* brick) const;   // Mac 0x2664d0, Win 0x58fd00
};

class BIStrategySubsteps : public BIStrategy {
public:
    BIStrategySubsteps() {}
    virtual ~BIStrategySubsteps();                                  // Mac 0x266400 / 0x266420, Win sdd 0x565270

    virtual String GetName() const;                    // Mac 0x2663d0, Win 0x5651a0 ("Substeps")
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);   // idx 5: Mac 0x266290, Win 0x58fd20

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;
};

} // namespace LEGO
