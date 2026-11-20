// LEGO Digital Designer — restored source
// TU 238 "GroupToBrickGroupCommand.cpp": Mac 0x26f520-0x26fbcb, Win 0x684b07-0x684c40.
#include "GroupToBrickGroupCommand.h"

namespace LEGO {

// Mac 0x26f520, Win 0x684b33
GroupToBrickGroupCommand::GroupToBrickGroupCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x26f730 (D1) / 0x26f7a0 (D0), Win sdd 0x684c40
GroupToBrickGroupCommand::~GroupToBrickGroupCommand()
{
}

// Mac 0x26f830
void GroupToBrickGroupCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26f640, Win 0x684b07
void GroupToBrickGroupCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (control && !control->GetSelection().IsEmpty())
        control->CreateGroup(0, control->GetSelection());
}

// Mac 0x26f690, Win 0x66e2c6
void GroupToBrickGroupCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->GetSelection().IsEmpty());
}

// Mac 0x26f8c0, Win 0x66e988
void GroupToBrickGroupCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x26f6d0 (thunk 0x26f700), Win 0x684af0
void GroupToBrickGroupCommand::OnEvent14(bool empty)
{
    SetEnable(!empty);
}

} // namespace LEGO
