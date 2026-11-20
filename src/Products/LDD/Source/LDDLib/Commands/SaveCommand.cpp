// LEGO Digital Designer — restored source
// TU 255 "SaveCommand.cpp": Mac 0x275570-0x275c0b, Win 0x6764a8-0x6765f8.
// Not code: Mac 0x275b90 = <iostream> + a header static.
#include "SaveCommand.h"
#include "Application.h"

namespace LEGO {

// Mac 0x275570, Win 0x6764eb
SaveCommand::SaveCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x275740 (D1) / 0x2757b0 (D0), Win sdd 0x6765f8
SaveCommand::~SaveCommand()
{
}

// Mac 0x275840 (thunks 0x275a00, 0x275b70)
void SaveCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x275690, Win 0x6764a8
void SaveCommand::Do()
{
    Application::GetInstance()->SaveDocument();
}

// Mac 0x2756b0, Win 0x6764b4
void SaveCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && control->IsModified() && !control->IsSceneEmpty());
}

// Mac 0x2758d0, Win 0x66e988
void SaveCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x275700 (thunk 0x275710), Win 0x6764e3
void SaveCommand::OnEvent13(bool modified)
{
    SetEnable(modified);
}

// Mac 0x2758e0 (thunk 0x275a20), Win 0x631956
void SaveCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
