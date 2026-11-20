// LEGO Digital Designer — restored source
// LEGO::BIFilter — a BIProgram made of BIStrategies, each with a Condition on the brick: the
//   non-filter strategies add up a score per candidate brick, the filter strategies (pass
//   score 0, fail score <= -1) reject the bricks they rate negatively; the candidate with the
//   best score that passes every filter is chosen (SelectBrick). When none passes, the first
//   strategy that can relax (BIStrategy::Relax) does so and the selection is repeated.
// LEGO::BIFilter::Condition / ConditionTrue — the brick predicate of an entry.
//
// Home TU 166 "BIFilter.cpp" (Mac 0x224750-0x2262ef); GetName and the destructors are emitted
// in TU 162 (Mac 0x222850-0x2229f0, 0x222c60-0x222df0).
// RTTI: class LEGO::BIFilter : BIProgram.
//   Mac sizeof 0x1c + Object: vptr +0, m_parent +4, m_strategies +8 (BasicArray<Pair<
//       RefPointer<Condition>, RefPointer<BIStrategy> > >), m_noFilters +0x18, Object +0x1c.
//       Vtable 0x72db8c {0x222840, GetName 0x222850, D1 0x222880, D0 0x2228f0, Reset 0x224d80,
//       SelectBrick 0x225640, Unknown06 0x225790, Unknown07 0x2257d0, Unknown08 0x222970,
//       FilterBricks 0x225810, CreateRelaxed 0x225b20, HasNoFilters 0x225e00, Check 0x224dc0};
//       base-object constructor 0x224750 (VTT), complete one inline (0x225b20).
//   Win: vfptr +0, vbptr +4, m_parent +8, m_strategies +0xc, m_noFilters +0x1c. Vtable 0xa2bc0c
//       {0x5913d0, 0x563f90, 0x58d110, 0x58d830, 0x58d9f0, 0x58da70, 0x563da0, 0x58dae0,
//       0x58dd40, 0x58df70, 0x58d180}, sdd 0x564200.
//   Condition: Mac abstract (no vtable of its own), Win vtable 0xa2bc50 {pure}, Object +4/+8.
//   ConditionTrue: Mac sizeof 0x0c, vtable 0x770320 {0x222d60, D1 0x222d70, D0 0x222d90};
//       Win vtable 0xa2bc6c {0x5e4100}, sdd 0x564120.
#pragma once
#include "BIProgram.h"
#include "BIStrategy.h"

namespace LEGO {

class BIBrickPrimitive;

class BIFilter : public BIProgram {
public:
    // Brick predicate of a strategy entry. (name inferred)
    class Condition : public virtual Object {
    public:
        virtual bool Evaluate(const BIBrick* brick) const = 0;             // slot 0 (name inferred)
    };
    // Always true. (Mac 0x222d60 / Win 0x5e4100; destructor Mac 0x222d70 / 0x222d90, Win sdd
    // 0x564120, all in TU 162)
    class ConditionTrue : public Condition {
    public:
        virtual bool Evaluate(const BIBrick* brick) const;
        virtual ~ConditionTrue();
    };
    typedef Pair<RefPointer<Condition>, RefPointer<BIStrategy> > Entry;
    typedef BIStrategy::ScoreMap ScoreMap;

    // No strategies, m_noFilters true. Mac 0x224750 (base-object constructor), inline in
    // CreateRelaxed and on Win
    BIFilter();
    virtual ~BIFilter();                                                   // Mac 0x222880 / 0x2228f0 (TU 162), Win sdd 0x564200

    virtual String GetName() const;                                        // Mac 0x222850, Win 0x563f90 ("?")
    // BIStrategy::Reset of every strategy. Mac 0x224d80, Win 0x58d110
    virtual void Reset();
    // Select(…, 0) until a brick is found or no strategy relaxes (Relax). Mac 0x225640,
    // Win 0x58d830
    virtual RefPointer<BIEvent> SelectBrick(bool destructive, const BasicSet<BIBrick*>& bricks, const EventList& events,
                                            BGScene* scene, const BasicArray<BIBrickSubstep*>& substeps);
    // BIStrategy::Unknown10 / Unknown11 of every strategy. Mac 0x225790 / 0x2257d0,
    // Win 0x58d9f0 / 0x58da70
    virtual void Unknown06(void* argument);
    virtual void Unknown07();
    // Mac 0x225810, Win 0x58dae0
    virtual void FilterBricks(bool destructive, const BasicArray<BIBrick*>& bricks, const EventList& events,
                              BGScene* scene, EventList& result);
    // A new BIFilter with every entry (conditions replaced by ConditionTrue) but the first
    // filter strategy. Mac 0x225b20, Win 0x58dd40
    virtual RefPointer<BIProgram> CreateRelaxed() const;
    // Mac 0x225e00, Win 0x58df70
    virtual bool HasNoFilters() const;
    // `brick` when every filter strategy whose condition holds rates it >= 0 (within 1e-5),
    // else null. `context` is passed (by address) to the strategies; `primitives` is not
    // used. Mac 0x224dc0, Win 0x58d180 (name inferred)
    virtual BIBrick* Check(bool destructive, BIBrick* brick, UInt32 context, BGScene* scene,
                           const BasicSet<BIBrickPrimitive*>& primitives, const EventList& events) const;   // idx 12

    // The best candidate: the non-filter strategies score every brick (ScoreRemove /
    // ScoreAdd(set)); bricks that intersect one of `substeps` get a tenth; the bricks with a
    // score >= threshold (within 1e-5) whose substep analyzer is active are tried from the
    // highest score down (equal scores: the last inserted first) with Check. Mac 0x224f10,
    // Win 0x58d2c0 (name inferred)
    RefPointer<BIEvent> Select(bool destructive, const BasicSet<BIBrick*>& bricks, const EventList& events,
                               BGScene* scene, const BasicArray<BIBrickSubstep*>& substeps, Real32 threshold);
    // The first strategy that relaxes (trace "Retrying"): true; false when none does. Inlined on
    // Mac, Win 0x58d910 (name inferred)
    bool Relax();
    // Appends the entry; a filter strategy clears m_noFilters. Mac 0x2247c0 (name inferred)
    void Add(const RefPointer<BIStrategy>& strategy, const RefPointer<Condition>& condition);
    // With a weight of at least 1e-5: sets the strategy's weight and adds it. Mac 0x224b00
    // (name inferred)
    void AddStrategy(const RefPointer<BIStrategy>& strategy, Real32 weight, const RefPointer<Condition>& condition);
    // With a weight of at least 1e-5: BIStrategy::Configure, the strategy's parent becomes this
    // filter, adds it. Mac 0x224c30 (name inferred)
    void AddStrategy(const RefPointer<BIStrategy>& strategy, Real32 passScore, Real32 failScore, Real32 weight,
                     const RefPointer<Condition>& condition);

    const BasicArray<Entry>& GetStrategies() const { return m_strategies; }

    BasicArray<Entry> m_strategies;   // +0x08 (name inferred)
    bool              m_noFilters;    // +0x18 (name inferred)
};

} // namespace LEGO
