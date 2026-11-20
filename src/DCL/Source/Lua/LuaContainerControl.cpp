// LEGO Digital Designer — restored source
// TU 386 "LuaContainerControl.cpp": Mac 0x373a90-0x375dab, Win 0x461560-0x4629cf.
#include "LuaContainerControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "RenderControl.h"
#include "ControlManager.h"

#include <math.h>

using namespace LuaPlus;

namespace LEGO {

namespace {

LuaContainerControl* CheckContainer(lua_State* L)                          // Mac 0x375b40
{
    return CheckBoundObject<LuaContainerControl>(LuaState::CastState(L), 1);
}

// The content's size times its scale, rounded up.
Vector2 ScaledSize(const Control* control)
{
    Vector2 size(control->GetSize());
    size.x = ceilf(size.x * control->GetScale().x);
    size.y = ceilf(size.y * control->GetScale().y);
    return size;
}

// self:GetContent() -> the content's table (nil without one). Mac 0x373f70
int LuaContainerGetContent(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaContainerControl* self = CheckContainer(L);
    LuaObject table = LuaControl::GetControlTable(state, self->GetContent());
    table.Push();
    return 1;
}

// self:SetContent(control). Mac 0x374000
int LuaContainerSetContent(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    Control* content = CheckLuaControl(LuaState::CastState(L), 2);
    self->SetContent(content);
    return 0;
}

// self:GetContentExtent() -> the content's scaled size. NOTE: no check for a missing content
// (both builds). Mac 0x374100
int LuaContainerGetContentExtent(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    Vector2 size = ScaledSize(self->GetContent());
    lua_pushnumber(L, size.x);
    lua_pushnumber(L, size.y);
    return 2;
}

// self:SetNotifyMouseWheelInContentRect([notify = true]). Mac 0x3741c0
int LuaContainerSetNotifyMouseWheelInContentRect(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    self->SetNotifyMouseWheelInContentRect(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetNotifyMouseWheelInContentRect(). Mac 0x374250
int LuaContainerGetNotifyMouseWheelInContentRect(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    lua_pushboolean(L, self->m_notifyInContentRect);
    return 1;
}

// self:SetNotifyMouseWheelForContentChildren([notify = true]). Mac 0x3742a0
int LuaContainerSetNotifyMouseWheelForContentChildren(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    self->SetNotifyMouseWheelForContentChildren(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetNotifyMouseWheelForContentChildren(). Mac 0x374330
int LuaContainerGetNotifyMouseWheelForContentChildren(lua_State* L)
{
    LuaContainerControl* self = CheckContainer(L);
    lua_pushboolean(L, self->m_notifyForContentChildren);
    return 1;
}

// ContainerControl.Create(self, parent): a transparent RenderControl. Mac 0x374380
int LuaContainerCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<RenderControl> control = new RenderControl(manager);
    control->Initialize();
    control->SetTransparent(true);
    RefPointer<LuaContainerControl> object = new LuaContainerControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x751ec4
    { "GetContent", LuaContainerGetContent },
    { "SetContent", LuaContainerSetContent },
    { "GetContentExtent", LuaContainerGetContentExtent },
    { "SetNotifyMouseWheelInContentRect", LuaContainerSetNotifyMouseWheelInContentRect },
    { "GetNotifyMouseWheelInContentRect", LuaContainerGetNotifyMouseWheelInContentRect },
    { "SetNotifyMouseWheelForContentChildren", LuaContainerSetNotifyMouseWheelForContentChildren },
    { "GetNotifyMouseWheelForContentChildren", LuaContainerGetNotifyMouseWheelForContentChildren },
    { 0, 0 }
};

} // namespace

LuaContainerControl::LuaContainerControl()                                  // Mac 0x373a90
    : m_notifyInContentRect(false), m_notifyForContentChildren(false), m_sizeDifference(),
      m_contentPosition(), m_contentSize(), m_contentScale(1.0f, 1.0f), m_unused80(0)
{
}

LuaContainerControl::~LuaContainerControl()                                 // Mac 0x3746c0 (D2 0x3745d0)
{
}

// Mac 0x374c40
void LuaContainerControl::OnFinalRelease()
{
    ControlManagerObserver::OnFinalRelease();
    LuaControl::OnFinalRelease();
}

// The content rectangle takes mouse input (hit-testable first) or stops (input first).
void LuaContainerControl::SetNotifyMouseWheelInContentRect(bool notify)
{
    m_notifyInContentRect = notify;
    if (!m_contentRect)
        return;
    if (notify) {
        m_contentRect->SetHitTestable(true);
        m_contentRect->SetInputFlags(m_contentRect->GetInputFlags() | 1);
    } else {
        m_contentRect->SetInputFlags(m_contentRect->GetInputFlags() & 0xfffffffe);
        m_contentRect->SetHitTestable(false);
    }
}

// Observes the control manager (while bound).
void LuaContainerControl::SetNotifyMouseWheelForContentChildren(bool notify)
{
    m_notifyForContentChildren = notify;
    if (!GetTargetControl())
        return;
    if (notify)
        ControlManagerObserver::Attach(GetTargetControl()->GetManager());
    else
        ControlManagerObserver::Detach();
}

// Mac 0x373bf0, Win 0x4615a0
void LuaContainerControl::SetContent(Control* content)
{
    RefPointer<Control> previous = m_content;
    m_content = content;
    if (previous.Get() == m_content.Get())
        return;
    if (previous) {
        ControlObserver::Detach(previous.Get());
        previous->SetParent(0, 0);
    }
    m_content->SetParent(m_contentRect.Get(), 0);
    ControlObserver::Attach(m_content.Get());
    m_content->SetShown(true);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnContentChanged", false, 0);
    NotifyChange(m_content->GetPosition(), m_contentPosition, "OnContentPositionChanged");
    NotifyChange(m_content->GetSize(), m_contentSize, "OnContentSizeChanged");
    NotifyChange(m_content->GetScale(), m_contentScale, "OnContentScaleChanged");
    UpdateSizeDifference();
}

// Mac 0x373de0, Win 0x4626c0
void LuaContainerControl::UpdateSizeDifference()
{
    if (!m_content)
        return;
    Vector2 rect(m_contentRect->GetSize());
    Vector2 difference = rect - ScaledSize(m_content.Get());
    NotifyChange(difference, m_sizeDifference, "OnSizeDifferenceChanged");
}

// self:name(x, y) (CallLuaMethod<LuaObject, LuaObject>, Mac 0x2929a0) with nil for an unchanged
// coordinate. Mac 0x375980, Win 0x462520
void LuaContainerControl::NotifyChange(const Vector2& value, Vector2& stored, const char* name)
{
    if (!(stored != value))
        return;
    Vector2 old(stored);
    stored = value;
    LuaState* state = GetLuaState();
    LuaObject nilObject(state);   // (not `nil`: a macro of the Mac SDK's MacTypes.h)
    LuaObject x, y;
    x.AssignNumber(state, stored.x);
    y.AssignNumber(state, stored.y);
    LuaObject argX(old.x != stored.x ? x : nilObject);
    LuaObject argY(old.y != stored.y ? y : nilObject);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), name, false, 0, argX, argY);
}

// Mac 0x374e30, Win 0x461f60
void LuaContainerControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args)
{
    LuaControl::OnLayoutApplied(layout, args);
    DefineLayoutFunction(layout, "OnContentChanged", "", args);
    DefineLayoutFunction(layout, "OnSizeDifferenceChanged", "x, y", args);
    DefineLayoutFunction(layout, "OnContentPositionChanged", "x, y", args);
    DefineLayoutFunction(layout, "OnContentSizeChanged", "x, y", args);
    DefineLayoutFunction(layout, "OnContentScaleChanged", "x, y", args);
    DefineLayoutFunction(layout, "OnContentMouseWheel", "axis, delta, type", args);

    Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
    bool found;
    {
        String value;
        UInt32 at = 0;
        found = layout->GetAttribute(String("contentRect"), value) && FromString(rect, value, at);
    }
    // contentRect="left top right bottom": the content rectangle's margins inside the container.
    if (found)
        m_contentRect->SetMargins(-rect.m_min.x, rect.m_max.x, -rect.m_min.y, rect.m_max.y, 0);
    {
        bool notify;
        String value;
        if (layout->GetAttribute(String("notifyMouseWheelInContentRect"), value) && FromString(notify, value))
            SetNotifyMouseWheelInContentRect(notify);
    }
    {
        bool notify;
        String value;
        if (layout->GetAttribute(String("notifyMouseWheelForContentChildren"), value) && FromString(notify, value))
            SetNotifyMouseWheelForContentChildren(notify);
    }
    const XMLDOMNode* contentNode = layout->FindChild(String("Content"));
    if (contentNode && !contentNode->GetChildren().IsEmpty()) {
        Control* content = 0;
        {
            LuaObject table = CreateChild(contentNode->GetChildren()[0].Get(), args);
            if (!table.IsNil()) {
                if (BindableLuaObject* bound = GetBoundObject(table))
                    if (LuaControl* object = dynamic_cast<LuaControl*>(bound))
                        content = object->GetTargetControl();
            }
        }
        if (content)
            SetContent(content);
    }
}

// Mac 0x375320, Win 0x4621f0
bool LuaContainerControl::Attach(Control* control, const String& name)
{
    if (!LuaControl::Attach(control, name))
        return false;
    m_contentRect = new Control(control->GetManager());
    m_contentRect->Setup(control, 0, 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    m_contentRect->SetMargins(0.0f, 0.0f, 0.0f, 0.0f, 0);
    ControlObserver::Attach(m_contentRect.Get());
    if (m_notifyInContentRect) {
        m_contentRect->SetHitTestable(true);
        m_contentRect->SetInputFlags(m_contentRect->GetInputFlags() | 1);
    }
    if (m_notifyForContentChildren)
        ControlManagerObserver::Attach(GetTargetControl()->GetManager());
    return true;
}

// Mac 0x3754d0, Win 0x462330
void LuaContainerControl::Detach()
{
    ControlManagerObserver::Detach();
    if (m_content) {
        ControlObserver::Detach(m_content.Get());
        m_content = 0;
    }
    if (m_contentRect) {
        ControlObserver::Detach(m_contentRect.Get());
        m_contentRect = 0;
    }
    LuaControl::Detach();
}

// Mac 0x3755a0, Win 0x4623d0
void LuaContainerControl::OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                             const Vector2& oldSize)
{
    LuaControl::OnRectangleChanged(control, oldPosition, oldSize);
    if (m_contentRect.Get() != control) {
        if (m_content.Get() != control)
            return;
        NotifyChange(control->GetPosition(), m_contentPosition, "OnContentPositionChanged");
        NotifyChange(m_content->GetSize(), m_contentSize, "OnContentSizeChanged");
    }
    UpdateSizeDifference();
}

void LuaContainerControl::OnScaleChanged(Control* control, const Vector2& /*oldScale*/) // Mac 0x3756b0, Win 0x462450
{
    if (m_content.Get() != control)
        return;
    NotifyChange(control->GetScale(), m_contentScale, "OnContentScaleChanged");
    UpdateSizeDifference();
}

// self:OnContentMouseWheel(axis, delta, type) (CallLuaMethod<UInt32, float, UInt32>, Mac 0x375c10).
// Mac 0x375740, Win 0x462480
void LuaContainerControl::OnMouseWheel(Control* control, UInt32 axis, Real32 delta)
{
    UInt32 type;
    if (m_content.Get() == control)
        type = 0;
    else if (m_notifyInContentRect && m_contentRect.Get() == control)
        type = 1;
    else
        return;
    LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnContentMouseWheel", false, 0, (unsigned int)axis, delta, (unsigned int)type);
}

// The control under the mouse lies below the content (not the content itself). Mac 0x375890, Win 0x4624d0
void LuaContainerControl::OnManagerMouseWheel(UInt32 axis, Real32 delta)
{
    if (!m_notifyForContentChildren)
        return;
    Control* control = ControlManagerObserver::GetSubject()->GetMouseControl();
    if (control == m_content.Get())
        return;
    for (; control; control = control->GetParent()) {
        if (control == m_content.Get()) {
            LuaObject self = LEGO::GetLuaObject(static_cast<BindableLuaObject*>(this));
            CallLuaMethod(self, "OnContentMouseWheel", false, 0, (unsigned int)axis, delta, (unsigned int)2);
            return;
        }
    }
}

void RegisterLuaContainerControl(LuaState* state, ControlManager* manager) // Mac 0x373ea0, Win 0x461bf0
{
    LuaObject luaClass = CreateLuaClass(state, "ContainerControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaContainerCreate, 1);
}

} // namespace LEGO
