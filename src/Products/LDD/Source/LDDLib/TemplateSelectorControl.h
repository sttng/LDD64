// LEGO Digital Designer — restored source
// LEGO::TemplateSelectorControl — the template palette ("templatePaletteControl" of the edit
//   mode): a MatrixLayoutControl of 128 x 128 StorageSceneControl items, one per model ("template")
//   of the brick palette whose brand is the application's brand. The items are cached per brand
//   and follow the palette (models added / removed, palette changed); a click on an item tells
//   the BricksControlItemPressedObservers (OnModelPressed); a size slider scales the control.
// LEGO::DeleteGroupAugmentation — the augmentation of a template item: frame image
//   "LDDgraphics01.templatebox" (".templateboxover" while the mouse is over the item or its
//   button) and a DeleteGroupButtonControl in the top right corner.
// LEGO::DeleteGroupButtonControl — the delete button of a template item: after a confirmation
//   the model is removed from the brick palette.
//
// Home TU 074 "TemplateSelectorControl.cpp" (Mac 0x166110-0x1697fb; Win 0x679dc9-0x67ad08). The
// TU also emits the inline StorageSceneControl constructor (Mac 0x167c70, StorageSceneControl.h)
// and destructor body of ModelControl (Mac 0x169450, ModelControl.h).
//
// TemplateSelectorControl: RTTI vmi {MatrixLayoutControl, SliderControlObserver (+0x17c),
//   BrickPaletteObserver (+0x190), BricksControlItemPressedNotifier (+0x198), ControlObserver
//   (+0x1bc)}. Mac layout (constructor 0x166790): m_sceneManager +0x1d0, m_cursor +0x1d4,
//   m_items[3] +0x1d8, Object +0x208 (sizeof 0x210). Win (constructor 0x67aa8f):
//   m_sceneManager +0x1f0, m_cursor +0x1f4, m_items +0x1f8 (3 x 0x10), Object +0x228 (new 0x234).
//   Vtables Mac 0x7115ac (43 entries: MatrixLayoutControl's 36, OnFinalRelease 2; Mac-only
//   overrider entries 36-42: OnBagAdded, OnModelAdded, OnModelRemoved, OnPaletteChanged,
//   OnAttached(SliderControl*), OnValueChanged, OnMouseClicked(Control*, …)).
// DeleteGroupAugmentation: RTTI si {ControlAugmentationControlObserver}. Mac layout (constructor
//   0x166ea0): ControlAugmentationControlObserver +0x00..+0x2c, m_button +0x2c, m_image +0x30,
//   m_item +0x34, Object +0x38 (new 0x40). Win (constructor 0x67a1e0): m_button +0x28, m_image
//   +0x2c, m_item +0x30, Object +0x34 (new 0x40). Vtable Mac 0x7683e4 (8 entries: Mac-only
//   overrider entries 6 / 7 = OnMouseEnter / OnMouseLeave of the ControlObserver base).
// DeleteGroupButtonControl: RTTI si {ButtonControl}; no members (Mac new 0x220, Win 0x240).
//   Vtable Mac 0x768768: overrides OnMouseClicked (21; Win 18 = 0x679fdc).
// Class names from RTTI; method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "BasicArray.h"
#include "MatrixLayoutControl.h"
#include "ButtonControl.h"
#include "SliderControl.h"
#include "ControlAugmentationControlObserver.h"
#include "BrickPalette.h"
#include "BricksControlItemPressedNotifier.h"
#include "StorageSceneControl.h"

namespace LEGO {

class BrickConnectSceneManager;
class StorageSceneControl;
class Texture;
struct StorageScene2;

class TemplateSelectorControl : public MatrixLayoutControl,
                                public SliderControlObserver,
                                public BrickPaletteObserver,
                                public BricksControlItemPressedNotifier,
                                public ControlObserver {
public:
    typedef BasicArray<RefPointer<StorageSceneControl> > ItemArray;

    // Observes `palette`, `observer` observes the control; cell 128 x 128, spacing 10; the items
    // of the current brand. `cursor` is the items' cursor (EditMode: 100).
    // Mac 0x166790, Win 0x67aa8f
    TemplateSelectorControl(ControlManager* manager, BrickConnectSceneManager* sceneManager,
                            BrickPalette* palette, BricksControlItemPressedObserver* observer,
                            UInt32 cursor);
    virtual ~TemplateSelectorControl();                                     // Mac 0x168210/0x1684a0, Win sdd 0x67a626

    const ItemArray& GetItems(UInt32 brand) const { return m_items[brand]; }

    // ---- BrickPaletteObserver (Mac-only primary entries 36-39) ----
    virtual void OnBagAdded(const BrickPalette::Bag* bag);                  // Mac 0x166c00 (empty; thunk 0x166c10)
    // A model of the application's brand gets an item; the control is rebuilt.
    // Mac 0x166c20 (thunk 0x1672a0), Win 0x67ac29
    virtual void OnModelAdded(StorageScene2* model);
    // The item of `model` (current brand) is removed; the control is rebuilt.
    // Mac 0x1672c0 (thunk 0x1673b0), Win 0x67aca7
    virtual void OnModelRemoved(StorageScene2* model);
    // Rebuilt. Mac 0x1673d0 (thunk 0x1673e0), Win 0x67a9ac
    virtual void OnPaletteChanged();

    // ---- SliderControlObserver (Mac-only primary entries 40 / 41): scale = value / 100 ----
    virtual void OnAttached(SliderControl* slider);                         // Mac 0x167400 (thunk 0x167480), Win 0x679dc9
    virtual void OnValueChanged(SliderControl* slider, Real32 value, Real32 oldValue); // Mac 0x167510 (thunk 0x167560), Win 0x679e1b

    // ---- ControlObserver (Mac-only primary entry 42) ----
    // Left click on an item: OnModelPressed(its model) to the observers.
    // Mac 0x1675b0 (thunk 0x1678d0), Win 0x679edd
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button);

protected:
    using SliderControlObserver::OnAttached;     // (the other subjects' overloads stay visible)
    using BrickPaletteObserver::OnAttached;
    using ControlObserver::OnAttached;
    using MatrixLayoutControl::OnMouseClicked;

    // Leaves the observed items, the observers of the notifier, the palette and the sliders,
    // then the Control part. Mac 0x168790 (thunks 0x1688d0 …), Win 0x67a653
    virtual void OnFinalRelease();

    // The items of the application's brand into the layout; made first (with their
    // DeleteGroupAugmentation) from the palette's models when the brand has none yet.
    // Mac 0x166670, Win 0x67a9ac (name inferred)
    void Rebuild();
    // The item of `model` (added to the layout, observed, cached for the current brand); null
    // when the model has no palette bricks. Mac 0x166cb0, Win 0x67a8d5 (name inferred)
    StorageSceneControl* AddModel(const StorageScene2* model);
    // A StorageSceneControl item of `model` below `parent` (cell size, buffered at twice the
    // resolution, m_cursor). Mac 0x167980, Win 0x67a7f7 (name inferred)
    StorageSceneControl* CreateItem(const StorageScene2* model, Control* parent);
    // OnModelPressed(model) to every observer. Mac 0x167650 (name inferred)
    void NotifyModelPressed(StorageScene2* model);

    // ---- data (Mac offsets / Win offsets) ----
    BrickConnectSceneManager* m_sceneManager;   // +0x1d0 / +0x1f0
    UInt32                    m_cursor;         // +0x1d4 / +0x1f4
    ItemArray                 m_items[3];       // +0x1d8 / +0x1f8 per Brand (vtable Mac 0x768f74)
                                                // Object +0x208 / +0x228
};

class DeleteGroupButtonControl : public ButtonControl {
public:
    explicit DeleteGroupButtonControl(ControlManager* manager) : ButtonControl(manager) {} // Win 0x679f3d (Mac inlined)
    virtual ~DeleteGroupButtonControl() {}                                  // Mac 0x168db0/0x168e10, Win sdd 0x67a5ee

    // "DELETETEMPLATEDIALOGUE" (title "…TITLE", YES / NO); YES: the model of the parent
    // StorageSceneControl is removed from the application's brick palette.
    // Mac 0x168e80, Win 0x679fdc
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);    // 21/18
};

class DeleteGroupAugmentation : public ControlAugmentationControlObserver {
public:
    // Frames `item`, creates the delete button on it, attaches as "DeleteGroupAugmentation"
    // and also observes the button. Mac 0x166ea0, Win 0x67a1e0
    explicit DeleteGroupAugmentation(StorageSceneControl* item);
    virtual ~DeleteGroupAugmentation();                                     // Mac 0x168970/0x1689d0, Win sdd 0x67a50b

protected:
    // ".templateboxover" / ".templatebox" (Mac-only primary entries 6 / 7).
    virtual void OnMouseEnter(Control* control, const Vector2& position);   // Mac 0x168a40 (thunk 0x168bf0), Win 0x67a40a
    virtual void OnMouseLeave(Control* control, const Vector2& position);   // Mac 0x168ab0 (thunk 0x168c60), Win 0x67a449

    // ---- data (Mac offsets / Win offsets) ----
    RefPointer<DeleteGroupButtonControl> m_button;   // +0x2c / +0x28
    RefPointer<Texture>                  m_image;    // +0x30 / +0x2c (never set; name inferred)
    StorageSceneControl*                 m_item;     // +0x34 / +0x30 (not referenced)
                                                     // Object +0x38 / +0x34
};

} // namespace LEGO
