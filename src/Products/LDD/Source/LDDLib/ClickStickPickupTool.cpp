// LEGO Digital Designer — restored source
// LEGO::ClickStickPickupTool — see ClickStickPickupTool.h.
// TU 416 "ClickStickPickupTool.cpp" (second object of the class): Mac 0x394f00-0x39532b, Win
// 0x6614de. Not code: Mac 0x3952b0 = <iostream> + a header static.
#include "ClickStickPickupTool.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "ControlManager.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x394f00, Win 0x6614de
void ClickStickPickupTool::OnPickDragged()
{
    BrickConnectSceneControl* control = GetSceneControl();
    BasicArray<RefPointer<Part> > parts = control->GetSelection();
    Part* part = m_picked->GetPart();
    bool found = false;
    for (UInt32 i = 0; i < parts.GetSize() && !found; ++i)
        found = parts[i].Get() == part;
    if (!found) {
        if (m_replaceSelection) {
            GetSceneControl()->ClearSelection();
            parts.Clear();
        }
        parts.PushBack(RefPointer<Part>(m_picked->GetPart()));
    }
    // (the element and the modifiers are read before the tool switch, which detaches this tool)
    PrimitiveElement* picked = m_picked;
    const UInt32 modifiers = GetSceneControl()->GetManager()->GetModifiers();
    GetSceneControl()->GetSceneManager()->SetActiveTool(m_clickStickTool);
    m_clickStickTool->PickUpParts(parts, picked, m_pickPoint, (modifiers & 2) != 0,
                                  (modifiers & 4) != 0, this);
}

} // namespace LEGO
