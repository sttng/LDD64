// LEGO Digital Designer — restored source
// LEGO::BrickCountAugmentation — the brick-count badge of a palette brick control: a hidden
//   FramedTextControl below the control (2, 2 from its corner, font 0, framed with the
//   EditMode_Final.png badge image), attached to the control as augmentation "BrickCount". The
//   palette controls look it up by that name and show the count in it.
//
// Home TU 063 "LDDUtilities.cpp" (Mac 0x147d60-0x148380, Win 0x637ce5 / 0x636b3e / 0x637816):
// created with the BricksControls of the palette (Mac 0x14a980 / 0x14abe0, new 0x24).
// RTTI: si, base ControlAugmentation (virtual Object).
// Vtables: Mac 0x70d07c {GetControl, Attach, Detach, D1 0x148250, D0 0x148310}, Object-in
//   0x70d0a8 {thunks 0x1482b0, 0x148380, Object slots}; Win 0x9d3784 {GetControl, Attach,
//   Detach}, Object-in 0x9d3770 {sdd 0x637816, …}.
// Layout: Mac {ControlAugmentation +0x00..+0x18, m_label +0x18, Object +0x1c} sizeof 0x24;
//   Win {vfptr, vbptr, control +0x08, name +0x0c, m_label +0x10, Object +0x14}.
// Member and method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "ControlAugmentation.h"
#include "FramedTextControl.h"

namespace LEGO {

class Control;

class BrickCountAugmentation : public ControlAugmentation {
public:
    // Builds the badge and attaches to `control` (NOTE: `control` must not be null).
    // Mac 0x147d60, Win 0x637ce5
    explicit BrickCountAugmentation(Control* control);
    virtual ~BrickCountAugmentation() {}                                   // Mac 0x148250/0x148310, Win 0x636b3e/sdd 0x637816

    FramedTextControl* GetLabel() const { return m_label; }

protected:
    RefPointer<FramedTextControl> m_label;   // Mac +0x18 / Win +0x10
};

} // namespace LEGO
