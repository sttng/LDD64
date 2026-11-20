// LEGO Digital Designer — restored source
// TU 364 "BGAutomaticApplication.cpp" (Mac 0x345270-0x34c7cf): LEGO::BGAutomaticApplication
// (+ the BasicArray / BasicSet / NotifierT template instances of the TU).
#include "BGAutomaticApplication.h"
#include "BI.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"
#include "BICameraChangeFinder.h"
#include "Texture.h"
#include "Material.h"
#include "BIEvent.h"
#include "BIGenerator.h"
#include "BIGeneratorConstructive.h"
#include "BIGraph.h"
#include "BIProducer.h"
#include "BIProgram.h"
#include "BIStorage.h"
#include "BISubstepFilterProgram.h"
#include "BGScene.h"
#include "PrimitiveElement.h"

namespace LEGO {

namespace {

// The mode of the constructive generators: BIGeneratorConstructive(application, 1).
const UInt32 kGeneratorMode = 1;

// The event at `index` of the generator's event list, null outside (inlined BIGenerator
// accessor; the original copies it into a RefPointer temporary).
BIEvent* EventAt(BIGenerator* generator, UInt32 index)
{
    if (index < generator->m_events.GetSize())
        return generator->m_events[index];
    return 0;
}

// The camera of the nearest step event with a camera before the generator's current position
// (null if none).
StorageScene2::Camera* FindPreviousCamera(BIGenerator* generator)
{
    Int32 index = (Int32)generator->GetCurrent();
    for (;;) {
        --index;
        if (index < 0)
            return 0;
        // NOTE: the original dereferences the event without a null check (the index is always
        // inside the list).
        BIEvent* event = EventAt(generator, (UInt32)index);
        if (event->GetType() == BIEvent::TypeStep && static_cast<BIEventStep*>(event)->m_camera.Get() != 0)
            return static_cast<BIEventStep*>(event)->m_camera.Get();
    }
}

// A substep brick event (the brick of the event is a BIBrickSubstep).
bool IsSubstepEvent(const BIEvent* event)
{
    return event != 0 && event->GetBrick() != 0 && event->GetBrick()->GetType() == BIBrick::TypeSubstep;
}

// A non-empty event list that is not a lone marker (worth storing for a substep).
bool HasRealEvents(const BGApplication::EventList& events)
{
    if (events.IsEmpty())
        return false;
    return !(events.GetSize() == 1 && events[0]->GetType() == BIEvent::TypeMarker);
}

// The same events (the same objects in the same order).
bool SameEvents(const BGApplication::EventList& a, const BGApplication::EventList& b)
{
    if (a.GetSize() != b.GetSize())
        return false;
    for (UInt32 i = 0; i < a.GetSize(); ++i) {
        if (a[i].Get() != b[i].Get())
            return false;
    }
    return true;
}

// Adds the primitives of the brick events' bricks.
void AddEventPrimitives(const BGApplication::EventList& events, BasicSet<BIBrickPrimitive*>& primitives)
{
    for (BGApplication::EventList::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        BIBrick* brick = (*it)->GetBrick();
        if (brick == 0)
            continue;
        const BasicSet<BIBrickPrimitive*>& own = brick->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator p = own.Begin(); p != own.End(); ++p)
            primitives.Insert(*p);
    }
}

// The candidate brick of `events` with the fewest primitives that `brick` intersects (the
// first of equal size).
BIBrick* SmallestIntersecting(BIBrick* brick, const BGApplication::EventList& events)
{
    BIBrick* best = 0;
    for (BGApplication::EventList::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        BIBrick* candidate = (*it)->GetBrick();
        if (best != 0 && !(candidate->GetPrimitiveCount() < best->GetPrimitiveCount()))
            continue;
        if (brick->Intersects(candidate))
            best = candidate;
    }
    return best;
}

} // namespace

// Mac 0x3452e0 (C1), Win 0x566470
BGAutomaticApplication::BGAutomaticApplication(Render* render)
    : m_scene(0),
      m_render(render),
      m_viewWidth(0.0f),
      m_viewHeight(0.0f),
      m_currentSubstep(0),
      m_sceneChanged(false),
      m_useGeneratorCandidates(false),
      m_stepCount(0),
      m_currentStep(0),
      m_modelCount(0),
      m_markedLevel(0),
      m_analyzeDepth(0),
      m_progressTotal(0),
      m_progress(0),
      m_savedProgress(0),
      m_cancelled(false),
      m_progressStep(1)
{
}

// Mac 0x3459d0 / 0x345b80 (base-object 0x345540), Win 0x5667a0 / sdd 0x56e0a0
BGAutomaticApplication::~BGAutomaticApplication()
{
}

// Mac 0x345d70 (thunks 0x345e80 / 0x345ea0 / 0x345ec0)
void BGAutomaticApplication::OnFinalRelease()
{
    Notifier<BGAutomaticApplicationObserver>::OnFinalRelease();
    SceneObserver::OnFinalRelease();
    Object::OnFinalRelease();
}

// ---- generation ------------------------------------------------------------------------------

// Mac 0x345ee0, Win 0x566990
bool BGAutomaticApplication::Start(Scene* scene, BIStorage* storage)
{
    m_cancelled = false;
    m_scene = scene;
    m_modelCount = 0;
    if (!scene->m_hingedElements.IsEmpty())
        m_modelCount = scene->m_hingedElements.GetSize();
    m_analyzeDepth = (m_modelCount != 1) + 1;
    m_producer = new BIProducer(0, String(""), this);
    m_filterProgram = new BISubstepFilterProgram();
    BIBrickRepository::GetInstance().Build(scene);
    BI::UpdateElementGraph(scene);
    SceneObserver::Attach(scene);
    storage->GetEvents(m_substepEvents);
    return true;
}

// Mac 0x3462e0; Win twin not identified (Win 0x566990 is Start: BuildingGuideGenerator::Start 0x68b7b9 uses its result)
bool BGAutomaticApplication::Stop()
{
    SceneObserver::Detach();
    m_generator = 0;
    m_producer = 0;
    m_program = 0;
    m_substeps.Clear();
    m_generators.Clear();
    m_candidates.Clear();
    m_substepEvents.Clear();
    return true;
}

// Mac 0x346790
bool BGAutomaticApplication::Finish(BIStorage* storage)
{
    if (!HasGenerators()) {
        storage->Clear();
    } else {
        while (ExitSubstep(0) != 0) {
        }
        ShowFreeBricks();
        BIGenerator* root = GetRootGenerator();
        EventList& top = m_substepEvents[0];
        if (&root->m_events != &top)
            top = root->m_events;
        storage->Clear();
        storage->SetEvents(m_substepEvents[0], m_substepEvents);
    }
    Stop();
    return true;
}

// Mac 0x346900
Int32 BGAutomaticApplication::Generate()
{
    UInt32 bricks = m_scene->m_bricks.GetSize();
    UInt32 analyzers = m_producer->GetAnalyzerCount();
    UInt32 share = (UInt32)((double)bricks * 0.05);
    // NOTE: the original divides by zero for a scene without hinged elements; fixed: a step of 1.
    UInt32 passes = analyzers * m_modelCount;
    m_progressStep = (passes != 0 ? share / passes : 0) + 1;
    m_progressTotal = m_progressStep * analyzers * m_modelCount + bricks;
    m_progress = 0;
    EventList& top = m_substepEvents[0];
    m_generators.Clear();
    m_substeps.Clear();
    m_elementTransforms.Clear();
    // NOTE: the original stores an identity matrix for every element (not its transform).
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = m_scene->m_primitiveElements.Begin();
         it != m_scene->m_primitiveElements.End(); ++it) {
        Matrix3D identity;
        m_elementTransforms.PushBack(Pair<PrimitiveElement*, Matrix3D>(*it, identity));
    }
    BasicSet<BIBrickPrimitive*> primitives;
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    repository.GetFreePrimitives(primitives);
    Int32 result;
    if (primitives.GetSize() == 1) {
        BIBrickPrimitive* primitive = *primitives.Begin();
        m_generator = new BIGeneratorConstructive(this, kGeneratorMode);
        EventList events;
        events.PushBack(RefPointer<BIEvent>(new BIEventStep()));
        events.PushBack(RefPointer<BIEvent>(new BIEventBrick(primitive)));
        EventList& stored = m_substepEvents[0];
        if (&events != &stored)
            stored = events;
        m_generator->SetEvents(events);
        GenerateBrick(primitive);
        m_generators.PushBack(m_generator);
        result = 0;
    } else if (primitives.GetSize() == 0) {
        m_generator = new BIGeneratorConstructive(this, kGeneratorMode);
        result = 0;
    } else if (m_substepEvents[0].IsEmpty()) {
        BIBrick* model = repository.GetSubstep(primitives);
        Int32 generated = GenerateSubstep(model, 0);
        result = -1;
        if (!IsCancelled()) {
            EventList events;
            m_generator->GetEvents(events);
            EventList& stored = m_substepEvents[0];
            if (&events != &stored)
                stored = events;
            GenerateSubsteps(0, 1);
            result = generated;
        }
    } else {
        result = Regenerate(top, false);
    }
    return result;
}

// Mac 0x347040
Int32 BGAutomaticApplication::GenerateBrick(BIBrick* brick)
{
    if (brick->GetType() != BIBrick::TypePrimitive) {
        SubstepEventMap::Iterator it = m_substepEvents.Find(static_cast<BIBrickSubstep*>(brick));
        if (it != m_substepEvents.End() && HasRealEvents(it->second))
            return Regenerate(it->second, false);
        return GenerateSubstep(brick, 0);
    }
    m_substeps.PushBack(brick);
    return 0;
}

// Mac 0x347530
Int32 BGAutomaticApplication::GenerateSubstep(BIBrick* brick, UInt32 depth)
{
    RefPointer<BIGenerator> generator = new BIGeneratorConstructive(this, kGeneratorMode);
    m_substeps.PushBack(brick);
    m_generators.PushBack(generator);
    Int32 result = generator->Create(brick, depth < m_analyzeDepth);
    if (IsCancelled())
        return -1;
    if (result == -1) {
        m_substeps.PopBack();
        m_generators.PopBack();
    } else {
        m_generator = generator;
        if (HasRealEvents(generator->m_events)) {
            EventList& stored = m_substepEvents[static_cast<BIBrickSubstep*>(brick)];
            if (&generator->m_events != &stored)
                stored = generator->m_events;
        }
    }
    return result;
}

// Mac 0x347930
void BGAutomaticApplication::GenerateSubsteps(BIBrickSubstep* substep, UInt32 depth)
{
    EventList& events = m_substepEvents[substep];
    for (UInt32 i = 0; i < events.GetSize(); ++i) {
        BIEvent* event = events[i];
        if (!IsSubstepEvent(event))
            continue;
        BIBrickSubstep* brick = static_cast<BIBrickSubstep*>(event->GetBrick());
        if (m_substepEvents.Find(brick) == m_substepEvents.End()) {
            Int32 result = GenerateSubstep(brick, depth);
            if (IsCancelled())
                return;
            if (result >= 0) {
                EventList generated;
                m_generator->GetEvents(generated);
                EventList& stored = m_substepEvents[brick];
                if (&generated != &stored)
                    stored = generated;
                m_substeps.PopBack();
                m_generators.PopBack();
                m_generator = m_generators.Back();
            }
        }
        GenerateSubsteps(brick, depth + 1);
    }
}

// Mac 0x347c70
Int32 BGAutomaticApplication::Regenerate(const EventList& events, bool fromRepository)
{
    RefPointer<BIGenerator> generator = new BIGeneratorConstructive(this, kGeneratorMode);
    BasicSet<BIBrickPrimitive*> primitives;
    if (!fromRepository)
        BI::GetPrimitives(events, primitives);
    else
        BIBrickRepository::GetInstance().GetFreePrimitives(primitives);
    BIBrick* brick = BIBrickRepository::GetInstance().GetSubstep(primitives);
    NotifyProgress(0.33f);
    m_substeps.PushBack(brick);
    m_generators.PushBack(generator);
    Int32 result = generator->Create(events);
    NotifyProgress(0.66f);
    if (IsCancelled())
        return -1;
    if (result == -1) {
        m_substeps.PopBack();
        m_generators.PopBack();
    } else {
        m_generator = generator;
        if (HasRealEvents(generator->m_events)) {
            EventList& stored = m_substepEvents[static_cast<BIBrickSubstep*>(brick)];
            if (&generator->m_events != &stored)
                stored = generator->m_events;
        }
    }
    NotifyProgress(1.0f);
    return result;
}

// Mac 0x34a1d0, Win 0x5699c0
void BGAutomaticApplication::Flatten(EventList& events)
{
    EventList result;
    BIEvent* previous = 0;
    for (EventList::Iterator it = events.Begin(); it != events.End(); ++it) {
        BIEvent* event = *it;
        if (previous == 0 || previous->GetType() != BIEvent::TypeStep || event->GetType() != BIEvent::TypeStep) {
            result.PushBack(*it);
        } else {
            BIEventStep* step = static_cast<BIEventStep*>(previous);
            StorageScene2::Camera* camera = static_cast<BIEventStep*>(event)->m_camera.Get();
            if (step->m_camera.Get() == 0 && camera != 0)
                step->m_camera = camera;
        }
        if (IsSubstepEvent(event))
            Flatten(m_substepEvents[static_cast<BIBrickSubstep*>(event->GetBrick())]);
        // NOTE: the merged step becomes `previous` too (the original keeps the last event).
        previous = event;
    }
    if (&result != &events)
        events = result;
}

// Mac 0x34a390, Win 0x569c60
void BGAutomaticApplication::ApplyEvents()
{
    if (m_generator.Get() == 0)
        return;
    if (m_substepEvents.Find(0) == m_substepEvents.End())
        return;
    Flatten(m_substepEvents[0]);
    m_generator->SetEvents(m_substepEvents[0]);
}

// Mac 0x34bd80
UInt32 BGAutomaticApplication::CountSteps(const EventList& events, UInt32 position, bool& found)
{
    if (events.IsEmpty())
        return 0;
    UInt32 count = 0;
    bool atPosition = position == 0;
    bool afterStep = false;
    bool primitiveSeen = false;
    UInt32 index = 0;
    for (EventList::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        if (atPosition && SameEvents(m_generator->m_events, events)) {
            found = true;
            return count;
        }
        BIEvent* event = *it;
        if (!primitiveSeen && event->GetBrick() != 0) {
            bool primitive = event->GetBrick()->GetType() == BIBrick::TypePrimitive;
            if (primitive) {
                primitiveSeen = true;
                ++count;
            }
        }
        if (event->GetBrick() != 0 && afterStep && event->GetBrick()->GetType() == BIBrick::TypeSubstep)
            --count;
        afterStep = index != 0 && event->GetType() == BIEvent::TypeStep;
        if (afterStep)
            ++count;
        if (IsSubstepEvent(event)) {
            SubstepEventMap::Iterator sub = m_substepEvents.Find(static_cast<BIBrickSubstep*>(event->GetBrick()));
            if (sub != m_substepEvents.End()) {
                UInt32 inner = CountSteps(sub->second, position, found);
                if (found)
                    return inner + count;
                count = inner + count + (primitiveSeen ? 1 : 0);
            }
        }
        ++index;
        atPosition = index == position;
    }
    if (index == position && SameEvents(m_generator->m_events, events))
        found = true;
    return count;
}

void BGAutomaticApplication::NotifyProgress(Real32 progress)
{
    Iterator it(*this);
    while (BGAutomaticApplicationObserver* observer = it.Next(m_observers))
        observer->OnProgress(progress);
}

// ---- BGApplication -------------------------------------------------------------------------

// Mac 0x348a00, Win 0x5677b0
UInt32 BGAutomaticApplication::FilterSubsteps(BasicSet<BIBrick*>& bricks)
{
    return m_filterProgram->Apply(bricks);
}

// Mac 0x348810, Win 0x567640
bool BGAutomaticApplication::AnalyzeBrick(BIBrick* brick)
{
    if (m_producer.Get() == 0)
        return false;
    BIGraph::GraphList graphs;
    BIGraph::Build(graphs, brick->GetPrimitiveSet());
    BasicArray<BIBrickSubstep*> substeps;
    m_producer->Analyze(graphs, substeps);
    if (IsCancelled())
        return false;
    for (BasicArray<BIBrickSubstep*>::Iterator it = substeps.Begin(); it != substeps.End(); ++it) {
        RefPointer<BIBrickSubstep> substep(*it);
        BIBrickRepository::GetInstance().AddSubstep(substep);
    }
    return true;
}

// Mac 0x348a20, Win 0x568000
void BGAutomaticApplication::SetProgram(BIProgram* program)
{
    if (m_program.Get() != program)
        m_program = program;
    if (m_generator.Get() == 0)
        return;
    m_generator->Unknown43();
    if (m_useGeneratorCandidates) {
        EventList candidates = m_generator->GetCandidates(0);
        if (&candidates != &m_candidates)
            m_candidates = candidates;
    }
    m_generator->m_allowedBricks.Clear();
}

// Mac 0x346880, Win 0x566fb0
void BGAutomaticApplication::CreateGenerator()
{
    m_generator = new BIGeneratorConstructive(this, kGeneratorMode);
}

// Mac 0x346470, Win 0x566c40
BIGenerator* BGAutomaticApplication::GetRootGenerator()
{
    return m_generators[0];
}

// Mac 0x346480, Win 0x566c70
const BGApplication::EventList& BGAutomaticApplication::GetEvents()
{
    SubstepEventMap::Iterator it = m_substepEvents.Find(0);
    // NOTE: the original returns the value slot of the map's end node; fixed: an empty list.
    if (it == m_substepEvents.End()) {
        static const EventList none;
        return none;
    }
    return it->second;
}

// Mac 0x3464c0, Win 0x566cb0
const BGApplication::SubstepEventMap& BGAutomaticApplication::GetSubstepEvents()
{
    return m_substepEvents;
}

// Mac 0x346450, Win 0x566c10
bool BGAutomaticApplication::HasGenerators()
{
    return !m_generators.IsEmpty();
}

// Mac 0x3464d0, Win 0x566cd0
bool BGAutomaticApplication::IsInSubstep()
{
    return m_generators.GetSize() > 1;
}

// Mac 0x3464f0, Win 0x566d00
void BGAutomaticApplication::ShowFreeBricks()
{
    BasicSet<BIBrick*> bricks;
    BIBrickRepository::GetInstance().GetFreeBricks(bricks);
    GetScene()->Clear();
    // NOTE: `update` false: the scene adds nothing (the original passes false).
    GetScene()->AddBrick(bricks, false);
}

// Mac 0x3465c0, Win 0x566db0
void BGAutomaticApplication::RestoreScenes()
{
    // NOTE: the event list and the brick set are shared by all generators (never cleared), so
    // every scene also gets the bricks of the generators before it.
    BasicSet<BIBrick*> bricks;
    EventList events;
    for (BasicArray<RefPointer<BIGenerator> >::Iterator it = m_generators.Begin(); it != m_generators.End(); ++it) {
        BIGenerator* generator = *it;
        generator->GetBrickEventsBefore(events);
        BI::GetBricks(events, bricks);
        generator->GetScene()->Clear();
        generator->GetScene()->AddBrick(bricks, true);
    }
}

// Mac 0x348ba0, Win 0x5680f0
BIEvent* BGAutomaticApplication::GetEvent(Int32 offset)
{
    if (m_generator.Get() != 0)
        return m_generator->GetEvent(offset);
    return 0;
}

// Mac 0x348bd0, Win 0x568560
BIEvent* BGAutomaticApplication::NextPart(bool substeps, StorageScene2::Camera** camera)
{
    if (m_sceneChanged) {
        m_sceneChanged = false;
        return m_generator->Next(m_nextArgument.Get());
    }
    m_generator->m_allowedBricks.Clear();
    m_sceneChanged = false;
    if (substeps && IsSubstepEvent(GetEvent(1))) {
        if (!EnterSubstep())
            return 0;
        return NextPart(true, camera);
    }
    BIEvent* event = m_generator->Next();
    if (event == 0) {
        if (substeps && m_substeps.GetSize() > 1)
            return ExitSubstep(camera);
        return 0;
    }
    if (event->GetType() == BIEvent::TypeStep) {
        StorageScene2::Camera* stepCamera = static_cast<BIEventStep*>(event)->m_camera.Get();
        if (camera != 0 && stepCamera != 0)
            *camera = stepCamera;
        return NextPart(substeps, camera);
    }
    return event;
}

// Mac 0x348d40, Win 0x568140
BIEvent* BGAutomaticApplication::PrevPart(bool substeps, StorageScene2::Camera** camera)
{
    if (m_sceneChanged) {
        m_sceneChanged = false;
        return m_generator->Previous(m_previousArgument.Get());
    }
    m_generator->m_allowedBricks.Clear();
    BIEvent* event = m_generator->Previous();
    m_sceneChanged = false;
    if (substeps) {
        if (IsSubstepEvent(GetEvent(1))) {
            if (!EnterSubstep())
                return 0;
            FastForward();
            if (camera == 0)
                return 0;
            *camera = FindPreviousCamera(m_generator);
            return 0;
        }
        if (m_substeps.GetSize() >= 2 && m_generator->GetCurrent() == 0) {
            ExitSubstep(camera);
            m_generator->Previous();
            return 0;
        }
    }
    while (GetEvent(0) != 0 && GetEvent(0)->GetType() == BIEvent::TypeStep)
        PrevPart(substeps, camera);
    if (camera != 0)
        *camera = FindPreviousCamera(m_generator);
    return event;
}

// Mac 0x348fb0, Win 0x568810
bool BGAutomaticApplication::NextStep(EventList& events, StorageScene2::Camera** camera)
{
    if (GetEvent(1) == 0) {
        RefPointer<BIEvent> event = ExitSubstep(camera);
        if (event.Get() == 0)
            return false;
        events.PushBack(event);
        return true;
    }
    if (GetEvent(1)->GetType() == BIEvent::TypeStep) {
        BIEvent* step = m_generator->Next();
        if (step->GetType() == BIEvent::TypeStep && camera != 0 && static_cast<BIEventStep*>(step)->m_camera.Get() != 0)
            *camera = static_cast<BIEventStep*>(step)->m_camera.Get();
        return NextStep(events, camera);
    }
    if (IsSubstepEvent(GetEvent(1))) {
        if (EnterSubstep() && NextStep(events, camera))
            return true;
        events.Clear();
        return false;
    }
    while (GetEvent(1) != 0) {
        if (GetEvent(1)->GetType() != BIEvent::TypeBrick)
            break;
        if (GetEvent(1)->GetBrick()->GetType() != BIBrick::TypePrimitive)
            break;
        RefPointer<BIEvent> event = m_generator->Next();
        events.PushBack(event);
    }
    return !events.IsEmpty();
}

// Mac 0x349220, Win 0x568330
bool BGAutomaticApplication::PrevStep(EventList& events, StorageScene2::Camera** camera)
{
    if (GetEvent(0) != 0 && GetEvent(0)->GetType() == BIEvent::TypeStep) {
        m_generator->Previous();
        return PrevStep(events, 0);
    }
    if (GetEvent(0) != 0) {
        // NOTE: the original reads the brick of the event without a null check.
        if (GetEvent(0)->GetBrick()->GetType() == BIBrick::TypeSubstep) {
            m_generator->Previous();
            if (EnterSubstep())
                FastForward();
        } else {
            while (GetEvent(0) != 0 && GetEvent(0)->GetType() == BIEvent::TypeBrick) {
                RefPointer<BIEvent> event = m_generator->Previous();
                events.PushBack(event);
            }
        }
    }
    while (GetEvent(0) != 0 && GetEvent(0)->GetType() == BIEvent::TypeStep)
        m_generator->Previous();
    if (GetEvent(0) == 0) {
        if (ExitSubstep(0) == 0)
            return false;
        m_generator->Previous();
        return true;
    }
    if (camera != 0)
        *camera = FindPreviousCamera(m_generator);
    return !events.IsEmpty();
}

// Mac 0x349480, Win 0x568a70
bool BGAutomaticApplication::InsertJumper()
{
    m_generator->Next(new BIEventJumper(2));
    UpdateView(true);
    return false;
}

// Mac 0x3495b0, Win 0x568ca0
bool BGAutomaticApplication::EnterSubstep()
{
    BIEvent* next = m_generator->GetEvent(1);
    if (next == 0)
        return false;
    if (!IsSubstepEvent(m_generator->GetEvent(1)) && m_generator->GetEvent(1)->GetType() != BIEvent::TypeStep)
        return false;
    if (m_generator->GetEvent(1)->GetType() == BIEvent::TypeMarker)
        return false;
    if (m_generator->GetEvent(1)->GetType() == BIEvent::TypeStep)
        m_generator->Next();
    BIBrick* brick = m_generator->GetEvent(1)->GetBrick();
    if (GenerateBrick(brick) < 0)
        return false;
    m_currentSubstep = brick;
    Rewind();
    return true;
}

// Mac 0x3496c0, Win 0x568e50
BIEvent* BGAutomaticApplication::ExitSubstep(StorageScene2::Camera** camera)
{
    if (m_substeps.GetSize() < 2 || m_substeps.Back() == 0 || m_generators.GetSize() < 2)
        return 0;
    EventList events;
    UInt32 marker = 0;
    m_generator->RemoveMarker(marker);
    m_generator->GetBrickEvents(events);
    RefPointer<BIBrick> brick;
    if (events.GetSize() == 1) {
        brick = events[0]->GetBrick();
    } else if (events.GetSize() > 1) {
        BasicSet<BIBrickPrimitive*> primitives;
        BI::GetPrimitives(events, primitives);
        brick = BIBrickRepository::GetInstance().GetSubstep(primitives);
    }
    RefPointer<BIGenerator> inner = m_generator;
    BIBrick* substep = m_substeps.Back();
    m_substeps.PopBack();
    m_generators.PopBack();
    m_generator = m_generators.Back();
    m_currentSubstep = substep;
    if (camera != 0)
        *camera = FindPreviousCamera(m_generator);
    if (events.IsEmpty())
        return 0;
    BasicSet<BIBrick*> bricks;
    bricks.Insert(brick.Get());
    BIEvent* event = m_generator->Next(bricks);
    if (event == 0) {
        BasicSet<BIBrick*> previous;
        previous.Insert(substep);
        m_generator->Next(previous);
        return 0;
    }
    if (brick->GetType() == BIBrick::TypeSubstep) {
        UInt32 innerMarker;
        inner->RemoveMarker(innerMarker);
        EventList& stored = m_substepEvents[static_cast<BIBrickSubstep*>(brick.Get())];
        if (&inner->m_events != &stored)
            stored = inner->m_events;
    }
    return event;
}

// Mac 0x349cf0, Win 0x5692d0
bool BGAutomaticApplication::Rewind()
{
    return m_generator->Rewind();
}

// Mac 0x349d10, Win 0x569300
bool BGAutomaticApplication::FastForward()
{
    if (m_generator.Get() == 0)
        return false;
    return m_generator->FastForward();
}

// Mac 0x349d40, Win 0x569340
void BGAutomaticApplication::MoveTo(Int32 position, EventList& back, EventList& forward)
{
    Int32 current = (Int32)m_generator->GetCurrent();
    Int32 delta = position - current;
    if (delta != -1) {
        m_generator->m_allowedBricks.Clear();
        if (delta + 1 < 1) {
            do {
                RefPointer<BIEvent> event = m_generator->Previous();
                back.PushBack(event);
                ++delta;
            } while (delta < -1);
            GetEvent(0);
            return;
        }
        Int32 count = position + 2 - current;
        do {
            RefPointer<BIEvent> event = m_generator->Next();
            forward.PushBack(event);
            --count;
        } while (count > 1);
    }
    GetEvent(0);
}

// Mac 0x349f30, Win 0x569560
RefPointer<BIEvent> BGAutomaticApplication::TakeEvent(bool checkNext, bool release)
{
    if (checkNext) {
        BIEvent* current = m_generator->GetEvent(0);
        if (current == 0 || current->GetType() == BIEvent::TypeMarker || PrevPart(false, 0) == 0)
            return RefPointer<BIEvent>();
    }
    RefPointer<BIEvent> event = m_generator->TakeNextEvent();
    if (event.Get() == 0)
        return RefPointer<BIEvent>();
    UInt32 marker = 0;
    m_generator->RemoveMarker(marker);
    m_generator->AppendMarker();
    if (release && IsSubstepEvent(event))
        BIBrickRepository::GetInstance().SetRemoved(event->GetBrick());
    return event;
}

// Mac 0x34a0a0, Win 0x569780
bool BGAutomaticApplication::Truncate()
{
    m_generator->Truncate();
    return true;
}

// Mac 0x34a0c0, Win 0x5697b0
bool BGAutomaticApplication::ToggleBrick(BIBrick* brick, BasicArray<Pair<bool, RefPointer<BIEvent> > >& changes)
{
    return m_generator->ToggleBrick(brick, changes);
}

// Mac 0x34b0f0, Win 0x56a8a0
BICameraChangeFinder* BGAutomaticApplication::GetCameraChangeFinder()
{
    if (m_cameraChangeFinder.Get() == 0) {
        BasicArray<BICameraPosition> positions;
        for (UInt32 corner = 0; corner < 8; ++corner) {
            Vector3 direction(GetCornerDirection(corner));
            positions.PushBack(BICameraPosition(direction, 0.0f <= direction.y));
        }
        m_cameraChangeFinder = new BICameraChangeFinder(m_render, 0x80, 0x80, 4, 0.2f, 0, positions);
    }
    m_cameraChangeFinder->SetViewportSize(m_viewWidth, m_viewHeight);
    return m_cameraChangeFinder;
}

// Mac 0x34c560, Win 0x566360
void BGAutomaticApplication::ClearEnabledGroups()
{
    m_enabledGroups.Clear();
}

// Mac 0x34c5a0, Win 0x566380
void BGAutomaticApplication::EnableGroup(const BIAnalyzer* group)
{
    m_enabledGroups.Insert(group);
}

// Mac 0x34c5d0, Win 0x5663b0
bool BGAutomaticApplication::IsGroupEnabled(const BIAnalyzer* group)
{
    return m_enabledGroups.Contains(group);
}

// Mac 0x34b290, Win 0x56aa20
BGScene* BGAutomaticApplication::GetScene()
{
    if (m_generator.Get() == 0)
        return 0;
    return m_generator->GetScene();
}

// Mac 0x34b2c0, Win 0x56aa60
Scene* BGAutomaticApplication::GetBrickScene()
{
    return m_scene;
}

// Mac 0x34b2d0, Win 0x56aa80
BIProgram* BGAutomaticApplication::GetProgram()
{
    return m_program;
}

// Mac 0x34c630, Win 0x5663f0
BIProducer* BGAutomaticApplication::GetProducer()
{
    return m_producer;
}

// Mac 0x34a9f0, Win 0x569f70
BGApplication::EventList& BGAutomaticApplication::GetCandidates()
{
    return m_candidates;
}

// Mac 0x34aa00, Win 0x569f90
BIBrick* BGAutomaticApplication::SelectCandidate(BIBrick* brick)
{
    if (!m_useGeneratorCandidates)
        return SmallestIntersecting(brick, m_candidates);
    EventList candidates = m_generator->GetCandidates(brick);
    if (brick == 0 || !candidates.IsEmpty())
        return SmallestIntersecting(brick, candidates);
    // No candidate for the brick: the candidates it intersects are dropped.
    EventList dropped;
    for (EventList::Iterator it = m_candidates.Begin(); it != m_candidates.End(); ++it) {
        if (brick->Intersects((*it)->GetBrick()))
            dropped.PushBack(*it);
    }
    for (EventList::Iterator it = dropped.Begin(); it != dropped.End(); ++it) {
        for (UInt32 i = 0; i < m_candidates.GetSize(); ++i) {
            if (m_candidates[i].Get() == it->Get()) {
                m_candidates.Erase(i);
                break;
            }
        }
    }
    Iterator observers(*this);
    while (BGAutomaticApplicationObserver* observer = observers.Next(m_observers))
        observer->OnCandidatesChanged();
    return 0;
}

// Mac 0x34b030, Win 0x56a540
bool BGAutomaticApplication::IsAtStart()
{
    if (m_substeps.GetSize() < 2)
        return m_generator->IsAtStart();
    return false;
}

// Mac 0x34b070, Win 0x56a5a0
bool BGAutomaticApplication::IsAtEnd()
{
    if (!m_sceneChanged && m_substeps.GetSize() < 2)
        return m_generator->IsAtEnd();
    return false;
}

// Mac 0x34af90, Win 0x56a430
UInt32 BGAutomaticApplication::GetCurrentStep()
{
    if (m_generator.Get() == 0)
        return 0;
    return m_generator->GetCurrent();
}

// Mac 0x34afc0, Win 0x56a480
UInt32 BGAutomaticApplication::GetStepCount()
{
    if (m_generator.Get() == 0)
        return 0;
    return m_generator->GetCompleteCount();
}

// Mac 0x34aff0, Win 0x56a4d0
BIGenerator* BGAutomaticApplication::GetGenerator()
{
    return m_generator;
}

// Mac 0x34c640, Win 0x566410
BasicArray<BIBrick*>& BGAutomaticApplication::GetSubsteps()
{
    return m_substeps;
}

// Mac 0x34b2e0, Win 0x56aaa0
void BGAutomaticApplication::GetPrimitives(BasicSet<BIBrickPrimitive*>& primitives, bool placed)
{
    if (m_substeps.GetSize() > 1) {
        // The primitives of every level, without the current substep's.
        for (BasicArray<RefPointer<BIGenerator> >::Iterator it = m_generators.Begin(); it != m_generators.End(); ++it)
            AddEventPrimitives((*it)->m_events, primitives);
        const BasicSet<BIBrickPrimitive*>& own = m_substeps.Back()->GetPrimitiveSet();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator p = own.Begin(); p != own.End(); ++p)
            primitives.Erase(*p);
    }
    EventList events;
    if (!placed)
        m_generators.Back()->GetBrickEventsBefore(events);
    else
        m_generators.Back()->GetEvents(events);
    AddEventPrimitives(events, primitives);
}

// Mac 0x34b0b0, Win 0x56a840
bool BGAutomaticApplication::IsCancelled()
{
    return m_cancelled;
}

// Mac 0x34b0c0, Win 0x56a860
void BGAutomaticApplication::Cancel()
{
    m_cancelled = true;
}

// Mac 0x34a430, Win 0x569d40
void BGAutomaticApplication::AdvanceProgress()
{
    ++m_progress;
    NotifyProgress((Real32)m_progress / (Real32)m_progressTotal);
}

// Mac 0x34a6f0, Win 0x569e30
void BGAutomaticApplication::StepProgress()
{
    m_progress += m_progressStep;
    NotifyProgress((Real32)m_progress / (Real32)m_progressTotal);
}

// Mac 0x34a9b0, Win 0x569f30
void BGAutomaticApplication::SaveProgress()
{
    m_savedProgress = m_progress;
}

// Mac 0x34a9d0, Win 0x569f50
void BGAutomaticApplication::RestoreProgress()
{
    m_progress = m_savedProgress;
}

// Mac 0x349ee0, Win 0x5694e0
Matrix3D BGAutomaticApplication::GetElementTransform(const PrimitiveElement* element)
{
    for (UInt32 i = m_elementTransforms.GetSize(); i != 0; --i) {
        if (m_elementTransforms[i - 1].first == element)
            return m_elementTransforms[i - 1].second;
    }
    return Matrix3D();
}

// Mac 0x34c020, Win 0x56b5e0
void BGAutomaticApplication::UpdateCurrentStep()
{
    SubstepEventMap::Iterator it = m_substepEvents.Find(0);
    if (it == m_substepEvents.End()) {
        m_currentStep = 0;
        return;
    }
    bool found = false;
    m_currentStep = CountSteps(m_substepEvents[0], m_generator->GetCurrent(), found);
}

// Mac 0x34a0e0, Win 0x5697f0
Vector3 BGAutomaticApplication::GetCornerDirection(UInt32 corner)
{
    Vector3 direction;
    switch (corner & 7) {
    case 1:  direction = Vector3(1.0f, 1.0f, -1.0f); break;
    case 2:  direction = Vector3(-1.0f, 1.0f, -1.0f); break;
    case 3:  direction = Vector3(-1.0f, 1.0f, 1.0f); break;
    case 4:  direction = Vector3(1.0f, -1.0f, 1.0f); break;
    case 5:  direction = Vector3(1.0f, -1.0f, -1.0f); break;
    case 6:  direction = Vector3(-1.0f, -1.0f, -1.0f); break;
    case 7:  direction = Vector3(-1.0f, -1.0f, 1.0f); break;
    default: direction = Vector3(1.0f, 1.0f, 1.0f); break;
    }
    return direction;
}

// Mac 0x34b700, Win 0x56afe0
void BGAutomaticApplication::UpdateView(bool stepChanged)
{
    UpdateCurrentStep();
    if (stepChanged) {
        if (m_substepEvents.Find(0) != m_substepEvents.End()) {
            bool found = false;
            m_stepCount = CountSteps(m_substepEvents[0], 10000000, found);
        } else {
            m_stepCount = 0;
        }
    }
    if (GetScene() == 0)
        return;
    {
        Iterator it(*this);
        while (BGAutomaticApplicationObserver* observer = it.Next(m_observers))
            observer->OnViewUpdated();
    }
    Iterator it(*this);
    while (BGAutomaticApplicationObserver* observer = it.Next(m_observers)) {
        bool atStep = true;
        if (!stepChanged) {
            atStep = (GetEvent(0) != 0 && GetEvent(0)->GetType() == BIEvent::TypeStep) ||
                     (GetEvent(1) != 0 && GetEvent(1)->GetType() == BIEvent::TypeStep);
        }
        observer->OnStepChanged(atStep);
    }
}

// ---- level marks (BGAutomaticMode) -------------------------------------------------------------

// Mac 0x349510
bool BGAutomaticApplication::IsBelowMarkedLevel()
{
    UInt32 levels = m_generators.GetSize();
    if (levels > 1 && m_markedLevel < levels)
        return levels != 2 || m_modelCount < 2;
    return false;
}

// Mac 0x349550
bool BGAutomaticApplication::IsAboveMarkedLevel()
{
    UInt32 levels = m_generators.GetSize();
    if (levels < m_markedLevel)
        return levels != 1 || m_modelCount < 2;
    return false;
}

// Mac 0x349590
void BGAutomaticApplication::MarkLevel()
{
    m_markedLevel = m_generators.GetSize();
}

// Mac 0x34b000
BIGenerator* BGAutomaticApplication::GetParentGenerator()
{
    UInt32 levels = m_generators.GetSize();
    if (levels == 1)
        return 0;
    return m_generators[levels - 2];
}

// Mac 0x34b0d0
void BGAutomaticApplication::SetViewSize(Real32 width, Real32 height)
{
    m_viewWidth = width;
    m_viewHeight = height;
}

// ---- SceneObserver ---------------------------------------------------------------------------

// Mac 0x34bd30 (thunk 0x34bd40), Win 0x56b1a0
void BGAutomaticApplication::OnSceneNotification5()
{
    m_sceneChanged = true;
}

// Mac 0x34bd50 (thunk 0x34bd60), Win 0x56b1c0
void BGAutomaticApplication::OnStructureChanged()
{
}

} // namespace LEGO
