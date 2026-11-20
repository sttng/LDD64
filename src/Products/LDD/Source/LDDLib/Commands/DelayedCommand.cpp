// LEGO Digital Designer — restored source
// TU 236 "DelayedCommand.cpp": Mac 0x26e9b0-0x26eecf, Win 0x5c9c0e / 0x5c9da6 / 0x609098.
// Not code: construction-vtable thunks, Mac 0x26ee90 = <iostream>.
#include "DelayedCommand.h"

namespace LEGO {

// Mac 0x26ea50 (D1), 0x26eac0 (D0), Win 0x5c9c0e (sdd 0x5c9da6)
DelayedCommand::~DelayedCommand()
{
}

// Mac 0x26ebe0, Win 0x5c8021
void DelayedCommand::Do()
{
}

// Mac 0x26ebf0, Win 0x630465
void DelayedCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x26e9b0, Win 0x609098
void DelayedCommand::Update()
{
    Do();
    UpdateObserver::Detach();
}

// Mac 0x26eb50
void DelayedCommand::OnFinalRelease()
{
    UpdateObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

} // namespace LEGO
