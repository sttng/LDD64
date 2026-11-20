// LEGO Digital Designer — restored source
// LEGO::StretchableButtonControl — a ButtonControl whose state image is drawn as a 9-slice (corner
// sizes from Create's corner rects; edges and centre stretched; own Draw). Fitting the height to
// the text adds twice the text margins. Base of LDDButtonControl (TU 200).
// TU 120 "StretchableButtonControl.cpp": Mac 0x1d67e0-0x1d80bb, Win 0x51a2a3-0x51b1ab (+ sdd
// 0x638cab); the ctor is inline.
//
// RTTI (Mac): one public base ButtonControl, virtual base Object. Primary vtable 0x7217ac
//   (49 entries): ButtonControl's with D1 0x1d7c30 / D0 0x1d7c90, OnFinalRelease 0x1d7d00 (2),
//   Draw 0x1d68f0 (33), ResizeHeightToFitText 0x1d7b40 (41), new slot 48 Create 0x1d67e0;
//   TextLayoutObserver-in 0x72187c, TextureObserver-in 0x7218a0, Object-in 0x7218d0 (-0x2b0).
//   Mac layout: ButtonControl +0x000..+0x218, nine Rects +0x218..+0x2a8, margins +0x2a8 / +0x2ac,
//   Object +0x2b0.
//   Win: vtable 0x9d3a64 (44 entries: Draw 30 = 0x51a360, ResizeHeightToFitText 38 = 0x51b0e7,
//   Create 43 = 0x51a2a3); Rects +0x234.., margins +0x2c4 / +0x2c8; Object-in 0x9d3a2c at +0x2d0.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"
#include "ButtonControl.h"

namespace LEGO {

class StretchableButtonControl : public ButtonControl {
public:
    explicit StretchableButtonControl(ControlManager* manager)
        : ButtonControl(manager),
          m_topLeft(0.0f, 0.0f, 0.0f, 0.0f), m_top(0.0f, 0.0f, 0.0f, 0.0f),
          m_topRight(0.0f, 0.0f, 0.0f, 0.0f), m_left(0.0f, 0.0f, 0.0f, 0.0f),
          m_center(0.0f, 0.0f, 0.0f, 0.0f), m_right(0.0f, 0.0f, 0.0f, 0.0f),
          m_bottomLeft(0.0f, 0.0f, 0.0f, 0.0f), m_bottom(0.0f, 0.0f, 0.0f, 0.0f),
          m_bottomRight(0.0f, 0.0f, 0.0f, 0.0f), m_textMarginY(0), m_textMarginX(0) {}
    virtual ~StretchableButtonControl();                                    // Mac 0x1d7c30/0x1d7c90, Win sdd 0x638cab

    // ---- overrides (Mac slot / Win slot) ----
    // The control's background: nothing at alpha 0 (unbuffered), else the plain colour (no
    // image) or the image's rect split into nine parts by the corner sizes and drawn onto
    // the control's corners / edges / centre; then the text (as RenderControl::Draw).
    // NOTE: original quirks kept (both builds): the top and bottom source parts are
    // W - 2 * topRight.width wide and topLeft.height high, the centre's height subtracts
    // topRight.height, the left destination edge uses the top-right corner's size.
    virtual void Draw(ControlDrawArgs& args);                               // 33/30 Mac 0x1d68f0, Win 0x51a360
    // Replaces RenderControl's: width = 2 * margin x + min(width, text width), height = 2 * margin y
    // + text height (0 × 0 without a text layout); returns SetSize's result.
    virtual bool ResizeHeightToFitText(UInt32 width);                       // 41/38 Mac 0x1d7b40, Win 0x51b0e7

    // ---- new virtual ----
    using ButtonControl::Create;
    // ButtonControl::Initialize, clears RenderControl::kInputInitialized again, stores the nine
    // image parts (only the corners' sizes are used) and the text margins.
    virtual bool Create(const Rect& topLeft, const Rect& top, const Rect& topRight,
                        const Rect& left, const Rect& center, const Rect& right,
                        const Rect& bottomLeft, const Rect& bottom, const Rect& bottomRight,
                        UInt32 textMarginY, UInt32 textMarginX);            // 48/43 Mac 0x1d67e0, Win 0x51a2a3

protected:
    // RenderControl::OnFinalRelease (own entry, same behaviour). Mac 0x1d7d00
    virtual void OnFinalRelease();

    Rect   m_topLeft;       // Mac +0x218, Win +0x234
    Rect   m_top;           // +0x228 (unused by Draw)
    Rect   m_topRight;      // +0x238
    Rect   m_left;          // +0x248 (unused by Draw)
    Rect   m_center;        // +0x258 (unused by Draw)
    Rect   m_right;         // +0x268 (unused by Draw)
    Rect   m_bottomLeft;    // +0x278
    Rect   m_bottom;        // +0x288 (unused by Draw)
    Rect   m_bottomRight;   // +0x298
    UInt32 m_textMarginY;   // Mac +0x2a8, Win +0x2c4
    UInt32 m_textMarginX;   // Mac +0x2ac, Win +0x2c8
};

} // namespace LEGO
