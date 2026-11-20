// LEGO Digital Designer — restored source
// TU 183 "BIStrategySweeping.cpp" (Mac 0x237970-0x237d9f): LEGO::BIStrategySweeping.
#include "BIStrategySweeping.h"
#include "BIBrick.h"
#include <math.h>
#include "MathUtil.h"

namespace LEGO {

// Mac 0x237c90 / 0x237cb0, Win sdd 0x564ca0
BIStrategySweeping::~BIStrategySweeping()
{
}

// Mac 0x237c60, Win 0x564bd0
String BIStrategySweeping::GetName() const
{
    return String("Sweeping");
}

// Mac 0x237970, Win 0x58ffd0
void BIStrategySweeping::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList&, BGScene*, ScoreMap& scores)
{
    // NOTE: the ranges are uninitialised in the original for no bricks (not read then).
    Real32 minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
    bool first = true;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        Real32 x = (*it)->GetPoint(BIBrick::PointMinimum).x;
        Real32 z = (*it)->GetPoint(BIBrick::PointMinimum).z;
        if (first) {
            minX = maxX = x;
            minZ = maxZ = z;
            first = false;
        }
        minX = x < minX ? x : minX;
        maxX = x > maxX ? x : maxX;
        minZ = z < minZ ? z : minZ;
        maxZ = z > maxZ ? z : maxZ;
    }
    Real32 rangeX = maxX - minX;
    Real32 rangeZ = maxZ - minZ;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        Real32 sx = fabsf(rangeX) < 1e-5f ? 0.0f : (Real32)(((EvalReal)brick->GetPoint(BIBrick::PointMinimum).x - minX) / ((EvalReal)maxX - minX));
        Real32 sz = fabsf(rangeZ) < 1e-5f ? 0.0f : (Real32)(((EvalReal)brick->GetPoint(BIBrick::PointMinimum).z - minZ) / ((EvalReal)maxZ - minZ));
        // (x87 on Win: each quotient rounded to float once, see MathUtil.h)
        Accumulate(scores, brick, (sx + sz) * 0.5f);
    }
}

} // namespace LEGO
