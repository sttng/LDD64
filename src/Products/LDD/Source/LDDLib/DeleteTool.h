// LEGO Digital Designer — restored source
// LEGO::DeleteTool — "DeleteTool" (Lua LDD.tools): removes the selection (when the tool becomes
//   active or a band ends), the clicked brick or the parts of a group tree click.
//
// TU 092 "DeleteTool.cpp" (Mac 0x1a7a90-0x1a7f9b; Win 0x67dfaa-0x67e13c).
// RTTI: DeleteTool : SelectionTool. Mac vtable 0x7193ac (39 entries), sizeof 0x128; Win 0x9db5e4.
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SelectionTool.h"

namespace LEGO {

class DeleteTool : public SelectionTool {
public:
    // SelectionTool(clickStickTool, true, true); cursor 0x6e. Mac 0x1a7a90, Win 0x67e034
    explicit DeleteTool(ClickStickTool* clickStickTool);
    virtual ~DeleteTool();                                                  // Mac 0x1a7c60/0x1a7cc0, Win 0x67e0cc/0x67e13c

    // RemoveParts. Mac 0x1a7b50, Win 0x67dfaa
    virtual void OnToolEvent24(const BasicArray<RefPointer<Part> >& parts); // 24/21
    // Empty. Mac 0x1a7d30, Win 0x5c8021 (ICF)
    virtual void OnPickDragged();                                           // 28/23
    // RemoveSelection, or RemoveBrick of the clicked part's brick when nothing is selected.
    // Mac 0x1a7ba0, Win 0x67dfe7
    virtual void OnPickClicked(bool flag);                                  // 29/24
    // RemoveSelection. Mac 0x1a7b80, Win 0x67dfbd
    virtual void OnPickReleased();                                          // 33/28

protected:
    // SelectionTool's, then RemoveSelection. Mac 0x1a7c10, Win 0x67dfc9
    virtual void OnAttached(BrickConnectSceneControl* control);
    // The selection is cleared; SelectionTool's. Mac 0x1a7c40, Win 0x67f627 (ICF with CloneTool's)
    virtual void OnDetached(BrickConnectSceneControl* control);
    using SelectionTool::OnAttached;
    using SelectionTool::OnDetached;
};

} // namespace LEGO
