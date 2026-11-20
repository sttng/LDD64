// LEGO Digital Designer — restored source
// LEGO::MaterialPickerAugmentation — "Material picker": the augmentation of a brick palette item
//   (BricksControl) whose design has colour variants: a small badge at the item's top right
//   (EditMode_Final.png (406, 175, 16, 16)) while enabled, and, after the mouse rested on the item
//   for 500 ms without a button held, the PaletteMaterialControl with the variants of the item's
//   brick, opened at the item.
//
// Home TU 138 "MaterialPickerAugmentation.cpp" (Mac 0x1f3190-0x1f40cb; Win 0x69181f-0x691c1b).
// RTTI vmi {ControlAugmentationControlObserver (ControlObserver +0x18, private), UpdateObserver
//   (+0x2c, subject +0x30)}, virtual base Object (Mac +0x50, sizeof 0x58).
// Virtual slots: Mac 0x726a5c (9 entries) {GetControl, Attach 0x1f3500, Detach, D1 0x1f3af0,
//   D0 0x1f3b50, OnFinalRelease 0x1f3bc0, and the Mac-only primary entries Update 0x1f35e0,
//   OnMouseEnter 0x1f37b0, OnMouseLeave 0x1f3a50}; ControlObserver-in 0x726a8c, UpdateObserver-in
//   0x726b20, Object-in 0x726b50. Win: Attach 0x6918bd, Update 0x6918f3, OnMouseLeave 0x691944,
//   OnMouseEnter 0x691be9, D 0x691ba3 (sdd 0x691bb6).
// Names inferred except the RTTI class name and the augmentation name.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Updater.h"
#include "ControlAugmentationControlObserver.h"

namespace LEGO {

class PaletteMaterialControl;
class RenderControl;
class Texture;

// "Material picker": the name of the augmentation (static String, Mac 0x77ae7c; static
// initialiser Mac 0x1f4020).
extern const String kMaterialPickerAugmentationName;

class MaterialPickerAugmentation : public ControlAugmentationControlObserver, public UpdateObserver {
public:
    // The badge (a RenderControl below `control`, hidden, aligned to its top right, 2 up) showing
    // `texture` (406, 175, 16, 16), then Attach(control, kMaterialPickerAugmentationName).
    // `materials` is referenced. Mac 0x1f3190 (BrickPaletteControl Mac 0x69670)
    MaterialPickerAugmentation(PaletteMaterialControl* materials, Control* control, Texture* texture);
    virtual ~MaterialPickerAugmentation() {}                                // Mac 0x1f3af0/0x1f3b50, Win 0x691ba3

    // Only BricksControls; observes the control, the hover timer stops. Mac 0x1f3500, Win 0x6918bd
    virtual bool Attach(Control* control, const String& name);              // 1

    // Badge and hover timer on / off (off: the timer stops). Mac 0x1f3490 (BrickPaletteControl
    // Mac 0x69670 / 0x6b470)
    void SetShowVariants(bool show);
    bool IsShowingVariants() const { return m_enabled; }

    // ---- UpdateObserver (the manager's updates while the mouse is over the item) ----
    // A held button restarts the delay; after the delay the variants are shown and the timer
    // stops. Mac 0x1f35e0 (thunk 0x1f3730), Win 0x6918f3
    virtual void Update();                                                  // 6/–

    // ---- ControlObserver ----
    // Enabled: the timer starts (observes the control's manager). Mac 0x1f37b0 (thunk 0x1f3a30),
    // Win 0x691be9
    virtual void OnMouseEnter(Control* control, const Vector2& position);   // 7/–
    // The timer stops. Mac 0x1f3a50 (thunk 0x1f3aa0), Win 0x691944
    virtual void OnMouseLeave(Control* control, const Vector2& position);   // 8/–

protected:
    // The timer and the control observer parts go. Mac 0x1f3bc0
    virtual void OnFinalRelease();
    using UpdateObserver::OnAttached;
    using UpdateObserver::OnDetached;

    // The PaletteMaterialControl with the variants of the item's palette brick (from the item's
    // camera), at the item, kept inside its parent. Mac 0x1f3660, Win 0x69181f
    void ShowVariants();

    // ---- data (Mac offsets) ----
    RefPointer<PaletteMaterialControl> m_materials;   // +0x34
    RefPointer<RenderControl>          m_badge;       // +0x38
    UInt64                             m_start;       // +0x3c GetMilliseconds of the last hover / press
    UInt64                             m_delay;       // +0x44 500 ms
    bool                               m_enabled;     // +0x4c false
                                                      // Object +0x50
};

} // namespace LEGO
