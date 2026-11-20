// LEGO Digital Designer — restored source
// TU 233 "CutCommand.cpp": Mac 0x26c150-0x26cb5b, Win 0x66e889-0x66ea13.
#include "CutCommand.h"

namespace LEGO {

// Mac 0x26c150, Win 0x66e8d8
CutCommand::CutCommand(BrickConnectSceneControl* sceneControl, BricksClipBoard* clipBoard, bool cut)
    : Command(false, false), m_cut(cut), m_clipBoard(clipBoard)
{
    BrickConnectSceneControlObserver::Attach(sceneControl);
    UpdateEnable();
}

// Mac 0x26c680 (D1) / 0x26c6e0 (D0), Win sdd 0x66ea13
CutCommand::~CutCommand()
{
}

// Mac 0x26c750 (thunks 0x26c8c0, 0x26c9c0)
void CutCommand::OnFinalRelease()
{
    BrickConnectSceneControlObserver::OnFinalRelease();
    Command::OnFinalRelease();
}

// Mac 0x26c2b0, Win 0x66e889
void CutCommand::Do()
{
    BrickConnectSceneControl* control = GetSubject();
    if (!control)
        return;
    const BasicArray<RefPointer<Part> >& selection = control->GetSelection();
    if (selection.IsEmpty())
        return;
    BricksClipBoard::BrickArray& bricks = m_clipBoard->GetBricks();
    bricks.Clear();
    if (m_cut)
        control->CutParts(selection, bricks);
    else
        control->GetElementBricks(selection, bricks);
    m_clipBoard->NotifyChanged();                                           // Mac 0x26c360
}

// Mac 0x26c5e0, Win 0x66e2c6
void CutCommand::UpdateEnable()
{
    BrickConnectSceneControl* control = GetSubject();
    SetEnable(control && !control->GetSelection().IsEmpty());
}

// Mac 0x26c7e0, Win 0x66e988
void CutCommand::OnSceneControlAttached(BrickConnectSceneControl* /*control*/)
{
    UpdateEnable();
}

// Mac 0x26c620 (thunk 0x26c650), Win 0x684af0
void CutCommand::OnEvent14(bool empty)
{
    SetEnable(!empty);
}

} // namespace LEGO
