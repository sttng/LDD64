// LEGO Digital Designer — restored source
// LEGO::MaterialGridControl — the colour picker pop-up of the edit mode ("materialGridControl")
//   and of the tool box swatch: an LDDPopUpControl holding a vertical ArrayLayoutControl with one
//   section per non-empty MaterialGrid row set (solid, transparent, metallic = special, legacy =
//   obsolete): a header text ("SolidColorHeader" …, font 14), a separator line and a
//   MatrixLayoutControl of MaterialItemControls (34 x 34, spacing 10), padded to the widest row.
//   A click on a swatch selects it, notifies the observers and hides the pop-up; a mouse-down
//   outside the pop-up hides it too.
// LEGO::MaterialGridControlObserver — Observer<MaterialGridControl, …>: a material was chosen.
//
// MaterialGridControl: home TU 185 "MaterialGridControl.cpp" (Mac 0x23cc30-0x23fcdb; Win
//   0x6640b7-0x664c47). RTTI vmi {LDDPopUpControl, ControlObserver (Mac +0x210),
//   ControlManagerObserver (+0x224), Notifier<MaterialGridControlObserver> (+0x22c)}; Mac sizeof
//   0x298 (Object +0x290), Win 0x2c8 (Object +0x2c0). The constructor is inline (emitted in
//   EditMode.cpp: Mac 0x18cb50, Win 0x62d415).
//   Virtual slots: Mac 0x72f46c (51 entries) = LDDPopUpControl's 48 with OnFinalRelease 0x23ef00,
//   OnShown (10) 0x23e150, OnHidden (11) 0x23e3d0, Create (46) 0x23cc30 + the Mac-only primary
//   entries 48 OnRectangleChanged (ControlObserver 9) 0x23e430, 49 OnMouseClicked (ControlObserver
//   22) 0x23e530, 50 OnManagerMouseDown (ControlManagerObserver 9) 0x23e890.
//   Win: Create 0x6642a0, OnRectangleChanged 0x6640b7, OnMouseClicked 0x664216, OnShown 0x664414.
// MaterialGridControlObserver: header-inline, vtable emitted in EditMode.cpp: Mac 0x7163bc {D1 0x190cc0,
//   D0 0x190cf0, OnFinalRelease, OnAttached 0x190d40, OnDetached 0x190d50, OnMaterialSelected
//   0x190d60 (empty)}; Win 0x9d0c14 {OnAttached, OnDetached, OnMaterialSelected}.
// Names inferred except the RTTI class names and the header keys.
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"
#include "BasicMap.h"
#include "BasicArray.h"
#include "Control.h"
#include "ControlManager.h"
#include "LDDPopUpControl.h"
#include "ArrayLayoutControl.h"

namespace LEGO {

class MaterialGridControl;
class MaterialItemControl;
class MatrixLayoutControl;
class BrickDatabase;
struct MaterialGrid;

class MaterialGridControlObserver : public Observer<MaterialGridControl, MaterialGridControlObserver> {
public:
    MaterialGridControlObserver() {}
    virtual ~MaterialGridControlObserver() {}                               // Mac 0x190cc0/0x190cf0
    // The user picked `material` in the grid.                              // 5 Mac 0x190d60, Win 2
    virtual void OnMaterialSelected(UInt32 /*material*/) {}
};

class MaterialGridControl : public LDDPopUpControl,
                            public ControlObserver,
                            public ControlManagerObserver,
                            public Notifier<MaterialGridControlObserver> {
public:
    // Item size 34 x 34, nothing selected. Mac 0x18cb50 (inline, emitted in EditMode.cpp), Win 0x62d415
    MaterialGridControl(ControlManager* manager, const BrickDatabase* database);
    virtual ~MaterialGridControl() {}                                       // Mac 0x23ee30/0x23ee90, Win sdd 0x62fe91

    // ---- LDDPopUpControl ----
    // The frame, then the section layout (vertical, resize both, spacing 30, no margins; observed),
    // enabled, transparent, hit-testable, mouse input. Mac 0x23cc30, Win 0x6642a0
    virtual bool Create(Control* parent, Control* after);                   // 46/41
    using LDDPopUpControl::Create;

    // The materials shown: one section per non-empty row set, then (offset != 0) an empty
    // `offset` high spacer; the pop-up fits its children. The selection is dropped.
    // (`offset`: 0x3c, or 0 for LDD Extended / the tool box swatch.) Mac 0x23ce20, Win 0x664aa9
    void SetMaterialGrid(const MaterialGrid* grid, UInt32 offset);
    // Frames the item of `material` (none when it is not shown). Mac 0x23ddf0, Win 0x6643aa
    void SetSelectedMaterial(UInt32 material);

    // ---- Control: observes the manager while shown ----
    using LDDPopUpControl::OnShown;
    using LDDPopUpControl::OnHidden;
    using ControlObserver::OnShown;
    using ControlObserver::OnHidden;
    virtual void OnShown();                                                 // 10/7 Mac 0x23e150, Win 0x664414
    virtual void OnHidden();                                                // 11/8 Mac 0x23e3d0

    // ---- ControlObserver ----
    using LDDPopUpControl::OnRectangleChanged;
    using LDDPopUpControl::OnMouseClicked;
    // The section layout moved / resized: back to (20, 20), the pop-up = its size + 40.
    // Mac 0x23e430 (thunk 0x23e4b0), Win 0x6640b7
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);
    // Button 0 on a swatch: select it and notify; any button-0 click hides the pop-up.
    // Mac 0x23e530 (thunk 0x23e860), Win 0x664216
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button);

    // ---- ControlManagerObserver ----
    // A mouse-down outside the pop-up hides it. Mac 0x23e890 (thunk 0x23e8e0)
    virtual void OnManagerMouseDown(const Vector2& position, UInt32 button);

    // (test / inspection accessors, not in the original)
    ArrayLayoutControl*  GetSectionLayout() const { return m_layout.Get(); }
    MaterialItemControl* GetSelectedItem() const { return m_selected; }
    MaterialItemControl* GetItem(UInt32 material) const;

protected:
    // The observers and every observed control / manager go, then the pop-up's. Mac 0x23ef00
    virtual void OnFinalRelease();
    using ControlObserver::OnAttached;
    using ControlObserver::OnDetached;

    // One section: group control (layout child), header text, separator line (LDDgraphics01
    // (169, 393, 3, 5)) and the item matrix, as wide as the widest row (44 per column - 10).
    // Nothing for empty `rows`. Mac 0x23d0e0, Win 0x66457c
    void AddSection(const BasicArray<BasicArray<UInt32> >& rows, const String& header);
    // An empty spacer of height `height` (nothing for 0). Mac 0x23dc40, Win 0x664121
    void AddSpacer(UInt32 height);
    // A swatch of `material` (an empty cell for 0) in `matrix`. Mac 0x23dec0, Win 0x66442d
    void AddItem(UInt32 material, MatrixLayoutControl* matrix);
    // The previous selection loses its frame. (inlined)
    void ClearSelection();

    typedef BasicMap<UInt32, MaterialItemControl*> ItemMap;

    // ---- data (Mac offsets) ----
    RefPointer<ArrayLayoutControl>   m_layout;     // +0x250 / Win +0x274
    ItemMap                          m_items;      // +0x254 (map +0x258) the items by material
    Vector2                          m_itemSize;   // +0x270 (34, 34)
    MaterialItemControl*             m_selected;   // +0x278
    BasicArray<RefPointer<Control> > m_controls;   // +0x27c the section controls (kept alive)
    const BrickDatabase*             m_database;   // +0x28c
                                                   // Object +0x290 / +0x2c0
};

} // namespace LEGO
