// LEGO Digital Designer — restored source
// LEGO::CloneTool — "CloneTool" (Lua LDD.tools): the selection (when the tool becomes active), a
//   clicked part or the parts of a group tree click are copied as element bricks and handed to the
//   click-stick tool, which carries the copies and returns to this tool afterwards.
//
// TU 091 "CloneTool.cpp" (Mac 0x1a72d0-0x1a7a8b; Win 0x67f627-0x67f8e8).
// RTTI: CloneTool : SelectionTool. Mac vtable 0x718a3c (39 entries), sizeof 0x128; Win 0x9dba04.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SelectionTool.h"

namespace LEGO {

class CloneTool : public SelectionTool {
public:
    // SelectionTool(clickStickTool, true, true); cursor 0x6c. Mac 0x1a72d0, Win 0x67f642
    explicit CloneTool(ClickStickTool* clickStickTool);
    virtual ~CloneTool();                                                   // Mac 0x1a7750/0x1a77b0, Win sdd 0x67f8c6

    // Element bricks of `parts` (CreateElementBricks) to the click-stick tool, which becomes
    // active: PickUp(bricks, false, this, true, false). Mac 0x1a73a0, Win 0x67f74a
    void Clone(const BasicArray<RefPointer<Part> >& parts);

    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21 Mac 0x1a7390, Win 0x67f8e8
    // Empty (no pick-up of the originals). Mac 0x1a7820, Win 0x5c8021 (ICF)
    virtual void OnPickDragged();                                           // 28/23
    // Clone of the selection, or of the clicked part when nothing is selected; then
    // SelectionTool::OnPickPressed(flag). Mac 0x1a7520, Win 0x67f7f3
    virtual void OnPickClicked(bool flag);                                  // 29/24
    // Clone of the selection when there is one. Mac 0x1a74d0, Win 0x67f7c4
    virtual void OnPickReleased();                                          // 33/28

protected:
    // SelectionTool's, then Clone of the selection when there is one. Mac 0x1a76e0, Win 0x67f898
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The selection is cleared; SelectionTool's. Mac 0x1a7730, Win 0x67f627
    virtual void OnDetached(BrickConnectSceneControl* control);
    using SelectionTool::OnAttached;
    using SelectionTool::OnDetached;
};

} // namespace LEGO
