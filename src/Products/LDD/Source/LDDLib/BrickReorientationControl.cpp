// LEGO Digital Designer — restored source
// TU 142 "BrickReorientationControl.cpp": Mac 0x203730-0x204d8b, Win 0x6488a7-0x648f10.
// Not code: Mac 0x203730 (Observer<LDDCursor, LDDCursorObserver>::Attach instance), the
// LDDCursorObserver / Observer<LDDCursor> vtable instances 0x204030-0x2042f0 (inline in
// LDDCursor.h), 0x204b90 (destructor body), construction-vtable thunks, 0x204d10 = <iostream>
// + a header static.
#include "BrickReorientationControl.h"
#include "RenderControlManager.h"
#include "LDDUtilities.h"
#include "StringStream.h"
#include "Texture.h"

namespace LEGO {

namespace {

const UInt32 kReorientationCursor = 0x65;   // LDD cursor shape (name inferred)

} // namespace

// Mac 0x2039c0, Win 0x648c3d
BrickReorientationControl::BrickReorientationControl(ControlManager* manager, LDDCursor* cursor)
    : RenderControl(manager), m_cursor(cursor), m_show(false)
{
    ControlObserver::Attach(m_cursor);
    LDDCursorObserver::Attach(m_cursor);
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        PreferencesObserver::Attach(preferences);
    }
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String value;
    if (preferences->GetValue(String("SHOWBRICKREORIENTATION"), value))
        FromString(m_show, value);
}

// Mac 0x204350 (D1, body 0x204b90), 0x2043b0 (D0), Win 0x648e03 (sdd 0x648eca)
BrickReorientationControl::~BrickReorientationControl()
{
}

// Mac 0x203cc0, Win 0x648ac8
bool BrickReorientationControl::Create(Control* parent)
{
    if (!RenderControl::Initialize())
        return false;
    RefPointer<Texture> image = LoadApplicationTexture(String("Graphics/General/Cursor_Rtt-Keys.png"),
        static_cast<RenderControlManager*>(m_manager)->GetRender());
    if (!image)
        return false;
    Setup(parent, 0, 6, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    SetImage(image, 0);
    return true;
}

// Mac 0x203e10, Win 0x648b78
void BrickReorientationControl::OnRectangleChanged(Control* control, const Vector2&, const Vector2&)
{
    if (control == m_cursor.Get()) {
        Vector2 position(control->GetPosition());
        SetPosition(Vector2(position.x, position.y));
    }
}

// Mac 0x203ed0, Win 0x6488a7
void BrickReorientationControl::OnCursorShapeChanged(UInt32 shape)
{
    SetShown(shape == kReorientationCursor && m_show);
}

// Mac 0x203f50, Win 0x648a36
void BrickReorientationControl::OnPreferenceChanged(const String& key)
{
    if (!key.EqualsNoCase(String("SHOWBRICKREORIENTATION")))
        return;
    String value;
    if (PreferencesObserver::GetSubject()->GetValue(key, value))
        FromString(m_show, value);
}

// Mac 0x204420 (thunks 0x204530 …)
void BrickReorientationControl::OnFinalRelease()
{
    // Observer<Preferences> and Observer<LDDCursor> parts (each Detach + Object's), the
    // ObserverN<Control> part, then RenderControl's.
    PreferencesObserver::OnFinalRelease();
    LDDCursorObserver::OnFinalRelease();
    ControlObserver::OnFinalRelease();
    RenderControl::OnFinalRelease();
}

} // namespace LEGO
