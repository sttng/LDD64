// LEGO Digital Designer — restored source
// LEGO::SurfaceColorPickerTool — see SurfaceColorPickerTool.h.
// TU 186 "SurfaceColorPickerTool.cpp": Mac 0x23fce0-0x24071b, Win 0x67d554 (ICF), 0x67f4a2-0x67f608.
// SurfacePickTool's destructor and empty virtuals of this object are in SurfacePickTool.cpp.
// Not code: the thunks, the BasicArray<Pair<RefPointer<Mesh>, Matrix3D> > instance (0x23fce0,
// 0x2405b0, 0x240620), Mac 0x2406a0 = <iostream> + a header static.
#include "SurfaceColorPickerTool.h"
#include "BrickConnectSceneControl.h"
#include "PrimitiveElement.h"
#include "Part.h"

namespace LEGO {

// Mac 0x23fe40, Win 0x67f4df
SurfaceColorPickerTool::SurfaceColorPickerTool()
    : SurfacePickTool(false), m_materialItemControl(0)
{
    m_materialSlot = true;
    SetCursor(0x6f);
}

// Mac 0x240390 / 0x240450, Win sdd 0x67f608
SurfaceColorPickerTool::~SurfaceColorPickerTool()
{
}

// Mac 0x23ff70, Win 0x67f4a2
void SurfaceColorPickerTool::OnPickClicked(bool flag)
{
    if (flag)
        return;
    if (GetSceneControl() != 0 && m_materialItemControl)
        m_materialItemControl->SetMaterial(m_pressedElement->GetPart()->GetMaterial(m_pressedSurface));
}

// Mac 0x23ffe0, Win 0x67d554
void SurfaceColorPickerTool::OnAttached(BrickConnectSceneControl* control)
{
    SurfacePickTool::OnAttached(control);
    GetSceneControl()->ClearSelection();
}

} // namespace LEGO
