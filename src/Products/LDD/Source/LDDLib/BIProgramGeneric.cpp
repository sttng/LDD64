// LEGO Digital Designer — restored source
// TU 162 "BIProgramGeneric.cpp" (Mac 0x221ab0-0x222f6f): LEGO::BIProgramGeneric, the inline
// functions of BIProgram and BIFilter::ConditionTrue (BIFilter's GetName and destructors of
// this TU are in BIFilter.cpp). Template code: BasicArray<BIFilter::Entry> destructors
// (0x222e40 / 0x222ef0, Win 0x565db0 / sdd 0x565dd0).
#include "BIProgramGeneric.h"
#include "BIBrick.h"
#include "BIStrategyArticulation.h"
#include "BIStrategyBalance.h"
#include "BIStrategyConnectivity.h"
#include "BIStrategyGroundUp.h"
#include "BIStrategyIntegration.h"
#include "BIStrategyNearby.h"
#include "BIStrategySubsteps.h"
#include "BIStrategySweeping.h"
#include <math.h>

namespace LEGO {

// ---- BIProgram (inline in the original) ------------------------------------------------------

// Mac 0x222a80 / 0x222aa0, Win 0x563db0 / sdd 0x563e20
BIProgram::~BIProgram()
{
}

// Mac 0x222840, Win 0x5913d0
UInt32 BIProgram::GetFacadeType() const
{
    return 2;
}

// Mac 0x222970, Win 0x563da0
void BIProgram::Unknown08()
{
}

// ---- BIFilter::ConditionTrue -------------------------------------------------------------------

// Mac 0x222d60, Win 0x5e4100
bool BIFilter::ConditionTrue::Evaluate(const BIBrick*) const
{
    return true;
}

// Mac 0x222d70 / 0x222d90, Win sdd 0x564120
BIFilter::ConditionTrue::~ConditionTrue()
{
}

// ---- BIProgramGeneric ----------------------------------------------------------------------------

namespace {

// A weight that switches a strategy on: >= 0 within 1e-5.
bool IsEnabled(Real32 weight)
{
    return 0.0f - weight <= 0.0f || fabsf(0.0f - weight) <= 1e-5f;
}

} // namespace

// Mac 0x221ab0
BIProgramGeneric::BIProgramGeneric(bool articulationFilter, bool connectivityFlag, Real32 groundUpWeight,
                                   Real32 nearbyWeight, Real32 balanceWeight)
{
    {
        RefPointer<BIStrategy> strategy = new BIStrategySubsteps;
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, 1.0f, condition);
    }
    {
        RefPointer<BIStrategy> strategy = new BIStrategySweeping;
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, 0.1f, condition);
    }
    {
        RefPointer<BIStrategy> strategy = new BIStrategyIntegration;
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, 1.0f, condition);
    }
    if (IsEnabled(groundUpWeight)) {
        RefPointer<BIStrategy> strategy = new BIStrategyGroundUp(BIBrick::PointMinimum);
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, groundUpWeight, condition);
    }
    if (IsEnabled(nearbyWeight)) {
        RefPointer<BIStrategy> strategy = new BIStrategyNearby(2);
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, nearbyWeight, condition);
    }
    if (IsEnabled(balanceWeight)) {
        RefPointer<BIStrategy> strategy = new BIStrategyBalance;
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, balanceWeight, condition);
    }
    {
        RefPointer<BIStrategy> strategy = new BIStrategyArticulation;
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, 0.0f, articulationFilter ? -1.0f : 1.0f, 1.0f, condition);
    }
    {
        RefPointer<BIStrategy> strategy = new BIStrategyConnectivity(connectivityFlag);
        RefPointer<Condition> condition = new ConditionTrue;
        AddStrategy(strategy, 0.0f, -1.0f, 1.0f, condition);
    }
}

// Mac 0x222b80 / 0x222be0, Win sdd 0x565cf0
BIProgramGeneric::~BIProgramGeneric()
{
}

// Mac 0x222b50, Win 0x564360
String BIProgramGeneric::GetName() const
{
    return String("Generic");
}

} // namespace LEGO
