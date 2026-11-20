// LEGO Digital Designer — restored source
// LEGO::ClickPullTool and ClickPullTool::Anchor. See ClickPullTool.h.
//
// Original object file: ClickPullTool.cpp (Mac TU 350 0x3305f0-0x33260b; Win 0x66c9ad-0x66d1xx).
#include "ClickPullTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ClickPullSolverCallback.h"
#include "ControlManager.h"
#include "DisconnectElement.h"
#include "LDDUtilities.h"
#include "Materials.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "RenderQueue.h"
#include "RigidElement.h"
#include "Scene.h"
#include "SystemTime.h"
#include "UndoManager.h"
#include "BrickPhysics.h"
#include "Color.h"

namespace LEGO {

namespace {

// The physics bodies of the rigid graph of `rigid` (through the rigid connections) into
// `bodies`; a rigid element with an element of a part of one element (no flexible part) ends
// the walk there (nothing is added). Mac 0x331920 (regparm: ecx rigid, edx bodies; a copy of
// StringPullTool's Mac 0x2b2c90)
void CollectPullBodies(RigidElement* rigid, BasicArray<PhysicsRigidElement*>& bodies)
{
    const BasicArray<RefPointer<PrimitiveRigidElement> >& rigids = rigid->GetElements();
    for (UInt32 r = 0; r < rigids.GetSize(); ++r) {
        const BasicArray<RefPointer<PrimitiveElement> >& elements = rigids[r]->GetElements();
        for (UInt32 e = 0; e < elements.GetSize(); ++e) {
            if (elements[e]->GetPart()->GetElementCount() == 1)
                return;
        }
    }
    PhysicsRigidElement* body = rigid->GetBody();
    for (UInt32 i = 0; i < bodies.GetSize(); ++i) {
        if (bodies[i] == body)
            return;
    }
    bodies.PushBack(body);
    const RigidElement::ConnectionMap& connections = rigid->GetConnections();
    for (RigidElement::ConnectionMap::ConstIterator it = connections.Begin(); it != connections.End(); ++it)
        CollectPullBodies(it->first, bodies);
}

// Invalidates the scene control unless the manager's updates are locked.
void Redraw(BrickConnectSceneControl* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

} // namespace

// ===========================================================================================
// ClickPullTool::Anchor
// ===========================================================================================

// Mac 0x331dc0 / 0x331e30, Win 0x66c9ad / 0x66ca05
ClickPullTool::Anchor::~Anchor()
{
    // NOTE: no null check (both builds; AddAnchor returns NULL without a running solver)
    m_handle->Remove();
    m_handle->Release();
}

// ===========================================================================================
// ClickPullTool
// ===========================================================================================

// Mac 0x3305f0, Win 0x66cace
bool ClickPullTool::Create(Render* render, BrickConnectSceneManager* sceneManager)
{
    m_boxMesh = CreateBoxMesh(render, Color::Green(), sceneManager->GetSilhouetteMaterial(), false);
    return true;
}

// Mac 0x331f80
void ClickPullTool::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

// Mac 0x330670
void ClickPullTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
    SetCursor(4);
}

// Mac 0x3306b0, Win 0x66cb9f
void ClickPullTool::OnDetached(BrickConnectSceneControl* control)
{
    if (!m_anchors.IsEmpty()) {
        control->SetTransforms(m_transforms, false);
        m_anchors.Clear();
    }
    m_highlighted.Clear();
    UpdateObserver::Detach();
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x330ec0, Win 0x66cb06
void ClickPullTool::OnEvent05(const Matrix3D& /*transform*/, Render* /*render*/,
                              RenderQueue* overlayQueue, RenderQueue* /*sceneQueue*/)
{
    for (UInt32 i = 0; i < m_highlighted.GetSize(); ++i) {
        Matrix3D box;
        GetElementBoxTransform(m_highlighted[i], box, false);
        overlayQueue->Add(m_boxMesh, box, 0.0f, 0, 1.0f);
    }
}

// Mac 0x330780 (thunk 0x3307e0), Win 0x66ca21
void ClickPullTool::OnMouseMove(Control* /*control*/, const Vector2& position)
{
    if (m_anchors.IsEmpty())
        return;
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetSceneControl()->GetPickRay(position, origin, direction, length);
    m_callback->UpdateAnchors(origin, direction);
}

// Mac 0x330850 (thunk 0x330bf0), Win 0x66cf51
void ClickPullTool::Update()
{
    UInt64 now = (UInt64)GetMilliseconds();
    UInt32 elapsed = (UInt32)now - (UInt32)m_lastTime;
    m_lastTime = now;
    // NOTE: the original steps with 1 / elapsed (ms), which is 1 / 0 = +inf for two updates in
    // the same millisecond; fixed: at least 1 ms (M15).
    if (elapsed == 0)
        elapsed = 1;
    Scene* scene = GetSceneControl()->GetScene();
    scene->GetPhysicsScene()->Step(1.0f / (Real32)elapsed);
    GetSceneControl()->GetScene()->NotifyObservers5();
    m_highlighted.Clear();
    if (m_callback->GetState() != 2)
        return;
    Scene::ConnectResult result;
    m_callback->CanConnect(result);
    for (UInt32 i = 0; i < result.m_connections.GetSize(); ++i) {
        const PrimitiveConnectionInfo& info = result.m_connections[i];
        // (Mac 0x330b10: push unless already listed)
        RefPointer<PrimitiveElement> element = info.m_element;
        bool found = false;
        for (UInt32 j = 0; j < m_highlighted.GetSize() && !found; ++j)
            found = m_highlighted[j] == element;
        if (!found)
            m_highlighted.PushBack(element);
        RefPointer<PrimitiveElement> other = info.m_otherElement;
        found = false;
        for (UInt32 j = 0; j < m_highlighted.GetSize() && !found; ++j)
            found = m_highlighted[j] == other;
        if (!found)
            m_highlighted.PushBack(other);
    }
}

// Mac 0x330c10 (thunk 0x330c90), Win 0x66ca75
void ClickPullTool::OnAttached(Updater* /*updater*/)
{
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetManager()->SetModalControl(true, control);
    GetSceneControl()->GetTransforms(m_transforms);
    m_callback = GetSceneControl()->GetScene()->CreateClickPullSolverCallback();
    m_lastTime = (UInt64)GetMilliseconds() - 16;
}

// Mac 0x330d10 (thunk 0x330ea0)
void ClickPullTool::OnDetached(Updater* /*updater*/)
{
    m_callback->Release();
    m_callback = 0;
    GetSceneControl()->GetScene()->PhysicsCall6(false);
    FinishMove();
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetManager()->SetModalControl(false, control);
    Redraw(GetSceneControl());
}

// Mac 0x330da0
void ClickPullTool::FinishMove()
{
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > changed;
    GetSceneControl()->GetChangedTransforms(m_transforms, changed);
    m_transforms.Clear();
    if (!changed.IsEmpty()) {
        UndoManager* undo = GetSceneControl()->GetUndoManager();
        undo->AddAction(new UndoMoveSnapshot(GetSceneControl(), changed, false));
    }
}

// Mac 0x330f30 (thunk 0x331da0), Win 0x66cbe4
void ClickPullTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    if (button != 0 || GetSceneControl() != control)
        return;
    if (!m_anchors.IsEmpty()) {
        // the second click: connect where the solver can, else restore
        bool connected = false;
        for (UInt32 step = 0; step < 20; ++step) {
            UInt32 state = m_callback->GetState();
            if (state == 1)
                break;
            if (state == 2) {
                Scene::ConnectResult result;
                m_callback->CanConnect(result);
                GetSceneControl()->GetScene()->Reconnect(result);
                Redraw(GetSceneControl());
                connected = true;
                break;
            }
            GetSceneControl()->GetScene()->GetPhysicsScene()->Step(0.016666668f);
        }
        if (!connected)
            GetSceneControl()->SetTransforms(m_transforms, false);
        m_anchors.Clear();
        m_highlighted.Clear();
        UpdateObserver::Detach();
        return;
    }

    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetSceneControl()->GetPickRay(position, origin, direction, length);
    Vector3 hit;
    BasicArray<RefPointer<Part> > noParts;
    BasicArray<RefPointer<Part> > noExtraParts;
    PrimitiveElement* element = GetSceneControl()->PickElement(position, &hit, true, 0, true,
                                                               noParts, noExtraParts);
    if (element == 0)
        return;
    DisconnectElement(element);
    BasicArray<PhysicsRigidElement*> bodies;
    RigidElement* rigid = element->GetRigidElement()->GetRigidElement();
    CollectPullBodies(rigid, bodies);
    if (bodies.IsEmpty())
        return;
    UpdateObserver::Attach(GetSceneControl()->GetManager());

    Matrix3D elementTransform;
    Matrix3D bodyTransform;
    Matrix3D inverse;
    element->GetTransform(elementTransform);
    rigid->GetTransform(bodyTransform);
    inverse.SetInverse(bodyTransform);
    Vector3 point = hit * elementTransform;
    RefPointer<Anchor> added = new Anchor;
    m_anchors.PushBack(added);
    Anchor* anchor = m_anchors.Back();
    anchor->m_element = element;
    anchor->m_target.SetTranslationMatrix(point);
    anchor->m_local.Multiply(inverse, anchor->m_target);
    anchor->m_handle = m_callback->AddAnchor(rigid->GetBody(), anchor->m_local, anchor->m_target,
                                             bodies.GetSize(), bodies.Begin(), origin, direction,
                                             element, point);
    m_callback->UpdateAnchors(origin, direction);
    Redraw(GetSceneControl());
}

} // namespace LEGO
