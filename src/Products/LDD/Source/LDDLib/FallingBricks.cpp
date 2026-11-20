// LEGO Digital Designer — restored source
// FallingBricks.cpp — LEGO::FallingBricks (see FallingBricks.h). TU 111: Mac 0x1ba5f0-0x1bf60b,
// Win 0x65a453-0x65b193 + 0x68c8ef-0x68d77b.
#include "FallingBricks.h"

#include <iostream>
#include <cmath>

#include "SystemTime.h"
#include "SystemUpdater.h"
#include "ControlManager.h"
#include "Scene.h"
#include "Part.h"
#include "Primitive.h"
#include "BrickDraw.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

namespace {

// Control::Invalidate unless the manager holds the redraws (inline everywhere).
void Redraw(BrickConnectSceneControl* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

} // namespace

// Mac 0x26cb60 (inline in DeconstructionCommand's constructor), Win 0x65ad6d
FallingBricks::FallingBricks(BrickConnectSceneControl* sceneControl, Render* render)
    : m_render(render), m_sceneControl(sceneControl), m_hideScene(false), m_running(false),
      m_reverse(false), m_rotate(false), m_easing(0), m_transparent(false), m_startTime(0),
      m_endTime(0), m_elapsed(0), m_stepDelay(0), m_duration(0)
{
    // NOTE: Mac zeroes +0x4d..+0x54 (m_running and the capacity end of m_conceptBricks) and
    // leaves +0x55..+0x5f (m_reverse, m_rotate, m_easing, m_transparent) uninitialised until
    // Start; Win zeroes +0x5c only. Initialised here.
}

// Mac 0x1bddf0 / 0x1bde50 (members + bases 0x1be450), Win 0x65aecc / sdd 0x65b193
FallingBricks::~FallingBricks()
{
}

// Mac 0x1bdec0 (thunks 0x1be090, 0x1be1a0, 0x1be2b0), Win 0x65ae9f
void FallingBricks::OnFinalRelease()
{
    Notifier<FallingBricksObserver>::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
}

// Mac 0x1ba5f0, Win 0x68d6dc
void FallingBricks::Start(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks, bool reverse,
                          UInt32 totalTime, Real32 stepRatio, Real32 distance, bool rotate,
                          UInt32 easing, bool transparent, bool hideScene,
                          const PositionMap* directions)
{
    UInt32 stepDelay = 0;
    UInt32 stepDuration = 0;
    if (!conceptBricks.IsEmpty()) {
        UInt32 count = conceptBricks.GetSize();
        if (stepRatio + 1e-05f > 1.0f)
            stepDelay = (UInt32)(Int64)(((Real32)totalTime / stepRatio) / (Real32)count);
        stepDuration = totalTime - (count - 1) * stepDelay;
    }
    StartSteps(conceptBricks, reverse, stepDuration, stepDelay, distance, rotate, easing,
               transparent, hideScene, directions);
}

// Mac 0x1ba6e0, Win 0x68d2da
void FallingBricks::StartSteps(const BasicArray<RefPointer<ConceptBrick> >& conceptBricks,
                               bool reverse, UInt32 stepDuration, UInt32 stepDelay,
                               Real32 distance, bool rotate, UInt32 easing, bool transparent,
                               bool hideScene, const PositionMap* directions)
{
    if (m_running)
        Stop();
    m_running = true;
    m_startTime = 0;
    m_endTime = 0;
    m_elapsed = 0;
    m_hideScene = hideScene;
    m_stepDelay = stepDelay;
    m_duration = stepDuration;
    m_reverse = reverse;
    m_rotate = rotate;
    m_easing = easing;
    m_transparent = transparent;
    if (&m_conceptBricks != &conceptBricks)
        m_conceptBricks = conceptBricks;

    // The element's own position is where it is assembled; the exploded one lies `distance`
    // away along the given direction, else away from the centre of the scene's box.
    PositionMap& assembled = reverse ? m_toPositions : m_fromPositions;
    PositionMap& exploded = reverse ? m_fromPositions : m_toPositions;
    for (BasicArray<RefPointer<ConceptBrick> >::Iterator brick = m_conceptBricks.Begin();
         brick != m_conceptBricks.End(); ++brick) {
        const BasicArray<RefPointer<PrimitiveElement> >& elements = (*brick)->GetElements();
        for (UInt32 i = 0; i < elements.GetSize(); ++i) {
            PrimitiveElement* element = elements[i];
            // NOTE: the scene box is computed again for every element (both builds).
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            element->GetScene()->GetBoundingBox(box, false);
            Vector3 center = box.GetCenter();
            Matrix3D transform;
            element->GetTransform(transform);
            assembled[element] = transform.GetTranslation();
            if (directions == 0) {
                Vector3 direction = assembled[element] - center;
                direction.Normalize();
                exploded[element] = assembled[element] + direction * distance;
            } else if (directions->Contains(element)) {
                exploded[element] = assembled[element] + directions->Find(element)->second * distance;
            } else {
                exploded[element] = assembled[element];
            }
        }
    }

    BrickConnectSceneControlObserver::Attach(m_sceneControl);
    BuildMeshes();
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    UpdateObserver::Attach(updater.Get());
}

// Mac 0x1bbad0, Win 0x68ce9f
void FallingBricks::Stop()
{
    if (!m_running)
        return;
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (m_reverse) {
        control->SetShowStatistics(true);
        Redraw(control);
    }
    BrickConnectSceneControlObserver::Detach();
    UpdateObserver::Detach();
    m_bricks.Clear();
    m_toPositions.Clear();
    m_fromPositions.Clear();
    m_running = false;
    Iterator pass(*this);
    while (FallingBricksObserver* observer = pass.Next(m_observers))
        observer->OnFinished(m_conceptBricks, m_reverse);
}

// Mac 0x1bbe70, Win 0x68cfd3
void FallingBricks::BuildMeshes()
{
    m_bricks.Clear();
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    if (!control)
        return;
    BrickConnectSceneManager* sceneManager = control->GetSceneManager();
    for (BasicArray<RefPointer<ConceptBrick> >::Iterator conceptBrick = m_conceptBricks.Begin();
         conceptBrick != m_conceptBricks.End(); ++conceptBrick) {
        m_bricks.PushBack(Brick());
        Brick& brick = m_bricks.Back();
        BasicArray<RefPointer<Part> > flexParts;
        const BasicArray<RefPointer<PrimitiveElement> >& elements = (*conceptBrick)->GetElements();
        for (UInt32 i = 0; i < elements.GetSize(); ++i) {
            PrimitiveElement* element = elements[i];
            Part* part = element->GetPart();
            if (part->GetElementCount() < 2) {
                Primitive primitive;
                primitive.m_element = element;
                BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
                BrickDraw::BuildMeshes(element->GetPrimitive()->GetDesignID(), part->GetMaterials(),
                                       part->GetDecorations(), sceneManager, primitive.m_meshes, box, 0);
                brick.m_primitives.PushBack(primitive);
            } else {
                BasicArray<RefPointer<Part> >::Iterator it = flexParts.Begin();
                while (it != flexParts.End() && it->Get() != part)
                    ++it;
                if (it == flexParts.End())
                    flexParts.PushBack(RefPointer<Part>(part));
            }
        }
        // A flexible part once: bone transforms relative to its first element.
        for (BasicArray<RefPointer<Part> >::Iterator it = flexParts.Begin(); it != flexParts.End(); ++it) {
            Part* part = *it;
            Primitive primitive;
            primitive.m_element = part->GetElement(0);
            BasicArray<Matrix3D> boneTransforms;
            boneTransforms.PushBack(Matrix3D());
            Matrix3D elementTransform;
            Matrix3D inverse;
            part->GetElement(0)->GetTransform(elementTransform);
            inverse.SetInverse(elementTransform);
            for (UInt32 e = 1; e < part->GetElementCount(); ++e) {
                part->GetElement(e)->GetTransform(elementTransform);
                boneTransforms.PushBack(Matrix3D(inverse, elementTransform));
            }
            BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
            BrickDraw::BuildFlexMeshes(part->GetDesignID(), boneTransforms, part->GetMaterials(),
                                       part->GetDecorations(), sceneManager, primitive.m_meshes, box, 0);
            m_bricks.Back().m_primitives.PushBack(primitive);
        }
        if (m_bricks.Back().m_primitives.IsEmpty())
            m_bricks.PopBack();
    }
    Redraw(control);
}

// Mac 0x1bd100, Win 0x68cf50
void FallingBricks::Update()
{
    BrickConnectSceneControl* control = BrickConnectSceneControlObserver::GetSubject();
    Redraw(control);
    Int64 now = GetMilliseconds();
    if (m_startTime == 0) {
        m_startTime = now;
        UInt32 steps = (m_conceptBricks.GetSize() - 1) * m_stepDelay;
        m_endTime = now + (Int64)(UInt64)m_duration + (Int64)(UInt64)steps;
        m_elapsed = 0;
        if (m_hideScene)
            control->SetShowStatistics(false);
    }
    if ((UInt64)now <= (UInt64)m_endTime)
        m_elapsed = (UInt32)now - (UInt32)m_startTime;
    else
        Stop();
}

// Mac 0x1bcfe0 (thunk 0x1bd0d0), Win 0x68cd06
void FallingBricks::OnEvent05(const Matrix3D& transform, Render* render, RenderQueue* queue,
                              RenderQueue* /*overlay*/)
{
    if (!m_reverse) {
        UInt32 step = 0;
        for (BasicArray<Brick>::Iterator brick = m_bricks.End(); brick != m_bricks.Begin(); )
            DrawBrick(*--brick, transform, render, queue, step++);
    } else if (m_startTime != 0) {
        UInt32 step = 0;
        for (BasicArray<Brick>::Iterator brick = m_bricks.Begin(); brick != m_bricks.End(); ++brick)
            DrawBrick(*brick, transform, render, queue, step++);
    }
}

// Mac 0x1bc860, Win 0x68ca04
void FallingBricks::DrawBrick(const Brick& brick, Matrix3D transform, Render* /*render*/,
                              RenderQueue* queue, UInt32 step)
{
    for (BasicArray<Primitive>::ConstIterator primitive = brick.m_primitives.Begin();
         primitive != brick.m_primitives.End(); ++primitive) {
        PrimitiveElement* element = primitive->m_element;
        Matrix3D elementTransform;
        element->GetTransform(elementTransform);
        UInt32 start = m_stepDelay * step;
        Matrix3D world;
        if (m_elapsed < start || m_duration + start < m_elapsed) {
            // before or after this step
            UInt32 end = start + m_duration;
            if (!m_reverse) {
                if (end < m_elapsed)
                    return;
                elementTransform.SetTranslation(m_fromPositions[element]);
            } else {
                if (m_elapsed < end)
                    return;
                elementTransform.SetTranslation(m_toPositions[element]);
            }
            world.Multiply(transform, elementTransform);
            for (BasicArray<RefPointer<Mesh> >::ConstIterator mesh = primitive->m_meshes.Begin();
                 mesh != primitive->m_meshes.End(); ++mesh)
                queue->Add(*mesh, world, 0.0f, 0, 1.0f);
            continue;
        }
        Real32 t = (Real32)(m_elapsed - start) / (Real32)m_duration;
        // NOTE: an easing value other than 0-2 leaves the factor at 12.5664 on Mac (the register
        // holding the 4 pi constant) and undefined on Win (both builds; not used).
        Real32 factor = 12.5664f;
        if (m_easing == EasingSquareRoot)
            factor = std::sqrt(t);
        else if (m_easing == EasingQuadratic)
            factor = t * t;
        else if (m_easing == EasingLinear)
            factor = t;
        if (m_rotate) {
            // spin about one axis (step % 6: x, y, z once, then twice), by the linear progress
            elementTransform.SetTranslation(Vector3());
            switch (step % 6) {
            case 0: elementTransform.RotateX(t * 6.2832f); break;
            case 1: elementTransform.RotateY(t * 6.2832f); break;
            case 2: elementTransform.RotateZ(t * 6.2832f); break;
            case 3: elementTransform.RotateX(t * 12.5664f); break;
            case 4: elementTransform.RotateY(t * 12.5664f); break;
            case 5: elementTransform.RotateZ(t * 12.5664f); break;
            }
        }
        const Vector3& from = m_fromPositions[element];
        const Vector3& to = m_toPositions[element];
        elementTransform.SetTranslation(from + (to - from) * factor);
        world.Multiply(transform, elementTransform);
        for (BasicArray<RefPointer<Mesh> >::ConstIterator mesh = primitive->m_meshes.Begin();
             mesh != primitive->m_meshes.End(); ++mesh) {
            if (!m_transparent)
                queue->Add(*mesh, world, 0.0f, 0, 1.0f);
            else
                queue->AddWithDepthPrepass(*mesh, world, 0.0f, 0, 0.5f, false);
        }
    }
}

} // namespace LEGO
