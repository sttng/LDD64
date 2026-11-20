// LEGO Digital Designer — restored source
// TU 366 "BGMode.cpp": Mac 0x34ce50-0x352ecb, Win 0x65b51c-0x65e6cd. See BGMode.h.
// Template code of the TU (not written out): BasicArray<Matrix3D> push (0x452d0 elsewhere),
// BasicArray<RefPointer<Mesh> > push 0x34f820, BasicArray<PrimitiveGeometry> D1/D0 0x351070 /
// 0x351120 (Win sdd 0x65c3ea), erase 0x3511f0, tree erase 0x3518b0 (Win 0x65e45a), the
// BasicMap<BIBrick*, BrickGeometry> D1/D0 0x34d0b0 / 0x351860 (Win 0x65e481) and operator[]
// 0x3528b0, BasicMap<BIBrick*, ActivePart*> D1/D0 0x34f7c0 / 0x352440 (Win 0x65c3a3), its
// insert 0x3524e0, erase(key) 0x3527a0, tree erase 0x352490 (Win 0x65c37c), BasicDeque<
// ActivePart> D1/D0 0x34d080 / 0x351ab0 (Win 0x65d711 / 0x65d738), constructor 0x3519b0,
// clear 0x351c50, map destroy 0x351e40, push_back 0x351f00, node copy 0x3521b0 / 0x352290 /
// 0x352310, BasicSet<BIBrick*> helpers 0x352ab0 / 0x352ca0, BasicArray<PrimitiveGeometry>
// copy 0x352d50. Mac 0x352e50 = <iostream> (not code).
#include "BGMode.h"
#include "SceneGraphUtil.h"

#include <iostream>

#include "SystemTime.h"
#include "ObjectRefPointer.h"
#include "Mesh.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "Primitive.h"
#include "BrickDatabase.h"
#include "RenderQueue.h"
#include "Camera.h"
#include "ControlManager.h"
#include "BrickPalette.h"
#include "LDDPart.h"
#include "PrimitiveElementBrick.h"
#include "BricksControl.h"
#include "BrickDraw.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BGApplication.h"
#include "BGScene.h"
#include "BI.h"
#include "BIBrick.h"
#include "BIBrickPrimitive.h"
#include "BIEvent.h"
#include "BIGenerator.h"
#include "BIStorage.h"

namespace LEGO {

namespace {

const Real32 kPartDuration = 500.0f;     // ms a part flies (Mac 0x63e16c)
const Real32 kMaxProgress  = 1.1f;       // the flight overshoots to 110 %
const UInt64 kStepDelay    = 66;         // ms between the parts of a step (0x42)

// Control::Invalidate unless the manager holds the redraws (inline in the original).
void RedrawControl(Control* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

} // namespace

// Mac 0x34ce50 (C2)
BGMode::BGMode(ControlManager* manager, Window* window, BrickConnectSceneControl* sceneControl)
    : BrickMode(manager, window, sceneControl),
      m_playing(false),
      m_animating(false)
{
}

// Mac 0x350270 (D1) / 0x3505e0 (D0) (members and bases 0x3500c0), Win sdd 0x65e6cd
BGMode::~BGMode()
{
}

// Mac 0x3509d0 (thunks 0x350a80 … 0x350b40)
void BGMode::OnFinalRelease()
{
    BrickMode::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
}

// Mac 0x34d1b0
bool BGMode::StoreBuildingGuide()
{
    RefPointer<BIStorage> storage = new BIStorage;
    BGApplication* application = GetApplication();
    if (!storage->SetEvents(application->GetEvents(), GetApplication()->GetSubstepEvents())) {
        GetSceneControl()->GetBuildingInstructions()->Reset();
        return false;
    }
    GetSceneControl()->GetBuildingInstructions()->SetStorage(storage);
    return true;
}

// Mac 0x34d0e0, Win 0x65d844
void BGMode::Reset()
{
    m_brickGeometry.Clear();
    m_hiddenBricks.Clear();
    m_activeParts.Clear();
    m_playing = false;
}

// Mac 0x34d370, Win 0x65d1a0
void BGMode::StartPart(BIEvent* event, bool insert, UInt64 delay)
{
    m_animating = true;
    if (!event->GetBrick())
        return;
    UInt64 start = (UInt64)GetMilliseconds() + delay;
    ActivePart part;
    part.m_event = event;
    part.m_mode = insert ? 1 : 2;
    part.m_startTime = start;
    m_activeParts.PushBack(part);
    UpdateParts(start);
}

// Mac 0x34e530
void BGMode::UpdateParts(UInt64 time)
{
    for (BasicDeque<ActivePart>::Iterator part = m_activeParts.Begin(); part != m_activeParts.End(); ++part) {
        Real32 elapsed = (Real32)(UInt64)(time - part->m_startTime);
        Real32 progress = 0.0f;
        if (part->m_startTime < time)
            progress = elapsed / kPartDuration;
        BIBrick* brick = part->m_event->GetBrick();
        const BasicArray<PrimitiveElement*>& elements = brick->GetElements();
        if (elements.IsEmpty())
            continue;
        if (kMaxProgress <= progress)
            progress = kMaxProgress;
        for (BasicArray<PrimitiveElement*>::ConstIterator it = elements.Begin(); it != elements.End(); ++it) {
            PrimitiveElement* element = *it;
            Matrix3D from;
            Matrix3D to;
            if (part->m_mode == 1) {
                element->GetTransform(from);
                from.Translate(brick->GetDirection(GetApplication()->GetScene(), true));
                element->GetTransform(to);
            } else if (part->m_mode == 2) {
                element->GetTransform(from);
                element->GetTransform(to);
                to.Translate(brick->GetDirection(GetApplication()->GetScene(), true));
            }
            Matrix3D& current = part->m_transforms[element];
            element->GetTransform(current);
            const Vector3& start = from.GetTranslation();
            Vector3 position = start + (to.GetTranslation() - from.GetTranslation()) * progress;
            part->m_transforms[element].SetTranslation(position);
        }
    }
}

// Mac 0x34d2d0, Win 0x65d45f
bool BGMode::NextPart(bool substeps)
{
    StorageScene2::Camera* camera = 0;
    BIEvent* event = GetApplication()->NextPart(substeps, &camera);
    SetCamera(camera);
    if (event) {
        StartPart(event, true, 0);
        GetApplication()->UpdateView(false);
    }
    return event != 0;
}

// Mac 0x34d5c0, Win 0x65d4c4
bool BGMode::PrevPart(bool substeps)
{
    StorageScene2::Camera* camera = 0;
    BIEvent* event = GetApplication()->PrevPart(substeps, &camera);
    SetCamera(camera);
    if (event) {
        StartPart(event, false, 0);
        GetApplication()->UpdateView(false);
    }
    return event != 0;
}

// (inlined in Mac 0x34d660 / 0x34deb0)
void BGMode::ShowPlacedParts()
{
    m_animating = false;
    m_activeParts.Clear();
    const BasicSet<BIBrickPrimitive*>& placed = GetApplication()->GetScene()->GetPrimitives();
    BasicArray<PrimitiveElement*> elements;
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = placed.Begin(); it != placed.End(); ++it) {
        const BasicArray<PrimitiveElement*>& own = (*it)->GetElements();
        elements.InsertRange(elements.End(), own.Begin(), own.End());
    }
    BasicArray<RefPointer<Part> > highlighted;
    BasicArray<RefPointer<Part> > parts;
    // (each part once: SceneGraph::PushBackUnique, Mac 0xb2fe0 / Win 0x49bd10)
    for (BasicArray<PrimitiveElement*>::Iterator it = elements.Begin(); it != elements.End(); ++it)
        SceneGraph::PushBackUnique(parts, RefPointer<Part>((*it)->GetPart()));
    GetSceneControl()->BuildRenderDataForParts(parts, highlighted);
}

// Mac 0x34d660, Win 0x65d8ae
bool BGMode::NextStep(bool /*unused*/, bool animate, BasicArray<BIBrick*>& bricks, bool setCamera)
{
    BasicArray<RefPointer<BIEvent> > events;
    StorageScene2::Camera* camera = 0;
    if (!GetApplication()->NextStep(events, &camera)) {
        GetApplication()->UpdateView(true);
        return false;
    }
    if (setCamera)
        SetCamera(camera);
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = events.Begin(); it != events.End(); ++it) {
        if (BIBrick* brick = (*it)->GetBrick())
            brick->SetEnabled(false);
    }
    UInt64 index = 0;
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = events.Begin(); it != events.End(); ++it) {
        BIEvent* event = *it;
        if (!event->GetBrick())
            continue;
        event->GetBrick()->SetEnabled(true);
        bricks.PushBack(event->GetBrick());
        StartPart(event, true, index * kStepDelay);
        ++index;
    }
    if (!animate)
        ShowPlacedParts();
    GetApplication()->UpdateView(true);
    return true;
}

// Mac 0x34deb0, Win 0x65db15
bool BGMode::PrevStep(bool /*unused*/, bool animate)
{
    BasicArray<RefPointer<BIEvent> > events;
    if (!GetApplication()->PrevStep(events, 0)) {
        GetApplication()->UpdateView(true);
        return false;
    }
    UInt64 index = 0;
    for (BasicArray<RefPointer<BIEvent> >::Iterator it = events.Begin(); it != events.End(); ++it) {
        BIEvent* event = *it;
        if (event->GetBrick()) {
            StartPart(event, false, index * kStepDelay);
            ++index;
        }
    }
    if (!animate)
        ShowPlacedParts();
    GetApplication()->UpdateView(true);
    return true;
}

// Mac 0x34e420, Win 0x65b51c
bool BGMode::Play(bool /*unused*/)
{
    m_playing = true;
    UpdateCommands();
    return true;
}

// Mac 0x34e450, Win 0x65b530
bool BGMode::Pause()
{
    m_playing = false;
    UpdateCommands();
    return true;
}

// Mac 0x34e480, Win 0x65b542
bool BGMode::IsPlaying() const
{
    return m_playing;
}

// Mac 0x34e490, Win 0x65b549
void BGMode::Rewind()
{
    if (GetApplication()->Rewind())
        GetApplication()->UpdateView(false);
}

// Mac 0x34e4e0, Win 0x65b577
void BGMode::FastForward()
{
    if (GetApplication()->FastForward())
        GetApplication()->UpdateView(false);
}

// Mac 0x350ee0, Win 0x65c859
void BGMode::ShowDirection(UInt32 corner)
{
    Vector3 direction = GetApplication()->GetCornerDirection(corner);
    (void)direction;
    BasicArray<RefPointer<BIEvent> > events;
    GetApplication()->GetGenerator()->GetEvents(events);
    BasicSet<BIBrickPrimitive*> primitives;
    BI::GetPrimitives(events, primitives);
}

// Mac 0x350b60 (thunk 0x350ec0), Win 0x65d528
void BGMode::Update()
{
    if (m_playing) {
        if (m_activeParts.IsEmpty()) {
            BasicArray<BIBrick*> bricks;
            NextStep(true, true, bricks, false);
        }
        if (m_playing && m_activeParts.IsEmpty())
            Pause();
    }
    UInt64 now = (UInt64)GetMilliseconds();
    if (m_activeParts.IsEmpty()) {
        m_animating = false;
        return;
    }
    // The parts whose time is over (counted over the whole deque) leave from the front.
    Int32 finished = 0;
    for (BasicDeque<ActivePart>::Iterator part = m_activeParts.Begin(); part != m_activeParts.End(); ++part) {
        if (part->m_startTime <= now) {
            Real32 rest = (Real32)(UInt64)(now - part->m_startTime) / kPartDuration - 1.0f;
            if ((rest < 0.0f ? -rest : rest) <= 1e-5f || 0.0f <= rest)
                ++finished;
        }
    }
    for (; finished > 0; --finished)
        m_activeParts.PopFront();
    UpdateParts(now);
    RedrawControl(GetSceneControl());
}

// Mac 0x34f0e0
void BGMode::BuildGeometry(const BasicArray<BIBrick*>& bricks)
{
    BrickConnectSceneManager* sceneManager = GetSceneControl()->GetSceneManager();
    for (BasicArray<BIBrick*>::ConstIterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = *it;
        m_brickGeometry[brick].m_primitives.Clear();
        BasicArray<RefPointer<Part> > flexParts;   // each flexible part once (Mac 0xb2fe0 / Win 0x49bd10)
        const BasicArray<PrimitiveElement*>& elements = brick->GetElements();
        for (BasicArray<PrimitiveElement*>::ConstIterator e = elements.Begin(); e != elements.End(); ++e) {
            PrimitiveElement* element = *e;
            Part* part = element->GetPart();
            if (part->GetElementCount() < 2) {
                PrimitiveGeometry geometry;
                geometry.m_element = element;
                BoundingBox box(0, 0, 0, 0, 0, 0);
                BrickDraw::BuildMeshes(element->GetPrimitive()->GetDesignID(), part->GetMaterials(),
                                       part->GetDecorations(), sceneManager, geometry.m_meshes, box, 2);
                m_brickGeometry[brick].m_primitives.PushBack(geometry);
            } else {
                SceneGraph::PushBackUnique(flexParts, RefPointer<Part>(part));
            }
        }
        for (BasicArray<RefPointer<Part> >::Iterator p = flexParts.Begin(); p != flexParts.End(); ++p) {
            Part* part = *p;
            PrimitiveGeometry geometry;
            PrimitiveElement* first = part->GetElement(0);
            geometry.m_element = first;
            // The bone transforms relative to the first element.
            BasicArray<Matrix3D> bones;
            bones.PushBack(Matrix3D());
            Matrix3D transform;
            Matrix3D inverse;
            first->GetTransform(transform);
            inverse.SetInverse(transform);
            for (UInt32 i = 1; i < part->GetElementCount(); ++i) {
                part->GetElement(i)->GetTransform(transform);
                bones.PushBack(Matrix3D(inverse, transform));
            }
            BoundingBox box(0, 0, 0, 0, 0, 0);
            BrickDraw::BuildFlexMeshes(part->GetDesignID(), bones, first->GetPart()->GetMaterials(),
                                       first->GetPart()->GetDecorations(), sceneManager,
                                       geometry.m_meshes, box, 2);
            m_brickGeometry[brick].m_primitives.PushBack(geometry);
        }
    }
}

// Mac 0x34e9e0 (thunk 0x34f7f0), Win 0x65e053
void BGMode::OnEvent05(const Matrix3D& transform, Render* /*render*/, RenderQueue* /*overlayQueue*/,
                       RenderQueue* queue)
{
    // The bricks to draw: the placed primitives, and the animated bricks with their part (an
    // animated substep instead of its primitives).
    BasicMap<BIBrick*, ActivePart*> bricks;
    {
        const BasicSet<BIBrickPrimitive*>& placed = GetApplication()->GetScene()->GetPrimitives();
        for (BasicSet<BIBrickPrimitive*>::ConstIterator it = placed.Begin();
             it != GetApplication()->GetScene()->GetPrimitives().End(); ++it)
            bricks[*it] = 0;
    }
    for (BasicDeque<ActivePart>::Iterator part = m_activeParts.Begin(); part != m_activeParts.End(); ++part) {
        BIBrick* brick = part->m_event->GetBrick();
        if (!brick)
            continue;
        if (brick->GetType() == BIBrick::TypeSubstep) {
            const BasicSet<BIBrickPrimitive*>& primitives = brick->GetPrimitiveSet();
            for (BasicSet<BIBrickPrimitive*>::ConstIterator it = primitives.Begin(); it != primitives.End(); ++it)
                bricks.Erase(*it);
        }
        bricks[part->m_event->GetBrick()] = &*part;
    }
    // NOTE: an empty brick set is built and dropped here (both builds).
    BasicSet<BIBrick*> unused;
    for (BasicMap<BIBrick*, ActivePart*>::Iterator it = bricks.Begin(); it != bricks.End(); ++it) {
        BIBrick* brick = it->first;
        if (brick->Intersects(m_hiddenBricks))
            continue;
        if (!m_brickGeometry.Contains(brick)) {
            BasicArray<BIBrick*> one;
            one.PushBack(brick);
            BuildGeometry(one);
        }
        BrickGeometry& geometry = m_brickGeometry[brick];
        for (BasicArray<PrimitiveGeometry>::Iterator g = geometry.m_primitives.Begin();
             g != geometry.m_primitives.End(); ++g) {
            PrimitiveElement* element = g->m_element;
            Matrix3D world;
            ActivePart* part = it->second;
            if (part && part->m_transforms.Contains(element)) {
                world.Multiply(transform, part->m_transforms[element]);
            } else {
                Matrix3D elementTransform;
                element->GetTransform(elementTransform);
                world.Multiply(transform, elementTransform);
            }
            for (BasicArray<RefPointer<Mesh> >::Iterator mesh = g->m_meshes.Begin();
                 mesh != g->m_meshes.End(); ++mesh)
                queue->Add(*mesh, world, 0.0f, 0, 1.0f);
        }
    }
}

// Mac 0x34e9a0, Win 0x65b9cf
void BGMode::QueueParts(RenderQueue* queue)
{
    OnEvent05(Matrix3D::Identity(), GetSceneManager()->GetRender(), queue, queue);
}

// Mac 0x34fd70
void BGMode::CollectBricks(BIBrick* brick, BasicArray<ObjectRefPointer<PrimitiveElementBrick> >& bricks)
{
    if (brick->GetType() == BIBrick::TypePrimitive) {
        Part* part = brick->GetElements()[0]->GetPart();
        ObjectRefPointer<PrimitiveElementBrick> copy =
            new PrimitiveElementBrick(static_cast<const LDDBrick*>(part->GetBrick()));
        bricks.PushBack(copy);
    } else {
        BasicSet<BIBrickPrimitive*> primitives;
        brick->GetPrimitives(primitives);
        for (BasicSet<BIBrickPrimitive*>::Iterator it = primitives.Begin(); it != primitives.End(); ++it)
            CollectBricks(*it, bricks);
    }
}

// Mac 0x34faf0
BricksControl* BGMode::CreateBrickPreview(BIBrick* brick)
{
    BricksControl::BrickArray bricks;
    for (BasicSet<BIBrickPrimitive*>::ConstIterator it = brick->GetPrimitiveSet().Begin();
         it != brick->GetPrimitiveSet().End(); ++it)
        CollectBricks(*it, bricks);
    if (bricks.IsEmpty())
        return 0;
    const BrickDatabase::Brick* entry = GetSceneControl()->GetSceneManager()->GetDatabase()->GetBrick(
        bricks[0]->GetBrick()->GetDesignID());
    BricksControl* control = new BricksControl(GetManager(), GetSceneControl()->GetSceneManager(), 2);
    control->Create(bricks, entry->m_defaultCamera);
    control->SetSize(Vector2(100.0f, 100.0f));
    return control;
}

// Mac 0x34fee0, Win 0x65b5a5
void BGMode::SetCamera(const StorageScene2::Camera* camera)
{
    if (!camera)
        return;
    Matrix3D transformation(camera->m_transformation);
    BrickConnectSceneControl* control = GetSceneControl();
    const Vector3& position = transformation.GetTranslation();
    Vector3 target = Vector3(0.0f, 0.0f, -camera->m_distance) * camera->m_transformation;
    control->SetCameraLookAt(position, target, GetSceneControl()->GetCameraUp());
}

// Mac 0x34ffa0, Win 0x65b61c
void BGMode::GetCamera(StorageScene2::Camera* camera)
{
    camera->m_transformation = GetSceneControl()->GetCamera()->GetViewMatrix();
    camera->m_distance = Distance(GetSceneControl()->GetCameraPosition(), GetSceneControl()->GetCameraTarget());
    Matrix3D inverse;
    inverse.SetInverse(camera->m_transformation);
    camera->m_transformation = inverse;
}

} // namespace LEGO
