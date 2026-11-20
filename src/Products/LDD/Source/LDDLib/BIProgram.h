// LEGO Digital Designer — restored source
// LEGO::BIProgram — the rule set a building-guide generator asks for the next brick: an
//   abstract interface implemented by BIFilter (a weighted list of BIStrategies) and its
//   preset BIProgramGeneric. The generators get it from the application (BGApplication slot
//   idx 31, BGAutomaticApplication +0x4c).
//
// Inline in the original (emitted in TU 162 "BIProgramGeneric.cpp": GetFacadeType Mac
// 0x222840, Unknown08 0x222970, destructor 0x222a80 / 0x222aa0).
// RTTI: class LEGO::BIProgram : BIFacade (: virtual Object). Mac vtable 0x72dc1c
// {GetFacadeType 0x222840, pure, D1 0x222a80, D0 0x222aa0, pure x4, Unknown08 0x222970,
// pure x3}; Win vtable 0xa2bb8c {0x5913d0, pure x5, 0x563da0, pure x3}, destructor 0x563db0,
// sdd 0x563e20. The slot order is the same on both builds.
// `destructive`: true when the generator takes the model apart (BIStrategy "Remove" scores),
// false when it builds it (the "Add" scores).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BIFacade.h"

namespace LEGO {

class BIBrick;
class BIBrickSubstep;
class BIEvent;
class BGScene;

class BIProgram : public BIFacade {
public:
    typedef BasicArray<RefPointer<BIEvent> > EventList;

    BIProgram() {}
    virtual ~BIProgram();                                                  // Mac 0x222a80 / 0x222aa0, Win 0x563db0 / sdd 0x563e20

    virtual UInt32 GetFacadeType() const;                                  // idx 0: Mac 0x222840, Win 0x5913d0 (2)
    // Resets the rules before a generation. (name inferred)
    virtual void Reset() = 0;                                              // idx 4
    // The event of the best candidate (null when none passes, even relaxed). `events`: the
    // brick events placed before the current position; `substeps`: candidates that intersect
    // one of these get a tenth of their score. (name inferred)
    virtual RefPointer<BIEvent> SelectBrick(bool destructive, const BasicSet<BIBrick*>& bricks, const EventList& events,
                                            BGScene* scene, const BasicArray<BIBrickSubstep*>& substeps) = 0;   // idx 5
    // Forwarded to the strategies (their slots idx 10 / 11; meaning unknown).
    virtual void Unknown06(void* argument) = 0;                            // idx 6
    virtual void Unknown07() = 0;                                          // idx 7
    // Empty. Mac 0x222970, Win 0x563da0 (meaning unknown)
    virtual void Unknown08();                                              // idx 8
    // Appends a brick event for every brick that passes the filter strategies. (name inferred)
    virtual void FilterBricks(bool destructive, const BasicArray<BIBrick*>& bricks, const EventList& events,
                              BGScene* scene, EventList& result) = 0;     // idx 9
    // A copy of the program without its first filter strategy. (name inferred)
    virtual RefPointer<BIProgram> CreateRelaxed() const = 0;               // idx 10
    // No strategy is a filter. (name inferred)
    virtual bool HasNoFilters() const = 0;                                 // idx 11
};

} // namespace LEGO
