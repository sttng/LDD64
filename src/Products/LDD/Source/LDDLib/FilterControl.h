// LEGO Digital Designer — restored source
// LEGO::FilterControl — the edit mode's brick-box filter pop-up ("filterControlBox"): the
//   FilterItems (BoxFilterControls, one per brick box) in a MatrixLayoutControl column and a
//   "remove box filter" button below. Choosing an item sets its filter on the palette's
//   FilterControlManager (AddFilter) and toggles only that item on; the button removes the
//   box filter. The pop-up hides itself after a choice and when the mouse goes down outside it.
//
// Home TU 081 "FilterControl.cpp" (Mac 0x16d290-0x16ee2b, Win 0x664c48-0x6652cc + 0x628328-
// 0x629136). The TU also emits LDDPopUpControl's destructors (Mac 0x16e290/0x16e300, Win sdd
// 0x628328) and a BasicArray<Control*> helper (Mac 0x16ebd0). Mac 0x16edb0 = <iostream> + a
// header static (not code). The constructor is inline (emitted in EditMode.cpp: Mac 0x18cf80,
// Win 0x628ac2).
// RTTI: vmi {LDDPopUpControl, ControlObserver (Mac +0x210, Win +0x244), ControlManagerObserver
//   (Mac +0x224, private)}, virtual base Object. Mac vtable 0x71379c (51 entries: OnShown 10 =
//   0x16de40, OnHidden 11 = 0x16e0d0, Create 46 = 0x16d290, then the Itanium primary entries of
//   the observer overriders OnMouseClicked 48 = 0x16dc90, OnManagerMouseEnter 49 = 0x16e130,
//   OnManagerMouseDown 50 = 0x16e1e0); ControlObserver-in 0x7138bc {22: 0x16de10},
//   ControlManagerObserver-in 0x713950 {6: 0x16e180, 9: 0x16e230}. Win 0x9d09f4 {OnShown 7 =
//   0x6652b5, OnHidden 8 = 0x664401, Create 41 = 0x664dd7}, ControlObserver-in 0x9d0954 {19:
//   0x664d93}, ControlManagerObserver-in 0x9d0904 {3: 0x664c48, 6: 0x664c81}.
// Layout (Mac): LDDPopUpControl +0x000..+0x210, ControlObserver +0x210, ControlManagerObserver
//   +0x224 (subject +0x228), clear button +0x22c, layout +0x230, default filter +0x234, filter
//   manager +0x238, mouse-inside flag +0x23c, Object +0x240 (sizeof 0x248; Win 0x270).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "BasicArray.h"
#include "LDDPopUpControl.h"
#include "ControlManager.h"
#include "ButtonControl.h"
#include "MatrixLayoutControl.h"
#include "BrickFilter.h"

namespace LEGO {

class FilterControlManager;

class FilterControl : public LDDPopUpControl, public ControlObserver,
                      private ControlManagerObserver {
public:
    // Mac 0x18cf80 (inline), Win 0x628ac2
    FilterControl(ControlManager* manager, FilterControlManager* filterManager)
        : LDDPopUpControl(manager), m_filterManager(filterManager), m_mouseInside(false) {}
    virtual ~FilterControl();                                               // Mac 0x16e650/0x16e6b0, Win sdd 0x629136

    // The pop-up frame (LDDPopUpControl::Create), the item column (MatrixLayoutControl: 15 px
    // from the top, 15 px margins) and the "remove box filter" button (130 x 55 states of
    // "Graphics/Icon_general02.png", tooltip REMOVEBOXFILTER, button sound, centred 15 px above
    // the bottom). False when a part fails.
    virtual bool Create(Control* parent, Control* after);                   // 46/41 Mac 0x16d290, Win 0x664dd7
    using LDDPopUpControl::Create;

    // The items (re-parented into the column, watched, mouse input on), the cell size
    // (`spacing.x` or, when 0, the column's width; `spacing.y`), the pop-up's size (the wider
    // of column and button + 30, their heights + 45); the first item's filter becomes the
    // default filter. Mac 0x16d960, Win 0x665154
    void SetItems(const BasicArray<RefPointer<Control> >& items, const Vector2& spacing);
    // A FilterItem: its filter is added (AddFilter), only it is toggled on, the pop-up hides.
    // Mac 0x16dd30, Win 0x664d40
    void SelectItem(Control* item);

protected:
    // ---- Control overrides ----
    using ControlObserver::OnShown;
    using ControlObserver::OnHidden;
    // Starts watching the manager's mouse (NOTE: no Control::OnShown: the control's own
    // observers are not told, both builds).
    virtual void OnShown();                                                 // 10/7 Mac 0x16de40, Win 0x6652b5
    virtual void OnHidden();                                                // 11/8 Mac 0x16e0d0, Win 0x664401

    // ---- ControlObserver ----
    using Control::OnMouseClicked;
    // Left button: every item untoggled; the clear button removes the default filter's kind
    // (RemoveFilter) and hides the pop-up, an item is selected (SelectItem).
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // 48 Mac 0x16dc90 (thunk 0x16de10), Win 0x664d93

    // ---- ControlManagerObserver ----
    // The mouse entered `control`: inside the pop-up it is noted; outside, after having been
    // inside, the pop-up hides.
    virtual void OnManagerMouseEnter(Control* control, const Vector2& position); // 49 Mac 0x16e130 (thunk 0x16e180), Win 0x664c48
    // A press outside the pop-up hides it.
    virtual void OnManagerMouseDown(const Vector2& position, UInt32 button); // 50 Mac 0x16e1e0 (thunk 0x16e230), Win 0x664c81

    // Both observer parts, then RenderControl's. Mac 0x16db30
    virtual void OnFinalRelease();

    RefPointer<ButtonControl>       m_clearButton;     // Mac +0x22c
    RefPointer<MatrixLayoutControl> m_layout;          // Mac +0x230
    RefPointer<const BrickFilter>   m_defaultFilter;   // Mac +0x234
    FilterControlManager*           m_filterManager;   // Mac +0x238
    bool                            m_mouseInside;     // Mac +0x23c
};

} // namespace LEGO
