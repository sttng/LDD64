// LEGO Digital Designer — restored source
// TU 230 "BIStrategyConnectivity.cpp" (Mac 0x26bc40-0x26bfbf): LEGO::BIStrategyConnectivity.
#include "BIStrategyConnectivity.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BGScene.h"
#include "Geometry.h"

namespace LEGO {

// Mac 0x26beb0 / 0x26bed0, Win sdd 0x564540
BIStrategyConnectivity::~BIStrategyConnectivity()
{
}

// Mac 0x26be80, Win 0x564470
String BIStrategyConnectivity::GetName() const
{
    return String("Connectivity");
}

// Mac 0x26bc40, Win 0x590200
Real32 BIStrategyConnectivity::ScoreRemove(BIBrick* brick, UInt32&, const EventList&, BGScene* scene)
{
    // NOTE: both builds look the brick's graph up and ignore it.
    scene->FindBrickGraph(brick);
    brick->ClearDirection();
    Vector3 direction;
    BasicSet<BIBrickPrimitive*> blocking;
    if (!scene->GetRemoveDirection(brick, direction, m_flag, blocking))
        return m_failScore;
    direction.Normalize();
    brick->SetDirection(direction * 10.0f);
    return m_passScore;
}

// Mac 0x26bd60, Win 0x590310
Real32 BIStrategyConnectivity::ScoreAdd(BIBrick* brick, UInt32&, const EventList&, BGScene* scene)
{
    brick->ClearDirection();
    Vector3 direction;
    BasicSet<BIBrickPrimitive*> blocking;
    if (!scene->GetInsertDirection(brick, direction, 1, m_flag, blocking))
        return m_failScore;
    direction.Normalize();
    brick->SetDirection(direction * 10.0f);
    return m_passScore;
}

} // namespace LEGO
