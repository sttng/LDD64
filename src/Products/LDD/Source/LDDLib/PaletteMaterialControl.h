// LEGO Digital Designer — restored source
// LEGO::PaletteMaterialControl — the material picker of the brick palette: the MaterialControl
//   pop-up with the palette bricks of one design (its colour variants), opened by the
//   MaterialPickerAugmentation of a palette item; a click on a variant is passed on to the
//   observers (BrickPaletteControl).
// LEGO::PaletteMaterialControlObserver — Observer<PaletteMaterialControl, …>.
//
// Home TU 140 "PaletteMaterialControl.cpp" (Mac 0x1f72a0-0x1f85eb; Win 0x66b4a9, 0x62bfe2-0x62c210).
// RTTI vmi {MaterialControl, Notifier<PaletteMaterialControlObserver> (Mac +0x260)}; Mac sizeof
//   0x28c (Object +0x284), Win 0x2bc (Object +0x2b4). The constructor is inline (emitted in
//   EditMode.cpp: Mac 0x18bf60, Win 0x62ac70).
// Virtual slots: Mac 0x727c4c (59 entries) = MaterialControl's with OnFinalRelease 0x1f7da0 and
//   OnBricksControlClicked (57) 0x1f7520; Win sdd 0x62c210.
// PaletteMaterialControlObserver: Mac vtable 0x7059ac {D1 0x6e870, D0 0x6e8a0, OnFinalRelease,
//   OnAttached, OnDetached, OnBricksControlClicked (pure)} (emitted with BrickPaletteControl,
//   TU 023); Win 0x9d766c.
// Names inferred except the RTTI class names.
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"
#include "MaterialControl.h"

namespace LEGO {

class PaletteMaterialControl;
class BricksControl;
class BrickConnectSceneManager;

class PaletteMaterialControlObserver
    : public Observer<PaletteMaterialControl, PaletteMaterialControlObserver> {
public:
    PaletteMaterialControlObserver() {}
    virtual ~PaletteMaterialControlObserver() {}                            // Mac 0x6e870/0x6e8a0
    // `item` (a colour variant) was clicked.                               // 5 Mac pure, Win 2
    virtual void OnBricksControlClicked(BricksControl* item) = 0;
};

class PaletteMaterialControl : public MaterialControl, public Notifier<PaletteMaterialControlObserver> {
public:
    // EditMode passes (10, 8). Mac 0x18bf60 (inline), Win 0x62ac70
    PaletteMaterialControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                           UInt32 rows, UInt32 columns);
    virtual ~PaletteMaterialControl();                                      // Mac 0x1f7ca0/0x1f7d60, Win sdd 0x62c210

    // Shows the bricks of the application palette's current brick set with the design of
    // `brick` (the first run of them in sort-key order), seen from `*camera`; no "more" button.
    // Mac 0x1f72a0, Win 0x66b4a9
    bool ShowVariants(const BrickPalette::Brick* brick, const Vector2* camera);

    // ---- MaterialControl ----
    // Notifies the observers. Mac 0x1f7520
    virtual void OnBricksControlClicked(BricksControl* item);               // 57/–

protected:
    // Detaches the observers (last first), then MaterialControl's. Mac 0x1f7da0
    virtual void OnFinalRelease();
};

} // namespace LEGO
