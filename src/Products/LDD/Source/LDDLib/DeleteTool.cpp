// LEGO Digital Designer — restored source
// LEGO::DeleteTool — see DeleteTool.h.
// TU 092 "DeleteTool.cpp": Mac 0x1a7a90-0x1a7f9b, Win 0x67dfaa-0x67e13c. Not code: the thunks,
// Mac 0x1a7f20 = <iostream> + a header static.
#include "DeleteTool.h"
#include "BrickConnectSceneControl.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x1a7a90, Win 0x67e034
DeleteTool::DeleteTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, true, true)
{
    SetCursor(0x6e);
}

// Mac 0x1a7c60 / 0x1a7cc0, Win 0x67e0cc / 0x67e13c
DeleteTool::~DeleteTool()
{
}

// Mac 0x1a7b50, Win 0x67dfaa
void DeleteTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    GetSceneControl()->RemoveParts(parts);
}

// Mac 0x1a7d30, Win 0x5c8021 (ICF)
void DeleteTool::OnPickDragged()
{
}

// Mac 0x1a7ba0, Win 0x67dfe7
void DeleteTool::OnPickClicked(bool flag)
{
    if (flag)
        return;
    if (!GetSceneControl()->GetSelection().IsEmpty())
        GetSceneControl()->RemoveSelection();
    else if (m_picked != 0)
        GetSceneControl()->RemoveBrick(m_picked->GetPart()->GetBrick());
}

// Mac 0x1a7b80, Win 0x67dfbd
void DeleteTool::OnPickReleased()
{
    GetSceneControl()->RemoveSelection();
}

// Mac 0x1a7c10, Win 0x67dfc9
void DeleteTool::OnAttached(BrickConnectSceneControl* control)
{
    SelectionTool::OnAttached(control);
    GetSceneControl()->RemoveSelection();
}

// Mac 0x1a7c40, Win 0x67f627 (ICF)
void DeleteTool::OnDetached(BrickConnectSceneControl* control)
{
    control->ClearSelection();
    SelectionTool::OnDetached(control);
}

} // namespace LEGO
