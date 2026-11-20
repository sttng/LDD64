// LEGO Digital Designer — restored source
// LEGO::BricksControlItemPressedObserver — ObserverN of the brick lists (brick palette, template
//   palette, group tree): a brick or a model item was pressed.
// LEGO::BricksControlItemPressedNotifier — Notifier<BricksControlItemPressedObserver>.
//
// The observer is header-inline; its vtable is emitted in EditMode.cpp (TU 085): Mac 0x71648c
//   {D1 0x190f90, D0 0x190fe0, OnFinalRelease 0x190800, OnAttached 0x191050, OnDetached 0x191060,
//   OnBrickPressed 0x191070, OnModelPressed 0x191080 (both empty)}. The notifier's vtable Mac
//   0x7057ac {0x6dbf0, 0x6dc60, 0x6dce0} (TU 023 BrickPaletteControl).
// Argument types from PickupBricksObserver (EditMode.cpp); names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"
#include "ObserverN.h"

namespace LEGO {

class BricksControlItemPressedNotifier;
class PrimitiveElementBrick;
struct StorageScene2;

class BricksControlItemPressedObserver
    : public ObserverN<BricksControlItemPressedNotifier, BricksControlItemPressedObserver> {
public:
    BricksControlItemPressedObserver() {}
    virtual ~BricksControlItemPressedObserver() {}                          // Mac 0x190f90/0x190fe0

    // A brick item was pressed (the brick to pick up).                     // 5 Mac 0x191070
    virtual void OnBrickPressed(PrimitiveElementBrick* /*brick*/) {}
    // A model item (template / group) was pressed (the model to import).   // 6 Mac 0x191080
    virtual void OnModelPressed(StorageScene2* /*model*/) {}
};

class BricksControlItemPressedNotifier : public Notifier<BricksControlItemPressedObserver> {
public:
    BricksControlItemPressedNotifier() {}
    virtual ~BricksControlItemPressedNotifier() {}

    // OnBrickPressed(brick) on every observer (guarded iteration).
    // Mac 0x16b6b0 (TU 076, its own object file), Win 0x6910f8
    void NotifyBrickPressed(PrimitiveElementBrick* brick);
};

} // namespace LEGO
