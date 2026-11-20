// LEGO Digital Designer — restored source
// LEGO::FramedTextControl — a RenderControl drawn as a framed box around its text: the tooltip
//   control of StandardTextTooltipAugmentation (ObjectRegistry 'STTC') and LDD info boxes.
//   Bodies in LDDUtilitiesUI.cpp (TU 063 emits the class's inline code).
// Win: constructor 0x513ae9 (thiscall(manager, mostDerived)), SetText 0x513bb7, destructor
//   0x513c10, sdd 0x513c6e; vtable 0x9d3844 (40 entries, SetText slot 36); sizeof 0x214.
//
// RTTI (Mac): si, base RenderControl. Vtable 0x7667a4 (45 entries: RenderControl's with D1
//   0x14e020, D0 0x14e080 and SetText 0x14e0f0); sizeof 0x1f4 (no members of its own; Object at
//   +0x1ec). Constructor Mac 0x147bd0 (inline code in TU 063).
// Method names inferred.
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"

namespace LEGO {

class FramedTextControl : public RenderControl {
public:
    explicit FramedTextControl(ControlManager* manager);                    // Mac 0x147bd0
    virtual ~FramedTextControl();                                           // Mac 0x14e020/0x14e080

    // Sets the text and resizes the box to it. Mac 0x14e0f0
    virtual bool SetText(const String& text);                               // 39/36
};

} // namespace LEGO
