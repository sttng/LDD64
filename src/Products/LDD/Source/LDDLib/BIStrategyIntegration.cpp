// LEGO Digital Designer — restored source
// TU 158 "BIStrategyIntegration.cpp" (Mac 0x221810-0x2219af): LEGO::BIStrategyIntegration.
#include "BIStrategyIntegration.h"
#include "BIGraph.h"
#include "BGScene.h"

namespace LEGO {

// Mac 0x2218a0 / 0x2218c0, Win sdd 0x5648c0
BIStrategyIntegration::~BIStrategyIntegration()
{
}

// Mac 0x221870, Win 0x5647f0
String BIStrategyIntegration::GetName() const
{
    return String("Integration");
}

// Mac 0x221810, Win 0x58cc80
Real32 BIStrategyIntegration::ScoreRemove(BIBrick* brick, UInt32&, const EventList&, BGScene* scene)
{
    // NOTE: the graph is not checked (a brick of no graph crashes the original).
    return 1.0f / scene->FindBrickGraph(brick)->GetPrimitiveCount();
}

} // namespace LEGO
