// LEGO Digital Designer — restored source
// TU 235 "DeleteCommand.cpp": Mac 0x26e310-0x26e9ab, Win 0x66e299-0x66e3fa.
#include "DeleteCommand.h"
#include "Scene.h"

namespace LEGO {

// Mac 0x26e310, Win 0x66e2ed
DeleteCommand::DeleteCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x26e510 (D1) / 0x26e580 (D0), Win sdd 0x66e3fa
DeleteCommand::~DeleteCommand()
{
}

// Mac 0x26e610
void DeleteCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26e430, Win 0x66e299
void DeleteCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (!control)
        return;
    control->RemoveSelection();
    if (control->GetScene()->IsEmpty())
        control->ResetCamera();
}

// Mac 0x26e470, Win 0x66e2c6
void DeleteCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->GetSelection().IsEmpty());
}

// Mac 0x26e6a0, Win 0x66e988
void DeleteCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x26e4b0 (thunk 0x26e4e0), Win 0x684af0
void DeleteCommand::OnEvent14(bool empty)
{
    SetEnable(!empty);
}

} // namespace LEGO
