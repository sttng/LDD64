// LEGO Digital Designer — restored source
// LEGO::RenderCursor — a mouse cursor drawn as a RenderControl in layer 2 of its manager; the
// Cursor's visibility and position map onto the control's. The shape slots stay abstract
// (LDDCursor, implements them).
// TU 030 "RenderCursor.cpp" (Mac 0xf1100-0xf198b, Win 0x648f18-0x6492ee), with Cursor::Attach /
// Detach. Mac ctor inlined into LDDCursor's; Win 0x649130.
// RTTI (Mac): vmi, bases RenderControl +0, Cursor +0x1ec. Vtable 0x7262ec (48 entries: the 45 of
//   RenderControl with Initialize 0x154790 kept, + GetCursorManager 0xf1270, SetCursorVisible
//   0xf1280, SetCursorPosition 0xf1290 (Mac-only primary entries)); Cursor-in 0x726400
//   {0xf1500, 0xf1510, 0xf1540, Attach 0xf1100, Detach 0xf1130, pure ×4, D1/D0 thunks,
//   OnFinalRelease thunk}. Mac layout: RenderControl +0..+0x1ec, Cursor vptr +0x1ec, Object +0x1f0.
//   Win: Cursor at +0x208, Object +0x214 (vtables 0x9d4ccc / 0x9d4c84).
// Class name from RTTI; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"
#include "Cursor.h"

namespace LEGO {

class RenderCursor : public RenderControl, public Cursor {
public:
    explicit RenderCursor(ControlManager* manager);                        // Win 0x649130 (Mac inlined)
    virtual ~RenderCursor();                                                // Mac 0xf1160/0xf11c0, Win 0x649214 (sdd 0x6492ee)

    // ---- Cursor ----
    // Control::GetManager. Mac 0xf1270 (thunk 0xf1500), Win 0x6491ef
    virtual ControlManager* GetCursorManager() const;
    // Control::SetShown. Mac 0xf1280 (thunk 0xf1510), Win 0x6491f6
    virtual void SetCursorVisible(bool visible);
    // Control::SetPosition. Mac 0xf1290 (thunk 0xf1540), Win 0x649201
    virtual void SetCursorPosition(const Vector2& position);
    // Attach / Detach mean the Cursor's (the observer bases have their own). (convenience)
    using Cursor::Attach;
    using Cursor::Detach;

protected:
    virtual void OnFinalRelease();                                          // Mac 0xf1230 (thunks 0xf1390, 0xf14c0, 0xf1650, 0xf1780)
};

} // namespace LEGO
