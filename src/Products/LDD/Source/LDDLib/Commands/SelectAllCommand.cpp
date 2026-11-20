// LEGO Digital Designer — restored source
// TU 257 "SelectAllCommand.cpp": Mac 0x276b70-0x2771fb, Win 0x66e151-0x66e27a.
#include "SelectAllCommand.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

// Mac 0x276b70, Win 0x66e174
SelectAllCommand::SelectAllCommand(BrickConnectSceneControl* sceneControl)
    : Command(false, false)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x276d40 (D1) / 0x276db0 (D0), Win sdd 0x66e27a
SelectAllCommand::~SelectAllCommand()
{
}

// Mac 0x276e40
void SelectAllCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x276c90, Win 0x66e151
void SelectAllCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (!control)
        return;
    control->GetSceneManager()->SetActiveTool(0);
    control->SelectAll();
}

// Mac 0x276cd0, Win 0x63120b
void SelectAllCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->IsSceneEmpty());
}

// Mac 0x276ed0 (thunk 0x277010), Win 0x631d53
void SelectAllCommand::OnAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x276d10 (thunk 0x276d20), Win 0x631956
void SelectAllCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
