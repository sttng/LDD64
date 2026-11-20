// LEGO Digital Designer — restored source (header; header-inline class)
// LEGO::Command — a UI action (menu item, button, key): Do() performs it; the enable (and toggle)
// state is announced to CommandObservers. LEGO::CommandObserver = Observer<Command, ...>.
// No home TU: the virtuals are weak, emitted wherever a subclass is defined (Mac
// 0x372e20-0x373100 with SaveTraceStringCommand, TU 385; Win 0x423a70-0x423b40 + the empty Do
// 0x459790). Directory UI/ is a guess (DCL range on Win); LDD's commands live in
// Products/LDD/Source/LDDLib/Commands/ (__FILE__ of ExportBOMCommand.cpp).
//
// Mac layout (ObjC type encoding of -[LEGOMenuItemWrapper
//   initWithTitle:keyEquivalent:KeyEquivalentModifierMask:command:window:], objc.json:
//   {Command=^^?{BasicArray<CommandObserver*>}{BasicArray<…Iterator*>}BBB^^?i}):
//   NotifierT +0x00..+0x24, enabled +0x24, toggled +0x25, toggleable +0x26, Object +0x28.
//   Win: vfptr, vbptr +4, arrays +8/+0x18, enabled +0x28 (GetEnable Win 0x423b30), Object +0x30.
// Vtables: Mac 0x737dbc {D1 0x1931e0, D0 0x193250, OnFinalRelease 0x1414d0 (NotifierT), Do
//   0x372e20, SetEnable 0x372e30, GetEnable 0x3730b0, UpdateEnable 0x3730c0};
//   Win 0x9ca1bc {Do 0x459790, SetEnable 0x423a70, GetEnable 0x423b30, UpdateEnable 0x423b40},
//   Object-in 0x9ca1a8 {sdd 0x44b240, OnFinalRelease 0x44b230, …}.
// Do / SetEnable / GetEnable / UpdateEnable are the Lua binding's names (LuaCommand, TU 306); the
// other names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Notifier.h"

namespace LEGO {

class Command;

// Mac vtable 0x74016c {D1 0x14e9a0, D0 0x14e9d0, OnFinalRelease, OnAttached, OnDetached,
// OnEnableChanged 0x14ea90, OnToggleChanged 0x14eaa0 (both empty)}; Win 0x9cfef0 {OnAttached,
// OnDetached, 0x40be80, 0x40be80 (ret 4)}.
class CommandObserver : public Observer<Command, CommandObserver> {
public:
    CommandObserver() {}
    virtual ~CommandObserver() {}                                            // Mac 0x14e9a0/0x14e9d0

    virtual void OnEnableChanged(bool /*enabled*/) {}                        // Mac 5, Win 2 (name inferred)
    virtual void OnToggleChanged(bool /*toggled*/) {}                        // Mac 6, Win 3 (name inferred)
};

class Command : public Notifier<CommandObserver> {
public:
    // (inlined in the subclass constructors; argument order of Lua Command:New)
    explicit Command(bool enabled = true, bool toggleable = false)
        : m_enabled(enabled), m_toggled(false), m_toggleable(toggleable) {}
    virtual ~Command() {}                                                    // Mac 0x1931e0/0x193250

    // Performs the command.
    virtual void Do() {}                                                     // Mac 3 0x372e20, Win 0 0x459790

    // Changes the enable state and tells the observers (only on a change).
    virtual void SetEnable(bool enable)                                      // Mac 4 0x372e30, Win 1 0x423a70
    {
        if (enable == m_enabled)
            return;
        m_enabled = enable;
        Iterator pass(*this);
        while (CommandObserver* observer = pass.Next(m_observers))
            observer->OnEnableChanged(m_enabled);
    }
    virtual bool GetEnable() const { return m_enabled; }                     // Mac 5 0x3730b0, Win 2 0x423b30
    // Re-evaluates the enable state; the default enables the command.
    virtual void UpdateEnable() { SetEnable(true); }                         // Mac 6 0x3730c0, Win 3 0x423b40

    bool IsToggleable() const { return m_toggleable; }                       // Mac 0x3730e0 (name inferred)
    bool IsToggled() const { return m_toggled; }                             // Mac 0x3730f0 (name inferred)
    // Changes the toggle state and tells the observers (only on a change). (name inferred)
    void SetToggled(bool toggled)                                            // Mac 0x373100
    {
        if (toggled == m_toggled)
            return;
        m_toggled = toggled;
        Iterator pass(*this);
        while (CommandObserver* observer = pass.Next(m_observers))
            observer->OnToggleChanged(m_toggled);
    }

protected:
    bool m_enabled;      // Mac +0x24, Win +0x28
    bool m_toggled;      // Mac +0x25
    bool m_toggleable;   // Mac +0x26
};

} // namespace LEGO
