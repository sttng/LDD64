// LEGO Digital Designer — restored source
// LEGO::BIStrategyArticulation — removing: the fail score for a brick that splits its graph (an
//   articulation primitive, a separating substep), else the pass score; adding: the pass score for
//   a brick connected to a primitive of the scene (or while nothing is placed), else the fail
//   score.
//
// Home TU 150 "BIStrategyArticulation.cpp" (Mac 0x2136c0-0x213a2f).
// RTTI: class LEGO::BIStrategyArticulation : BIStrategy. Mac sizeof 0x1c + Object. Mac vtable
// 0x72b93c (GetName 0x2138f0, D1 0x213920, D0 0x213940, ScoreRemove(brick) 0x2136c0,
// ScoreAdd(brick) 0x213720), Win vtable 0xa2bd14 (GetName 0x591950 (shared with
// BIAnalyzerArticulation), ScoreRemove(brick) 0x58fe90, ScoreAdd(brick) 0x58ff30), sdd 0x5646d0.
// Constructed (inline) by BIProgramGeneric::BIProgramGeneric (Mac 0x221ab0).
#pragma once
#include "BIStrategy.h"

namespace LEGO {

class BIStrategyArticulation : public BIStrategy {
public:
    BIStrategyArticulation() {}
    virtual ~BIStrategyArticulation();                                  // Mac 0x213920 / 0x213940, Win sdd 0x5646d0

    virtual String GetName() const;                    // Mac 0x2138f0, Win 0x591950 (shared with BIAnalyzerArticulation) ("Articulation")
    virtual Real32 ScoreRemove(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);   // idx 7: Mac 0x2136c0, Win 0x58fe90
    virtual Real32 ScoreAdd(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);      // idx 8: Mac 0x213720, Win 0x58ff30

    using BIStrategy::ScoreRemove;
    using BIStrategy::ScoreAdd;
};

} // namespace LEGO
