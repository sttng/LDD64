// LEGO Digital Designer — restored source
// LEGO::CloneTool — see CloneTool.h.
// TU 091 "CloneTool.cpp": Mac 0x1a72d0-0x1a7a8b, Win 0x67f627-0x67f8e8. Not code: the thunks,
// Mac 0x1a7a10 = <iostream> + a header static.
#include "CloneTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "PrimitiveElement.h"
#include "PrimitiveElementBrick.h"
#include "Part.h"
#include "LDDUtilities.h"

namespace LEGO {

// Mac 0x1a72d0, Win 0x67f642
CloneTool::CloneTool(ClickStickTool* clickStickTool)
    : SelectionTool(clickStickTool, true, true)
{
    SetCursor(0x6c);
}

// Mac 0x1a7750 / 0x1a77b0, Win sdd 0x67f8c6
CloneTool::~CloneTool()
{
}

// Mac 0x1a73a0, Win 0x67f74a
void CloneTool::Clone(const BasicArray<RefPointer<Part> >& parts)
{
    BasicArray<ObjectRefPointer<PrimitiveElementBrick> > bricks;
    CreateElementBricks(parts, bricks);
    GetSceneControl()->GetSceneManager()->SetActiveTool(m_clickStickTool);
    m_clickStickTool->PickUp(bricks, false, this, true, false);
}

// Mac 0x1a7390, Win 0x67f8e8
void CloneTool::OnToolEvent24(const BasicArray<RefPointer<Part> >& parts)
{
    Clone(parts);
}

// Mac 0x1a7820, Win 0x5c8021 (ICF)
void CloneTool::OnPickDragged()
{
}

// Mac 0x1a7520, Win 0x67f7f3
void CloneTool::OnPickClicked(bool flag)
{
    if (!flag) {
        const BasicArray<RefPointer<Part> >& selection = GetSceneControl()->GetSelection();
        if (!selection.IsEmpty()) {
            Clone(selection);
        } else if (m_picked != 0) {
            BasicArray<RefPointer<Part> > parts;
            parts.PushBack(RefPointer<Part>(m_picked->GetPart()));
            Clone(parts);
        }
    }
    // NOTE: OnPickPressed (slot 27), not OnPickClicked, in both builds.
    SelectionTool::OnPickPressed(flag);
}

// Mac 0x1a74d0, Win 0x67f7c4
void CloneTool::OnPickReleased()
{
    if (!GetSceneControl()->GetSelection().IsEmpty())
        Clone(GetSceneControl()->GetSelection());
}

// Mac 0x1a76e0, Win 0x67f898
void CloneTool::OnAttached(BrickConnectSceneControl* control)
{
    SelectionTool::OnAttached(control);
    const BasicArray<RefPointer<Part> >& selection = GetSceneControl()->GetSelection();
    if (!selection.IsEmpty())
        Clone(selection);
}

// Mac 0x1a7730, Win 0x67f627
void CloneTool::OnDetached(BrickConnectSceneControl* control)
{
    control->ClearSelection();
    SelectionTool::OnDetached(control);
}

} // namespace LEGO
