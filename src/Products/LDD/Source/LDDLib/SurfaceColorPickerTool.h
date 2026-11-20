// LEGO Digital Designer — restored source
// LEGO::SurfaceColorPickerTool — "colorPickerTool": a click on a surface puts its material into
//   the edit mode's material item control (the tool box swatch of the paint tools).
//
// TU 186 "SurfaceColorPickerTool.cpp" (Mac 0x23fce0-0x24071b; Win 0x67d554, 0x67f4a2-0x67f608).
// The object also holds SurfacePickTool's destructor and empty virtuals.
// RTTI: SurfaceColorPickerTool : SurfacePickTool. Mac vtable 0x73018c (37 entries), sizeof 0x6c
//   (m_materialItemControl +0x60, Object +0x64); Win 0x9db8e4, sizeof 0x7c. Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "SurfacePickTool.h"
#include "MaterialItemControl.h"

namespace LEGO {

class SurfaceColorPickerTool : public SurfacePickTool {
public:
    // SurfacePickTool(false) picking material slots; cursor 0x6f. Mac 0x23fe40, Win 0x67f4df
    SurfaceColorPickerTool();
    virtual ~SurfaceColorPickerTool();                                      // Mac 0x240390/0x240450, Win sdd 0x67f608

    // The control that shows / receives the picked material. NOTE: an inline RefPointer
    // assignment in the original (Mac +0x60; EditMode::Initialize Mac 0x186715).
    void SetMaterialItemControl(MaterialItemControl* control) { m_materialItemControl = control; }
    MaterialItemControl* GetMaterialItemControl() const { return m_materialItemControl; }

    // The control shows the material of the clicked surface. Mac 0x23ff70, Win 0x67f4a2
    virtual void OnPickClicked(bool flag);                                  // 28/23

protected:
    // SurfacePickTool's; the selection is cleared. Mac 0x23ffe0, Win 0x67d554 (ICF with
    // DecorationTool's)
    virtual void OnAttached(BrickConnectSceneControl* control);
    using SurfacePickTool::OnAttached;

    RefPointer<MaterialItemControl> m_materialItemControl;   // +0x60
};

} // namespace LEGO
