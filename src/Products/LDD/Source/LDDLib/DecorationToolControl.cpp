// LEGO Digital Designer — restored source
// TU 197 "DecorationToolControl.cpp": Mac 0x24a330-0x24ab6b, Win 0x69349f, 0x6938cd.
// Mac 0x24a330 (DecorationControl's C2 constructor) is restored in DecorationControl.cpp.
// Not code: construction-vtable thunks (0x24a8b0-0x24aa50), Mac 0x24aaf0 = <iostream> + a
// header static.
#include "DecorationToolControl.h"
#include "BricksControl.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x24a620
DecorationToolControl::DecorationToolControl(ControlManager* manager,
                                             BrickConnectSceneManager* sceneManager,
                                             UInt32 rows, UInt32 columns)
    : DecorationControl(manager, sceneManager, rows, columns), m_picked()
{
}

// Mac 0x24a780 (D1), 0x24a870 (D0), Win sdd 0x6938cd
DecorationToolControl::~DecorationToolControl()
{
}

// Mac 0x24a6f0
bool DecorationToolControl::SetPickedElement(PrimitiveElement* picked, UInt32 surface)
{
    m_picked = picked;
    return ShowDecorations(picked, surface);
}

// Mac 0x24a740, Win 0x69349f
void DecorationToolControl::OnBricksControlClicked(BricksControl* item)
{
    BrickConnectSceneControl* sceneControl = m_sceneManager->GetSceneControl();
    if (!sceneControl)
        return;
    sceneControl->SetBrickDescription(m_picked->GetPart()->GetBrick(), item->GetBrick());
}

} // namespace LEGO
