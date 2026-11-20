// LEGO Digital Designer — restored source
// LEGO::LDDButtonControl — LDD's text push button: a StretchableButtonControl with the rounded
//   button images of "Graphics/LDDgraphics01.png" (four 16 x 26 states, 7 x 8 corners), font 9
//   of the application, centred text, at least 50 wide (text margins 10 / 4).
//
// TU 200 "LDDButtonControl.cpp": Mac 0x24eae0-0x24f79b, Win 0x689a0e-0x68a010. The constructor
// is inline (its copies are inlined at the callers, e.g. LDDAlertBox Mac 0x150ad7 / 0x150beb /
// 0x150cd7 and ProgressBarDialog: StretchableButtonControl's C2 0x151e10, then the vtables and
// m_minimumWidth = 50).
// Vtables: Mac 0x73591c (50 entries = StretchableButtonControl's 49 with D1 0x24f380 / D0
//   0x24f3e0 and ResizeHeightToFitText 41 overridden + Create 49), Win 0x9d5b7c (sdd 0x6503d6).
// Mac layout: StretchableButtonControl +0x000..+0x2b0, minimum width +0x2b0, Object +0x2b4
//   (sizeof 0x2bc). Win: minimum width after the margins (+0x2cc).
// Names inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "StretchableButtonControl.h"

namespace LEGO {

class LDDButtonControl : public StretchableButtonControl {
public:
    // (inline) Every caller of both builds uses a minimum width of 50. (name inferred)
    explicit LDDButtonControl(ControlManager* manager, Real32 minimumWidth = 50.0f)
        : StretchableButtonControl(manager), m_minimumWidth(minimumWidth) {}
    virtual ~LDDButtonControl();                                            // Mac 0x24f380/0x24f3e0, Win sdd 0x6503d6

    // As StretchableButtonControl's, but the text width is at least m_minimumWidth.
    virtual bool ResizeHeightToFitText(UInt32 width);                       // 41/38 Mac 0x24f290, Win 0x689f47

    // The 9-slice images (margins 4 / 10), Setup below `parent` at `position` (shown, enabled,
    // active, no size), the four state images with `text` in font 9, centred, then the size for
    // the text (at most 300 wide). False without the texture or when Create of the base fails.
    // Mac 0x24eae0 (slot 49), Win 0x689a0e (slot 44)
    virtual bool Create(const String& text, Control* parent, const Vector2& position);
    using StretchableButtonControl::Create;

protected:
    Real32 m_minimumWidth;   // Mac +0x2b0
};

} // namespace LEGO
