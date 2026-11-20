// LEGO Digital Designer — restored source
// LEGO::ToolboxMaterialItemControl — the colour swatch of the paint tools in the edit mode's tool
//   box ("materialItemControl", Lua LDD.tools): a MaterialItemControl that owns a
//   MaterialGridControl pop-up (shown at the swatch on a click) and paints the scene selection
//   with the material picked there.
//
// Home TU 198 "ToolboxMaterialItemControl.cpp" (Mac 0x24ab70-0x24badb; Win 0x663cc1-0x66403d).
// RTTI vmi {MaterialItemControl, MaterialGridControlObserver (Mac +0x224, subject +0x228)};
//   Mac sizeof 0x23c (scene control +0x22c, grid +0x230, Object +0x234), Win 0x214.
// Virtual slots: Mac 0x734d2c (48 entries) = MaterialItemControl's 46 with OnFinalRelease 0x24b6d0,
//   OnMouseClicked (21) 0x24b980, SetMaterial (45) 0x24b040, new 46 Create 0x24ac40 and the
//   Mac-only primary entry 47 OnMaterialSelected 0x24b9f0 (thunk 0x24ba20).
//   Win: SetMaterial 0x663ecc, OnMouseClicked 0x663eeb, Create 0x663f83, OnMaterialSelected
//   0x663cc1, SetMaterialGrid 0x663e73, constructor 0x663d7a, sdd 0x663f45.
// Names inferred except the RTTI class names.
#pragma once
#include "LEGO/Types.h"
#include "MaterialItemControl.h"
#include "MaterialGridControl.h"

namespace LEGO {

class BrickDatabase;
class BrickConnectSceneControl;
struct MaterialGrid;

class ToolboxMaterialItemControl : public MaterialItemControl, public MaterialGridControlObserver {
public:
    // Mac 0x24ab70, Win 0x663d7a
    ToolboxMaterialItemControl(ControlManager* manager, const BrickDatabase* database,
                               BrickConnectSceneControl* sceneControl);
    virtual ~ToolboxMaterialItemControl() {}                                // Mac 0x24b150/0x24b3e0, Win sdd 0x663f45

    // MaterialItemControl::Initialize, Setup below `parent` behind `after` (shown, enabled,
    // active; hit-testable), then the pop-up (a MaterialGridControl created below `gridParent`)
    // observed. EditMode passes (scene control, scene control, null).
    // Mac slot 46 (+0xb8) 0x24ac40, Win slot 41 (+0xa4) 0x663f83
    virtual bool Create(Control* gridParent, Control* parent, Control* after);
    // The materials of the pop-up (no spacer); the swatch takes the first material of the first
    // non-empty solid / transparent / special rows. Mac 0x24aff0, Win 0x663e73
    void SetMaterialGrid(const MaterialGrid* grid);

    // ---- MaterialItemControl ----
    // The pop-up's selection follows. Mac 0x24b040, Win 0x663ecc
    virtual void SetMaterial(UInt32 material);                              // 45/40
    // Shows the pop-up at the swatch, then Control's. Mac 0x24b980, Win 0x663eeb
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18

    // ---- MaterialGridControlObserver ----
    // SetMaterial, and the scene selection gets the material. Mac 0x24b9f0 (thunk 0x24ba20),
    // Win 0x663cc1
    virtual void OnMaterialSelected(UInt32 material);                       // 47/–

    MaterialGridControl* GetMaterialGridControl() const { return m_grid.Get(); }   // (accessor)

protected:
    // The pop-up observer leaves, then MaterialItemControl's. Mac 0x24b6d0
    virtual void OnFinalRelease();
    using MaterialGridControlObserver::OnAttached;
    using MaterialGridControlObserver::OnDetached;

    BrickConnectSceneControl*       m_sceneControl;   // Mac +0x22c
    RefPointer<MaterialGridControl> m_grid;           // Mac +0x230
};

} // namespace LEGO
