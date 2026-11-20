// LEGO Digital Designer — restored source
// TU 151 "BIStrategyGroundUp.cpp" (Mac 0x213a30-0x213dff): LEGO::BIStrategyGroundUp.
#include "BIStrategyGroundUp.h"
#include "BIBrick.h"
#include <math.h>
#include "MathUtil.h"

namespace LEGO {

namespace {

// The lowest and highest point height of the bricks (0, 0 for none). Inline in both slots.
void GetHeightRange(const BasicSet<BIBrick*>& bricks, UInt32 point, Real32& minimum, Real32& maximum)
{
    minimum = 0.0f;
    maximum = 0.0f;
    bool first = true;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        Real32 y = (*it)->GetPoint(point).y;
        minimum = first ? y : (y < minimum ? y : minimum);
        maximum = first ? y : (y > maximum ? y : maximum);
        first = false;
    }
}

} // namespace

// Mac 0x213cf0 / 0x213d10, Win sdd 0x564ab0
BIStrategyGroundUp::~BIStrategyGroundUp()
{
}

// Mac 0x213cc0, Win 0x5649e0
String BIStrategyGroundUp::GetName() const
{
    return String("GroundUp");
}

// Mac 0x213b80, Win 0x58f750
void BIStrategyGroundUp::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList&, BGScene*, ScoreMap& scores)
{
    Real32 minimum, maximum;
    GetHeightRange(bricks, m_point, minimum, maximum);
    Real32 range = maximum - minimum;
    if (fabsf(range) < 1e-5f)
        return;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        // (x87 on Win: the quotient is rounded to float once, see MathUtil.h)
        Accumulate(scores, brick, (Real32)(((EvalReal)brick->GetPoint(m_point).y - minimum) / ((EvalReal)maximum - minimum)));
    }
}

// Mac 0x213a30, Win 0x58f5d0
void BIStrategyGroundUp::ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList&, BGScene*, ScoreMap& scores)
{
    Real32 minimum, maximum;
    GetHeightRange(bricks, m_point, minimum, maximum);
    Real32 range = maximum - minimum;
    if (fabsf(range) < 1e-5f)
        return;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        // (x87 on Win: the quotient is rounded to float once, see MathUtil.h)
        Accumulate(scores, brick,
                   (Real32)((EvalReal)1.0f - ((EvalReal)brick->GetPoint(m_point).y - minimum) / ((EvalReal)maximum - minimum)));
    }
}

} // namespace LEGO
