// LEGO Digital Designer — restored source
// TU 358 "BIStorage.cpp" (Mac 0x33f030-0x34274f; modmap guess StorageScene2_BICamera.cpp).
// See BIStorage.h. Windows inlines most of the conversion code (Win 0x55a670-0x55b24f,
// 0x55cef0-0x55d20f).
#include "BIStorage.h"
#include "BIEvent.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIBrickSubstep.h"
#include "BIBrickRepository.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x33f030
BIStorage::BIStorage()
{
}

// Mac 0x3418a0 / D0 0x3418b0 (body 0x342660), Win 0x55a920 / sdd 0x55aa60
BIStorage::~BIStorage()
{
}

// Mac 0x33f750
void BIStorage::Clear()
{
    m_name.Clear();
    m_events.Clear();
}

// Mac 0x33f8d0
void BIStorage::Part::AddElement(PrimitiveElement* element)
{
    RefPointer<PrimitiveElement> reference = element;
    for (BasicArray<RefPointer<PrimitiveElement> >::Iterator it = m_elements.Begin(); it != m_elements.End(); ++it)
        if (it->Get() == element)
            return;
    m_elements.PushBack(reference);
}

// ---- StorageScene2 -> storage --------------------------------------------------------------

// Mac 0x33f110
bool BIStorage::Load(const StorageScene2::BI& instructions, Scene::StorageMap& map, Scene* scene)
{
    m_name = instructions.m_name;
    BIBrickRepository::GetInstance().Build(scene);
    if (LoadEvents(instructions.m_events, map, scene, m_events)) {
        SubstepMap substeps;
        if (GetEvents(substeps))
            return true;
    }
    m_name.Clear();
    m_events.Clear();
    return false;
}

// Mac 0x33f250
bool BIStorage::LoadEvents(const BasicArray<RefPointer<StorageScene2::BIEvent> >& source, Scene::StorageMap& map,
                           Scene* scene, EventArray& events)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (BasicArray<RefPointer<StorageScene2::BIEvent> >::ConstIterator it = source.Begin(); it != source.End(); ++it) {
        StorageScene2::BIEvent* event = *it;
        if (event->m_type == StorageScene2::BIEvent::TypePart) {
            const StorageScene2::Part* storagePart = static_cast<StorageScene2::BIPart*>(event)->m_part;
            // NOTE: the original reads the bones of a null part reference (<PartRef/>) through a
            // null pointer; fixed: the load fails.
            if (storagePart == 0)
                return false;
            RefPointer<Part> part = new Part;
            for (BasicArray<RefPointer<StorageScene2::Bone> >::ConstIterator b = storagePart->m_bones.Begin();
                 b != storagePart->m_bones.End(); ++b) {
                BasicMap<const StorageScene2::Bone*, PrimitiveElement*>::Iterator found = map.m_elements.Find(b->Get());
                if (found == map.m_elements.End())
                    continue;
                if (repository.GetPrimitive(found->second, true) == 0)
                    return false;
                part->AddElement(found->second);
            }
            if (part->m_elements.IsEmpty())
                return false;
            events.PushBack(RefPointer<Event>(part.Get()));
        } else if (event->m_type == StorageScene2::BIEvent::TypeCamera) {
            // NOTE: the original adds a reference to the camera without a null check
            // (<Camera/> without cameraRef crashes); RefPointer tolerates null here.
            StorageScene2::Camera* camera = static_cast<StorageScene2::BICamera*>(event)->m_camera;
            events.PushBack(RefPointer<Event>(new Camera(camera)));
        } else if (event->m_type == StorageScene2::BIEvent::TypeStep) {
            const StorageScene2::BIStep* storageStep = static_cast<StorageScene2::BIStep*>(event);
            if (storageStep->m_events.IsEmpty())
                continue;
            RefPointer<Step> step = new Step;
            step->m_exploded = storageStep->m_exploded;
            step->m_explode = storageStep->m_explode;
            // NOTE: the result of the nested steps is ignored (both builds).
            LoadEvents(storageStep->m_events, map, scene, step->m_events);
            events.PushBack(RefPointer<Event>(step.Get()));
        }
    }
    return true;
}

// ---- storage -> StorageScene2 --------------------------------------------------------------

// Mac 0x33fa30, Win 0x55b040
bool BIStorage::Save(StorageScene2& storage, Scene::SaveMap& map)
{
    if (map.m_elements.IsEmpty())
        return false;
    StorageScene2::BI instructions;
    instructions.m_name = m_name;
    storage.m_buildingInstructions.PushBack(instructions);
    String prefix(m_name);
    prefix << "Step";
    if (!SaveEvents(m_events, storage.m_buildingInstructions.Back().m_events, String(prefix), map, storage)) {
        storage.m_buildingInstructions.PopBack();
        return false;
    }
    return true;
}

// Mac 0x33fcc0, Win 0x55b180
bool BIStorage::SaveEvents(const EventArray& events, BasicArray<RefPointer<StorageScene2::BIEvent> >& target,
                           const String& prefix, Scene::SaveMap& map, StorageScene2& storage)
{
    UInt32 stepNumber = 0;
    for (EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        Event* event = *it;
        if (event->m_type == Event::TypeStep) {
            ++stepNumber;
            String subPrefix(prefix);
            subPrefix << stepNumber;
            subPrefix << "Substep";
            String stepName(prefix);
            stepName << stepNumber;
            RefPointer<StorageScene2::BIStep> step = new StorageScene2::BIStep;
            step->m_name = stepName;
            // NOTE: m_exploded / m_explode of the stored step are not written (both builds).
            if (!SaveEvents(static_cast<Step*>(event)->m_events, step->m_events, String(subPrefix), map, storage))
                return false;
            target.PushBack(RefPointer<StorageScene2::BIEvent>(step.Get()));
        } else if (event->m_type == Event::TypeCamera) {
            StorageScene2::Camera* camera = static_cast<Camera*>(event)->m_camera;
            target.PushBack(RefPointer<StorageScene2::BIEvent>(new StorageScene2::BICamera(camera)));
            bool known = false;
            for (BasicArray<RefPointer<StorageScene2::Camera> >::Iterator c = storage.m_cameras.Begin();
                 c != storage.m_cameras.End(); ++c)
                if (c->Get() == camera) {
                    known = true;
                    break;
                }
            if (!known)
                storage.m_cameras.PushBack(RefPointer<StorageScene2::Camera>(camera));
        } else if (event->m_type == Event::TypePart) {
            RefPointer<StorageScene2::BIPart> part = new StorageScene2::BIPart(RefPointer<StorageScene2::Part>());
            BasicSet<const LEGO::Part*> multiBoneParts;
            const BasicArray<RefPointer<PrimitiveElement> >& elements = static_cast<Part*>(event)->m_elements;
            for (BasicArray<RefPointer<PrimitiveElement> >::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
                if (!map.m_elements.Contains(e->Get()))
                    return false;
                const LEGO::Part* scenePart = (*e)->GetPart();
                if (scenePart->GetElementCount() > 1) {
                    if (multiBoneParts.Contains(scenePart))
                        continue;
                    multiBoneParts.Insert(scenePart);
                }
                // NOTE: every element overwrites the part reference: the last one wins.
                part->m_part = map.m_parts[scenePart];
            }
            target.PushBack(RefPointer<StorageScene2::BIEvent>(part.Get()));
        }
    }
    return true;
}

// ---- storage -> BIEvents -------------------------------------------------------------------

// Mac 0x33f5c0
bool BIStorage::GetEvents(SubstepMap& substeps)
{
    BasicArray<RefPointer<BIEvent> > events;
    if (!ConvertEvents(events, substeps, m_events)) {
        substeps.Clear();
        return false;
    }
    substeps[0] = events;
    return true;
}

// First element of a stored part (the parts are never empty after Load).
static PrimitiveElement* FirstElement(BIStorage::Event* event)
{
    return static_cast<BIStorage::Part*>(event)->m_elements[0];
}

static bool HasEventOfType(const BIStorage::EventArray& events, UInt32 type)
{
    for (BIStorage::EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it)
        if ((*it)->m_type == type)
            return true;
    return false;
}

// Mac 0x340b40
bool BIStorage::ConvertEvents(BasicArray<RefPointer<BIEvent> >& target, SubstepMap& substeps, const EventArray& events)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    bool ok = true;
    StorageScene2::Camera* camera = 0;
    for (EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        Event* event = *it;
        if (event->m_type == Event::TypeCamera) {
            camera = static_cast<Camera*>(event)->m_camera;
            continue;
        }
        if (event->m_type != Event::TypeStep)
            return false;
        target.PushBack(RefPointer<BIEvent>(new BIEventStep(camera)));
        camera = 0;
        const EventArray& stepEvents = static_cast<Step*>(event)->m_events;
        if (stepEvents.IsEmpty())
            return false;
        bool hasParts = HasEventOfType(stepEvents, Event::TypePart);
        bool hasSteps = HasEventOfType(stepEvents, Event::TypeStep);
        if (hasParts && !hasSteps) {
            ok = ok ? ConvertParts(target, stepEvents) : false;
        } else if (!hasParts && hasSteps) {
            BasicSet<BIBrickPrimitive*> primitives;
            CollectPrimitives(stepEvents, primitives);
            if (primitives.GetSize() < 2)
                return false;
            BIBrick* substep = repository.GetSubstep(primitives);
            target.PushBack(RefPointer<BIEvent>(new BIEventBrick(substep)));
            BasicArray<RefPointer<BIEvent> >& substepEvents = substeps[static_cast<BIBrickSubstep*>(substep)];
            substepEvents.Clear();
            ok = ok ? ConvertEvents(substeps[static_cast<BIBrickSubstep*>(substep)], substeps, stepEvents) : false;
        } else if (hasParts && hasSteps) {
            // NOTE: called whatever `ok` is (both builds).
            ok = ConvertMixed(target, substeps, stepEvents);
        } else {
            return false;
        }
    }
    return ok;
}

// Mac 0x340f90
bool BIStorage::ConvertParts(BasicArray<RefPointer<BIEvent> >& target, const EventArray& events)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    BasicSet<BIBrickPrimitive*> added;
    for (EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        if ((*it)->m_type != Event::TypePart)
            return false;
        BIBrickPrimitive* primitive = repository.GetPrimitive(FirstElement(*it), true);
        if (added.Contains(primitive))
            continue;
        target.PushBack(RefPointer<BIEvent>(new BIEventBrick(primitive)));
        added.Insert(primitive);
    }
    return true;
}

// Mac 0x341180
void BIStorage::CollectPrimitives(const EventArray& events, BasicSet<BIBrickPrimitive*>& primitives)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    for (EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        if ((*it)->m_type == Event::TypePart)
            primitives.Insert(repository.GetPrimitive(FirstElement(*it), true));
        else if ((*it)->m_type == Event::TypeStep)
            CollectPrimitives(static_cast<Step*>(it->Get())->m_events, primitives);
    }
}

// Mac 0x341220
bool BIStorage::ConvertMixed(BasicArray<RefPointer<BIEvent> >& target, SubstepMap& substeps, const EventArray& events)
{
    BIBrickRepository& repository = BIBrickRepository::GetInstance();
    BasicSet<BIBrickPrimitive*> added;
    bool ok = true;
    StorageScene2::Camera* camera = 0;
    for (EventArray::ConstIterator it = events.Begin(); it != events.End(); ++it) {
        Event* event = *it;
        if (event->m_type == Event::TypePart) {
            BIBrickPrimitive* primitive = repository.GetPrimitive(FirstElement(event), true);
            if (!added.Contains(primitive)) {
                target.PushBack(RefPointer<BIEvent>(new BIEventBrick(primitive)));
                added.Insert(primitive);
            }
            continue;
        }
        if (event->m_type == Event::TypeCamera) {
            camera = static_cast<Camera*>(event)->m_camera;
            continue;
        }
        if (event->m_type != Event::TypeStep)
            return false;
        const EventArray& stepEvents = static_cast<Step*>(event)->m_events;
        BasicSet<BIBrickPrimitive*> primitives;
        CollectPrimitives(stepEvents, primitives);
        if (primitives.GetSize() == 0)
            continue;
        BIBrickSubstep* substep = static_cast<BIBrickSubstep*>(repository.GetSubstep(primitives));
        target.PushBack(RefPointer<BIEvent>(new BIEventBrick(substep)));
        substeps[substep].Clear();
        if (stepEvents[0]->m_type == Event::TypePart)
            substeps[substep].PushBack(RefPointer<BIEvent>(new BIEventStep(camera)));
        bool hasParts = HasEventOfType(stepEvents, Event::TypePart);
        bool hasSteps = HasEventOfType(stepEvents, Event::TypeStep);
        if (hasParts && hasSteps) {
            // NOTE: the nested mixed step goes into `target`, not into the substep's events,
            // and replaces `ok` (both builds).
            ok = ConvertMixed(target, substeps, stepEvents);
            camera = 0;
        } else if (hasParts) {
            camera = 0;
            ok = ok ? ConvertParts(substeps[substep], stepEvents) : false;
        } else if (hasSteps) {
            camera = 0;
            ok = ok ? ConvertEvents(substeps[substep], substeps, stepEvents) : false;
        } else {
            return false;
        }
    }
    return ok;
}

// ---- BIEvents -> storage -------------------------------------------------------------------

// Mac 0x340550
bool BIStorage::SetEvents(const BasicArray<RefPointer<BIEvent> >& events, const SubstepMap& substeps)
{
    Clear();
    return FromEvents(events, substeps, m_events);
}

// Mac 0x3405c0
bool BIStorage::FromEvents(const BasicArray<RefPointer<BIEvent> >& source, const SubstepMap& substeps, EventArray& events)
{
    if (source.IsEmpty())
        return true;
    bool ok = true;
    Step* step = 0;
    bool nested = false;
    for (UInt32 i = 0; i < source.GetSize(); ++i) {
        BIEvent* event = source[i];
        if (event->m_type == BIEvent::TypeBrick) {
            BIBrick* brick = event->GetBrick();
            // NOTE: a brick event before the first step event (or a null brick) dereferences a
            // null pointer in the original; fixed: the conversion fails.
            if (step == 0 || brick == 0)
                return false;
            if (brick->GetType() == BIBrick::TypeSubstep) {
                // NOTE: the original reads past the tree header for a substep without events;
                // fixed: an empty event list is used instead.
                SubstepMap::ConstIterator found = substeps.Find(static_cast<BIBrickSubstep*>(brick));
                static const BasicArray<RefPointer<BIEvent> > s_none;
                const BasicArray<RefPointer<BIEvent> >& substepEvents = found != substeps.End() ? found->second : s_none;
                UInt32 steps = 0;
                bool split = false;
                for (BasicArray<RefPointer<BIEvent> >::ConstIterator e = substepEvents.Begin(); e != substepEvents.End(); ++e)
                    if ((*e)->m_type == BIEvent::TypeStep && ++steps > 1) {
                        split = nested;
                        break;
                    }
                if (split) {
                    step->m_events.PushBack(RefPointer<Event>(new Step));
                    ok = ok ? FromEvents(substepEvents, substeps, static_cast<Step*>(step->m_events.Back().Get())->m_events)
                            : false;
                } else {
                    ok = ok ? FromEvents(substepEvents, substeps, step->m_events) : false;
                }
            } else if (brick->GetType() == BIBrick::TypePrimitive) {
                AddPrimitiveParts(static_cast<BIBrickPrimitive*>(brick), step->m_events);
            } else {
                return false;
            }
        } else if (event->m_type == BIEvent::TypeStep) {
            StorageScene2::Camera* camera = static_cast<BIEventStep*>(event)->m_camera;
            if (camera)
                events.PushBack(RefPointer<Event>(new Camera(camera)));
            events.PushBack(RefPointer<Event>(new Step));
            step = static_cast<Step*>(events.Back().Get());
            // A substep becomes a nested step when this step also has a primitive brick
            // before the next step event.
            nested = false;
            for (UInt32 j = i + 1; j < source.GetSize(); ++j) {
                BIEvent* next = source[j];
                if (next->m_type == BIEvent::TypeBrick) {
                    if (next->GetBrick()->GetType() == BIBrick::TypePrimitive) {
                        nested = true;
                        break;
                    }
                } else if (next->m_type == BIEvent::TypeStep) {
                    break;
                }
            }
        } else {
            return false;
        }
    }
    return ok;
}

// Mac 0x3409a0
void BIStorage::AddPrimitiveParts(const BIBrickPrimitive* primitive, EventArray& events)
{
    // The scene parts of the primitive's elements, in pointer order (std::set).
    BasicSet<LEGO::Part*> parts;
    for (BasicArray<PrimitiveElement*>::ConstIterator it = primitive->m_elements.Begin(); it != primitive->m_elements.End(); ++it)
        parts.Insert((*it)->GetPart());
    for (BasicSet<LEGO::Part*>::Iterator it = parts.Begin(); it != parts.End(); ++it) {
        events.PushBack(RefPointer<Event>(new Part));
        Part* part = static_cast<Part*>(events.Back().Get());
        for (UInt32 i = 0; i < (*it)->GetElementCount(); ++i)
            part->AddElement((*it)->GetElement(i));
    }
}

} // namespace LEGO
