// LEGO Digital Designer — restored source
// TU 191 "BIStrategyNearby.cpp" (Mac 0x245f70-0x2469ef): LEGO::BIStrategyNearby.
#include "BIStrategyNearby.h"
#include "BIBrick.h"
#include "BIEvent.h"
#include "Geometry.h"
#include <math.h>
#include "MathUtil.h"

namespace LEGO {

// Mac 0x2468e0 / 0x246900, Win sdd 0x564e90
BIStrategyNearby::~BIStrategyNearby()
{
}

// Mac 0x2468b0, Win 0x564dc0
String BIStrategyNearby::GetName() const
{
    return String("Nearby");
}

// Mac 0x245f70
void BIStrategyNearby::ComputeCloseness(const BasicSet<BIBrick*>& bricks, BIBrick* reference, ScoreMap& closeness)
{
    ScoreMap negated;
    Real32 minimum = 0.0f, maximum = 0.0f;
    bool first = true;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        Vector3 from = reference->GetCenterOfMass();
        Vector3 to = brick->GetCenterOfMass();
        negated[brick] = -from.GetDistanceSquared(to);
        if (first || minimum > negated[brick])
            minimum = negated[brick];
        if (first || negated[brick] > maximum)
            maximum = negated[brick];
        first = false;
    }
    Real32 range = maximum - minimum;
    if (fabsf(range) < 1e-5f) {
        for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
            closeness[*it] = 1.0f;
    } else {
        for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
            closeness[*it] = (Real32)(((EvalReal)negated[*it] - minimum) / ((EvalReal)maximum - minimum));   // (x87 on Win)
    }
}

// Mac 0x246660, Win 0x58c740
void BIStrategyNearby::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene*,
                                   ScoreMap& scores)
{
    if (events.IsEmpty() || bricks.GetSize() == 1)
        return;
    UInt32 count = events.GetSize();
    if (count >= m_count)
        count = m_count;
    if (count == 0)
        return;
    Real32 total = (Real32)count * (Real32)(count + 1) * 0.5f;
    for (UInt32 k = 1; k <= count; ++k) {
        // the k-th last placed brick, weighted k
        BIEvent* event = events[events.GetSize() - k];
        ScoreMap closeness;
        ComputeCloseness(bricks, event->GetBrick(), closeness);
        Real32 weight = (Real32)k;
        for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
            Accumulate(scores, *it, (Real32)((EvalReal)weight * closeness[*it] / total));   // (x87 on Win)
    }
}

// Mac 0x2468a0, Win 0x58c900
void BIStrategyNearby::ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                                ScoreMap& scores)
{
    ScoreRemove(bricks, events, scene, scores);
}

} // namespace LEGO
