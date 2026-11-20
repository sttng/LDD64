// LEGO Digital Designer — restored source
// LEGO::ControlDragHelper — moving / resizing a control with the mouse (title bars, resize edges,
// Lua drag augmentations): grab point, mouse limits, and what is allowed.
// TU 032 (Mac 0xf1a10-0xf200b, file name unknown; Win 0x515d17 + 0x515fbe). Callers:
// WindowControl Mac 0xefd20 (struct at WindowControl +0x1dc), LuaDragAugmentation Mac 0x2b81f0 /
// 0x2b8290. No RTTI; struct and member names inferred.
// Layout (Mac = Win): Vector2 grab point +0x00, Vector2 minimum size +0x08, Rect limits +0x10,
// bool resizable +0x20, bool movable +0x21, bool limited +0x22 (sizeof 0x24).
#pragma once
#include "LEGO/Types.h"
#include "Geometry.h"

namespace LEGO {

class Control;

struct ControlDragHelper {
    // Drag modes: kMove, or a combination of the edge bits (resize).
    enum {
        kNone   = 0,
        kMove   = 1,
        kLeft   = 0x04,
        kTop    = 0x08,
        kRight  = 0x10,
        kBottom = 0x20
    };

    ControlDragHelper()
        : m_grab(0.0f, 0.0f), m_minimumSize(0.0f, 0.0f), m_limits(0.0f, 0.0f, 0.0f, 0.0f),
          m_resizable(false), m_movable(false), m_limited(false) {}

    // The mouse is at `mouse` (relative to the control's position): moves (kMove, when movable) or
    // resizes (edge bits, when resizable) `control`. With m_limited the mouse is first clamped so
    // that position + mouse lies in m_limits (parent coordinates). Mac 0xf1a10, Win 0x515d17
    void Drag(UInt32 mode, const Vector2& mouse, Control* control);

    Vector2 m_grab;       // +0x00 grab point relative to the control's position
    Vector2 m_minimumSize; // +0x08 resizing stops at this size
    Rect    m_limits;     // +0x10 allowed mouse positions (parent coordinates)
    bool    m_resizable;  // +0x20
    bool    m_movable;    // +0x21
    bool    m_limited;    // +0x22

private:
    // position -= positionDelta, size += sizeDelta, at least m_minimumSize (near edge: the position
    // follows; `keepX` / `keepY`: the far edge moves and the grab point with it); if the control
    // refuses the rectangle (its own limits) the opposite edges stay. Mac 0xf1db0, Win 0x515fbe
    void Resize(UInt32 mode, const Vector2& positionDelta, const Vector2& sizeDelta, bool keepX,
                bool keepY, Control* control);
};

} // namespace LEGO
