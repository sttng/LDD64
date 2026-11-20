// LEGO Digital Designer — restored source
// TU 190 "BIStrategyBalance.cpp" (Mac 0x245710-0x245f6f): LEGO::BIStrategyBalance.
#include "BIStrategyBalance.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BGScene.h"
#include "Geometry.h"
#include <math.h>
#include "MathUtil.h"

namespace LEGO {

// Mac 0x245e60 / 0x245e80, Win sdd 0x565080
BIStrategyBalance::~BIStrategyBalance()
{
}

// Mac 0x245e30, Win 0x564fb0
String BIStrategyBalance::GetName() const
{
    return String("Balance");
}

// Mac 0x245710, Win 0x58f8d0
void BIStrategyBalance::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                                    ScoreMap& scores)
{
    if (events.IsEmpty() || bricks.GetSize() == 1)
        return;

    // the volume-weighted centre of the placed primitives
    Real32 volume = 0.0f;
    const BasicSet<BIBrickPrimitive*>& placed = scene->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = placed.Begin(); it != placed.End(); ++it)
        volume += (*it)->GetVolume();
    Vector3 center(0.0f, 0.0f, 0.0f);
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = scene->GetPrimitives().Begin(); it != scene->GetPrimitives().End(); ++it)
        center += (*it)->ComputeCenter(volume);

    ScoreMap distances;
    Real32 minimum = 0.0f, maximum = 0.0f;
    bool first = true;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        distances[brick] = brick->GetCenterOfMass().GetDistanceSquared(center);
        if (first || distances[brick] < minimum)
            minimum = distances[brick];
        if (first || maximum < distances[brick])
            maximum = distances[brick];
        first = false;
    }

    Real32 range = maximum - minimum;
    if (fabsf(range) >= 1e-5f) {
        for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
            Accumulate(scores, *it, (Real32)(((EvalReal)distances[*it] - minimum) / ((EvalReal)maximum - minimum)));   // (x87 on Win)
    } else {
        // NOTE: equal distances overwrite the scores with 1 (not accumulated, not weighted).
        for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
            scores[*it] = 1.0f;
    }
}

} // namespace LEGO
