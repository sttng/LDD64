// LEGO Digital Designer — restored source
// TU 150 "BIStrategyArticulation.cpp" (Mac 0x2136c0-0x213a2f): LEGO::BIStrategyArticulation.
// The TU also emits BIStrategy's inline virtuals (Mac 0x2137b0-0x2138a0, restored in
// BIStrategy.cpp).
#include "BIStrategyArticulation.h"
#include "BIGraph.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BGScene.h"

namespace LEGO {

// Mac 0x213920 / 0x213940, Win sdd 0x5646d0
BIStrategyArticulation::~BIStrategyArticulation()
{
}

// Mac 0x2138f0, Win 0x591950
String BIStrategyArticulation::GetName() const
{
    return String("Articulation");
}

// Mac 0x2136c0, Win 0x58fe90
Real32 BIStrategyArticulation::ScoreRemove(BIBrick* brick, UInt32&, const EventList&, BGScene* scene)
{
    bool splits;
    if (brick->GetType() == BIBrick::TypePrimitive)
        splits = BIGraph::IsArticulation(scene->GetGraphs(), static_cast<BIBrickPrimitive*>(brick));
    else
        splits = BIGraph::IsSeparating(scene->GetGraphs(), brick->GetPrimitiveSet());
    return splits ? m_failScore : m_passScore;
}

// Mac 0x213720, Win 0x58ff30
Real32 BIStrategyArticulation::ScoreAdd(BIBrick* brick, UInt32&, const EventList& events, BGScene* scene)
{
    if (events.IsEmpty())
        return m_passScore;
    const BasicSet<BIBrickPrimitive*>& placed = scene->GetPrimitives();
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = placed.Begin(); it != placed.End(); ++it) {
        if (brick->IsConnectedTo(*it))
            return m_passScore;
    }
    return m_failScore;
}

} // namespace LEGO
