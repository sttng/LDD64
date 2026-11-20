// LEGO Digital Designer — restored source
// TU 249 "PasteCommand.cpp": Mac 0x273380-0x27454b, Win 0x66e587-0x66e7e0.
#include "PasteCommand.h"
#include "BrickConnectSceneControl.h"

namespace LEGO {

typedef Observer<BricksClipBoard, BricksClipBoardObserver>                   ClipBoardPart;
typedef Observer<BrickPalette, BrickPaletteObserver>                         PalettePart;
typedef Observer<BrickConnectSceneManager, BrickConnectSceneManagerObserver> SceneManagerPart;

// Mac 0x2735d0, Win 0x66e629
PasteCommand::PasteCommand(BrickConnectSceneManager* sceneManager, ClickStickTool* clickStickTool,
                           BricksClipBoard* clipBoard)
    : Command(false, false), m_clickStickTool(clickStickTool)
{
    ClipBoardPart::Attach(clipBoard);                                       // Mac 0x273380
    SceneManagerPart::Attach(sceneManager);
}

// Mac 0x273cf0 (D1) / 0x273d50 (D0), Win sdd 0x66e7c1
PasteCommand::~PasteCommand()
{
}

// Mac 0x273dc0 (thunks 0x273fe0, 0x274130, 0x274260, 0x274360), Win 0x66e7e0
void PasteCommand::OnFinalRelease()
{
    SceneManagerPart::OnFinalRelease();
    PalettePart::OnFinalRelease();
    ClipBoardPart::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x273760, Win 0x66e5ef
void PasteCommand::Do()
{
    if (!GetEnable())
        return;
    SceneManagerPart::GetSubject()->SetActiveTool(m_clickStickTool);
    m_clickStickTool->PickUp(ClipBoardPart::GetSubject()->GetBricks(), false, 0, true, false);
}

// Mac 0x2737b0, Win 0x66e587
void PasteCommand::UpdateEnable()
{
    BricksClipBoard* clipBoard = ClipBoardPart::GetSubject();
    if (!clipBoard || !PalettePart::GetSubject())
        SetEnable(false);
    else
        SetEnable(!clipBoard->GetBricks().IsEmpty());
}

// Mac 0x273ec0 (thunk 0x274000), Win 0x631d53
void PasteCommand::OnAttached(BricksClipBoard* /*clipBoard*/)
{
    UpdateEnable();
}

// Mac 0x273ed0 (thunk 0x274020), Win 0x631d53
void PasteCommand::OnDetached(BricksClipBoard* /*clipBoard*/)
{
    UpdateEnable();
}

// Mac 0x273ee0 (thunk 0x274040), Win 0x631d53
void PasteCommand::OnClipBoardChanged(bool /*empty*/)
{
    UpdateEnable();
}

// Mac 0x273ef0 (thunk 0x274150), Win 0x66e70e
void PasteCommand::OnAttached(BrickPalette* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x273f00 (thunk 0x274170), Win 0x66e70e
void PasteCommand::OnDetached(BrickPalette* /*palette*/)
{
    UpdateEnable();
}

// Mac 0x2737f0 (thunk 0x273830), Win 0x676911
void PasteCommand::OnAttached(BrickConnectSceneManager* sceneManager)
{
    // NOTE: as NewCommand, a control without a palette is not checked for.
    if (BrickConnectSceneControl* control = sceneManager->GetSceneControl())
        PalettePart::Attach(control->GetBrickPalette());
}

// Mac 0x273870 (thunk 0x2738c0), Win 0x66e61e
void PasteCommand::OnDetached(BrickConnectSceneManager* /*sceneManager*/)
{
    PalettePart::Detach();
}

// Mac 0x273910 (thunk 0x273970), Win 0x66e804
void PasteCommand::OnSceneControlChanged(BrickConnectSceneControl* /*previous*/,
                                         BrickConnectSceneControl* current)
{
    PalettePart::Detach();
    if (current)
        PalettePart::Attach(current->GetBrickPalette());
}

} // namespace LEGO
