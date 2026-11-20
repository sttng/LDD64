// LEGO Digital Designer — restored source
// TU 218 "BIStrategySubsteps.cpp" (Mac 0x266290-0x26651f): LEGO::BIStrategySubsteps,
// LEGO::BIPredicateIsSubstep.
#include "BIStrategySubsteps.h"
#include "BIBrick.h"
#include "BI.h"

namespace LEGO {

// Mac 0x2664d0, Win 0x58fd00
bool BIPredicateIsSubstep::operator()(const BIBrick* brick) const
{
    return brick->GetType() == BIBrick::TypeSubstep;
}

// Mac 0x266400 / 0x266420, Win sdd 0x565270
BIStrategySubsteps::~BIStrategySubsteps()
{
}

// Mac 0x2663d0, Win 0x5651a0
String BIStrategySubsteps::GetName() const
{
    return String("Substeps");
}

// Mac 0x266290, Win 0x58fd20
void BIStrategySubsteps::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene*,
                                     ScoreMap& scores)
{
    BasicArray<BIBrick*> placed;
    BI::GetBricks(events, placed);
    BIPredicateIsSubstep isSubstep;
    const BIPredicate& predicate = isSubstep;
    for (BasicArray<BIBrick*>::ConstIterator it = placed.Begin(); it != placed.End(); ++it) {
        if (!predicate(*it))
            return;
    }
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        if (brick->GetType() == BIBrick::TypeSubstep)
            Accumulate(scores, brick, (Real32)brick->GetPrimitiveCount());
    }
}

} // namespace LEGO
