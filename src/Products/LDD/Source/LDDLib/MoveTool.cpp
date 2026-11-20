// LEGO Digital Designer — restored source
// LEGO::MoveTool / LEGO::MoveTool::UndoSetContent — see MoveTool.h.
// TU 067 "MoveTool.cpp": Mac 0x15b6b0-0x15e71b, Win 0x67bb5a-0x67cb9d.
// Not code: the thunks, the BasicArray instances (0x15b6b0 = the inline BrickDraw constructor),
// Mac 0x15e6a0 = <iostream> + a header static.
#include "MoveTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "BrickConnect.h"
#include "ControlManager.h"
#include "Window.h"
#include "Key.h"
#include "Camera.h"
#include "LDDUtilities.h"
#include "PrimitiveElement.h"
#include "Materials.h"
#include "RenderQueue.h"
#include "Mesh.h"
#include "Render.h"
#include "SimpleSound.h"
#include "Part.h"

#include <float.h>

namespace LEGO {

namespace {

// The tool cursor during a move. (BrickConnectSceneControl cursor ids)
const UInt32 kMoveCursor = 0x65;

} // namespace

// =============================================================================================
// MoveTool::UndoSetContent
// =============================================================================================

// Mac 0x15dae0, Win 0x67c3b8
void MoveTool::UndoSetContent::Execute()
{
    m_tool->ClearResult();
    m_tool->Drop();
    m_sceneManager->SetActiveTool(0);
}

// =============================================================================================
// MoveTool
// =============================================================================================

// Mac 0x15b7f0, Win 0x67c3da
MoveTool::MoveTool(BrickConnectSceneManager* sceneManager)
    : m_state(0), m_picked(0), m_axis(3), m_sourceControl(0),
      m_brickConnect(sceneManager->GetBrickConnect()), m_draw(sceneManager)
{
}

// Mac 0x15db20 (D1) / 0x15db80 (D0), body 0x15e210; Win sdd 0x67cb9d, body 0x67c5bb
MoveTool::~MoveTool()
{
}

// Mac 0x15dbf0 (thunks 0x15dd30, 0x15de80, 0x15e0e0)
void MoveTool::OnFinalRelease()
{
    PreferencesObserver::OnFinalRelease();
    BrickConnectSceneControlTool::OnFinalRelease();
}

// Mac 0x15bb30, Win 0x67c03c
bool MoveTool::Create(Render* render, BrickConnectSceneManager* sceneManager)
{
    m_render = render;
    m_simple = false;
    SetActive(true);
    m_dragScene = m_brickConnect->NewScene(0);
    if (m_dragScene == 0)
        return false;
    m_boxMesh = CreateBoxMesh(m_render, Color::Green(), sceneManager->GetSilhouetteMaterial(), false);
    m_axesMesh = CreateAxesMesh(m_render, Color(1.0f, 0.0f, 0.0f, 0.5f), Color(0.0f, 1.0f, 0.0f, 0.5f),
                                Color(0.0f, 0.0f, 1.0f, 0.5f),
                                sceneManager->GetAxesMesh()->GetMaterial(), false);
    m_sound = LDDUtilities::GetInstance()->CreateSound(String("IA_drop.mp3"));
    PreferencesObserver::Attach(Preferences::GetInstance().Get());
    return true;
}

// Mac 0x15c5b0, Win 0x67c2d0
void MoveTool::RebuildDraw(PrimitiveElement* first, bool single)
{
    m_draw.Clear();
    if (GetSceneControl() == 0)
        return;
    BasicArray<RefPointer<Part> > parts;
    m_dragScene->GetParts(parts);
    for (BasicArray<RefPointer<Part> >::ConstIterator it = parts.Begin(); it != parts.End(); ++it)
        m_draw.AddPart(*it, 0);
    Begin(GetSceneControl()->GetScene(), m_dragScene, first, single);
    GetSceneControl()->Redraw();
}

// Mac 0x15c120, Win 0x67c6d3
void MoveTool::PickUp(const BasicArray<RefPointer<Part> >& parts, PrimitiveElement* picked,
                      const Vector3& point, bool single)
{
    Drop();
    m_drawTransform.SetIdentity();
    m_pivot.SetIdentity();
    m_sourceControl = GetSceneControl();
    m_sourceControl->MoveParts(parts, m_dragScene);
    if (m_dragScene->IsEmpty()) {
        GetSceneControl()->GetSceneManager()->SetActiveTool(0);
        return;
    }
    m_grip.SetTranslation(-point);
    RebuildDraw(picked, single);
    UpdateHeightOffset();
    m_undo = new UndoSetContent(GetSceneControl()->GetSceneManager(), this);
    GetSceneControl()->GetUndoManager()->AddAction(m_undo);
    ApplyCursor(kMoveCursor);
}

// Mac 0x15c2d0, Win 0x67c1d4
void MoveTool::Drop()
{
    if (m_sourceControl == 0)
        return;
    if (!m_connected) {
        // back where they were, when they still connect there
        Scene::ConnectResult result;
        Matrix3D identity;
        const bool connects = m_scene->CanConnectDrag(identity, result);
        End(true);
        if (connects)
            m_sourceControl->MergeScene(m_dragScene, result, true);
    } else {
        End(false);
        m_sourceControl->MergeScene(m_dragScene, m_result, true);
        ClearResult();
    }
    m_sourceControl->ClearCarriedBricks();
    m_sourceControl = 0;
    m_draw.Clear();
    if (m_undo != 0) {
        GetSceneControl()->GetUndoManager()->RemoveAction(m_undo);
        m_undo = 0;
    }
}

// Mac 0x15c800, Win 0x67bb5a
void MoveTool::OnAttached(BrickConnectSceneControl* control)
{
    BrickConnectSceneControlTool::OnAttached(control);
    m_state = 0;
    m_axis = 3;
}

// Mac 0x15c840, Win 0x67c383
void MoveTool::OnDetached(BrickConnectSceneControl* control)
{
    Drop();
    control->UpdateGrid(0);
    control->Redraw();
    m_draw.Clear();
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x15c940 (thunk 0x15d130), Win 0x67c7d8
void MoveTool::OnMouseMove(Control* control, const Vector2& position)
{
    if ((control->GetPressedButtons() & 1) == 0 || GetSceneControl() != control)
        return;
    if (m_state == 1) {
        if (m_axis > 2)
            return;
        Vector3 direction;
        Vector3 origin;
        Real32 length;
        GetSceneControl()->GetPickRay(position, origin, direction, length);
        Vector3 anchor = m_axisPoint * m_moveTransform;
        Vector3 axis;
        ((Real32*)axis)[m_axis] = 1.0f;
        if (GetSceneControl()->GetManager()->GetWindow()->IsKeyPressed(kKeySpace)) {
            // Space: in the plane across the axis
            Vector3 normal = LEGO::Rotate(axis, m_moveTransform);
            Plane plane(normal, anchor);
            Vector3 hit;
            if (LinePlaneIntersect(origin, direction, plane, hit))
                m_moveTransform.SetTranslation(hit - LEGO::Rotate(m_axisPoint, m_moveTransform));
        } else {
            // the point of the axis nearest to the ray
            Vector3 onRay;
            Vector3 onAxis;
            Vector3 rayEnd = origin + direction;
            Vector3 axisEnd = anchor + LEGO::Rotate(axis, m_moveTransform);
            Real32 s;
            Real32 t;
            if (ClosestPointsOnLines(origin, rayEnd, anchor, axisEnd, onRay, onAxis, s, t))
                m_moveTransform.SetTranslation(onAxis - LEGO::Rotate(m_axisPoint, m_moveTransform));
        }
        Matrix3D transform;
        transform.SetTranslationMatrix(m_moveTransform.GetTranslation());
        transform.Multiply(m_pivot);
        transform.Multiply(m_grip);
        Cast(transform);
        m_drawTransform = m_transform;
        GetConnectedParts(m_connectedParts);
        BoundingBox box(0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f);
        m_dragScene->GetBoundingBox(box, true);
        BoundingBox placed = box * m_drawTransform;
        placed.m_min.y = 0.0f;
        placed.m_max.y = 0.0f;
        GetSceneControl()->UpdateGrid(&placed);
        GetSceneControl()->Redraw();
        return;
    }
    if (m_state != 0)
        return;
    // a drag of more than 3 x 1 pixels picks the selection up
    Rect drag(m_pressPosition.x, m_pressPosition.y, position.x, position.y);
    drag.Normalize();
    bool moved;
    if (drag.GetWidth() > 3.0f && drag.GetHeight() > 1.0f) {
        moved = true;
    } else {
        if (!(drag.GetWidth() > 1.0f))
            return;
        moved = drag.GetHeight() > 3.0f;
    }
    if (m_picked == 0 || !moved)
        return;
    BasicArray<RefPointer<Part> > parts(GetSceneControl()->GetSelection());
    RefPointer<Part> part(m_picked->GetPart());
    bool found = false;
    for (UInt32 i = 0; i < parts.GetSize() && !found; ++i)
        found = parts[i].Get() == part.Get();
    if (!found)
        parts.PushBack(part);
    const bool single = (GetSceneControl()->GetManager()->GetModifiers() & 2) != 0;
    PickUp(parts, m_picked, m_pickPoint, single);
    m_picked->GetTransform(m_moveTransform);
    m_moveTransform.SetTranslation(m_pickPoint);
    m_state = 1;
}

// Mac 0x15d150 (thunk 0x15d750), Win 0x67be06
void MoveTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    if (button != 0 || GetSceneControl() != control)
        return;
    if (m_state == 1) {
        // the axis handle under the mouse: cylinders of length 1 and radius 0.1 from the
        // frame's origin along x, y, z; the nearest hit wins
        Vector3 direction;
        Vector3 origin;
        Real32 length;
        GetSceneControl()->GetPickRay(position, origin, direction, length);
        Vector3 best;
        UInt32 axis = 3;
        Real32 bestDistance = FLT_MAX;
        for (UInt32 i = 0; i < 3; ++i) {
            Vector3 center;
            ((Real32*)center)[i] = 0.5f;
            Vector3 unit;
            ((Real32*)unit)[i] = 1.0f;
            Vector3 hit;
            Vector3 centerWorld = center * m_moveTransform;
            Vector3 axisWorld = LEGO::Rotate(unit, m_moveTransform);
            if (RayCylinderIntersect(origin, direction, centerWorld, axisWorld, 0.5f, 0.1f, &hit)) {
                Real32 distance = origin.GetDistanceSquared(hit);
                if (bestDistance > distance) {
                    best = hit;
                    bestDistance = distance;
                    axis = i;
                }
            }
        }
        if (axis < 3) {
            m_axis = axis;
            Matrix3D inverse;
            inverse.SetInverse(m_moveTransform);
            m_axisPoint = best * inverse;
        } else {
            m_axis = 3;
        }
    } else if (m_state == 0) {
        m_pressPosition = position;
        BasicArray<RefPointer<Part> > filter;
        BasicArray<RefPointer<Part> > extra;
        m_picked = GetSceneControl()->PickElement(position, &m_pickPoint, false, 0, true, filter, extra);
    }
}

// Mac 0x15d770 (thunk 0x15d780), Win 0x67bb7b
void MoveTool::OnMouseReleased(Control* /*control*/, const Vector2& /*position*/, UInt32 /*button*/)
{
    // NOTE: only reads the scene control (both builds).
    GetSceneControl();
}

// Mac 0x15d7a0, Win 0x67bc16
void MoveTool::OnEvent05(const Matrix3D& transform, Render* /*render*/, RenderQueue* overlayQueue,
                         RenderQueue* sceneQueue)
{
    const Matrix3D& camera = GetSceneControl()->GetCamera()->GetViewMatrix();
    Matrix3D world;
    world.Multiply(transform, m_drawTransform);
    if (m_connected) {
        m_draw.Draw(sceneQueue, camera, world);
        for (BasicArray<RefPointer<Part> >::ConstIterator it = m_connectedParts.Begin();
             it != m_connectedParts.End(); ++it) {
            Matrix3D box;
            GetPartBoxTransform(*it, box, false);
            overlayQueue->Add(m_boxMesh, box, 0.0f, 0, 1.0f);
        }
    } else {
        m_draw.DrawWithDepthPrepass(sceneQueue, camera, world);
    }
}

// Mac 0x15d8a0, Win 0x67bccd
void MoveTool::OnEvent06(Render* render)
{
    if (m_state != 1)
        return;
    // through the scene, blended
    render->EnableBlend(true);
    render->EnableDepthTest(false);
    render->EnableDepthWrite(false);
    m_axesMesh->Bind(render, 0.0f, true);
    render->SetWorldMatrix(m_moveTransform);
    render->DrawVertexBuffer();
    // then where visible
    render->EnableDepthWrite(true);
    render->EnableDepthTest(true);
    render->EnableBlend(false);
    m_axesMesh->Bind(render, 0.0f, true);
    render->SetWorldMatrix(m_moveTransform);
    render->DrawVertexBuffer();
}

// Mac 0x15d990 (thunk 0x15dac0), Win 0x67bd76
void MoveTool::OnPreferenceChanged(const String& key)
{
    if (key.EqualsNoCase(String("PLAYSOUND")))
        m_sound = LDDUtilities::GetInstance()->CreateSound(String("IA_drop.mp3"));
}

} // namespace LEGO
