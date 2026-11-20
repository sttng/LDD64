// LEGO Digital Designer — restored source
// LEGO::BIStrategy — base of the scoring rules of a BIFilter: for the candidate bricks of the
//   next building-guide event a strategy adds a score per brick (Score…(set), through
//   Accumulate) or rates one brick (Score…(brick)). "Remove" scores rank the bricks to take away
//   next (destructive generation), "Add" scores the bricks to put in next (constructive). A
//   strategy configured with pass score 0 and fail score <= -1 is a filter: the BIFilter then
//   rejects every brick it rates negatively (BIFilter::Check) and leaves it out of the sums.
//   Subclasses (each in its own TU): BIStrategyArticulation, BIStrategyBalance,
//   BIStrategyConnectivity, BIStrategyGroundUp, BIStrategyIntegration, BIStrategyNearby,
//   BIStrategySubsteps, BIStrategySweeping.
//
// Home TU 179 "BIStrategy.cpp" (Mac 0x234900-0x234dcf); the inline virtuals are emitted in TU
// 150 (Mac 0x2137b0-0x2138a0).
// RTTI: class LEGO::BIStrategy : BIFacade (: virtual Object).
//   Mac sizeof 0x1c + Object: vptr +0, m_parent +4, m_isFilter +8, m_isFilter2 +0xc,
//       m_weight +0x10, m_passScore +0x14, m_failScore +0x18, Object +0x1c (+0x20 for the
//       subclasses with one member at +0x1c). Vtable 0x72b9cc {GetFacadeType 0x2137b0, pure,
//       D1 0x2137c0, D0 0x2137e0, Reset 0x213820, ScoreRemove(set) 0x234960, ScoreAdd(set)
//       0x234c90, ScoreRemove(brick) 0x213830, ScoreAdd(brick) 0x213840, Relax 0x213850,
//       Unknown10 0x213860, Unknown11 0x213870}; base-object constructor 0x234900 (VTT).
//   Win: vfptr +0, vbptr +4, m_parent +8, then the members +4. Vtable 0xa2bbcc {0x563e90, pure,
//       0x56b1c0, 0x563ea0, 0x58bee0, 0x563ea0, 0x58bf80, 0x57ba30, 0x559e90, 0x56b1c0}: MSVC
//       groups the overloads — Win slot 3 = ScoreRemove(brick), 4 = ScoreRemove(set),
//       5 = ScoreAdd(brick), 6 = ScoreAdd(set). sdd 0x563f20.
// The `events` argument is the brick events placed before the current position (the
// generator's GetBrickEventsBefore), `scene` the generator's BGScene; `context` points at a
// UInt32 that the caller initialises to 0 (no restored strategy reads it).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "BIFacade.h"

namespace LEGO {

class BIBrick;
class BIEvent;
class BGScene;

class BIStrategy : public BIFacade {
public:
    typedef BasicMap<BIBrick*, Real32> ScoreMap;   // (name inferred)
    typedef BasicArray<RefPointer<BIEvent> > EventList;

    // pass score 1, fail score 0, weight 0, not a filter. Mac 0x234900 (base-object
    // constructor), inlined on Win
    BIStrategy();
    virtual ~BIStrategy();                                                 // Mac 0x2137c0 / 0x2137e0, Win sdd 0x563f20

    virtual UInt32 GetFacadeType() const;                                  // idx 0: Mac 0x2137b0, Win 0x563e90 (4)
    // Called before a new generation (BIFilter::Reset). Mac 0x213820, Win 0x56b1c0
    // (name inferred)
    virtual void Reset();                                                  // idx 4
    // Accumulate(scores, brick, ScoreRemove(brick, …)) for every brick. Mac 0x234960,
    // Win 0x58bee0 (name inferred)
    virtual void ScoreRemove(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                             ScoreMap& scores);                            // idx 5
    // The same with ScoreAdd(brick, …). Mac 0x234c90, Win 0x58bf80 (name inferred)
    virtual void ScoreAdd(const BasicSet<BIBrick*>& bricks, const EventList& events, BGScene* scene,
                          ScoreMap& scores);                               // idx 6
    // 0. Mac 0x213830 / 0x213840, Win 0x563ea0 (names inferred)
    virtual Real32 ScoreRemove(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);   // idx 7
    virtual Real32 ScoreAdd(BIBrick* brick, UInt32& context, const EventList& events, BGScene* scene);      // idx 8
    // Loosens the strategy for another try when no brick passed (BIFilter::Relax); false: it
    // cannot. Mac 0x213850, Win 0x57ba30 (name inferred)
    virtual bool Relax();                                                  // idx 9
    // Forwarded by BIFilter (slots 6 / 7 of BIProgram); empty here and in every restored
    // strategy. Mac 0x213860 / 0x213870, Win 0x559e90 / 0x56b1c0 (meaning unknown)
    virtual void Unknown10(void* argument);                                // idx 10
    virtual void Unknown11();                                              // idx 11

    // Adds `score` to the brick's entry (a substep's score is first multiplied by its primitive
    // count, its analyzer's weight and its own weight) and the result by m_weight — when the
    // score and the entry are both >= 0 (within 1e-5). Otherwise the entry becomes the smaller
    // of the two. Mac 0x2349e0, Win 0x58c0d0 (name inferred)
    void Accumulate(ScoreMap& scores, BIBrick* brick, Real32 score);
    // Mac 0x234d10 (inlined on Win; name inferred)
    void Configure(Real32 passScore, Real32 failScore, Real32 weight);

    bool IsFilter() const { return m_isFilter != 0; }
    Real32 GetWeight() const { return m_weight; }
    void SetWeight(Real32 weight) { m_weight = weight; }

    UInt32 m_isFilter;    // +0x08 pass score 0 and fail score <= -1 (name inferred)
    UInt32 m_isFilter2;   // +0x0c the same value, never read here (name inferred)
    Real32 m_weight;      // +0x10 (name inferred)
    Real32 m_passScore;   // +0x14 (name inferred)
    Real32 m_failScore;   // +0x18 (name inferred)
};

} // namespace LEGO
