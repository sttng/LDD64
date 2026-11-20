// LEGO Digital Designer — restored source
// TU 149 "BIGenerator.cpp" (Mac 0x20f5b0-0x2136bf): BIFacade and BIGenerator. See
// BIGenerator.h. The Windows build inlines nearly everything (Win 0x55cdd0-0x56039f).
#include "BIGenerator.h"
#include "BIGeneratorDestructive.h"
#include "BI.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"
#include "BIGraph.h"
#include "BICameraChangeFinder.h"
#include "BGApplication.h"
#include "BGScene.h"

namespace LEGO {

// ---- BIFacade ---------------------------------------------------------------------------------

// Mac 0x212910 / D0 0x212930 (thunks 0x212970 / 0x212990), Win sdd 0x560160
BIFacade::~BIFacade()
{
}

// ---- BIGenerator ------------------------------------------------------------------------------

// Mac 0x20f5b0
BIGenerator::BIGenerator(BGApplication* application)
    : m_application(application), m_current(0), m_changed(true)
{
}

// Mac 0x2129f0 / D0 0x212a50 (body 0x212c50; thunks 0x212b70 / 0x212bd0), Win sdd 0x560360
BIGenerator::~BIGenerator()
{
}

// Mac 0x2129e0, Win 0x58a960
UInt32 BIGenerator::GetFacadeType() const
{
    return 1;
}

// Mac 0x212540
BIEvent* BIGenerator::GetEvent(Int32 offset)
{
    UInt32 index = m_current + offset;
    if ((Int32)index > 0 && index <= m_events.GetSize())
        return m_events[index - 1];
    return 0;
}

// Mac 0x211890
bool BIGenerator::Rewind()
{
    GetScene()->Clear();
    m_current = 0;
    m_allowedBricks.Clear();
    return true;
}

// Mac 0x2118f0
bool BIGenerator::FastForward()
{
    BasicSet<BIBrick*> bricks;
    BasicArray<RefPointer<BIEvent> > events;
    GetBrickEventsAfter(events);
    BasicSet<BIBrickPrimitive*> primitives;
    BI::GetPrimitives(events, primitives);
    BI::GetBricks(primitives, bricks);
    if (GetScene()->AddBrick(bricks, true)) {
        m_current = m_events.GetSize();
        m_allowedBricks.Clear();
    }
    return true;
}

// Mac 0x212730
bool BIGenerator::FindBrick(const BIBrick* brick, Int32& offset)
{
    for (UInt32 i = 0; i < m_events.GetSize(); ++i)
        if (m_events[i]->GetBrick() == brick) {
            offset = (Int32)(i - m_current);
            return true;
        }
    return false;
}

// Mac 0x212ac0
UInt32 BIGenerator::GetCurrent() const
{
    return m_current;
}

// Mac 0x212ad0
UInt32 BIGenerator::GetCompleteCount()
{
    if (IsComplete())
        return m_events.GetSize();
    return 0;
}

// Mac 0x211b10
bool BIGenerator::IsAtStart() const
{
    return m_current == 0;
}

// Mac 0x211b20
bool BIGenerator::IsAtEnd()
{
    if (GetEvent(1) != 0)
        return false;
    return IsComplete();
}

// Mac 0x211f00
bool BIGenerator::IsComplete()
{
    BasicSet<BIBrickPrimitive*> primitives;
    BasicArray<RefPointer<BIEvent> > none;
    CollectRemainingPrimitives(primitives, false, false, none);
    return primitives.GetSize() == 0;
}

// Mac 0x20fc30
RefPointer<BIEvent> BIGenerator::TakeNextEvent()
{
    RefPointer<BIEvent> next = GetEvent(1);
    if (next.IsNull())
        return RefPointer<BIEvent>();
    BasicArray<RefPointer<BIEvent> > rest;
    RefPointer<BIEvent> taken;
    UInt32 i = 0;
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = m_events.Begin(); it != m_events.End(); ++it, ++i) {
        if (i == m_current)
            taken = *it;
        else
            rest.PushBack(*it);
    }
    m_events = rest;
    m_changed = true;
    return taken;
}

// Mac 0x20fe60
void BIGenerator::Truncate()
{
    while (m_current < m_events.GetSize())
        m_events.PopBack();
}

// Mac 0x20fec0
bool BIGenerator::Order(BasicArray<RefPointer<BIEvent> >& events, BasicSet<BIBrickPrimitive*>& primitives)
{
    RefPointer<BIGeneratorDestructive> generator = new BIGeneratorDestructive(m_application);
    return generator->Order(events, primitives);
}

// Mac 0x20f690
bool BIGenerator::BuildCameraSteps()
{
    BasicArray<BIBrick*>& substeps = m_application->GetSubsteps();
    if (substeps.IsEmpty())
        return true;
    BasicSet<BIBrickPrimitive*> remaining(substeps.Back()->GetPrimitiveSet());
    BICameraChangeFinder* finder = m_application->GetCameraChangeFinder();
    finder->Begin(remaining);
    UInt32 groups = 0;
    while (remaining.GetSize() != 0) {
        BasicSet<BIBrickPrimitive*> component;
        (*remaining.Begin())->CollectConnected(component, remaining);
        BIBrick* brick;
        if (component.GetSize() == 1) {
            brick = *component.Begin();
        } else {
            RefPointer<BIBrickSubstep> substep = new BIBrickSubstep(component, 0, 1.0f);
            brick = BIBrickRepository::GetInstance().AddSubstep(substep);
        }
        finder->AddBrick(brick);
        ++groups;
        for (BasicSet<BIBrickPrimitive*>::Iterator it = component.Begin(); it != component.End(); ++it)
            remaining.Erase(*it);
    }
    if (groups > 1) {
        BasicArray<RefPointer<BIEvent> > found;
        finder->GetEvents(found);
        BasicArray<RefPointer<BIEvent> > events;
        events.PushBack(found[0]);
        for (UInt32 i = 1; i < found.GetSize(); ++i) {
            if (found[i]->GetBrick() != 0 && found[i - 1]->m_type != BIEvent::TypeStep)
                events.PushBack(RefPointer<BIEvent>(new BIEventStep));
            events.PushBack(found[i]);
        }
        SetEvents(events);
    }
    return true;
}

// Mac 0x20ff70
bool BIGenerator::InsertBrick(BIBrick* brick)
{
    BasicArray<RefPointer<BIEvent> > before;       // before the current position, before the brick
    BasicArray<RefPointer<BIEvent> > beforeAfter;  // before the current position, after the brick
    BasicArray<RefPointer<BIEvent> > inserted;
    BasicArray<RefPointer<BIEvent> > after;        // from the current position on
    UInt32 i = 0;
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = m_events.Begin(); it != m_events.End(); ++it, ++i) {
        BIBrick* eventBrick = (*it)->GetBrick();
        if (eventBrick == 0 || !eventBrick->Intersects(brick)) {
            if (i < m_current) {
                if (inserted.IsEmpty())
                    before.PushBack(*it);
                else
                    beforeAfter.PushBack(*it);
            } else {
                after.PushBack(*it);
            }
        } else if (!eventBrick->Intersects(inserted)) {
            inserted.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
        }
    }
    if (inserted.IsEmpty())
        inserted.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
    BasicArray<RefPointer<BIEvent> > rest(beforeAfter);
    rest.InsertRange(rest.End(), inserted.Begin(), inserted.End());
    rest.InsertRange(rest.End(), after.Begin(), after.End());
    BasicSet<BIBrickPrimitive*> placed;
    BasicArray<RefPointer<BIEvent> > none;
    CollectRemainingPrimitives(placed, false, false, none);
    BI::GetPrimitives(before, placed);
    if (!Order(rest, placed))
        return false;
    m_events = before;
    m_events.InsertRange(m_events.End(), rest.Begin(), rest.End());
    m_changed = true;
    m_current = beforeAfter.GetSize() + before.GetSize();
    return true;
}

// Brick events before the current position (`beforeCurrent`) or all of them.
static void BrickEvents(const BasicArray<RefPointer<BIEvent> >& events, UInt32 current, bool beforeCurrent,
                        BasicArray<RefPointer<BIEvent> >& out)
{
    UInt32 end = beforeCurrent ? current : events.GetSize();
    for (UInt32 i = 0; i < end; ++i)
        if (events[i]->GetBrick() != 0)
            out.PushBack(events[i]);
}

// Primitives of the bricks of `extra` (their primitive sets).
static void ExtraPrimitives(const BasicArray<RefPointer<BIEvent> >& extra, BasicSet<BIBrickPrimitive*>& primitives)
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = extra.Begin(); it != extra.End(); ++it)
        if (BIBrick* brick = (*it)->GetBrick())
            for (BasicSet<BIBrickPrimitive*>::ConstIterator p = brick->GetPrimitiveSet().Begin();
                 p != brick->GetPrimitiveSet().End(); ++p)
                primitives.Insert(*p);
}

// Mac 0x210ad0
void BIGenerator::CollectRemainingPrimitives(BasicSet<BIBrickPrimitive*>& primitives, bool beforeCurrent,
                                             bool fromApplication, const BasicArray<RefPointer<BIEvent> >& extra)
{
    if (m_application->GetSubsteps().IsEmpty())
        return;
    BasicSet<BIBrickPrimitive*> placed;
    if (!fromApplication) {
        BasicArray<RefPointer<BIEvent> > events;
        BrickEvents(m_events, m_current, beforeCurrent, events);
        BI::GetPrimitives(events, placed);
        ExtraPrimitives(extra, placed);
        const BasicSet<BIBrickPrimitive*>& all = m_application->GetSubsteps().Back()->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = all.Begin(); it != all.End(); ++it)
            if (!(*it)->Intersects(placed))
                primitives.Insert(*it);
    } else {
        m_application->GetPrimitives(placed, !beforeCurrent);
        ExtraPrimitives(extra, placed);
        BasicSet<BIBrickPrimitive*> free;
        BIBrickRepository::GetInstance().GetFreePrimitives(free);
        for (BasicSet<BIBrickPrimitive*>::Iterator it = free.Begin(); it != free.End(); ++it)
            if (!(*it)->Intersects(placed))
                primitives.Insert(*it);
    }
}

// Brick events from the first marker on, if that marker is at or after `current`.
static UInt32 FirstMarker(const BasicArray<RefPointer<BIEvent> >& events)
{
    for (UInt32 i = 0; i < events.GetSize(); ++i)
        if (events[i]->m_type == BIEvent::TypeMarker)
            return i;
    return events.GetSize();
}

// Mac 0x2110b0
void BIGenerator::CollectRemainingBricks(BasicSet<BIBrick*>& bricks, bool beforeCurrent, bool fromApplication,
                                         const BasicArray<RefPointer<BIEvent> >& extra)
{
    if (m_application->GetSubsteps().IsEmpty())
        return;
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    BasicSet<BIBrickPrimitive*> placed;
    BasicSet<BIBrick*> candidates;
    UInt32 marker = FirstMarker(m_events);
    if (!fromApplication) {
        BasicArray<RefPointer<BIEvent> > events;
        BrickEvents(m_events, m_current, beforeCurrent, events);
        if (beforeCurrent && marker < m_events.GetSize() && (Int32)(marker - m_current) >= 0)
            for (UInt32 i = marker; i < m_events.GetSize(); ++i)
                if (m_events[i]->GetBrick() != 0)
                    events.PushBack(m_events[i]);
        BI::GetPrimitives(events, placed);
        ExtraPrimitives(extra, placed);
        repository.GetFreeBricksWithin(m_application->GetSubsteps().Back()->GetPrimitiveSet(), candidates);
    } else {
        m_application->GetPrimitives(placed, !beforeCurrent);
        if (marker < m_events.GetSize() && (Int32)(marker - m_current) >= 0)
            for (UInt32 i = marker; i < m_events.GetSize(); ++i)
                if (BIBrick* brick = m_events[i]->GetBrick())
                    brick->GetPrimitives(placed);
        ExtraPrimitives(extra, placed);
        repository.GetFreeBricks(candidates);
    }
    for (BasicSet<BIBrick*>::Iterator it = candidates.Begin(); it != candidates.End(); ++it)
        if (!(*it)->Intersects(placed))
            bricks.Insert(*it);
}

// Mac 0x210fc0
Int32 BIGenerator::Generate()
{
    BasicSet<BIBrick*> bricks;
    BasicArray<RefPointer<BIEvent> > none;
    CollectRemainingBricks(bricks, false, false, none);
    return Generate(bricks, true);
}

// Mac 0x211b70
void BIGenerator::GetSteps(BasicArray<BI::Step>& steps)
{
    BasicArray<RefPointer<BIEvent> > events;
    GetEvents(events);
    BI::GetSteps(events, steps);
}

// Mac 0x212b00
void BIGenerator::GetEvents(BasicArray<RefPointer<BIEvent> >& events) const
{
    if (&events != &m_events)
        events = m_events;
}

// Mac 0x212b40
UInt32 BIGenerator::GetEventCount() const
{
    return m_events.GetSize();
}

// Mac 0x211770
void BIGenerator::GetBrickEvents(BasicArray<RefPointer<BIEvent> >& events) const
{
    for (BasicArray<RefPointer<BIEvent> >::ConstIterator it = m_events.Begin(); it != m_events.End(); ++it)
        if ((*it)->GetBrick() != 0)
            events.PushBack(*it);
}

// Mac 0x2126f0
void BIGenerator::SetEvents(const BasicArray<RefPointer<BIEvent> >& events)
{
    m_changed = true;
    if (&m_events != &events)
        m_events = events;
}

// Mac 0x212570
void BIGenerator::GetEventsBefore(BasicArray<RefPointer<BIEvent> >& events) const
{
    for (UInt32 i = 0; i < m_current; ++i)
        events.PushBack(m_events[i]);
}

// Mac 0x2117c0
void BIGenerator::GetBrickEventsBefore(BasicArray<RefPointer<BIEvent> >& events) const
{
    for (UInt32 i = 0; i < m_current; ++i)
        if (m_events[i]->GetBrick() != 0)
            events.PushBack(m_events[i]);
}

// Mac 0x2125c0
void BIGenerator::GetBrickEventsReversed(BasicArray<RefPointer<BIEvent> >& events) const
{
    UInt32 size = m_events.GetSize();
    if (m_current > size - 1)
        return;
    for (UInt32 i = size; i-- > 0;) {
        if (m_events[i]->m_type == BIEvent::TypeBrick)
            events.PushBack(m_events[i]);
        // NOTE: with m_current == 0 the original keeps going below index 0 (reads before the
        // array); fixed: stopped at index 0.
        if (i - 1 < m_current || i == 0)
            break;
    }
}

// Mac 0x212620
void BIGenerator::GetEventsAfter(BasicArray<RefPointer<BIEvent> >& events) const
{
    for (UInt32 i = m_current; (Int32)i < (Int32)m_events.GetSize(); ++i)
        events.PushBack(m_events[i]);
}

// Mac 0x212680
void BIGenerator::GetBrickEventsAfter(BasicArray<RefPointer<BIEvent> >& events) const
{
    for (UInt32 i = m_current; (Int32)i < (Int32)m_events.GetSize(); ++i)
        if (m_events[i]->GetBrick() != 0)
            events.PushBack(m_events[i]);
}

// Mac 0x211c60
void BIGenerator::GetTrailingSubsteps(BasicArray<BIBrick*>& bricks) const
{
    UInt32 start = m_events.GetSize();
    while (start > 0) {
        // NOTE: GetBrick of a non-brick event is null and dereferenced (both builds); guarded.
        BIBrick* brick = m_events[start - 1]->GetBrick();
        if (brick == 0 || brick->GetType() != BIBrick::TypeSubstep)
            break;
        --start;
    }
    for (UInt32 i = start; i < m_events.GetSize(); ++i)
        bricks.PushBack(m_events[i]->GetBrick());
}

// Mac 0x212b60
void BIGenerator::Unknown43()
{
}

// Mac 0x211820
bool BIGenerator::IsAllowed(BIBrick* brick)
{
    if (m_allowedBricks.Contains(brick))
        return true;
    return m_application->IsGroupEnabled(brick->GetGroup());
}

// Mac 0x211b50
bool BIGenerator::HasEvents() const
{
    return !m_events.IsEmpty();
}

// Mac 0x211fd0
void BIGenerator::GetCurrentStepBricks(BasicArray<RefPointer<BIEvent> >& events)
{
    BasicArray<RefPointer<BIEvent> > before;
    GetEventsBefore(before);
    if (before.IsEmpty())
        return;
    UInt32 end = before.GetSize();
    while (end > 0 && before[end - 1]->m_type == BIEvent::TypeStep)
        --end;
    UInt32 start = end;
    while (start > 0 && before[start - 1]->m_type != BIEvent::TypeStep)
        --start;
    for (UInt32 i = start; i < before.GetSize(); ++i)
        if (before[i]->m_type == BIEvent::TypeBrick)
            events.PushBack(before[i]);
    BasicArray<RefPointer<BIEvent> > after;
    GetEventsAfter(after);
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = after.Begin(); it != after.End(); ++it) {
        if ((*it)->m_type == BIEvent::TypeBrick)
            events.PushBack(*it);
        else if ((*it)->m_type == BIEvent::TypeStep)
            break;
    }
}

// Mac 0x212270
bool BIGenerator::FindBrickOfType(UInt32 type, Int32& offset, Int32 start)
{
    for (BIEvent* event = GetEvent(start); event != 0; event = GetEvent(++start))
        if (event->GetBrick() != 0 && event->GetBrick()->GetType() == type) {
            offset = start;
            return true;
        }
    return false;
}

// Mac 0x2122f0
BIEvent* BIGenerator::FindNextBrickOfType(UInt32 type, Int32 start)
{
    for (BIEvent* event = GetEvent(start); event != 0; event = GetEvent(++start))
        if (event->GetBrick() != 0 && event->GetBrick()->GetType() == type)
            return GetEvent(start);
    return 0;
}

// Mac 0x212360
BIEvent* BIGenerator::FindPreviousBrickOfType(UInt32 type, Int32 start)
{
    for (BIEvent* event = GetEvent(start); event != 0; event = GetEvent(--start))
        if (event->GetBrick() != 0 && event->GetBrick()->GetType() == type)
            return GetEvent(start);
    return 0;
}

// Mac 0x2123d0
RefPointer<BIBrick> BIGenerator::MakeBrick(UInt32 from, UInt32 to)
{
    UInt32 size = m_events.GetSize();
    if (size < from || to <= from || size < to)
        return RefPointer<BIBrick>();
    BasicSet<BIBrickPrimitive*> primitives;
    for (UInt32 i = from; i < to; ++i)
        if (BIBrick* brick = m_events[i]->GetBrick())
            brick->GetPrimitives(primitives);
    if (primitives.GetSize() == 1)
        return RefPointer<BIBrick>(*primitives.Begin());
    if (primitives.GetSize() == 0)
        return RefPointer<BIBrick>();
    return RefPointer<BIBrick>(new BIBrickSubstep(primitives, 0, 1.0f));
}

// Mac 0x212790
bool BIGenerator::AppendMarker()
{
    for (UInt32 i = 0; i < m_events.GetSize(); ++i)
        if (m_events[i]->m_type == BIEvent::TypeMarker)
            return false;
    m_events.PushBack(RefPointer<BIEvent>(new BIEventUnknown));
    return true;
}

// Mac 0x212870
bool BIGenerator::RemoveMarker(UInt32& index)
{
    for (UInt32 i = 0; i < m_events.GetSize(); ++i)
        if (m_events[i]->m_type == BIEvent::TypeMarker) {
            index = i;
            m_events.Erase(i);
            m_changed = true;
            return true;
        }
    return false;
}

} // namespace LEGO
