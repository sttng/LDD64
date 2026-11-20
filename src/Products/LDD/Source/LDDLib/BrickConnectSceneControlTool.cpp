// LEGO Digital Designer — restored source
// LEGO::BrickConnectSceneControlTool — base class of the scene control's tools. See the header.
// TU 094 "BrickConnectSceneControlTool.cpp": Mac 0x1a9040-0x1a996b, Win 0x6611fb-0x66148d
// (+ 0x67bb7b). Not code: the construction-vtable thunks, Mac 0x1a98f0 = <iostream> + a header
// static.
#include "BrickConnectSceneControlTool.h"
#include "BrickConnectSceneControl.h"
#include "ControlManager.h"

namespace LEGO {

namespace {

// The scene control has its manager's keyboard focus. (inlined)
bool HasKeyboardFocus(BrickConnectSceneControl* control)
{
    return control->GetManager()->GetKeyboardFocus() == control;
}

} // namespace

// Mac 0x1a90f0 (C1), 0x1a9040 (C2), Win 0x6612ae
BrickConnectSceneControlTool::BrickConnectSceneControlTool(bool cursorFlag)
    : m_active(false), m_focused(false), m_cursor(1), m_cursorFlag(cursorFlag), m_sceneControl(0)
{
}

// Mac 0x1a92f0 (D1, body 0x1a9220), 0x1a9410 (D0), Win 0x6613d8 (sdd 0x6613eb / 0x66148d)
BrickConnectSceneControlTool::~BrickConnectSceneControlTool()
{
}

// Mac 0x127fb0, Win 0x627d5b (weak, emitted elsewhere)
BrickConnectSceneControlTool* BrickConnectSceneControlTool::GetTool()
{
    return this;
}

// Mac 0x127fc0, Win 0x65a12a (weak, emitted elsewhere)
void BrickConnectSceneControlTool::OnToolEvent24(const BasicArray<RefPointer<Part> >&)
{
}

// Mac 0x1a9170
void BrickConnectSceneControlTool::SetActive(bool active)
{
    if (active == m_active)
        return;
    m_active = active;
    if (m_sceneControl && HasKeyboardFocus(m_sceneControl)) {
        m_focused = active;
        m_sceneControl->AddPressedButton(active);
    }
}

// Mac 0x1a91b0, Win 0x6611fb
BrickConnectSceneControl* BrickConnectSceneControlTool::GetSceneControl() const
{
    return m_sceneControl;
}

// Mac 0x1a91c0
void BrickConnectSceneControlTool::SetCursor(UInt32 cursor)
{
    m_cursor = cursor;
    if (m_sceneControl)
        m_sceneControl->SetToolCursor(cursor, m_cursorFlag);
}

// Mac 0x1a91f0, Win 0x661294
void BrickConnectSceneControlTool::ApplyCursor(UInt32 cursor)
{
    if (m_sceneControl)
        m_sceneControl->SetToolCursor(cursor, m_cursorFlag);
}

// Mac 0x1a9570 (thunks 0x1a9600, 0x1a9690)
void BrickConnectSceneControlTool::OnFinalRelease()
{
    // ObserverN<Control, …>::OnFinalRelease, then Observer<BrickConnectSceneControl, …>'s
    // (which ends in OnDetached(control)). NOTE: the scene control reference stays until the
    // destructor.
    ControlObserver::OnFinalRelease();
    BrickConnectSceneControlObserver::OnFinalRelease();
}

// Mac 0x1a9730, Win 0x66140a
void BrickConnectSceneControlTool::OnAttached(BrickConnectSceneControl* control)
{
    m_sceneControl = control;
    ControlObserver::Attach(control);
    if (m_active && HasKeyboardFocus(control)) {
        m_focused = true;
        control->AddPressedButton(true);
    }
    ApplyCursor(m_cursor);
}

// Mac 0x1a97d0, Win 0x661459
void BrickConnectSceneControlTool::OnDetached(BrickConnectSceneControl* control)
{
    if (m_focused) {
        m_focused = false;
        control->AddPressedButton(false);
    }
    ControlObserver::Detach(control);
}

// Mac 0x1a9810 (thunk 0x1a9850), Win 0x6611ff
void BrickConnectSceneControlTool::OnKeyboardFocusGained(Control* control, bool, Control*, UInt32)
{
    if (control == m_sceneControl.Get() && m_active && !m_focused) {
        m_focused = true;
        m_sceneControl->AddPressedButton(true);
    }
}

// Mac 0x1a9890 (thunk 0x1a98c0), Win 0x661230
void BrickConnectSceneControlTool::OnKeyboardFocusLost(Control* control, Control*)
{
    if (control == m_sceneControl.Get() && m_focused) {
        m_focused = false;
        m_sceneControl->AddPressedButton(false);
    }
}

} // namespace LEGO
