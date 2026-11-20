// LEGO Digital Designer — restored source
// TU 166 "BIFilter.cpp" (Mac 0x224750-0x2262ef): LEGO::BIFilter. See BIFilter.h.
// Template code of the TU: BasicArray<BIFilter::Entry> (PushBack 0x224930, Clear 0x225e10,
// Win 0x58df90), BasicMap<BIBrick*, Real32> (operator[] 0x225f50, destructors 0x225610 /
// 0x225eb0, Win sdd 0x58ca00; erase 0x225f00, Win 0x58c9a0), BasicMultimap<Real32, BIBrick*>
// (destructors 0x2255e0 / 0x226210, Win sdd 0x58e1b0; erase 0x226260, Win 0x58e070; insert
// 0x226140).
#include "BIFilter.h"
#include "BI.h"
#include "BIBrick.h"
#include "BIBrickSubstep.h"
#include "BIAnalyzer.h"
#include "BIEvent.h"
#include <math.h>

namespace LEGO {

// Mac 0x224750 (base-object constructor), inline in CreateRelaxed and on Win
BIFilter::BIFilter()
    : m_noFilters(true)
{
}

// Mac 0x222880 / 0x2228f0 (TU 162), Win sdd 0x564200
BIFilter::~BIFilter()
{
}

// Mac 0x222850 (TU 162), Win 0x563f90
String BIFilter::GetName() const
{
    return String("?");
}

// Mac 0x2247c0
void BIFilter::Add(const RefPointer<BIStrategy>& strategy, const RefPointer<Condition>& condition)
{
    if (strategy->m_isFilter == 1)
        m_noFilters = false;
    m_strategies.PushBack(Entry(condition, strategy));
}

// Mac 0x224b00
void BIFilter::AddStrategy(const RefPointer<BIStrategy>& strategy, Real32 weight, const RefPointer<Condition>& condition)
{
    if (fabsf(weight) >= 1e-5f) {
        // NOTE: unlike the other overload, the strategy's parent is not set.
        strategy->SetWeight(weight);
        Add(strategy, condition);
    }
}

// Mac 0x224c30
void BIFilter::AddStrategy(const RefPointer<BIStrategy>& strategy, Real32 passScore, Real32 failScore, Real32 weight,
                           const RefPointer<Condition>& condition)
{
    if (fabsf(weight) >= 1e-5f) {
        strategy->Configure(passScore, failScore, weight);
        strategy->m_parent = this;
        Add(strategy, condition);
    }
}

// Mac 0x224d80, Win 0x58d110
void BIFilter::Reset()
{
    for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End(); ++it)
        it->second->Reset();
}

// Mac 0x224dc0, Win 0x58d180
BIBrick* BIFilter::Check(bool destructive, BIBrick* brick, UInt32 context, BGScene* scene,
                         const BasicSet<BIBrickPrimitive*>&, const EventList& events) const
{
    for (BasicArray<Entry>::ConstIterator it = m_strategies.Begin(); it != m_strategies.End(); ++it) {
        BIStrategy* strategy = it->second;
        if (strategy->m_isFilter == 0 || !it->first->Evaluate(brick))
            continue;
        Real32 score = destructive ? strategy->ScoreRemove(brick, context, events, scene)
                                   : strategy->ScoreAdd(brick, context, events, scene);
        Real32 negative = 0.0f - score;
        if (negative > 0.0f && fabsf(negative) > 1e-5f)
            return 0;
    }
    return brick;
}

// Mac 0x224f10, Win 0x58d2c0
RefPointer<BIEvent> BIFilter::Select(bool destructive, const BasicSet<BIBrick*>& bricks, const EventList& events,
                                     BGScene* scene, const BasicArray<BIBrickSubstep*>& substeps, Real32 threshold)
{
    ScoreMap scores;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it)
        scores[*it] = 0.0f;
    for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End(); ++it) {
        BIStrategy* strategy = it->second;
        if (strategy->m_isFilter != 0)
            continue;
        if (!destructive)
            strategy->ScoreAdd(bricks, events, scene, scores);
        else
            strategy->ScoreRemove(bricks, events, scene, scores);
    }

    // candidates that overlap a proposed substep count a tenth
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        if ((*it)->Intersects(substeps))
            scores[*it] = scores[*it] * 0.1f;
    }

    BasicMultimap<Real32, BIBrick*> ranking;
    for (BasicSet<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        Real32 below = threshold - scores[brick];
        if (!(below <= 0.0f || fabsf(below) <= 1e-5f))
            continue;
        if (brick->GetType() == BIBrick::TypeSubstep && brick->GetGroup() != 0 && !brick->GetGroup()->IsActive())
            continue;
        ranking.Insert(scores[brick], brick);
    }

    BasicSet<BIBrickPrimitive*> primitives;
    BI::GetPrimitives(bricks, primitives);
    for (BasicMultimap<Real32, BIBrick*>::Iterator it = ranking.End(); it != ranking.Begin();) {
        --it;
        BIBrick* chosen = Check(destructive, it->second, 0, scene, primitives, events);
        if (chosen != 0)
            return RefPointer<BIEvent>(new BIEventBrick(chosen));
    }
    return RefPointer<BIEvent>();
}

// inlined on Mac, Win 0x58d910
bool BIFilter::Relax()
{
    for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End(); ++it) {
        if (it->second->Relax()) {
            BI::Trace(String("Retrying"), 1);
            return true;
        }
    }
    return false;
}

// Mac 0x225640, Win 0x58d830
RefPointer<BIEvent> BIFilter::SelectBrick(bool destructive, const BasicSet<BIBrick*>& bricks, const EventList& events,
                                          BGScene* scene, const BasicArray<BIBrickSubstep*>& substeps)
{
    RefPointer<BIEvent> event;
    do {
        event = Select(destructive, bricks, events, scene, substeps, 0.0f);
        if (!event.IsNull())
            return event;
    } while (Relax());
    return RefPointer<BIEvent>();
}

// Mac 0x225790, Win 0x58d9f0
void BIFilter::Unknown06(void* argument)
{
    for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End(); ++it)
        it->second->Unknown10(argument);
}

// Mac 0x2257d0, Win 0x58da70
void BIFilter::Unknown07()
{
    for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End(); ++it)
        it->second->Unknown11();
}

// Mac 0x225810, Win 0x58dae0
void BIFilter::FilterBricks(bool destructive, const BasicArray<BIBrick*>& bricks, const EventList& events,
                            BGScene* scene, EventList& result)
{
    // NOTE: the original also fills a score map (0 per brick) that it never reads.
    ScoreMap unused;
    for (BasicArray<BIBrick*>::ConstIterator b = bricks.Begin(); b != bricks.End(); ++b) {
        BIBrick* brick = *b;
        unused[brick] = 0.0f;
        UInt32 context = 0;
        bool passes = true;
        for (BasicArray<Entry>::Iterator it = m_strategies.Begin(); it != m_strategies.End() && passes; ++it) {
            BIStrategy* strategy = it->second;
            if (strategy->m_isFilter == 0 || !it->first->Evaluate(brick))
                continue;
            Real32 score = destructive ? strategy->ScoreRemove(brick, context, events, scene)
                                       : strategy->ScoreAdd(brick, context, events, scene);
            Real32 negative = 0.0f - score;
            if (negative > 0.0f && fabsf(negative) > 1e-5f)
                passes = false;
        }
        if (passes)
            result.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
    }
}

// Mac 0x225b20, Win 0x58dd40
RefPointer<BIProgram> BIFilter::CreateRelaxed() const
{
    RefPointer<BIFilter> filter = new BIFilter;
    bool filterSkipped = false;
    for (BasicArray<Entry>::ConstIterator it = m_strategies.Begin(); it != m_strategies.End(); ++it) {
        const RefPointer<BIStrategy>& strategy = it->second;
        if (strategy->m_isFilter != 0) {
            bool keep = filterSkipped;
            filterSkipped = true;
            if (!keep)
                continue;
        }
        filter->Add(strategy, RefPointer<Condition>(new ConditionTrue));
    }
    return RefPointer<BIProgram>(filter);
}

// Mac 0x225e00, Win 0x58df70
bool BIFilter::HasNoFilters() const
{
    return m_noFilters;
}

} // namespace LEGO
