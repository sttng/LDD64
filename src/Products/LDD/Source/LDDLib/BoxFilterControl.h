// LEGO Digital Designer — restored source
// LEGO::FilterItem — interface of an item of the filter pop-up (FilterControl): the brick filter
//   it stands for.
// LEGO::BoxFilterControl — the toggle button of one brick box (bag) in the edit mode's filter
//   pop-up: a 9-slice StretchableToggleControl with the bag's name (indented unless the bag has
//   its own colour grid); its filter is a BoxBrickFilter of the bag. A click tells the
//   BoxFilterControlObservers (it does not toggle itself: FilterControl selects).
// LEGO::BoxFilterControlObserver — Observer<BoxFilterControl, …>.
//
// Home TU 152 "BoxFilterControl.cpp" (Mac 0x213e00-0x215aab, Win 0x66d067-0x66d1f5 + dtors
// 0x62c001-0x62c31d). The TU also emits StretchableToggleControl's destructors (Mac 0x2142b0 /
// 0x214310, Win sdd 0x62c2fb) and the Notifier<BoxFilterControlObserver> / NotifierT template
// code (Mac 0x214740-0x214b70, 0x215460-0x215830). Mac 0x215a30 = <iostream> + a header static.
// RTTI: FilterItem polymorphic, no bases, Mac vtable 0x76d560 {pure}; BoxFilterControl vmi
//   {StretchableToggleControl, FilterItem (Mac +0x270), Notifier<BoxFilterControlObserver> (Mac
//   +0x274)}, virtual base Object. Mac vtable 0x72bbfc (57 entries: OnMouseClicked 21 = 0x214010,
//   Create 55 = 0x213e00, GetFilter 56 = 0x213ff0), FilterItem-in 0x72bd48 {GetFilter thunk
//   0x214000}; Win: Create 0x66d067, GetFilter 0x66d18d, OnMouseClicked 0x66d191.
// Layout (Mac): StretchableToggleControl +0x000..+0x270, FilterItem +0x270, Notifier +0x274
//   (observers +0x278, iterations +0x288), filter +0x298, bag +0x29c, Object +0x2a0 (sizeof
//   0x2a8); Win sizeof 0x2cc, bag +0x2b8.
// The constructor is inline (emitted in EditMode.cpp: Mac 0x18ffe0, Win 0x62c9f4 area).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Notifier.h"
#include "StretchableToggleControl.h"
#include "BrickPalette.h"
#include "BrickFilter.h"

namespace LEGO {

class BoxFilterControl;

class FilterItem {
public:
    // The filter the item applies.
    virtual const BrickFilter* GetFilter() const = 0;                        // slot 0
};

class BoxFilterControlObserver : public Observer<BoxFilterControl, BoxFilterControlObserver> {
public:
    virtual ~BoxFilterControlObserver() {}
    // The box button was clicked. (name inferred)
    virtual void OnBoxFilterClicked(BoxFilterControl* /*control*/) {}        // Mac 5
};

class BoxFilterControl : public StretchableToggleControl, public FilterItem,
                         public Notifier<BoxFilterControlObserver> {
public:
    // Mac 0x18ffe0 (inline, emitted in EditMode.cpp): a BoxBrickFilter of `bag`.
    BoxFilterControl(ControlManager* manager, const BrickPalette::Bag* bag)
        : StretchableToggleControl(manager), m_filter(new BoxBrickFilter(bag)), m_bag(bag) {}
    virtual ~BoxFilterControl();                                            // Mac 0x214c10/0x214c70, Win sdd 0x62c31d

    // Tells the observers (OnBoxFilterClicked), then Control::OnMouseClicked (NOTE: not
    // ToggleControl's: a click does not toggle the button).
    virtual void OnMouseClicked(const Vector2& position, UInt32 button);   // 21/18 Mac 0x214010, Win 0x66d191

    // The 9-slice image (StretchableToggleControl::Create), Setup below `parent` behind `after`
    // (flags 7, empty rectangle), transparent, text left-aligned 5 px in, vertically centred,
    // font 14 of the application, the bag's name ("      " in front unless the bag has its own
    // colour grid), the height follows the text. Mac slot 55 (+0xdc), Win slot 50
    virtual bool Create(Texture* texture, const Vector3& columns, const Vector3& rows,
                        Control* parent, Control* after);                   // Mac 0x213e00, Win 0x66d067
    using StretchableToggleControl::Create;

    // FilterItem: the BoxBrickFilter of the bag.
    virtual const BrickFilter* GetFilter() const;                           // 56 Mac 0x213ff0 (thunk 0x214000), Win 0x66d18d

    // The bag of the box (inline read in EditMode Mac 0x18d3d7, Win 0x62c352).
    const BrickPalette::Bag* GetBag() const { return m_bag; }

protected:
    // The Notifier part (leaves every observer), then ToggleControl's. Mac 0x214ce0
    virtual void OnFinalRelease();

    RefPointer<BoxBrickFilter> m_filter;   // Mac +0x298
    const BrickPalette::Bag*   m_bag;      // Mac +0x29c, Win +0x2b8
};

} // namespace LEGO
