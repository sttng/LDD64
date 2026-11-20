// LEGO Digital Designer — restored source
// LEGO::CommandButtonControl — a ButtonControl bound to a Command: it observes the command
//   (CommandObserver), keeps a reference to it while attached, follows its enable state and
//   performs it on a left click. Base of the group tree's AddSubGroupButtonControl,
//   AddPartsToGroupButtonControl and MovePartsToParentGroupButtonControl (GroupTreeViewControl.h).
//
// Header-inline in the original: no constructor of its own survives (inlined into the
// subclasses, Mac 0x27ec10 ff.); the virtuals are weak code emitted in LDDUtilities (TU 063,
// Mac 0x14ecc0-0x14f5af: D1 0x14ecc0, D0 0x14ed20, OnFinalRelease 0x14ed90, OnMouseClicked
// 0x14edf0, OnEnableChanged 0x14ee30, OnAttached 0x14ee40, OnDetached 0x14eeb0, thunks
// 0x14eef0-0x14f4c0, destructor body 0x14f530); Win sdd 0x638c89, OnMouseClicked 0x638350.
// RTTI vmi {ButtonControl, CommandObserver (+0x218)}. Mac layout: ButtonControl +0x000..+0x218,
//   CommandObserver +0x218 (subject +0x21c), m_command +0x220, Object +0x224 (sizeof 0x22c).
// Vtable Mac 0x77238c (51 entries: ButtonControl's 48, OnFinalRelease 2, OnMouseClicked 21;
//   Mac-only overrider entries 48-50 = OnEnableChanged, OnAttached, OnDetached of the
//   CommandObserver base); CommandObserver-in 0x772464 + 0xa8.
// Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ButtonControl.h"
#include "Command.h"

namespace LEGO {

class CommandButtonControl : public ButtonControl, public CommandObserver {
public:
    explicit CommandButtonControl(ControlManager* manager) : ButtonControl(manager), m_command() {}
    virtual ~CommandButtonControl() {}                                      // Mac 0x14ecc0/0x14ed20, Win sdd 0x638c89

    Command* GetCommand() const { return m_command.Get(); }

    // ---- Control override ----
    // Left button: the observed command is performed (after the Control notification).
    // Mac 0x14edf0, Win 0x638350
    virtual void OnMouseClicked(const Vector2& position, UInt32 button)    // 21/18
    {
        ButtonControl::OnMouseClicked(position, button);
        if (button == 0) {
            Command* command = CommandObserver::GetSubject();
            if (command)
                command->Do();
        }
    }

    // ---- CommandObserver (Mac-only primary entries 48-50) ----
    // Enabled with the command. Mac 0x14ee30 (thunk 0x14f3a0)
    virtual void OnEnableChanged(bool enabled) { SetEnabled(enabled); }
    // Keeps the command, enabled as it is. Mac 0x14ee40 (thunk 0x14f300)
    virtual void OnAttached(Command* command)
    {
        m_command = command;
        SetEnabled(command->GetEnable());
    }
    // Drops the command. Mac 0x14eeb0 (thunk 0x14f370)
    virtual void OnDetached(Command* /*command*/) { m_command = 0; }

protected:
    // (the text layout / texture observer overloads stay visible)
    using Observer<TextLayout, TextLayoutObserver>::OnAttached;
    using Observer<TextLayout, TextLayoutObserver>::OnDetached;
    using Observer<Texture, TextureObserver>::OnAttached;
    using Observer<Texture, TextureObserver>::OnDetached;

    // Leaves the command, then RenderControl's. Mac 0x14ed90 (thunks 0x14efe0 …)
    virtual void OnFinalRelease()
    {
        CommandObserver::Detach();
        ButtonControl::OnFinalRelease();
    }

    RefPointer<Command> m_command;   // Mac +0x220 (set while attached)
                                     // Object Mac +0x224
};

} // namespace LEGO
