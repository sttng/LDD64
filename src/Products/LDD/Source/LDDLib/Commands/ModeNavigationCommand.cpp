// LEGO Digital Designer — restored source
// TU 293 "ModeNavigationCommand.cpp": Mac 0x2a17e0-0x2a265b, Win 0x631cdc-0x632049.
// Not code: Mac 0x2a1920 (Observer<Mode, ModeObserver>::Attach instance), 0x2a17e0 / 0x2a1e30
// (array / destructor helpers), the ModeObserver / Observer<Mode> vtable instances
// (0x2a2270-0x2a2580, Win 0x631d78-0x631f05), construction-vtable thunks, 0x2a25e0 =
// <iostream> + a header static.
#include "ModeNavigationCommand.h"

namespace LEGO {

// Mac 0x2a1bb0, Win 0x631f96
ModeNavigationCommand::ModeNavigationCommand(Mode* mode)
    : Command(false)
{
    ModeObserver::Attach(mode);
}

// Mac 0x2a1ea0 (D1), 0x2a2020 (D0), Win 0x631f24 (sdd 0x63202a)
ModeNavigationCommand::~ModeNavigationCommand()
{
}

// Mac 0x2a1cd0, Win 0x631cdc
void ModeNavigationCommand::Do()
{
    if (Mode::GetCurrentMode() && !Mode::GetCurrentMode()->CanExit())
        return;
    Mode* target = ModeObserver::GetSubject();
    if (!target->CanEnter())
        return;
    if (Mode::GetCurrentMode())
        Mode::GetCurrentMode()->Exit();
    target->Enter();
}

// Mac 0x2a1d40, Win 0x631d30
void ModeNavigationCommand::UpdateEnable()
{
    Mode* target = ModeObserver::GetSubject();
    SetEnable(target != 0 && !target->IsCurrent());
}

// Mac 0x2a1d80, Win 0x631d53
void ModeNavigationCommand::OnAttached(Mode*)
{
    UpdateEnable();
}

// Mac 0x2a1db0, Win 0x631d5e
void ModeNavigationCommand::OnEnter(Mode*)
{
    SetEnable(false);
}

// Mac 0x2a1df0, Win 0x631d6b
void ModeNavigationCommand::OnExit(Mode*)
{
    SetEnable(true);
}

// Mac 0x2a21e0 (thunks 0x2a22c0, 0x2a22e0), Win 0x632049
void ModeNavigationCommand::OnFinalRelease()
{
    ModeObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

} // namespace LEGO
