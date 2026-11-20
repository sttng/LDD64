// LEGO Digital Designer — restored source
// TU 212 "BIGeneratorConstructive.cpp" (Mac 0x25ab70-0x25e34f). See BIGeneratorConstructive.h.
#include "BIGeneratorConstructive.h"
#include "BIGeneratorDestructive.h"
#include "BI.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"
#include "BIAnalyzer.h"
#include "BIProducer.h"
#include "BIProgram.h"
#include "BIStrategyArticulation.h"
#include "BGApplication.h"
#include "BGScene.h"
#include "BGAutomaticScene.h"
#include "Trace.h"

namespace LEGO {

// Mac 0x25ab70
BIGeneratorConstructive::BIGeneratorConstructive(BGApplication* application, UInt32 mode)
    : BIGenerator(application), m_mode(mode), m_scene(0)
{
    m_scene = new BGAutomaticScene(application->GetBrickScene());
    m_scene->AddRef();
}

// Mac 0x25e010 / 0x25e070 (body 0x25e1d0), Win 0x592ff0 / 0x593090
BIGeneratorConstructive::~BIGeneratorConstructive()
{
    if (m_scene != 0)
        m_scene->Release();
}

// Mac 0x25dfe0, Win 0x592f90
String BIGeneratorConstructive::GetName() const
{
    return String("Constructive");
}

// Mac 0x25acd0, Win 0x593270
Int32 BIGeneratorConstructive::Create(BIBrick* brick, bool analyze)
{
    if (!BIGenerator::BuildCameraSteps())
        return -1;
    if (!m_events.IsEmpty())
        return 0;
    BasicSet<BIBrick*> bricks;
    if (!analyze) {
        BI::GetBricks(brick->GetPrimitiveSet(), bricks);
    } else {
        m_application->AnalyzeBrick(brick);
        if (m_application->IsCancelled())
            return -1;
        BIBrickRepository::GetInstance().GetFreeBricksWithin(brick->GetPrimitiveSet(), bricks);
        m_application->FilterSubsteps(bricks);
    }
    Int32 result = Generate(bricks, analyze);
    if (m_application->IsCancelled())
        return -1;
    if (result >= 0)
        m_saved = m_events;
    return result;
}

// Mac 0x25ae70, Win 0x593420
Int32 BIGeneratorConstructive::Create(const EventList& events)
{
    SetEvents(events);
    m_saved = m_events;
    return 0;
}

// Mac 0x25b3c0, Win 0x5935e0
BIEvent* BIGeneratorConstructive::Next()
{
    BIEvent* event = GetEvent(1);
    if (event == 0)
        return 0;
    event->Apply(m_scene, true);
    ++m_current;
    m_saved = m_events;
    return event;
}

// Mac 0x25b440, Win 0x593650
BIEvent* BIGeneratorConstructive::Previous()
{
    BIEvent* event = GetEvent(0);
    if (event == 0)
        return 0;
    event->Apply(m_scene, false);
    --m_current;
    m_saved = m_events;
    return event;
}

// Mac 0x25aec0, Win 0x593460
BIEvent* BIGeneratorConstructive::Next(const BasicSet<BIBrick*>& bricks)
{
    BIEvent* event = GetEvent(1);
    if (event != 0 && bricks.Contains(event->GetBrick()))
        return Next();
    return 0;
}

// Mac 0x25b4c0, Win 0x57b9e0
BIEvent* BIGeneratorConstructive::Next(Object* /*unknown*/)
{
    return 0;
}

// Mac 0x25b4d0, Win 0x57b9e0
BIEvent* BIGeneratorConstructive::Previous(Object* /*unknown*/)
{
    return 0;
}

// Mac 0x25b4e0, Win 0x5936c0
BIEvent* BIGeneratorConstructive::Next(BIEvent* event)
{
    m_events.Insert(m_events.Begin() + m_current, 1, RefPointer<BIEvent>(event));
    m_changed = true;
    return m_events[m_current++];
}

// Mac 0x25b590 (same code), Win 0x5936c0
BIEvent* BIGeneratorConstructive::Next(BIEventJumper* jumper)
{
    m_events.Insert(m_events.Begin() + m_current, 1, RefPointer<BIEvent>((BIEvent*)jumper));
    m_changed = true;
    return m_events[m_current++];
}

// Mac 0x25dfd0, Win 0x57b910
BGScene* BIGeneratorConstructive::GetScene()
{
    return m_scene;
}

// Mac 0x25bb50, Win 0x593aa0
bool BIGeneratorConstructive::ToggleBrick(BIBrick* brick, ChangeList& changes)
{
    if (brick == 0)
        return Retry(changes);
    bool placed = m_scene->Intersects(brick);
    bool ok = PlaceBrick(brick, m_events);
    if (!placed) {
        if (!ok)
            return false;
        ++m_current;
        // NOTE: the event is used without a null check (both builds); PlaceBrick put it there.
        RefPointer<BIEvent> event = GetEvent(0);
        event->Apply(m_scene, true);
        changes.PushBack(Pair<bool, RefPointer<BIEvent> >(true, event));
    } else {
        if (!ok)
            return false;
        RefPointer<BIEvent> event = GetEvent(1);
        event->Apply(m_scene, false);
        changes.PushBack(Pair<bool, RefPointer<BIEvent> >(false, event));
    }
    return true;
}

// Mac 0x25b640
bool BIGeneratorConstructive::Retry(ChangeList& changes)
{
    if (m_current == 0)
        return false;
    if (GetEvent(0)->m_type != BIEvent::TypeBrick)
        return false;
    RefPointer<BIEvent> last = GetEvent(0);
    last->Apply(m_scene, false);
    --m_current;
    RefPointer<BIEvent> taken = TakeNextEvent();
    m_allowedBricks.Insert(last->GetBrick());
    BIEvent* current = 0;
    {
        RefPointer<BIEvent> selected = SelectNext(m_saved);
        if (!selected.IsNull()) {
            m_events[m_current]->Apply(m_scene, true);
            current = m_events[m_current++];
        }
    }
    if (current == 0) {
        m_allowedBricks.Clear();
        RefPointer<BIEvent> selected = SelectNext(m_saved);
        if (!selected.IsNull()) {
            m_events[m_current]->Apply(m_scene, true);
            current = m_events[m_current++];
        }
        if (current == 0) {
            m_events.Insert(m_events.Begin() + m_current, 1, taken);
            taken->Apply(m_scene, true);
            ++m_current;
            return false;
        }
    }
    if (current->GetBrick() == last->GetBrick())
        return false;
    changes.PushBack(Pair<bool, RefPointer<BIEvent> >(false, last));
    changes.PushBack(Pair<bool, RefPointer<BIEvent> >(true, RefPointer<BIEvent>(current)));
    return true;
}

// Mac 0x25af50
RefPointer<BIEvent> BIGeneratorConstructive::SelectNext(const EventList& events)
{
    EventList before;
    GetBrickEventsBefore(before);
    BasicSet<BIBrick*> candidates;
    for (EventList::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        BIBrick* brick = (*it)->GetBrick();
        if (brick != 0 && !brick->Intersects(before) && !IsAllowed(brick))
            candidates.Insert(brick);
    }
    for (;;) {
        if (candidates.GetSize() == 0) {
            Trace(String("No bricks found!"), true);
            return RefPointer<BIEvent>();
        }
        m_application->GetProgram()->Reset();
        BasicArray<BIBrickSubstep*> substeps;
        RefPointer<BIEvent> selected =
            m_application->GetProgram()->SelectBrick(false, candidates, before, m_scene, substeps);
        m_application->GetProgram()->Unknown07();
        if (selected.IsNull())
            return RefPointer<BIEvent>();
        if (PlaceBrick(selected->GetBrick(), events)) {
            Trace(String("Brick found"), true);
            return selected;
        }
        candidates.Erase(selected->GetBrick());
        Trace(String("Trying another..."), true);
    }
}

// Mac 0x25bd60
bool BIGeneratorConstructive::PlaceBrick(BIBrick* brick, const EventList& events)
{
    EventList before;
    GetEventsBefore(before);
    EventList placed;
    bool clear = true;
    for (EventList::Iterator it = before.Begin(); it != before.End(); ++it) {
        BIBrick* other = (*it)->GetBrick();
        if (other == 0) {
            placed.PushBack(*it);
            continue;
        }
        BasicSet<BIBrickPrimitive*> inside;
        BasicSet<BIBrickPrimitive*> outside;
        if (!other->Split(brick, inside, outside)) {
            placed.PushBack(*it);
            continue;
        }
        for (BasicSet<BIBrickPrimitive*>::Iterator p = outside.Begin(); p != outside.End(); ++p)
            placed.PushBack(RefPointer<BIEvent>(new BIEventBrick(*p)));
        clear = false;
    }
    if (!clear) {
        EventList after;
        GetEventsAfter(after);
        BasicSet<BIBrickPrimitive*> none;
        if (!Order(placed, none))
            return false;
        SetEvents(placed);
        m_events.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
        m_events.InsertRange(m_events.End(), after.Begin(), after.End());
        m_current = placed.GetSize();
        return true;
    }
    placed.PushBack(RefPointer<BIEvent>(new BIEventBrick(brick)));
    EventList rest;
    for (EventList::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        BIBrick* other = (*it)->GetBrick();
        if (other == 0) {
            rest.PushBack(*it);
            continue;
        }
        BasicSet<BIBrickPrimitive*> inside;
        BasicSet<BIBrickPrimitive*> outside;
        other->Split(placed, inside, outside);
        if (inside.GetSize() == 0) {
            rest.PushBack(*it);
        } else {
            for (BasicSet<BIBrickPrimitive*>::Iterator p = outside.Begin(); p != outside.End(); ++p)
                rest.PushBack(RefPointer<BIEvent>(new BIEventBrick(*p)));
        }
    }
    RefPointer<BIGeneratorDestructive> generator = new BIGeneratorDestructive(m_application);
    if (!generator->GenerateReverse(rest, placed))
        return false;
    EventList ordered;
    generator->GetEvents(ordered);
    SetEvents(ordered);
    return true;
}

// Mac 0x25c990, Win 0x593ca0
BIGeneratorConstructive::EventList BIGeneratorConstructive::GetCandidates(BIBrick* brick)
{
    EventList result;
    if (brick == 0) {
        // every placed primitive, and the free bricks the articulation rule lets add
        const BasicSet<BIBrickPrimitive*>& scenePrimitives = m_scene->GetPrimitives();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = scenePrimitives.Begin(); it != scenePrimitives.End(); ++it)
            result.PushBack(RefPointer<BIEvent>(new BIEventBrick(*it)));
        EventList before;
        GetBrickEventsBefore(before);
        BasicSet<BIBrick*> remaining;
        EventList none;
        CollectRemainingBricks(remaining, true, false, none);
        for (BasicSet<BIBrick*>::Iterator it = remaining.Begin(); it != remaining.End(); ++it) {
            BIBrick* candidate = *it;
            if (m_application->IsGroupEnabled(candidate->GetGroup()))
                continue;
            if (candidate->IsEqual(m_application->GetSubsteps().Back()))
                continue;
            // NOTE: a one-element array is built and not used (both builds).
            BasicArray<BIBrick*> single;
            single.PushBack(candidate);
            RefPointer<BIStrategyArticulation> strategy = new BIStrategyArticulation;
            UInt32 context = 0;   // NOTE: uninitialised in both builds; not read by the strategy
            Real32 score = strategy->ScoreAdd(candidate, context, before, m_scene);
            if (score > 0.5f)
                result.PushBack(RefPointer<BIEvent>(new BIEventBrick(*it)));
        }
        return result;
    }
    if (!m_scene->Intersects(brick)) {
        // not placed: the free bricks that intersect it, then the brick itself
        BasicSet<BIBrick*> remaining;
        EventList none;
        CollectRemainingBricks(remaining, true, false, none);
        for (BasicSet<BIBrick*>::Iterator it = remaining.Begin(); it != remaining.End(); ++it) {
            if (!(*it)->Intersects(brick))
                continue;
            EventList before;
            GetBrickEventsBefore(before);
            BasicArray<BIBrick*> single;
            single.PushBack(*it);
            m_application->GetProgram()->FilterBricks(false, single, before, m_scene, result);
        }
        EventList before;
        GetBrickEventsBefore(before);
        BasicArray<BIBrick*> single;
        single.PushBack(brick);
        m_application->GetProgram()->FilterBricks(false, single, before, m_scene, result);
        return result;
    }
    // placed: the first earlier brick that intersects it, then the brick itself
    EventList before;
    GetEventsBefore(before);
    for (EventList::Iterator it = before.Begin(); it != before.End(); ++it) {
        BIBrick* other = (*it)->GetBrick();
        if (other != 0 && other->Intersects(brick)) {
            EventList none;
            BasicArray<BIBrick*> single;
            single.PushBack(other);
            m_application->GetProgram()->FilterBricks(true, single, none, m_scene, result);
            break;
        }
    }
    EventList none;
    BasicArray<BIBrick*> single;
    single.PushBack(brick);
    m_application->GetProgram()->FilterBricks(true, single, none, m_scene, result);
    return result;
}

// Mac 0x25db00, Win 0x594ac0
bool BIGeneratorConstructive::BuildCameraSteps()
{
    m_events.Clear();
    m_scene->Clear();
    m_current = 0;
    return BIGenerator::Generate() >= 0;
}

// Mac 0x25c980, Win 0x56b1c0
void BIGeneratorConstructive::Unknown29()
{
}

// Mac 0x25db70, Win 0x595460
Int32 BIGeneratorConstructive::Generate(const BasicSet<BIBrick*>& bricks, bool analyze)
{
    if (IsComplete())
        return 0;
    RefPointer<BIProgram> program = m_application->GetProgram();
    m_application->ClearEnabledGroups();
    if (m_application->GetProducer() != 0) {
        const BasicArray<RefPointer<BIAnalyzer> >& analyzers = m_application->GetProducer()->GetAnalyzers();
        for (UInt32 i = 0; i < m_application->GetProducer()->GetAnalyzers().GetSize(); ++i)
            if ((double)analyzers[i]->GetWeight() < 0.8)
                m_application->EnableGroup(analyzers[i]);
    }
    RefPointer<BIGeneratorDestructive> generator = new BIGeneratorDestructive(m_application);
    m_application->SaveProgress();
    Int32 relaxations = 0;
    if (!generator->GenerateBricks(bricks, m_events, program, analyze)) {
        if (m_application->IsCancelled())
            return 0;
        m_application->ClearEnabledGroups();
        m_application->RestoreProgress();
        m_application->SaveProgress();
        relaxations = 1;
        while (!generator->GenerateBricks(bricks, m_events, program, analyze)) {
            m_application->RestoreProgress();
            m_application->SaveProgress();
            if (m_application->IsCancelled())
                return 0;
            program = program->CreateRelaxed();
            ++relaxations;
        }
    }
    EventList events;
    generator->GetEvents(events);
    SetEvents(events);
    return relaxations;
}

} // namespace LEGO
