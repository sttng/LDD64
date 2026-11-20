// LEGO Digital Designer — restored source
// LEGO::BrickReorientationControl — the small "rotate with the arrow keys" image
//   (Graphics/General/Cursor_Rtt-Keys.png) that follows the LDD mouse cursor while its shape is
//   0x65 (the brick reorientation cursor) and the preference SHOWBRICKREORIENTATION is on.
//
// TU 142 "BrickReorientationControl.cpp" (Mac 0x203730-0x204d8b; Win 0x6488a7-0x648f10).
// Mac layout (constructor 0x2039c0): RenderControl +0x000..+0x1ec, ControlObserver +0x1ec
//   (subjects +0x1f0), LDDCursorObserver +0x200 (subject +0x204), PreferencesObserver +0x208
//   (subject +0x20c), cursor +0x210, shown flag +0x214, Object +0x218 (sizeof 0x220).
//   Win: ControlObserver +0x208, LDDCursorObserver +0x220, PreferencesObserver +0x22c, Object
//   +0x244 (vtable 0x9d4a88).
// Virtual slots: Mac 0x7290cc (49 entries) = RenderControl's 45 (OnFinalRelease 0x204420) + new
//   45 Create 0x203cc0 + the Mac-only primary entries of the overriders 46 OnRectangleChanged
//   (ControlObserver, 0x203e10), 47 OnCursorShapeChanged (0x203ed0), 48 OnPreferenceChanged
//   (0x203f50); Win 0x9d4b4c (41 entries: RenderControl's 40 + Create 0x648ac8),
//   ControlObserver-in 0x9d4aac (slot 6 0x648b78), LDDCursorObserver-in 0x9d4bf4 (0x6488a7),
//   PreferencesObserver-in 0x9d4a9c (0x648a36).
// Names inferred except the RTTI class names.
#pragma once
#include "LEGO/Types.h"
#include "RenderControl.h"
#include "Control.h"
#include "LDDCursor.h"
#include "Preferences.h"

namespace LEGO {

class BrickReorientationControl : public RenderControl,
                                  public ControlObserver,
                                  public LDDCursorObserver,
                                  public PreferencesObserver {
public:
    // Observes `cursor` (as a Control and as an LDDCursor) and the Preferences; shown flag from
    // SHOWBRICKREORIENTATION (false without it). Mac 0x2039c0 (C1), Win 0x648c3d
    BrickReorientationControl(ControlManager* manager, LDDCursor* cursor);
    virtual ~BrickReorientationControl();                                   // Mac 0x204350/0x2043b0, Win sdd 0x648eca

    // RenderControl::Initialize, the image Graphics/General/Cursor_Rtt-Keys.png (through the
    // manager's Render), then below `parent` (flags 6: enabled + active, hidden). False when the
    // image cannot be loaded. Mac 0x203cc0, Win 0x648ac8
    virtual bool Create(Control* parent);                                   // 45/40

    // ---- observer overriders ----
    using RenderControl::OnRectangleChanged;
    // The cursor moved: follow it (same position). Mac 0x203e10 (thunk 0x203e70), Win 0x648b78
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);                // 46/–
    // Shown while the shape is 0x65 and the preference allows it. Mac 0x203ed0 (thunk
    // 0x203f10), Win 0x6488a7
    virtual void OnCursorShapeChanged(UInt32 shape);                        // 47/–
    // SHOWBRICKREORIENTATION re-read. Mac 0x203f50 (thunk 0x204010), Win 0x648a36
    virtual void OnPreferenceChanged(const String& key);                    // 48/–

    bool IsReorientationShown() const { return m_show; }   // (accessor, name inferred)

protected:
    // Preferences, LDDCursor and Control observers leave, then RenderControl's. Mac 0x204420
    virtual void OnFinalRelease();
    using ControlObserver::OnAttached;
    using ControlObserver::OnDetached;

    RefPointer<LDDCursor> m_cursor;   // Mac +0x210
    bool                  m_show;     // Mac +0x214 (SHOWBRICKREORIENTATION)
};

} // namespace LEGO
