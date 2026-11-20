// LEGO Digital Designer — restored source
// TU 375 "BGModeCommand.cpp": Mac 0x362ed0-0x36363b, Win 0x631b41-0x631c19.
// Not code: Mac 0x3635c0 = <iostream> + a header static.
#include "BGModeCommand.h"
#include "Scene.h"

namespace LEGO {

// Mac 0x362ed0, Win 0x631c19
BGModeCommand::BGModeCommand(BGAutomaticMode* mode, BrickConnectSceneControl* sceneControl)
    : ModeNavigationCommand(mode), m_sceneControl(sceneControl)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
}

// Mac 0x363050 (D1) / 0x363220 (D0), Win sdd 0x631bfa
BGModeCommand::~BGModeCommand()
{
}

// Mac 0x363430 (thunks 0x363490 … 0x363550)
void BGModeCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    ModeNavigationCommand::OnFinalRelease();
}

// Mac 0x362fa0, Win 0x631b41
void BGModeCommand::UpdateEnable()
{
    ModeNavigationCommand::UpdateEnable();
    bool enable = true;
    if (GetEnable())
        enable = !m_sceneControl->GetScene()->IsEmpty();
    SetEnable(enable);
}

// Mac 0x362ff0 (thunk 0x363000), Win 0x631b78
void BGModeCommand::OnEvent18()
{
    UpdateEnable();
}

// Mac 0x363020 (thunk 0x363030), Win 0x631b78
void BGModeCommand::OnEvent19()
{
    UpdateEnable();
}

} // namespace LEGO
