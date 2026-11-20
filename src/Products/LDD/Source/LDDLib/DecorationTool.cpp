// LEGO Digital Designer — restored source
// LEGO::DecorationTool — see DecorationTool.h.
// TU 195 "DecorationTool.cpp": Mac 0x2476b0-0x247d0b, Win 0x67d554-0x67da07. Not code: the thunks,
// Mac 0x247c90 = <iostream> + a header static.
#include "DecorationTool.h"
#include "BrickConnectSceneControl.h"
#include "RenderControlManager.h"
#include "ControlManager.h"

namespace LEGO {

// Mac 0x2476b0, Win 0x67d8ba
DecorationTool::DecorationTool()
    : SurfacePickTool(false), m_control(0)
{
    SetCursor(0x70);
}

// Mac 0x2478c0 / 0x2479e0, Win sdd 0x67da07
DecorationTool::~DecorationTool()
{
}

// Mac 0x2477e0, Win 0x67d5b6
bool DecorationTool::Create(BrickConnectSceneControl* sceneControl, ControlManager* manager,
                            BrickConnectSceneManager* sceneManager)
{
    if (!SurfacePickTool::Create(static_cast<RenderControlManager*>(manager)->GetRender(), sceneManager))
        return false;
    m_control = new DecorationToolControl(manager, sceneManager, 10, 8);
    return m_control->Create(sceneControl, 0);
}

// Mac 0x247ae0, Win 0x67d636
void DecorationTool::OnPickClicked(bool flag)
{
    if (flag || m_pressedElement == 0)
        return;
    m_control->SetPickedElement(m_pressedElement, m_pressedSurface);
    m_control->SetPosition(Vector2(m_pressPosition.x + -10.0f, m_pressPosition.y + -10.0f));
    m_control->MoveInsideParent();
}

// Mac 0x247b70, Win 0x67d69e
bool DecorationTool::IsPickable(PrimitiveElement* element, UInt32 surface)
{
    return m_control->HasDecorations(element, surface);
}

// Mac 0x247ba0, Win 0x67d554
void DecorationTool::OnAttached(BrickConnectSceneControl* control)
{
    SurfacePickTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
}

// Mac 0x247bd0, Win 0x67d6a9
void DecorationTool::OnDetached(BrickConnectSceneControl* control)
{
    m_control->SetShown(false);
    SurfacePickTool::OnDetached(control);
}

// Mac 0x247bf0 (thunk 0x247c20), Win 0x67d572
void DecorationTool::OnMousePressed(Control* control, const Vector2& position, UInt32 button)
{
    m_pressPosition = position;
    SurfacePickTool::OnMousePressed(control, position, button);
}

// Mac 0x247c60, Win 0x67d6c5
void DecorationTool::OnEvent19()
{
    SurfacePickTool::OnEvent19();
    m_control->SetShown(false);
}

} // namespace LEGO
