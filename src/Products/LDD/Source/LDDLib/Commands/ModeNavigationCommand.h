// LEGO Digital Designer — restored source
// LEGO::ModeNavigationCommand — switches to its mode (exits the current one when it may, enters
//   the target when it may); enabled while the target is not the current mode (it observes it).
//
// TU 293 "ModeNavigationCommand.cpp" (Mac 0x2a17e0-0x2a265b; Win 0x631cdc-0x632049) with the
// ModeObserver / Observer<Mode, ModeObserver> instances (Mac 0x2a2270-0x2a2580).
// Mac layout (constructor 0x2a1bb0): Command +0x00..+0x28, ModeObserver +0x28 (subject = the
// target mode +0x2c), Object +0x30 (sizeof 0x38). Win (ctor 0x631f96): ModeObserver +0x2c
// (subject +0x34), Object +0x3c.
// Vtables: Mac 0x743acc {D1 0x2a1ea0, D0 0x2a2020, OnFinalRelease 0x2a21e0, Do 0x2a1cd0,
// SetEnable, GetEnable, UpdateEnable 0x2a1d40, then the Mac-only primary entries of the
// ModeObserver overriders OnAttached 0x2a1d80, OnEnter 0x2a1db0, OnExit 0x2a1df0};
// Win 0x9d30ec {Do 0x631cdc, …, UpdateEnable 0x631d30}, ModeObserver-in 0x9d30d8 {OnAttached
// 0x631d53, OnDetached, OnEnter 0x631d5e, OnExit 0x631d6b}.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "Mode.h"

namespace LEGO {

class ModeNavigationCommand : public Command, public ModeObserver {
public:
    // Disabled until attached (OnAttached runs UpdateEnable). Mac 0x2a1bb0, Win 0x631f96
    explicit ModeNavigationCommand(Mode* mode);
    virtual ~ModeNavigationCommand();                                       // Mac 0x2a1ea0/0x2a2020, Win 0x631f24 (sdd 0x63202a)

    // Current mode: CanExit (else nothing); target: CanEnter (else nothing); Exit the current
    // one, Enter the target.
    virtual void Do();                                                      // 3/0 Mac 0x2a1cd0, Win 0x631cdc
    // Enabled while the target is not current.
    virtual void UpdateEnable();                                            // 6/3 Mac 0x2a1d40, Win 0x631d30

    // ---- ModeObserver ----
    virtual void OnEnter(Mode* mode);                                       // 8/– Mac 0x2a1db0 (thunk 0x2a1dd0), Win 0x631d5e
    virtual void OnExit(Mode* mode);                                        // 9/– Mac 0x2a1df0 (thunk 0x2a1e10), Win 0x631d6b

protected:
    virtual void OnAttached(Mode* mode);                                    // 7/– Mac 0x2a1d80 (thunk 0x2a1d90), Win 0x631d53
    using ModeObserver::OnDetached;
    // Observer<Mode> part, then NotifierT<Command>'s. Mac 0x2a21e0, Win 0x632049
    virtual void OnFinalRelease();
};

} // namespace LEGO
