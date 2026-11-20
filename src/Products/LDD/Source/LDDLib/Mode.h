// LEGO Digital Designer — restored source
// LEGO::Mode — one screen of LDD (start, build/edit, view, building instructions …): a
//   RenderControl that owns the window's menu, its controls and commands and the keyboard
//   short-cuts; Enter / Exit switch it on and off and tell the ModeObservers. One mode is the
//   current one (GetCurrentMode).
// LEGO::ModeObserver — Observer<Mode, ModeObserver>.
// LEGO::KeyboardShortCut — one key short-cut of a mode: a CommandObserver of its command plus
//   the key and the modifiers (header-inline; its vtable is emitted in TU 367 BGAutomaticMode).
// LEGO::ControlManagerWindowObserver — the WindowObserver a mode attaches to its window while
//   it is current: runs the key short-cuts, asks the application before the window closes.
//
// TU 086 "Mode.cpp" (Mac 0x196ea0-0x198dfb; Win 0x61184a-0x6121da). The LDDLib/ directory is a
// guess from the Win LDD range.
//
// Mac layout (constructor 0x196ec0, VTT form): RenderControl +0x000..+0x1ec,
//   Notifier<ModeObserver> +0x1ec (observers +0x1f0, iterations +0x200), members below,
//   Object +0x24c (destructor 0x198120). Win: Notifier<ModeObserver> +0x208 (observers +0x20c,
//   iterations +0x21c), window +0x22c, menu +0x230, Object +0x26c (vtable 0x9cf008).
// Virtual slots: Mac 0x7165fc / 0x714c4c (49 entries), Win 0x9cf03c (44 entries = Mac 3..48 minus
// the Mac-only 43/44 of RenderControl): new Enter 45/40, Exit 46/41, CanEnter 47/42,
// CanExit 48/43; overrides OnFinalRelease (Mac 2, 0x1985b0: detaches every ModeObserver).
// OnEnter / OnExit are the Lua callbacks of LuaMode (TU 380); other names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "Notifier.h"
#include "RenderControl.h"
#include "Window.h"
#include "Command.h"

namespace LEGO {

class Mode;
class Menu;
class ControlManager;
class TextInputControl;

// Mac vtable 0x75033c {D1 0x2a2300, D0 0x2a2330, OnFinalRelease 0x2a2270, OnAttached 0x2a2380,
// OnDetached 0x2a2390, OnEnter 0x2a23a0, OnExit 0x2a23b0 (empty)}; Win 0x9d30b0 {OnAttached,
// OnDetached, OnEnter, OnExit: 0x65a12a (ret 4)}.
class ModeObserver : public Observer<Mode, ModeObserver> {
public:
    ModeObserver() {}
    virtual ~ModeObserver() {}                                               // Mac 0x2a2300/0x2a2330

    virtual void OnEnter(Mode* /*mode*/) {}                                  // Mac 5, Win 2 (Lua name)
    virtual void OnExit(Mode* /*mode*/) {}                                   // Mac 6, Win 3 (Lua name)
};

// ===========================================================================================
// KeyboardShortCut — Mac vtable 0x77815c (CommandObserver's slots, no overrides; D1 0x3595a0,
// D0 0x3595d0), Win 0x9cff18. Mac layout: vptr, Command* subject +0x04, key +0x08, modifiers
// +0x0c, Object +0x10 (sizeof 0x18). Win: vbptr +4, subject +8, key +0xc, modifiers +0x10.
// (class name RTTI; member names inferred)
// ===========================================================================================
class KeyboardShortCut : public CommandObserver {
public:
    KeyboardShortCut(UInt32 key, UInt32 modifiers) : m_key(key), m_modifiers(modifiers) {}
    virtual ~KeyboardShortCut() {}                                           // Mac 0x3595a0/0x3595d0, Win sdd 0x624b12

    UInt32 GetKey() const { return m_key; }
    UInt32 GetModifiers() const { return m_modifiers; }

    // Runs the command when it is enabled and `key` / `modifiers` match. Win 0x611818 (inlined
    // in the Mac OnKeyDown 0x1988b0). (name inferred)
    void Process(UInt32 key, UInt32 modifiers)
    {
        Command* command = GetSubject();
        if (command && command->GetEnable() && m_key == key && m_modifiers == modifiers)
            command->Do();
    }

private:
    UInt32 m_key;         // Mac +0x08
    UInt32 m_modifiers;   // Mac +0x0c (KeyModifier bits)
};

// ===========================================================================================
// ControlManagerWindowObserver — Mac vtable 0x7167d0 (26 entries: WindowObserver's, overriding
// OnKeyDown 5 = 0x1988b0, OnCloseRequest 15 = 0x198860, OnClose 16 = 0x198880, slot 24 =
// 0x1989c0; D1 0x198a20, D0 0x198b10); Win 0x9cef94 (OnKeyDown 0x611967, 13 = 0x611894,
// 14 = 0x6118a0, 22 = 0x6118bb), Object-in 0x9cef74 (sdd 0x611d54).
// Mac layout: WindowInput vptr +0, Observer<Window> +0x04 (subject +0x08), manager +0x0c,
// short-cuts +0x10, Object +0x20 (sizeof 0x28). Win: manager +0x14, short-cuts +0x18.
// ===========================================================================================
class ControlManagerWindowObserver : public WindowObserver {
public:
    explicit ControlManagerWindowObserver(ControlManager* manager) : m_manager(manager) {}
    virtual ~ControlManagerWindowObserver() {}                                // Mac 0x198a20/0x198b10, Win sdd 0x611d54

    // Adds a short-cut that runs `command` (observed by the short-cut). Weak instance in TU 367
    // (BGAutomaticMode), Mac 0x3555a0. (name inferred)
    void AddKeyboardShortCut(Command* command, UInt32 key, UInt32 modifiers)
    {
        RefPointer<KeyboardShortCut> shortCut = new KeyboardShortCut(key, modifiers);
        if (command)
            shortCut->Attach(command);
        m_shortCuts.PushBack(shortCut);
    }
    const BasicArray<RefPointer<KeyboardShortCut> >& GetKeyboardShortCuts() const { return m_shortCuts; }

    // Short-cuts while no modal control is up and no text input control has the keyboard
    // focus; with a text input focused, key 1 (escape) clears it and drops the focus.
    virtual void OnKeyDown(UInt32 key, UInt32 modifiers, bool repeat);        // 5  Mac 0x1988b0, Win 0x611967
    // Application::CanQuit.
    virtual bool OnCloseRequest();                                           // 15 Mac 0x198860, Win 0x611894
    // Application::OnQuit, then StopRunLoop(true).
    virtual void OnClose();                                                  // 16 Mac 0x198880, Win 0x6118a0
    // False while a modal control is up or a text input control has the keyboard focus.
    virtual bool OnObserverQuery24();                                        // 24 Mac 0x1989c0, Win 0x6118bb

private:
    // No modal control, and the keyboard focus is not a TextInputControl (returned through
    // `textInput` when it is). (restoration helper; inlined in the three handlers)
    bool IsKeyboardFree(TextInputControl*& textInput) const;

    ControlManager*                        m_manager;    // Mac +0x0c, Win +0x14
    BasicArray<RefPointer<KeyboardShortCut> > m_shortCuts; // Mac +0x10, Win +0x18
};

class Mode : public RenderControl, public Notifier<ModeObserver> {
public:
    // Mac 0x196ec0 (C2 with VTT): keeps `window`, creates its Menu and the short-cut observer;
    // opacity 0 (only the children are drawn).
    Mode(ControlManager* manager, Window* window);
    virtual ~Mode();                                                        // Mac 0x198120/0x198340, Win sdd 0x61210e

    // The current mode (null when none). Mac 0x196ea0, Win 0x61184a (name inferred)
    static Mode* GetCurrentMode();
    // This is the current mode. Mac 0x1973e0, Win 0x611887 (name inferred)
    bool IsCurrent() const;

    // Enables and shows the mode, attaches the short-cut observer to the window, installs the
    // menu, becomes the current mode and notifies OnEnter. (names inferred)
    virtual void Enter();                                                   // 45/40 Mac 0x197400, Win 0x612130
    // The reverse; notifies OnExit, then no mode is current.
    virtual void Exit();                                                    // 46/41 Mac 0x197970, Win 0x61204c
    virtual bool CanEnter() { return true; }                                // 47/42 Mac 0x197c80, Win 0x5fbd39
    virtual bool CanExit() { return true; }                                 // 48/43 Mac 0x197c90, Win 0x5fbd39

    // Below `parent` behind `after` (Control::SetParent), then stretched over the parent
    // (margins 0). False when SetParent failed. Mac 0x1973a0, Win 0x611850 (name inferred)
    bool AttachTo(Control* parent, Control* after);

    Window* GetWindow() const { return m_window; }
    Menu*   GetMenu() const { return m_menu; }
    // A new menu; installed in the window at once when the mode is current (LuaMode "Menu").
    // Mac 0x197ca0 (name inferred)
    void SetMenu(Menu* menu);
    // The key short-cuts of the mode (BGAutomaticMode adds its own). (accessor, name inferred)
    ControlManagerWindowObserver* GetKeyboardShortCuts() const { return m_shortCuts; }

protected:
    virtual void OnFinalRelease();                                          // 2/– Mac 0x1985b0

    // ---- data (Mac offsets) ----
    RefPointer<Window>                       m_window;          // +0x210 / Win +0x22c
    RefPointer<Menu>                         m_menu;            // +0x214 / Win +0x230 (new Menu, 0x1c bytes)
    BasicArray<RefPointer<Control> >         m_controls;        // +0x218 (inferred)
    BasicArray<RefPointer<Command> >         m_commands;        // +0x228 (inferred)
    // Objects the mode keeps alive (released through a non-virtual Object base); EditMode
    // keeps its PickupBricksObserver here.
    BasicArray<RefPointer<Object> >          m_array238;        // +0x238
    RefPointer<ControlManagerWindowObserver> m_shortCuts;       // +0x248 (0x28-byte observer)
                                                                // Object +0x24c

private:
    static Mode* s_currentMode;                                 // Mac 0x77d978, Win 0xaef090
};

} // namespace LEGO
