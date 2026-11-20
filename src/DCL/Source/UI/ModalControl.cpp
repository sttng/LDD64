// LEGO Digital Designer — restored source
// TU 125 "ModalControl.cpp" (Mac 0x1db100-0x1dc2bb, Win 0x64fa02-0x64fe0d).
#include "ModalControl.h"
#include "LDDWindowControl.h"
#include "Key.h"
#include "Color.h"
#include "LEGO/String.h"

namespace LEGO {

// Mac 0x1db100 (C2, VTT)
ModalControl::ModalControl(ControlManager* manager)
    : RenderControl(manager)
{
}

// Mac 0x1dbaa0 (D1), 0x1db920 (D2 body), 0x1dbcc0 (D0) and thunks 0x1dbb00-0x1dbeb0;
// Win sdd 0x64fb7e
ModalControl::~ModalControl()
{
}

// Mac 0x1db1b0, Win 0x64fba0
bool ModalControl::Initialize()
{
    if (!RenderControl::Initialize())
        return false;
    SetColor(Color(0.5f, 0.5f, 0.5f, 0.4f));
    SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
    SetEnabled(true);
    SetShown(false);
    SetActive(true);
    m_window = new LDDWindowControl(m_manager);
    if (!m_window->Create(false, false)) {
        m_window = 0;
        return false;
    }
    m_window->Setup(this, 0, 7, Rect(Vector2(300.0f, 200.0f)), 0, false);
    m_window->SetResizable(false);
    RefPointer<RenderControl> content = new RenderControl(m_manager);
    if (!content->Initialize())
        return false;
    content->SetTransparent(true);
    m_window->SetContent(content);
    m_window->SetAxisAlignment(0, 2, 2, 0.0f, 0);
    m_window->SetAxisAlignment(1, 2, 2, 0.0f, 0);
    return true;
}

// Mac 0x1db4d0, Win 0x64fb23
bool ModalControl::SetTitle(const String& title)
{
    if (m_window.Get())
        return m_window->SetTitle(title);
    return false;
}

// Mac 0x1db900
Control* ModalControl::GetContent() const
{
    return m_window->GetContent();
}

// Mac 0x1db500, Win 0x64fdaf
void ModalControl::ShowModal()
{
    m_previousFocus = m_manager->GetKeyboardFocus();
    SetParent(m_manager->GetLayer(1), 0);
    SetShown(true);
    ControlManagerObserver::Attach(m_manager);
    m_manager->SetModalControl(true, this);
}

// Mac 0x1db800, Win 0x64fd58
void ModalControl::EndModal()
{
    m_manager->SetModalControl(false, this);
    ControlManagerObserver::Detach();
    SetParent(0, 0);
    if (m_previousFocus.Get()) {
        m_previousFocus->SetKeyboardFocus(true);
        m_previousFocus = 0;
    }
}

// Mac 0x1dc200, Win 0x64f950
void ModalControl::OnAccept()
{
    EndModal();
}

// Mac 0x1dc220, Win 0x64f950 (ICF with OnAccept)
void ModalControl::OnCancel()
{
    EndModal();
}

// Mac 0x1dc160, Win 0x64fb44
void ModalControl::OnKeyPressed(UInt32 key, UInt32, bool)
{
    if (m_defaultButton.Get() && (key == kKeySpace || key == kKeyReturn)) {
        OnAccept();
        return;
    }
    if (key == kKeyEscape && m_cancelButton.Get())
        OnCancel();
}

// Mac 0x1dc1b0, Win 0x5fd347 (thunks to the same function)
void ModalControl::OnManagerKeyPressed(UInt32 key, UInt32 modifiers, bool repeat)
{
    ModalControl::OnKeyPressed(key, modifiers, repeat);
}

// Mac 0x1dbf30
void ModalControl::OnFinalRelease()
{
    ControlManagerObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
