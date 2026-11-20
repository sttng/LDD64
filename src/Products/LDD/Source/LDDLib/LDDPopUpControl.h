// LEGO Digital Designer — restored source
// LEGO::LDDPopUpControl — LDD's pop-up frame: a StretchableControl showing the 9-slice frame of
//   "Graphics/LDDgraphics01.png" (corners 22/17 px, 5 px edges), created hidden on a parent and
//   shown at a position (moved inside the parent). Base of FilterControl (EditMode's
//   "filterControlBox") and the other LDD pop-ups.
//
// TU 192 "LDDPopUpControl.cpp": Mac 0x2469f0-0x246ecb, Win 0x6603d3-0x660741. The constructor and
// destructor are inline (no copy in the TU; D1/D0 Mac 0x16e290/0x16e300 are weak copies emitted
// with StretchableControl in TU 080; Win dtor inside StretchableControl's 0x627bf6).
// Vtables: Mac 0x713a8c (48 entries = StretchableControl's 46 + Create 46 + Show 47; also the
//   construction vtables 0x72f78c / 0x73225c of its subclasses), Win 0x9d064c (StretchableControl's
//   41 + Create 41 + Show 42). No members of its own (Mac sizeof 0x218 like StretchableControl).
// Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"
#include "StretchableControl.h"

namespace LEGO {

class LDDPopUpControl : public StretchableControl {
public:
    explicit LDDPopUpControl(ControlManager* manager) : StretchableControl(manager) {}
    virtual ~LDDPopUpControl() {}                                           // Mac 0x16e290/0x16e300 (weak)

    // The frame (StretchableControl::Create with the LDDgraphics01 rectangles), then Setup below
    // `parent` behind `after`: hidden (flags 6 = enabled + active), empty rectangle, mouse input,
    // hit-testable. False without the texture or when a part fails.
    virtual bool Create(Control* parent, Control* after);                   // 46/41 Mac 0x2469f0, Win 0x6603fc
    // Brings the frame to the front, shows it at `position` and keeps it inside the parent.
    virtual void Show(const Vector2& position);                            // 47/42 Mac 0x246e10, Win 0x6603d3
    using StretchableControl::Create;
};

} // namespace LEGO
