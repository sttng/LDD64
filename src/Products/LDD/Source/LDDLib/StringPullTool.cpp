// LEGO Digital Designer — restored source
// LEGO::StringPullTool and StringPullTool::Anchor. See StringPullTool.h.
//
// Original object file: StringPullTool.cpp (Mac TU 304 0x2b14a0-0x2b3b1b; Win 0x66bc55-0x66c9ac).
#include "StringPullTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "StringPullSolverCallback.h"
#include "ControlManager.h"
#include "DisconnectElement.h"
#include "Materials.h"
#include "Part.h"
#include "PrimitiveElement.h"
#include "PrimitiveRigidElement.h"
#include "Render.h"
#include "RigidElement.h"
#include "Scene.h"
#include "SystemTime.h"
#include "UndoManager.h"
#include "Window.h"
#include "BrickPhysics.h"
#include "Color.h"
#include "ObjectFactory.h"

#include <typeinfo>

namespace LEGO {

namespace {

// The physics bodies of the rigid graph of `rigid` (through the rigid connections) into
// `bodies`; a rigid element with an element of a part of one element (no flexible part) ends
// the walk there (nothing is added). Mac 0x2b2c90 (regparm: ecx rigid, edx bodies; a copy of
// ClickPullTool's Mac 0x331920)
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

// The unit vector of `axis` (0-2) scaled by `length`. (inlined)
Vector3 AxisVector(UInt32 axis, Real32 length)
{
    Vector3 v;
    ((Real32*)v)[axis] = length;
    return v;
}

} // namespace

// ===========================================================================================
// StringPullTool::Anchor
// ===========================================================================================

// Mac 0x2b32b0 / 0x2b3320, Win 0x66bc55 / 0x66bc96
StringPullTool::Anchor::~Anchor()
{
    // NOTE: no null check (both builds; CreateAnchor returns NULL without a running solver);
    // fixed: checked, as CreateAnchor also returns NULL once the solver has ended.
    if (m_handle == 0)
        return;
    m_handle->Remove();
    m_handle->Release();
}

// ===========================================================================================
// StringPullTool
// ===========================================================================================

// Mac 0x2b14a0, Win 0x66c1a8
bool StringPullTool::Create(Render* render, BrickConnectSceneManager* sceneManager)
{
    VertexBuffer::CreateArgs args;
    args.vertexCount = 2;
    args.indexCount = 2;
    args.frameCount = 0;
    args.dynamic = false;
    args.hasNormals = false;
    args.colorSetCount = 1;
    Object* object = render->GetObjectFactory()->CreateObject(typeid(VertexBuffer), args);
    m_lineBuffer = object ? dynamic_cast<VertexBuffer*>(object) : 0;
    m_lineBuffer->SetPrimitiveType(1);
    const Color colors[2] = {Color(1.0f, 1.0f, 1.0f, 0.5f), Color(1.0f, 1.0f, 1.0f, 0.5f)};
    const UInt32 indices[2] = {0, 1};
    m_lineBuffer->SetIndices(indices, 0, 2, 0);
    m_lineBuffer->SetColors(colors, 0, 2, 0);
    m_lineMesh = new SingleMesh(sceneManager->GetSilhouetteMaterial(), m_lineBuffer);
    m_axesMesh = CreateAxesMesh(render, Color(1.0f, 0.0f, 0.0f, 0.5f), Color(0.0f, 1.0f, 0.0f, 0.5f),
                                Color(0.0f, 0.0f, 1.0f, 0.5f), sceneManager->GetAxesMesh()->GetMaterial(),
                                false);
    return true;
}

// Mac 0x2b3470
void StringPullTool::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

// Mac 0x2b1830
void StringPullTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
    SetCursor(4);
}

// Mac 0x2b1870, Win 0x66c50e
void StringPullTool::OnDetached(BrickConnectSceneControl* control)
{
    m_activeAnchor = -1;
    m_anchors.Clear();
    UpdateObserver::Detach();
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x2b1b50
void StringPullTool::OnEvent19()
{
}

// Mac 0x2b1f30, Win 0x66bec7
void StringPullTool::OnEvent06(Render* render)
{
    for (UInt32 i = 0; i < m_anchors.GetSize(); ++i) {
        Anchor* anchor = m_anchors[i];
        Matrix3D elementTransform;
        anchor->m_element->GetTransform(elementTransform);
        // NOTE: the body-space point is placed with the element's transform (both builds)
        Vector3 line[2];
        line[0] = anchor->m_local.GetTranslation() * elementTransform;
        line[1] = Vector3(anchor->m_target.GetTranslation());
        m_lineBuffer->SetPositions(line, 0, 2, 0);

        render->EnableBlend(true);
        render->EnableDepthTest(false);
        render->EnableDepthWrite(false);
        m_lineMesh->Bind(render, 0.0f, true);
        render->SetWorldMatrix(Matrix3D());
        render->DrawVertexBuffer();
        m_axesMesh->Bind(render, 0.0f, true);
        render->SetWorldMatrix(anchor->m_target);
        render->DrawVertexBuffer();

        render->EnableDepthWrite(true);
        render->EnableDepthTest(true);
        render->EnableBlend(false);
        m_lineMesh->Bind(render, 0.0f, true);
        render->SetWorldMatrix(Matrix3D());
        render->DrawVertexBuffer();
        m_axesMesh->Bind(render, 0.0f, true);
        render->SetWorldMatrix(anchor->m_target);
        render->DrawVertexBuffer();
    }
}

// Mac 0x2b18f0 (thunk 0x2b1b30), Win 0x66bd0b
void StringPullTool::OnMouseMove(Control* control, const Vector2& position)
{
    if (m_activeAnchor < 0 || GetSceneControl() != control || m_activeAxis >= 3)
        return;
    if (!(GetSceneControl()->GetPressedButtons() & 1))
        return;
    Anchor* anchor = m_anchors[m_activeAnchor];
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetSceneControl()->GetPickRay(position, origin, direction, length);
    Vector3 grab = anchor->m_grab * anchor->m_target;
    Vector3 axis = AxisVector(m_activeAxis, 1.0f);
    Vector3 point;
    if (!GetSceneControl()->GetManager()->GetWindow()->IsKeyPressed(0x39)) {
        // along the axis: the point of the axis line nearest to the ray
        Vector3 onRay;
        Real32 s;
        Real32 t;
        Vector3 rayEnd = origin + direction;
        Vector3 axisEnd = grab + Rotate(axis, anchor->m_target);
        if (!ClosestPointsOnLines(origin, rayEnd, grab, axisEnd, onRay, point, s, t))
            return;
    } else {
        // in the plane normal to the axis through the grabbed point
        Plane plane(Rotate(axis, anchor->m_target), grab);
        point = Vector3();
        if (!LinePlaneIntersect(origin, direction, plane, point))
            return;
    }
    // NOTE: the grab offset is subtracted in target space without the target's rotation
    // (both builds)
    anchor->m_target.SetTranslation(point - anchor->m_grab);
    // NOTE: the original moves the solver anchor without a check (NULL without a running
    // solver); fixed: checked.
    if (anchor->m_handle != 0)
        anchor->m_handle->SetTargetTransform(anchor->m_target);
    Redraw(GetSceneControl());
}

// Mac 0x2b1b60 (thunk 0x2b1bf0), Win 0x66c358
void StringPullTool::Update()
{
    UInt64 now = (UInt64)GetMilliseconds();
    UInt32 elapsed = (UInt32)now - (UInt32)m_lastTime;
    m_lastTime = now;
    // NOTE: the original steps with 1 / elapsed (ms), which is 1 / 0 = +inf for two updates in
    // the same millisecond; fixed: at least 1 ms.
    if (elapsed == 0)
        elapsed = 1;
    Scene* scene = GetSceneControl()->GetScene();
    scene->GetPhysicsScene()->Step(1.0f / (Real32)elapsed);
    GetSceneControl()->GetScene()->NotifyObservers5();
}

// Mac 0x2b1c80 (thunk 0x2b1d00)
void StringPullTool::OnAttached(Updater* /*updater*/)
{
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetManager()->SetModalControl(true, control);
    GetSceneControl()->GetTransforms(m_transforms);
    m_callback = GetSceneControl()->GetScene()->CreateStringPullSolverCallback();
    m_lastTime = (UInt64)GetMilliseconds() - 16;
}

// Mac 0x2b1d80 (thunk 0x2b1f10)
void StringPullTool::OnDetached(Updater* /*updater*/)
{
    m_callback->Release();
    m_callback = 0;
    GetSceneControl()->GetScene()->PhysicsCall6(false);
    FinishMove();
    BrickConnectSceneControl* control = GetSceneControl();
    control->GetManager()->SetModalControl(false, control);
    Redraw(GetSceneControl());
}

// Mac 0x2b1e10
void StringPullTool::FinishMove()
{
    BasicArray<Pair<PrimitiveElement*, Matrix3D> > changed;
    GetSceneControl()->GetChangedTransforms(m_transforms, changed);
    m_transforms.Clear();
    if (!changed.IsEmpty()) {
        UndoManager* undo = GetSceneControl()->GetUndoManager();
        undo->AddAction(new UndoMoveSnapshot(GetSceneControl(), changed, false));
    }
}

// Mac 0x2b2160 (thunk 0x2b3140), Win 0x66c534
void StringPullTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    if (button != 0 || GetSceneControl() != control)
        return;
    Vector3 direction;
    Vector3 origin;
    Real32 length;
    GetSceneControl()->GetPickRay(position, origin, direction, length);

    // the axis handles of the anchor targets (cylinders 0.5 from the target, half height 0.5,
    // radius 0.1)
    if (!m_anchors.IsEmpty()) {
        Vector3 best;
        Real32 bestDistance = 3.4028235e+38f;   // FLT_MAX
        UInt32 bestAxis = 3;
        Int32 bestAnchor = -1;
        for (UInt32 i = 0; i < m_anchors.GetSize(); ++i) {
            const Matrix3D& target = m_anchors[i]->m_target;
            for (UInt32 axis = 0; axis < 3; ++axis) {
                Vector3 center = AxisVector(axis, 0.5f) * target;
                Vector3 direction2 = Rotate(AxisVector(axis, 1.0f), target);
                Vector3 hit;
                if (RayCylinderIntersect(origin, direction, center, direction2, 0.5f, 0.1f, &hit)) {
                    Real32 distance = origin.GetDistanceSquared(hit);
                    if (distance < bestDistance) {
                        bestDistance = distance;
                        best = hit;
                        bestAxis = axis;
                        bestAnchor = (Int32)i;
                    }
                }
            }
        }
        if (bestAnchor >= 0) {
            m_activeAnchor = bestAnchor;
            m_activeAxis = bestAxis;
            Anchor* anchor = m_anchors[bestAnchor];
            Matrix3D inverse;
            inverse.SetInverse(anchor->m_target);
            anchor->m_grab = best * inverse;
            return;
        }
    }

    Vector3 hit;
    BasicArray<RefPointer<Part> > noParts;
    BasicArray<RefPointer<Part> > noExtraParts;
    PrimitiveElement* element = GetSceneControl()->PickElement(position, &hit, true, 0, true,
                                                               noParts, noExtraParts);
    if (element == 0) {
        m_activeAxis = 3;
        m_activeAnchor = -1;
        return;
    }
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
    anchor->m_handle = m_callback->CreateAnchor(rigid->GetBody(), anchor->m_local, anchor->m_target,
                                                bodies.GetSize(), bodies.Begin());
    Redraw(GetSceneControl());
}

// Mac 0x2b3160 (thunk 0x2b3270), Win 0x66c41e
void StringPullTool::OnKeyPressed(Control* control, UInt32 key, UInt32 /*modifiers*/, bool /*repeat*/)
{
    if (key != 0xd3 || m_activeAnchor < 0 || GetSceneControl() != control)
        return;
    m_anchors.Erase((UInt32)m_activeAnchor);
    m_activeAxis = 3;
    m_activeAnchor = -1;
    Redraw(GetSceneControl());
}

} // namespace LEGO
