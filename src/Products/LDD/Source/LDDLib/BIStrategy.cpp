// LEGO Digital Designer — restored source
// TU 179 "BIStrategy.cpp" (Mac 0x234900-0x234dcf): LEGO::BIStrategy. See BIStrategy.h.
// GetFacadeType, the destructor and the empty / zero virtuals are inline in the original
// (emitted in TU 150, Mac 0x2137b0-0x2138a0).
#include "BIStrategy.h"
#include "BIBrick.h"
#include "BIAnalyzer.h"
#include <math.h>
#include "MathUtil.h"

namespace LEGO {

// Mac 0x234900 (base-object constructor), inlined on Win
BIStrategy::BIStrategy()
    : m_isFilter(0),
      m_isFilter2(0),
      m_weight(0.0f),
      m_passScore(1.0f),
      m_failScore(0.0f)
{
}

// Mac 0x2137c0 / 0x2137e0 (TU 150), Win sdd 0x563f20
BIStrategy::~BIStrategy()
{
}

// Mac 0x2137b0 (TU 150), Win 0x563e90
UInt32 BIStrategy::GetFacadeType() const
{
    return 4;
}

// Mac 0x213820 (TU 150), Win 0x56b1c0
void BIStrategy::Reset()
{
}

// Mac 0x234960, Win 0x58bee0
void BIStrategy::ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores)
{
    UInt32 context = 0;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        Accumulate(scores, brick, ScoreRemove(brick, context, events, scene));
    }
}

// Mac 0x234c90, Win 0x58bf80
void BIStrategy::ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                          ScoreMap& scores)
{
    UInt32 context = 0;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        Accumulate(scores, brick, ScoreAdd(brick, context, events, scene));
    }
}

// Mac 0x213830 (TU 150), Win 0x563ea0
Real32 BIStrategy::ScoreRemove(BIBrick*, UInt32&, const EventList&, BGScene*)
{
    return 0.0f;
}

// Mac 0x213840 (TU 150), Win 0x563ea0
Real32 BIStrategy::ScoreAdd(BIBrick*, UInt32&, const EventList&, BGScene*)
{
    return 0.0f;
}

// Mac 0x213850 (TU 150), Win 0x57ba30
bool BIStrategy::Relax()
{
    return false;
}

// Mac 0x213860 (TU 150), Win 0x559e90
void BIStrategy::Unknown10(void*)
{
}

// Mac 0x213870 (TU 150), Win 0x56b1c0
void BIStrategy::Unknown11()
{
}

// Mac 0x2349e0, Win 0x58c0d0
void BIStrategy::Accumulate(ScoreMap& scores, BIBrick* brick, Real32 score)
{
    Real32 current = scores[brick];
    Real32 lower = current < score ? current : score;
    Real32 negative = 0.0f - lower;
    if (fabsf(negative) > 1e-5f && negative > 0.0f) {
        // one of them is negative: keep the smaller
        Real32 entry = scores[brick];
        scores[brick] = score <= entry ? score : entry;
        return;
    }
    if (brick->GetType() == BIBrick::TypeSubstep) {
        const BIAnalyzer* analyzer = brick->GetGroup();
        Real32 groupWeight = analyzer != 0 ? analyzer->GetWeight() : 1.0f;
        // (Win x87: the weight product is kept in double, the factor rounded once; see MathUtil.h)
        score = (Real32)((EvalReal)brick->GetPrimitiveCount() * ((EvalReal)brick->GetWeight() * groupWeight) * score);
    }
    // (Win x87: score * weight is not rounded before the sum)
    Real32& entry = scores[brick];
    entry = (Real32)((EvalReal)score * m_weight + entry);
}

// Mac 0x234d10, inlined on Win
void BIStrategy::Configure(Real32 passScore, Real32 failScore, Real32 weight)
{
    m_passScore = passScore;
    m_failScore = failScore;
    m_weight = weight;
    UInt32 filter;
    if (fabsf(passScore) >= 1e-5f)
        filter = 0;
    else
        filter = fabsf(failScore + 1.0f) <= 1e-5f || failScore + 1.0f <= 0.0f;
    m_isFilter = filter;
    m_isFilter2 = filter;
}

} // namespace LEGO
