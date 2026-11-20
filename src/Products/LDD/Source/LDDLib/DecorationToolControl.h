// LEGO Digital Designer — restored source
// LEGO::DecorationToolControl — the DecorationControl pop-up of the decoration tool
//   ("decorationTool"): a click on an item gives the picked part's brick that item's decorations
//   (BrickConnectSceneControl::SetBrickDescription from the item's brick snapshot).
//
// Home TU 197 "DecorationToolControl.cpp" (Mac 0x24a330-0x24ab6b; Win 0x69349f, 0x6938cd); the
// TU also holds DecorationControl's base-object constructor (Mac 0x24a330, DecorationControl.cpp).
// RTTI si {DecorationControl}; Mac sizeof 0x264 (picked element +0x254, Object +0x25c).
// Virtual slots: Mac 0x733ebc (56 entries) = DecorationControl's with D1 0x24a780 / D0 0x24a870
//   and OnBricksControlClicked (55) 0x24a740. Win: OnBricksControlClicked 0x69349f, sdd 0x6938cd.
// Callers: DecorationTool::Create (Mac 0x2477e0: new, (manager, scene manager, 10, 8)),
//   DecorationTool slot 27 (Mac 0x247ae0: SetPickedElement)
// Names inferred except the RTTI class name.
#pragma once
#include "LEGO/Types.h"
#include "DecorationControl.h"

namespace LEGO {

class PrimitiveElement;

class DecorationToolControl : public DecorationControl {
public:
    // Mac 0x24a620
    DecorationToolControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                          UInt32 rows, UInt32 columns);
    virtual ~DecorationToolControl();                                       // Mac 0x24a780/0x24a870, Win sdd 0x6938cd

    // The picked element (referenced) and its decorations on `surface` (ShowDecorations).
    // Mac 0x24a6f0
    bool SetPickedElement(PrimitiveElement* picked, UInt32 surface);

    // ---- DecorationControl ----
    // Mac 0x24a740, Win 0x69349f
    virtual void OnBricksControlClicked(BricksControl* item);               // 55/–

protected:
    RefPointer<PrimitiveElement> m_picked;   // Mac +0x254
};

} // namespace LEGO
