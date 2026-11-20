// LEGO Digital Designer — restored source (bodies: TU 030, RenderCursor.cpp)
// LEGO::Cursor — the mouse cursor interface of a ControlManager (+0x6c), used by TU 027a; the
// implementation RenderCursor (: RenderControl, Cursor) is TU 030 "RenderCursor.cpp" (Mac
// 0xf1100-0xf198b, Win 0x648f18…).
//
// Mac vtable 0x72692c (12 entries): slots 0-2 pure, 3 Attach 0xf1100, 4 Detach 0xf1130, 5-8 pure,
//   D1 0xf17d0, D0 0xf17f0, OnFinalRelease 0xf1830 (Mac-only primary entry); virtual base Object
//   (Mac layout: vptr, Object +4). Win vtable 0x9d4c38 (9 entries: _purecall ×3, Attach 0x648f18,
//   Detach 0x648f8c, _purecall ×4), Object-in 0x9d4c24.
// RenderCursor implements slot 0 by returning Control::m_manager (Mac 0xf1270), slot 1 with
//   Mac 0xdf320 and slot 2 with Mac 0xe02d0 (Control members).
// Slot meaning from the ControlManager call sites (Mac 0xd9400, 0xd95b0, 0xd9120, 0xdd7b0); names
// inferred; slots 5 and 8 are never called from TU 027 (signature unknown).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"

namespace LEGO {

class ControlManager;

class Cursor : public virtual Object {
public:
    Cursor() {}

    // The manager the cursor belongs to.
    virtual ControlManager* GetCursorManager() const = 0;             // slot 0
    // ControlManager +0x92 (SetCursorVisible / OnInput21) is handed down here.
    virtual void SetCursorVisible(bool visible) = 0;                  // slot 1
    // Mouse position in window coordinates (ControlManager +0x98).
    virtual void SetCursorPosition(const Vector2& position) = 0;      // slot 2
    // GetCursorManager()->SetCursor(this).                             Mac 0xf1100, Win 0x648f18
    virtual bool Attach();                                            // slot 3
    // GetCursorManager()->RemoveCursor(this) when there is a manager.  Mac 0xf1130, Win 0x648f8c
    virtual void Detach();                                            // slot 4
    // The current shape (LDDCursor +0x218).
    virtual UInt32 GetShape() const = 0;                              // slot 5
    // Shape of the control under the mouse (Control +0x128) when the control's +0x12c flag is
    // set; slot 7 otherwise, with 0 as second argument. False for an unknown shape.
    virtual bool SetShape(UInt32 shape) = 0;                          // slot 6
    virtual bool SetShapeEx(UInt32 shape, UInt32 arg) = 0;            // slot 7 (not an overload:
                                                                      // MSVC would reorder overloads)
    virtual bool CursorSlot8() = 0;                                   // slot 8 (meaning unknown; LDDCursor: false)
    virtual ~Cursor() {}                                              // Mac D1 0xf17d0 / D0 0xf17f0

protected:
    // Detach, then the Object base. Mac 0xf1830 (thunks 0xf18d0 …), Win 0x648fc4
    virtual void OnFinalRelease();
};

} // namespace LEGO
