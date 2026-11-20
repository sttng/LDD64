// LEGO Digital Designer — restored source
// LEGO::PickTool — see PickTool.h.
// TU 068 "PickTool.cpp": Mac 0x15e720-0x15fa2b, Win 0x6607bd-0x6611d9. Not code: the thunks
// (0x15ed70 …), the BasicMap<Part*, UInt32> instance (0x15ed40, 0x15f650-0x15f8e0), Mac 0x15f9b0
// = <iostream> + a header static. The base-object constructor (Mac 0x161a80) is emitted in
// SelectionTool.cpp.
#include "PickTool.h"
#include "BrickConnectSceneControl.h"
#include "ControlManager.h"
#include "Control.h"
#include "PrimitiveElement.h"
#include "Part.h"
#include "Color.h"

namespace LEGO {

namespace {

// The control is redrawn unless its manager is drawing (inline everywhere).
void RedrawControl(Control* control)
{
    if (!control->GetManager()->IsUpdateLocked())
        control->Invalidate(false);
}

} // namespace

// Mac 0x161a80 (C2, emitted in SelectionTool.cpp), MaterialTool Mac 0x1a7fa0 (inline)
PickTool::PickTool(bool cursorFlag)
    : BrickConnectSceneControlTool(cursorFlag),
      m_rubberBand(0.0f, 0.0f, 0.0f, 0.0f),
      m_rubberBandActive(false),
      m_dragging(false),
      m_pressed(false),
      m_clickFlag(false),
      m_picked(0)
{
}

// Mac 0x15f330 / 0x15f470 (body 0x15f0d0), Win sdd 0x6611d9
PickTool::~PickTool()
{
}

// Mac 0x15f5e0 … 0x15f640 (all empty), Win 0x65a12a / 0x5c8021 (ICF)
void PickTool::OnPickPressed(bool) {}
void PickTool::OnPickDragged() {}
void PickTool::OnPickClicked(bool) {}
void PickTool::OnRubberBandStarted() {}
void PickTool::OnRubberBandChanged() {}
void PickTool::OnRubberBandFinished() {}
void PickTool::OnPickReleased() {}

// Mac 0x15e720, Win 0x6609ed
void PickTool::OnDetached(BrickConnectSceneControl* control)
{
    m_picked = 0;
    m_pickResult.Clear();
    m_rubberBandParts.Clear();
    if (m_pressed) {
        OnPickReleased();
        m_pressed = false;
    }
    if (m_rubberBandActive)
        m_sceneControl->ReleaseMouseCapture();
    BrickConnectSceneControlTool::OnDetached(control);
}

// Mac 0x15e7d0 (thunk 0x15ed70), Win 0x660d9c
void PickTool::OnMouseMove(Control* control, const Vector2& position)
{
    if (GetSceneControl() != control)
        return;
    if (!(control->GetPressedButtons() & 1))
        return;

    m_rubberBand.Set(Rect(m_pressPosition.x, m_pressPosition.y, position.x, position.y));
    m_rubberBand.Normalize();
    const bool moved = (m_rubberBand.GetWidth() > 3.0f && m_rubberBand.GetHeight() > 1.0f) ||
                       (m_rubberBand.GetWidth() > 1.0f && m_rubberBand.GetHeight() > 3.0f);

    if (m_picked != 0) {
        if (!m_dragging && moved)
            m_dragging = true;
        else if (!m_dragging)
            return;
        OnPickDragged();
        return;
    }

    if (!m_rubberBandActive && moved) {
        if (m_sceneControl->CaptureMouse()) {
            m_rubberBandActive = true;
            OnRubberBandStarted();
            SetActive(true);
        }
    }
    if (!m_rubberBandActive)
        return;

    m_pickResult.Clear();
    m_rubberBandParts.Clear();
    if (moved) {
        Frustum frustum;
        GetSceneControl()->GetPickFrustum(m_rubberBand, frustum);
        GetSceneControl()->PickFrustum(frustum, m_pickResult);
        // A part with one element is in the band when its element was picked, a part with
        // several when all of them were (counted per part; NOTE: these come out in pointer order).
        BasicMap<Part*, UInt32> counts;
        for (Scene::PickResult::Iterator it = m_pickResult.Begin(); it != m_pickResult.End(); ++it) {
            Part* part = it->m_element->GetPart();
            if (part->GetElementCount() == 1)
                m_rubberBandParts.PushBack(RefPointer<Part>(part));
            else
                ++counts[part];
        }
        for (BasicMap<Part*, UInt32>::Iterator it = counts.Begin(); it != counts.End(); ++it) {
            if (it->first->GetElementCount() == it->second)
                m_rubberBandParts.PushBack(RefPointer<Part>(it->first));
        }
        RedrawControl(GetSceneControl());
    }
    OnRubberBandChanged();
}

// Mac 0x15ed90 (thunk 0x15ee40), Win 0x660742
void PickTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    if (button != 0 || GetSceneControl() != control)
        return;
    // (Mac: one chained Vector2 assignment: press position = band min = band max)
    m_pressPosition = position;
    m_rubberBand.Set(Rect(position.x, position.y, position.x, position.y));
    m_picked = GetSceneControl()->PickElement(position, &m_pickPoint, false, 0, true,
                                              m_filterParts, m_extraParts);
    m_pressed = true;
    m_clickFlag = false;
    OnPickPressed(false);
}

// Mac 0x15ee60 (thunk 0x15ef20), Win 0x660885
void PickTool::OnMouseReleased(Control* control, const Vector2& position, UInt32 button)
{
    if (button != 0 || GetSceneControl() != control)
        return;
    if (m_picked == 0) {
        if (m_rubberBandActive)
            m_sceneControl->ReleaseMouseCapture();
    } else {
        PrimitiveElement* element = GetSceneControl()->PickElement(position, &m_pickPoint, false, 0,
                                                                   true, m_filterParts, m_extraParts);
        if (element == m_picked)
            OnPickClicked(m_clickFlag);
        m_picked = 0;
        m_dragging = false;
    }
    OnPickReleased();
    m_pressed = false;
}

// Mac 0x15ef40 (thunk 0x15ef80), Win 0x660905
void PickTool::OnMouseCaptureLost(Control* control)
{
    if (m_rubberBandActive && control == m_sceneControl.Get()) {
        m_rubberBandActive = false;
        OnRubberBandFinished();
        SetActive(false);
    }
}

// Mac 0x15efc0, Win 0x6607bd
void PickTool::OnEvent07(Render*, ControlDrawArgs& args)
{
    if (!m_rubberBandActive)
        return;
    const Vector2 min = m_rubberBand.GetMin();
    const Vector2 max = m_rubberBand.GetMax();
    args.DrawLine(min, m_rubberBand.GetTopRight(), Color::Black(), false);
    args.DrawLine(m_rubberBand.GetTopRight(), max, Color::Black(), false);
    args.DrawLine(max, m_rubberBand.GetBottomLeft(), Color::Black(), false);
    args.DrawLine(m_rubberBand.GetBottomLeft(), min, Color::Black(), false);
}

// Mac 0x15f0b0, Win 0x66087a
void PickTool::OnEvent19()
{
    if (m_picked != 0)
        m_picked = 0;
}

} // namespace LEGO
