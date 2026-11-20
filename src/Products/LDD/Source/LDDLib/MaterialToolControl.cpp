// LEGO Digital Designer — restored source
// TU 095 "MaterialToolControl.cpp": Mac 0x1a9970-0x1aa15b, Win 0x67dbf2, 0x693b00.
// Not code: construction-vtable thunks (0x1a9e40-0x1aa0c0), Mac 0x1aa0e0 = <iostream> + a header
// static.
#include "MaterialToolControl.h"
#include "MaterialTool.h"
#include "BricksControl.h"
#include "BrickConnectSceneControl.h"
#include "BrickConnectSceneManager.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x1a9970
MaterialToolControl::MaterialToolControl(ControlManager* manager,
                                         BrickConnectSceneManager* sceneManager, MaterialTool* tool,
                                         CompositeBrickFilter* filter, UInt32 rows, UInt32 columns)
    : MaterialControl(manager, sceneManager, rows, columns), m_picked(), m_tool(tool)
{
    if (filter)
        SetBrickFilter(filter);
}

// Mac 0x1a9d10 (D1), 0x1a9e00 (D0), Win sdd 0x693b00
MaterialToolControl::~MaterialToolControl()
{
}

// Mac 0x1a9aa0, Win 0x67dbf2
bool MaterialToolControl::SetPickedElement(PrimitiveElement* picked, BrickArray bricks, bool withButton)
{
    m_picked = picked;
    return SetBricks(bricks, 0, withButton);
}

// Mac 0x1a9c30
void MaterialToolControl::OnBricksControlClicked(BricksControl* item)
{
    BrickConnectSceneControl* sceneControl = m_sceneManager->GetSceneControl();
    if (!sceneControl)
        return;
    sceneControl->SetBrickDescription(m_picked->GetPart()->GetBrick(), item->GetPaletteBrick());
    m_tool->OnMaterialPicked(m_picked->GetPart()->GetBrick(), item->GetPaletteBrick());
}

// Mac 0x1a9cb0
void MaterialToolControl::OnMoreClicked()
{
    BrickConnectSceneControl* sceneControl = m_sceneManager->GetSceneControl();
    if (!sceneControl)
        return;
    sceneControl->RemoveBrick(m_picked->GetPart()->GetBrick());
    m_tool->OnMaterialPicked(m_picked->GetPart()->GetBrick(), 0);
}

} // namespace LEGO
