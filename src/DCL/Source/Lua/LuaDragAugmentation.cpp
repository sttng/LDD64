// LEGO Digital Designer — restored source
// TU 309 "LuaDragAugmentation.cpp": Mac 0x2b6e70-0x2b842b, Win 0x45cf40-0x45db8f.
// The Mac static initialiser 0x2b83b0 is not code.
#include "LuaDragAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "LayoutResources.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "ControlManager.h"
#include "BitmapSelectionMask.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// DragAugmentation.Create(self [, control]). Mac 0x2b7090, Win 0x45d050
int LuaDragAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaDragAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaDragAugmentation;
        AttachLuaObject(self, static_cast<LuaControlAugmentation*>(object.Get()));
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("drag"));
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x745edc
    { "Create", LuaDragAugmentationCreate },
    { 0, 0 }
};

} // namespace

LuaDragAugmentation::LuaDragAugmentation()                                   // Mac 0x2b6e70, Win 0x45cf40
    : m_mode(0), m_constrainToParent(true), m_frameLeft(0.0f), m_frameTop(0.0f),
      m_frameRight(0.0f), m_frameBottom(0.0f), m_title(0.0f)
{
    // NOTE: the original leaves the helper's grab point and minimum size uninitialised (Vector2
    // default ctors); ControlDragHelper zeroes them here.
    m_drag.m_resizable = true;
    m_drag.m_movable = true;
    m_drag.m_limited = false;
}

LuaDragAugmentation::~LuaDragAugmentation()                                  // Mac 0x2b75d0 (body 0x2b7520)
{
}

void LuaDragAugmentation::UnbindLuaObject()                                  // Mac 0x2b7b10
{
    Detach();
}

void LuaDragAugmentation::OnFinalRelease()                                   // Mac 0x2b7930, Win 0x45d4c0
{
    ControlAugmentationControlObserver::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

bool LuaDragAugmentation::Attach(Control* control, const String& name)      // Mac 0x2b7270, Win 0x45d250
{
    if (!ControlAugmentationControlObserver::Attach(control, name))
        return false;
    control->SetInputFlags(control->GetInputFlags() | 1);
    if (!m_mask) {
        RefPointer<const SelectionMask> mask = control->GetSelectionMask();
        if (!mask)
            control->SetHitTestable(true);
    }
    if (control->GetStatus().m_mouseInside)
        UpdateCursor(control->GetManager()->GetMousePosition(control));
    return true;
}

void LuaDragAugmentation::UpdateCursor(const Vector2& position)              // Mac 0x2b73a0, Win 0x45da70
{
    UInt32 mode = m_mode;
    if (!mode)
        mode = HitTest(position);
    Control* control = GetControl();
    UInt32 cursor;
    switch (mode) {
    case ControlDragHelper::kMove:
        cursor = 5;
        break;
    case ControlDragHelper::kLeft:
        cursor = 12;
        break;
    case ControlDragHelper::kTop:
        cursor = 10;
        break;
    case ControlDragHelper::kLeft | ControlDragHelper::kTop:
    case ControlDragHelper::kRight | ControlDragHelper::kBottom:
        cursor = 15;
        break;
    case ControlDragHelper::kRight:
        cursor = 13;
        break;
    case ControlDragHelper::kRight | ControlDragHelper::kTop:
    case ControlDragHelper::kLeft | ControlDragHelper::kBottom:
        cursor = 14;
        break;
    case ControlDragHelper::kBottom:
        cursor = 11;
        break;
    default:
        cursor = 1;
        break;
    }
    control->SetCursor(cursor, true);
}

UInt32 LuaDragAugmentation::HitTest(const Vector2& position) const          // Mac 0x2b8040, Win 0x45d8f0
{
    Vector2 size(GetControl()->GetSize());
    Real32 x = position.x;
    if (x < 0.0f)
        return 0;
    Real32 y = position.y;
    if (y < 0.0f)
        return 0;
    if (size.x <= x)
        return 0;
    if (!(y < size.y))
        return 0;
    if (m_mask) {
        if (m_drag.m_movable)
            return m_mask->Contains(position) ? ControlDragHelper::kMove : 0;
        return 0;
    }
    if (m_frameLeft == 0.0f && m_frameTop == 0.0f && m_frameRight == 0.0f &&
        m_frameBottom == 0.0f && m_title == 0.0f)
        return m_drag.m_movable ? ControlDragHelper::kMove : 0;
    UInt32 mode = 0;
    if (m_drag.m_resizable) {
        UInt32 horizontal = x < m_frameLeft ? ControlDragHelper::kLeft : 0;
        if (size.x - m_frameRight <= x)
            horizontal += ControlDragHelper::kRight;
        mode = y < m_frameTop ? (horizontal | ControlDragHelper::kTop) : horizontal;
        if (size.y - m_frameBottom <= y)
            return mode | ControlDragHelper::kBottom;
    }
    if (mode == 0 && m_drag.m_movable && y < m_title)
        return ControlDragHelper::kMove;
    return mode;
}

void LuaDragAugmentation::OnMousePressed(Control* control, const Vector2& position, UInt32 button) // Mac 0x2b7fe0, Win 0x45d7c0
{
    if (GetControl() != control)
        return;
    if (button == 0) {
        m_mode = HitTest(position);
        if (m_mode)
            m_drag.m_grab = position;
    }
    UpdateCursor(position);
}

void LuaDragAugmentation::OnMouseMove(Control* control, const Vector2& position) // Mac 0x2b81f0, Win 0x45d820
{
    if (GetControl() != control)
        return;
    if (m_mode) {
        bool constrain = m_constrainToParent;
        if (constrain)
            m_drag.m_limits.Set(Rect(control->GetParent()->GetSize()));
        m_drag.m_limited = constrain;
        m_drag.Drag(m_mode, position, control);
    }
    UpdateCursor(position);
}

void LuaDragAugmentation::OnMouseReleased(Control* control, const Vector2& position, UInt32 button) // Mac 0x2b8330, Win 0x45d8b0
{
    if (GetControl() != control)
        return;
    if (button == 0)
        m_mode = 0;
    UpdateCursor(position);
}

void LuaDragAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2b7b30, Win 0x45d4d0
{
    LuaLayout::OnLayoutApplied(layout, args);
    {
        RefPointer<SelectionMask> mask;
        if (ParseSelectionMask(layout, mask)) {
            m_mask = mask;
            m_mode = 0;
            Control* control = GetControl();
            if (control && control->GetStatus().m_mouseInside)
                UpdateCursor(control->GetManager()->GetMousePosition(control));
        }
    }
    bool changed = false;
    {
        String value;
        if (layout->GetAttribute(String("frame"), value)) {
            m_frameTop = 0.0f;
            m_frameLeft = 0.0f;
            m_frameBottom = 0.0f;
            m_frameRight = 0.0f;
            UInt32 position = 0;
            FromString(m_frameLeft, value, position);
            ++position;
            FromString(m_frameTop, value, position);
            ++position;
            FromString(m_frameRight, value, position);
            ++position;
            FromString(m_frameBottom, value, position);
            changed = true;
        }
    }
    {
        String value;
        if (layout->GetAttribute(String("title"), value)) {
            m_title = 0.0f;
            changed = true;
            FromString(m_title, value);
        }
    }
    {
        String value;
        bool flag;
        if (layout->GetAttribute(String("allowSizeChange"), value) && FromString(flag, value)) {
            m_drag.m_resizable = flag;
            changed = true;
        }
    }
    {
        String value;
        bool flag;
        if (layout->GetAttribute(String("allowPositionChange"), value) && FromString(flag, value)) {
            m_drag.m_movable = flag;
            changed = true;
        }
    }
    {
        String value;
        bool flag;
        if (layout->GetAttribute(String("constrainToParent"), value) && FromString(flag, value)) {
            m_constrainToParent = flag;
            changed = true;
        }
    }
    Control* control = GetControl();
    if (changed && control && control->GetStatus().m_mouseInside)
        UpdateCursor(control->GetManager()->GetMousePosition(control));
}

void RegisterLuaDragAugmentation(LuaState* state)                            // Mac 0x2b7010, Win 0x45d1b0
{
    LuaObject luaClass = CreateLuaClass(state, "DragAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
