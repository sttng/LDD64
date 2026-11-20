// LEGO Digital Designer — restored source
// TU 237 "ExportCommand.cpp": Mac 0x26eed0-0x26f51b, Win 0x676617-0x676730.
#include "ExportCommand.h"
#include "LDDApplication.h"

namespace LEGO {

// Mac 0x26eed0, Win 0x676623
ExportCommand::ExportCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x26f050 (D1) / 0x26f0c0 (D0), Win sdd 0x676730
ExportCommand::~ExportCommand()
{
}

// Mac 0x26f150
void ExportCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26eff0, Win 0x676617
void ExportCommand::Do()
{
    LDDApplication::GetInstance()->Export();
}

// Mac 0x26f010, Win 0x63120b
void ExportCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->IsSceneEmpty());
}

// Mac 0x26f1e0, Win 0x66e988
void ExportCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x26f1f0 (thunk 0x26f330), Win 0x631956
void ExportCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
