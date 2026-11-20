// LEGO Digital Designer — restored source
// TU 198 "ToolboxMaterialItemControl.cpp": Mac 0x24ab70-0x24badb, Win 0x663cc1-0x66403d.
// Not code: construction-vtable thunks (0x24b1b0-0x24b900), the destructor body 0x24b070,
// Mac 0x24ba60 = <iostream> + a header static.
#include "ToolboxMaterialItemControl.h"
#include "BrickConnectSceneControl.h"
#include "BrickPalette.h"

namespace LEGO {

// Mac 0x24ab70, Win 0x663d7a
ToolboxMaterialItemControl::ToolboxMaterialItemControl(ControlManager* manager,
                                                       const BrickDatabase* database,
                                                       BrickConnectSceneControl* sceneControl)
    : MaterialItemControl(manager, database), m_sceneControl(sceneControl), m_grid()
{
}

// Mac 0x24ac40, Win 0x663f83
bool ToolboxMaterialItemControl::Create(Control* gridParent, Control* parent, Control* after)
{
    if (!MaterialItemControl::Initialize())
        return false;
    Setup(parent, after, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, true);
    m_grid = new MaterialGridControl(m_manager, m_database);
    if (!m_grid->Create(gridParent, 0))
        return false;
    MaterialGridControlObserver::Attach(m_grid.Get());
    return true;
}

// Mac 0x24aff0, Win 0x663e73
void ToolboxMaterialItemControl::SetMaterialGrid(const MaterialGrid* grid)
{
    m_grid->SetMaterialGrid(grid, 0);
    const MaterialGrid::Rows* rows = 0;
    if (!grid->m_solidRows.IsEmpty())
        rows = &grid->m_solidRows;
    else if (!grid->m_transparentRows.IsEmpty())
        rows = &grid->m_transparentRows;
    else if (!grid->m_specialRows.IsEmpty())
        rows = &grid->m_specialRows;
    // NOTE: the first row is not checked for emptiness (both builds); MaterialGrid never makes
    // empty rows.
    if (rows)
        SetMaterial((*rows)[0][0]);
}

// Mac 0x24b040, Win 0x663ecc
void ToolboxMaterialItemControl::SetMaterial(UInt32 material)
{
    MaterialItemControl::SetMaterial(material);
    m_grid->SetSelectedMaterial(material);
}

// Mac 0x24b980, Win 0x663eeb
void ToolboxMaterialItemControl::OnMouseClicked(const Vector2& position, UInt32 button)
{
    m_grid->Show(LocalToControl(m_grid->GetParent(), Vector2(0.0f, 0.0f)));
    Control::OnMouseClicked(position, button);
}

// Mac 0x24b9f0, Win 0x663cc1
void ToolboxMaterialItemControl::OnMaterialSelected(UInt32 material)
{
    SetMaterial(material);
    m_sceneControl->SetSelectionMaterial(material);
}

// Mac 0x24b6d0
void ToolboxMaterialItemControl::OnFinalRelease()
{
    MaterialGridControlObserver::OnFinalRelease();
    MaterialItemControl::OnFinalRelease();
}

} // namespace LEGO
