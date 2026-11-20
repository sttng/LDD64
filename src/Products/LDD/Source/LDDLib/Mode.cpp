// LEGO Digital Designer — restored source
// LEGO::Mode and its key short-cut window observer (ControlManagerWindowObserver).
// TU 086 "Mode.cpp": Mac 0x196ea0-0x198dfb, Win 0x61184a-0x6121da. See Mode.h.
// Not code: the BasicArray<RefPointer<Command>> / <RefPointer<KeyboardShortCut>> / <ModeObserver*>
// instances and the construction-vtable thunks of the TU; Mac 0x198d80 = <iostream> and a
// header static.
#include "Mode.h"
#include "Application.h"
#include "ControlManager.h"
#include "TextInputControl.h"
#include "Menu.h"
#include "Window.h"

namespace LEGO {

Mode* Mode::s_currentMode = 0;                                              // Mac 0x77d978, Win 0xaef090

// =============================================================================================
// ControlManagerWindowObserver
// =============================================================================================

bool ControlManagerWindowObserver::IsKeyboardFree(TextInputControl*& textInput) const
{
    textInput = 0;
    if (m_manager->GetModalControl())
        return false;
    Control* focus = m_manager->GetKeyboardFocus();
    if (focus)
        textInput = dynamic_cast<TextInputControl*>(focus);
    return textInput == 0;
}

// Mac 0x1988b0, Win 0x611967
void ControlManagerWindowObserver::OnKeyDown(UInt32 key, UInt32 modifiers, bool /*repeat*/)
{
    if (m_manager->GetModalControl())
        return;
    TextInputControl* textInput;
    if (IsKeyboardFree(textInput)) {
        // NOTE: the end is re-read every step (a command may add short-cuts).
        for (UInt32 i = 0; i < m_shortCuts.GetSize(); ++i)
            m_shortCuts[i]->Process(key, modifiers);
    } else if (key == 1) {
        // Escape in a text field: clear it and leave it.
        textInput->SetText(String());
        m_manager->SetKeyboardFocus(0);
    }
}

// Mac 0x198860, Win 0x611894
bool ControlManagerWindowObserver::OnCloseRequest()
{
    return Application::GetInstance()->CanQuit();
}

// Mac 0x198880, Win 0x6118a0
void ControlManagerWindowObserver::OnClose()
{
    Application::GetInstance()->OnQuit();
    Application::GetInstance()->StopRunLoop(true);
}

// Mac 0x1989c0, Win 0x6118bb
bool ControlManagerWindowObserver::OnObserverQuery24()
{
    TextInputControl* textInput;
    return IsKeyboardFree(textInput);
}

// =============================================================================================
// Mode
// =============================================================================================

// Mac 0x196ec0 (C2; C1 inlined into the subclasses), Win 0x611d73
Mode::Mode(ControlManager* manager, Window* window)
    : RenderControl(manager), m_window(window), m_menu(new Menu())
{
    SetOpacity(0.0f);
    m_shortCuts = new ControlManagerWindowObserver(m_manager);
}

// Mac 0x198120 (D1), 0x198340 (D0), Win 0x612036 (sdd 0x61210e)
Mode::~Mode()
{
}

// Mac 0x196ea0, Win 0x61184a
Mode* Mode::GetCurrentMode()
{
    return s_currentMode;
}

// Mac 0x1973e0, Win 0x611887
bool Mode::IsCurrent() const
{
    return GetCurrentMode() == this;
}

// Mac 0x1973a0, Win 0x611850
bool Mode::AttachTo(Control* parent, Control* after)
{
    bool attached = SetParent(parent, after);
    if (attached)
        SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
    return attached;
}

// Mac 0x197400, Win 0x612130
void Mode::Enter()
{
    SetEnabled(true);
    SetShown(true);
    m_shortCuts->Attach(m_window);
    if (m_menu)
        m_window->SetMenu(m_menu);
    s_currentMode = this;
    NotifierT<Mode, ModeObserver>::Iterator pass(*this);
    while (ModeObserver* observer = pass.Next(NotifierT<Mode, ModeObserver>::m_observers))
        observer->OnEnter(this);
}

// Mac 0x197970, Win 0x61204c
void Mode::Exit()
{
    SetEnabled(false);
    SetShown(false);
    m_shortCuts->Detach();
    {
        NotifierT<Mode, ModeObserver>::Iterator pass(*this);
        while (ModeObserver* observer = pass.Next(NotifierT<Mode, ModeObserver>::m_observers))
            observer->OnExit(this);
    }
    s_currentMode = 0;
}

// Mac 0x197ca0
void Mode::SetMenu(Menu* menu)
{
    m_menu = menu;
    if (GetCurrentMode() == this)
        m_window->SetMenu(m_menu);
}

// Mac 0x1985b0 (thunks 0x198640, 0x1986c0, 0x198740, 0x1987c0), Win 0x6120c7 (Notifier part)
void Mode::OnFinalRelease()
{
    // NotifierT<Mode, ModeObserver>::OnFinalRelease inlined, then RenderControl's.
    while (!NotifierT<Mode, ModeObserver>::m_observers.IsEmpty())
        NotifierT<Mode, ModeObserver>::DetachObserver(NotifierT<Mode, ModeObserver>::m_observers.Back(), this);
    Object::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
