// LEGO Digital Designer — restored source
// TU 030 "RenderCursor.cpp" (Mac 0xf1100-0xf198b, Win 0x648f18-0x6492ee): Cursor's non-pure
// members and RenderCursor.
#include "RenderCursor.h"
#include "ControlManager.h"

namespace LEGO {

// ===========================================================================================
// Cursor
// ===========================================================================================

// Mac 0xf1100, Win 0x648f18
bool Cursor::Attach()
{
    return GetCursorManager()->SetCursor(this);
}

// Mac 0xf1130, Win 0x648f8c
void Cursor::Detach()
{
    if (ControlManager* manager = GetCursorManager())
        manager->RemoveCursor(this);
}

// Mac 0xf1830 (thunks 0xf18d0 …), Win 0x648fc4
void Cursor::OnFinalRelease()
{
    Detach();
    Object::OnFinalRelease();
}

// ===========================================================================================
// RenderCursor
// ===========================================================================================

// Win 0x649130 (Mac: inlined in LDDCursor's constructor)
RenderCursor::RenderCursor(ControlManager* manager)
    : RenderControl(manager)
{
}

// Mac 0xf1160/0xf11c0, Win 0x649214 (sdd 0x6492ee)
RenderCursor::~RenderCursor()
{
}

// Mac 0xf1230 (thunks 0xf1390, 0xf14c0, 0xf1650, 0xf1780)
void RenderCursor::OnFinalRelease()
{
    RenderControl::OnFinalRelease();
    Cursor::OnFinalRelease();
}

// Mac 0xf1270 (thunk 0xf1500), Win 0x6491ef
ControlManager* RenderCursor::GetCursorManager() const
{
    return m_manager;
}

// Mac 0xf1280 (thunk 0xf1510), Win 0x6491f6
void RenderCursor::SetCursorVisible(bool visible)
{
    SetShown(visible);
}

// Mac 0xf1290 (thunk 0xf1540), Win 0x649201
void RenderCursor::SetCursorPosition(const Vector2& position)
{
    SetPosition(position);
}

} // namespace LEGO
