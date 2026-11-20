// LEGO Digital Designer — restored source
// TU 199 "BIGeneratorDestructive.cpp" (Mac 0x24bae0-0x24eadf). See BIGeneratorDestructive.h.
#include "BIGeneratorDestructive.h"
#include "BI.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"
#include "BIAnalyzer.h"
#include "BIGraph.h"
#include "BIProgram.h"
#include "BICameraChangeFinder.h"
#include "BGApplication.h"
#include "BGScene.h"
#include "BGAutomaticScene.h"
#include "PrimitiveElement.h"
#include "MathUtil.h"
#include "Trace.h"

namespace LEGO {

// Mac 0x24bb10, Win 0x579960
BIGeneratorDestructive::BIGeneratorDestructive(BGApplication* application)
    : BIGenerator(application), m_forceOnVeto(false), m_scene(0)
{
    m_scene = new BGAutomaticScene(application->GetBrickScene());
    m_scene->AddRef();
}

// Mac 0x24e7c0 / 0x24e820 (body 0x24e9b0), Win 0x579850 / 0x5798f0
BIGeneratorDestructive::~BIGeneratorDestructive()
{
    if (m_scene != 0)
        m_scene->Release();
}

// Mac 0x24e790, Win 0x5797f0
String BIGeneratorDestructive::GetName() const
{
    return String("Destructive");
}

// Mac 0x24bc40, Win 0x579a80
Int32 BIGeneratorDestructive::Create(BIBrick* brick, bool /*analyze*/)
{
    m_application->AnalyzeBrick(brick);
    if (m_application->IsCancelled())
        return 0;
    BasicSet<BIBrick*> bricks;
    BI::GetBricks(brick->GetPrimitiveSet(), bricks);
    EventList none;
    return GenerateBricks(bricks, none, 0, true) ? 0 : -1;
}

// Mac 0x24c8b0, Win 0x579b70
Int32 BIGeneratorDestructive::Create(const EventList& /*events*/)
{
    BasicSet<BIBrick*> bricks;
    EventList none;
    CollectRemainingBricks(bricks, false, false, none);
    EventList noEvents;
    return GenerateBricks(bricks, noEvents, 0, true) ? 0 : -1;
}

// Mac 0x24e640, Win 0x57b930
BIEvent* BIGeneratorDestructive::Next()
{
    UInt32 current = m_current;
    if (current < m_events.GetSize()) {
        m_current = current + 1;
        return m_events[current];
    }
    return 0;
}

// Mac 0x24e670, Win 0x57b990
BIEvent* BIGeneratorDestructive::Previous()
{
    if (m_current != 0) {
        --m_current;
        return m_events[m_current];
    }
    return 0;
}

// Mac 0x24e690, Win 0x57b9e0
BIEvent* BIGeneratorDestructive::Next(const BasicSet<BIBrick*>& /*bricks*/)
{
    return 0;
}

// Mac 0x24e6a0, Win 0x57b9e0
BIEvent* BIGeneratorDestructive::Next(Object* /*unknown*/)
{
    return 0;
}

// Mac 0x24e6b0, Win 0x57b9e0
BIEvent* BIGeneratorDestructive::Previous(Object* /*unknown*/)
{
    return 0;
}

// Mac 0x24e6c0, Win 0x57b9e0
BIEvent* BIGeneratorDestructive::Next(BIEvent* /*event*/)
{
    return 0;
}

// Mac 0x24e6d0, Win 0x57b9e0
BIEvent* BIGeneratorDestructive::Next(BIEventJumper* /*jumper*/)
{
    return 0;
}

// Mac 0x24e6e0, Win 0x57b9f0
bool BIGeneratorDestructive::Rewind()
{
    m_current = 0;
    return false;
}

// Mac 0x24e700, Win 0x57ba10
bool BIGeneratorDestructive::FastForward()
{
    m_current = m_events.GetSize();
    return false;
}

// Mac 0x24e890, Win 0x59c480
bool BIGeneratorDestructive::IsComplete()
{
    return true;
}

// Mac 0x24e630, Win 0x57b910
BGScene* BIGeneratorDestructive::GetScene()
{
    return m_scene;
}

// Mac 0x24e730, Win 0x57ba40
RefPointer<BIEvent> BIGeneratorDestructive::TakeNextEvent()
{
    return RefPointer<BIEvent>();
}

// Mac 0x24e740, Win 0x56b1c0
void BIGeneratorDestructive::Truncate()
{
}

// Mac 0x24e750, Win 0x57baa0
bool BIGeneratorDestructive::ToggleBrick(BIBrick* /*brick*/, BasicArray<Pair<bool, RefPointer<BIEvent> > >& /*changes*/)
{
    return false;
}

// Mac 0x24e760, Win 0x57bab0
BIGeneratorDestructive::EventList BIGeneratorDestructive::GetCandidates(BIBrick* /*brick*/)
{
    return EventList();
}

// Mac 0x24e720, Win 0x57ba30
bool BIGeneratorDestructive::BuildCameraSteps()
{
    return false;
}

// Mac 0x24e8a0, Win 0x56b1c0
void BIGeneratorDestructive::Unknown29()
{
}

// Mac 0x24e8b0, Win 0x59c690
Int32 BIGeneratorDestructive::Generate(const BasicSet<BIBrick*>& /*bricks*/, bool /*analyze*/)
{
    return 0;
}

// Removes `brick` (its primitives, for a substep) from the scene. Mac 0x24d620 (name inferred)
static void RemoveFromScene(BIBrick* brick, BGScene* scene)
{
    if (brick->GetType() == BIBrick::TypeSubstep) {
        BasicSet<BIBrickPrimitive*> primitives;
        brick->GetPrimitives(primitives);
        for (BasicSet<BIBrickPrimitive*>::Iterator it = primitives.Begin(); it != primitives.End(); ++it)
            scene->RemoveBrick(*it, true);
    } else if (brick->GetType() == BIBrick::TypePrimitive) {
        scene->RemoveBrick(brick, true);
    }
}

// False for a substep that holds exactly the primitives of the remaining primitive bricks
// (it is dropped from `remaining`: it would be the whole rest). Mac 0x24ceb0 (name inferred)
static bool CheckBrick(BIBrick* brick, BasicSet<BIBrick*>& remaining)
{
    if (brick->GetType() != BIBrick::TypeSubstep)
        return true;
    BasicSet<BIBrickPrimitive*> primitives;
    for (BasicSet<BIBrick*>::Iterator it = remaining.Begin(); it != remaining.End(); ++it)
        if ((*it)->GetType() == BIBrick::TypePrimitive)
            (*it)->GetPrimitives(primitives);
    if (brick->IsEqual(primitives)) {
        remaining.Erase(brick);
        return false;
    }
    return true;
}

// Mac 0x24bdd0, Win 0x57a120
bool BIGeneratorDestructive::GenerateBricks(const BasicSet<BIBrick*>& bricks, const EventList& placed,
                                            BIProgram* program, bool useGraphs)
{
    if (program == 0)
        program = m_application->GetProgram();
    m_scene->Clear();
    BasicSet<BIBrickPrimitive*> primitives;
    BI::GetPrimitives(bricks, primitives);
    BI::GetPrimitives(placed, primitives);
    BasicSet<BIBrick*> sceneBricks;
    BI::GetBricks(primitives, sceneBricks);
    m_scene->AddBrick(sceneBricks, true);
    program->Reset();

    BasicSet<BIBrick*> remaining(bricks);
    BasicSet<BIBrickPrimitive*> remainingPrimitives;
    BI::GetPrimitives(remaining, remainingPrimitives);
    BICameraChangeFinder* finder = m_application->GetCameraChangeFinder();
    finder->Begin(remainingPrimitives);

    EventList taken;
    while (remaining.GetSize() != 0) {
        if (m_application->IsCancelled())
            return false;
        RefPointer<BIEvent> event;
        if (useGraphs && m_scene->GetGraphs().GetSize() >= 2) {
            // the smallest group of the scene that only holds remaining bricks
            BasicSet<BIBrickPrimitive*> rest;
            BI::GetPrimitives(remaining, rest);
            BIGraph* best = 0;
            UInt32 bestSize = 0;
            BasicArray<RefPointer<BIGraph> >& graphs = m_scene->GetGraphs();
            for (BasicArray<RefPointer<BIGraph> >::Iterator it = graphs.Begin(); it != graphs.End(); ++it) {
                if (best != 0 && (*it)->GetPrimitiveCount() >= bestSize)
                    continue;
                if ((*it)->IsContainedIn(rest)) {
                    best = *it;
                    bestSize = best->GetPrimitiveCount();
                }
            }
            if (best != 0) {
                if (best->GetPrimitiveCount() == 1) {
                    event = new BIEventBrick(*best->GetPrimitives().Begin());
                } else {
                    RefPointer<BIBrickSubstep> substep = new BIBrickSubstep(best->GetPrimitives(), 0, 2.0f);
                    event = new BIEventBrick(BIBrickRepository::GetInstance().AddSubstep(substep));
                }
            }
        }
        if (event.IsNull()) {
            BasicArray<BIBrickSubstep*> substeps;
            event = program->SelectBrick(true, remaining, taken, m_scene, substeps);
            if (event.IsNull()) {
                if (!m_forceOnVeto)
                    break;
                event = new BIEventBrick(*remaining.Begin());
                Trace(String("VETO"), true);
            }
        }
        if (CheckBrick(event->GetBrick(), remaining)) {
            program->Unknown06(event->GetBrick());
            taken.PushBack(event);
            finder->AddBrick(event->GetBrick());
            RemoveIntersecting(event->GetBrick(), remaining, m_scene);
            RemoveFromScene(event->GetBrick(), m_scene);
            if (event->GetBrick()->GetType() == BIBrick::TypePrimitive)
                m_application->AdvanceProgress();
        }
        if (m_application->IsCancelled())
            return false;
    }
    m_events.Clear();
    if (remaining.GetSize() == 0) {
        finder->GetEvents(m_events);
        InsertSteps();
        m_current = 0;
    }
    program->Unknown07();
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<BIBrickSubstep*>::Iterator it = m_substeps.Begin(); it != m_substeps.End(); ++it)
        repository.SetUsed(*it);
    return !m_events.IsEmpty();
}

// Mac 0x24d720, Win 0x57abb0
bool BIGeneratorDestructive::GenerateReverse(const EventList& events, const EventList& placed)
{
    m_scene->Clear();
    BasicSet<BIBrickPrimitive*> primitives;
    BI::GetPrimitives(placed, primitives);
    BI::GetPrimitives(events, primitives);
    BasicSet<BIBrick*> sceneBricks;
    BI::GetBricks(primitives, sceneBricks);
    m_scene->AddBrick(sceneBricks, true);
    m_application->GetProgram()->Reset();

    BasicSet<BIBrick*> remaining;
    BI::GetBricks(events, remaining);
    EventList taken;
    // NOTE: computed and not used (both builds).
    BasicSet<BIBrickPrimitive*> eventPrimitives;
    BI::GetPrimitives(events, eventPrimitives);

    while (remaining.GetSize() != 0) {
        if (m_application->IsCancelled())
            return false;
        RefPointer<BIEvent> event;
        for (EventList::ConstIterator it = events.End(); it != events.Begin();) {
            --it;
            BIBrick* brick = (*it)->GetBrick();
            if (brick != 0 && (brick->Intersects(taken) || brick->Intersects(placed)))
                continue;
            if (brick != 0) {
                BasicSet<BIBrick*> single;
                single.Insert(brick);
                BasicArray<BIBrickSubstep*> substeps;
                event = m_application->GetProgram()->SelectBrick(true, single, taken, m_scene, substeps);
                if (!event.IsNull())
                    break;
            }
        }
        if (event.IsNull()) {
            if (!m_forceOnVeto)
                break;
            event = new BIEventBrick(*remaining.Begin());
            Trace(String("VETO"), true);
        }
        m_application->GetProgram()->Unknown06(event->GetBrick());
        taken.PushBack(event);
        RemoveIntersecting(event->GetBrick(), remaining, m_scene);
        RemoveFromScene(event->GetBrick(), m_scene);
        if (event->GetBrick()->GetType() == BIBrick::TypePrimitive) {
            if (m_application->IsCancelled())
                return false;
            m_application->AdvanceProgress();
        }
    }
    m_events.Clear();
    if (remaining.GetSize() == 0) {
        for (EventList::ConstIterator it = placed.Begin(); it != placed.End(); ++it)
            m_events.PushBack(*it);
        for (EventList::Iterator it = taken.End(); it != taken.Begin();) {
            --it;
            m_events.PushBack(*it);
        }
        m_current = 0;
    }
    m_application->GetProgram()->Unknown07();
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<BIBrickSubstep*>::Iterator it = m_substeps.Begin(); it != m_substeps.End(); ++it)
        repository.SetUsed(*it);
    return remaining.GetSize() == 0;
}

// Mac 0x24cfe0, Win 0x57bc30
void BIGeneratorDestructive::RemoveIntersecting(BIBrick* brick, BasicSet<BIBrick*>& remaining, BGScene* scene)
{
    BasicSet<BIBrick*> removed;
    BasicSet<BIBrickPrimitive*> primitives;
    BasicSet<BIBrick*> added;
    brick->GetPrimitives(primitives);
    for (BasicSet<BIBrick*>::Iterator it = remaining.Begin(); it != remaining.End(); ++it) {
        if (!brick->Intersects(*it))
            continue;
        removed.Insert(*it);
        BIBrick* other = *it;
        if (other->GetType() != BIBrick::TypeSubstep || other->GetGroup() == 0)
            continue;
        if (!(other->GetGroup()->GetName() == String("Direction")))
            continue;
        BIGraph* graph = scene->FindGraph(*other->GetPrimitiveSet().Begin());
        RefPointer<BIBrickSubstep> rest = static_cast<BIBrickSubstep*>(other)->Subtract(primitives, graph);
        BIBrick* registered = BIBrickRepository::GetInstance().AddSubstep(rest);
        if (registered == 0)
            continue;
        bool known = false;
        for (BasicSet<BIBrick*>::Iterator r = remaining.Begin(); r != remaining.End(); ++r)
            if ((*r)->GetType() == BIBrick::TypeSubstep && registered->IsEqual(*r)) {
                known = true;
                break;
            }
        if (known)
            continue;
        added.Insert(registered);
        m_substeps.PushBack(static_cast<BIBrickSubstep*>(registered));
    }
    for (BasicSet<BIBrick*>::Iterator it = removed.Begin(); it != removed.End(); ++it)
        remaining.Erase(*it);
    for (BasicSet<BIBrick*>::Iterator it = added.Begin(); it != added.End(); ++it)
        remaining.Insert(*it);
}

// A single-element primitive whose bone is larger than `size` in at least `dimensions` axes.
// Inlined on Mac (0x24cae0), Win 0x579c40 (name inferred)
static bool IsLargeBrick(const BIEvent* event, Real32 size, UInt32 dimensions)
{
    BIBrick* brick = event->GetBrick();
    if (brick == 0 || brick->GetType() != BIBrick::TypePrimitive || brick->GetElements().GetSize() != 1)
        return false;
    Vector3 extent = brick->GetElements()[0]->GetBone().m_bounding.GetSize();
    UInt32 count = 0;
    if (IsGreater(extent.x, size, EPSILON))
        ++count;
    if (IsGreater(extent.y, size, EPSILON))
        ++count;
    if (IsGreater(extent.z, size, EPSILON))
        ++count;
    return count >= dimensions;
}

// Mac 0x24cae0, Win 0x579d80
void BIGeneratorDestructive::InsertSteps()
{
    EventList result;
    for (UInt32 i = 0; i < m_events.GetSize(); ++i) {
        BIEvent* event = m_events[i];
        bool substep = event->GetBrick() != 0 && event->GetBrick()->GetType() == BIBrick::TypeSubstep;
        bool large = substep || IsLargeBrick(event, 8.0f, 2);
        // NOTE: both builds read m_events[i - 1] without checking i, so for the first event
        // they read the word before the array; fixed: i == 0 counts as "no step before" (a large
        // first event gets a step in front).
        if (large && (i == 0 || m_events[i - 1]->m_type != BIEvent::TypeStep))
            result.PushBack(RefPointer<BIEvent>(new BIEventStep));
        result.PushBack(m_events[i]);
        if (large && i < m_events.GetSize() - 1)
            result.PushBack(RefPointer<BIEvent>(new BIEventStep));
    }
    if (&result != &m_events)
        m_events = result;
}

// Mac 0x24e000, Win 0x57b500
bool BIGeneratorDestructive::Order(EventList& events, BasicSet<BIBrickPrimitive*>& primitives)
{
    m_scene->Clear();
    BasicSet<BIBrickPrimitive*> all(primitives);
    BI::GetPrimitives(events, all);
    BasicSet<BIBrick*> sceneBricks;
    BI::GetBricks(all, sceneBricks);
    m_scene->AddBrick(sceneBricks, true);
    m_application->GetProgram()->Reset();
    EventList pending(events);
    EventList taken;
    while (!pending.IsEmpty()) {
        BIEvent* event = pending.Back();
        pending.PopBack();   // still referenced by `events`
        BIBrick* brick = event->GetBrick();
        if (brick == 0)
            continue;
        BasicSet<BIBrick*> single;
        single.Insert(brick);
        BasicArray<BIBrickSubstep*> substeps;
        RefPointer<BIEvent> selected =
            m_application->GetProgram()->SelectBrick(true, single, taken, m_scene, substeps);
        if (selected.IsNull())
            break;
        selected = 0;
        m_application->GetProgram()->Unknown06(brick);
        taken.PushBack(RefPointer<BIEvent>(event));
        RemoveFromScene(brick, m_scene);
    }
    m_application->GetProgram()->Unknown07();
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<BIBrickSubstep*>::Iterator it = m_substeps.Begin(); it != m_substeps.End(); ++it)
        repository.SetUsed(*it);
    return pending.IsEmpty();
}

} // namespace LEGO
