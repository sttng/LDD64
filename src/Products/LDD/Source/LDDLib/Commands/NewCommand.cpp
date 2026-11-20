// LEGO Digital Designer — restored source
// TU 243 "NewCommand.cpp": Mac 0x271a30-0x2724ab, Win 0x676764-0x6768f5.
#include "NewCommand.h"
#include "BrickConnectSceneControl.h"
#include "Application.h"

namespace LEGO {

// Mac 0x271a30, Win 0x676781
NewCommand::NewCommand(BrickConnectSceneManager* sceneManager, Command* showStartHelp)
    : Command(false, false), m_showStartHelp(showStartHelp)
{
    Observer<BrickConnectSceneManager, BrickConnectSceneManagerObserver>::Attach(sceneManager);
}

// Mac 0x271dd0 (D1) / 0x271e30 (D0), Win sdd 0x6768d6
NewCommand::~NewCommand()
{
}

// Mac 0x271ea0 (thunks 0x272080, 0x2721f0, 0x2722f0), Win 0x6768f5
void NewCommand::OnFinalRelease()
{
    Observer<BrickConnectSceneManager, BrickConnectSceneManagerObserver>::OnFinalRelease();
    Observer<BrickPalette, BrickPaletteObserver>::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x271b90, Win 0x676764
void NewCommand::Do()
{
    if (Application::GetInstance()->CloseDocument())
        m_showStartHelp->Do();
}

// Mac 0x271bc0, Win 0x630465
void NewCommand::UpdateEnable()
{
    SetEnable(true);
}

// Mac 0x271f70 (thunk 0x2720a0), Win 0x631d53
void NewCommand::OnAttached(BrickPalette* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x271f80 (thunk 0x2720c0), Win 0x631d53
void NewCommand::OnDetached(BrickPalette* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x271f90 (thunk 0x2720e0), Win 0x631d53
void NewCommand::OnBagAdded(const BrickPalette::Bag* /*bag*/)
{
    UpdateEnable();
}

// Mac 0x271fa0 (thunk 0x272100), Win 0x631956
void NewCommand::OnPaletteChanged()
{
    UpdateEnable();
}

// Mac 0x271be0 (thunk 0x271c20), Win 0x676911
void NewCommand::OnAttached(BrickConnectSceneManager* sceneManager)
{
    // NOTE: a control without a palette would be attached as a null subject (no check in the
    // original either; LDD's scene controls always have the application's palette).
    if (BrickConnectSceneControl* control = sceneManager->GetSceneControl())
        Observer<BrickPalette, BrickPaletteObserver>::Attach(control->GetBrickPalette());
}

// Mac 0x271c60 (thunk 0x271cb0), Win 0x66e61e
void NewCommand::OnDetached(BrickConnectSceneManager* /*sceneManager*/)
{
    Observer<BrickPalette, BrickPaletteObserver>::Detach();
}

// Mac 0x271d00 (thunk 0x271d60), Win 0x66e804
void NewCommand::OnSceneControlChanged(BrickConnectSceneControl* /*previous*/,
                                       BrickConnectSceneControl* current)
{
    Observer<BrickPalette, BrickPaletteObserver>::Detach();
    if (current)
        Observer<BrickPalette, BrickPaletteObserver>::Attach(current->GetBrickPalette());
}

} // namespace LEGO
