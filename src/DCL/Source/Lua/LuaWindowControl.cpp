// LEGO Digital Designer — restored source
// TU 318 "LuaWindowControl.cpp": Mac 0x2c70e0-0x2c880b, Win 0x462f90-0x463caf.
#include "LuaWindowControl.h"
#include "LuaBindings.h"
#include "XMLDOMNode.h"
#include "StringStream.h"
#include "Preferences.h"
#include "ControlManager.h"
#include "RenderControl.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:Minimize(). Mac 0x2c72d0, Win 0x4631f0
int LuaWindowControlMinimize(lua_State* L)
{
    LuaWindowControl* self = CheckBoundObject<LuaWindowControl>(LuaState::CastState(L), 1); // Mac 0x2c86c0
    self->SetMinimized(true);
    return 0;
}

// self:Restore(). Mac 0x2c7310, Win 0x463220
int LuaWindowControlRestore(lua_State* L)
{
    LuaWindowControl* self = CheckBoundObject<LuaWindowControl>(LuaState::CastState(L), 1);
    self->SetMinimized(false);
    return 0;
}

// WindowControl.Create(self, parent): as Control.Create (transparent RenderControl, not sized
// by its text) with a LuaWindowControl. Mac 0x2c7350, Win 0x462fc0
int LuaWindowControlCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    ControlManager* manager = (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
    RefPointer<RenderControl> control = new RenderControl(manager);
    control->Initialize();
    control->SetTransparent(true);
    control->SetInputFlags(control->GetInputFlags() & 0xbfffffff);
    RefPointer<LuaWindowControl> object = new LuaWindowControl;
    LuaObject parent(state, 2);
    LuaObject after(state);
    object->Initialize(self, parent, after, control.Get(), 7, Rect(0.0f, 0.0f, 0.0f, 0.0f), 0, false);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x748f30
    { "Minimize", LuaWindowControlMinimize },
    { "Restore", LuaWindowControlRestore },
    { 0, 0 }
};

} // namespace

LuaWindowControl::LuaWindowControl()                                        // Mac 0x2c70e0, Win 0x463330
    : m_minimized(false), m_recursion(0), m_restoredSize(),
      m_contentRect(0.0f, 0.0f, 0.0f, 0.0f), m_saveSize(true), m_savePosition(true)
{
}

LuaWindowControl::~LuaWindowControl()                                       // Mac 0x2c7670 (base 0x2c75c0)
{
}

void LuaWindowControl::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2c7b00, Win 0x4633e0
{
    LuaControl::OnLayoutApplied(layout, args);

    {
        Rect rect(0.0f, 0.0f, 0.0f, 0.0f);
        String value;
        UInt32 at = 0;
        if (layout->GetAttribute(String("contentRect"), value) && FromString(rect, value, at)) {
            m_contentRect.Set(rect);
            Layout();
        }
    }

    const XMLDOMNode* contentNode = layout->FindChild(String("Content"));
    if (contentNode && !contentNode->GetChildren().IsEmpty()) {
        LuaObject table = CreateChild(contentNode->GetChildren()[0].Get(), args);
        if (!table.IsNil()) {
            RefPointer<Control> previous = m_content;
            Control* content = 0;
            if (BindableLuaObject* bound = GetBoundObject(table))
                if (LuaControl* object = dynamic_cast<LuaControl*>(bound))
                    content = object->GetTargetControl();
            m_content = content;
            if (previous.Get() != m_content.Get()) {
                if (previous) {
                    ControlObserver::Detach(previous.Get());
                    previous->SetParent(0, 0);
                }
                ControlObserver::Attach(m_content.Get());
                m_content->SetShown(true);
                if (m_content->GetSize() != m_content->GetMinimumSize()) {
                    Vector2 size(m_content->GetSize());
                    size.x = m_contentRect.m_min.x + m_contentRect.m_max.x + size.x;
                    size.y = m_contentRect.m_min.y + m_contentRect.m_max.y + size.y;
                    GetTargetControl()->SetSize(size);
                }
                Layout();
            }
        }
    }

    {
        String value;
        if (layout->GetAttribute(String("saveSize"), value))
            FromString(m_saveSize, value);
    }
    {
        String value;
        if (layout->GetAttribute(String("savePosition"), value))
            FromString(m_savePosition, value);
    }
    {
        String value;
        if (layout->GetAttribute(String("preference"), value)) {
            m_preference = value;
            if (GetTargetControl())
                LoadPreference();
        }
    }
}

void LuaWindowControl::Layout()                                             // Mac 0x2c8000, Win 0x463760
{
    if (!m_content)
        return;
    Vector2 size(m_restoredSize);
    size.x = size.x - (m_contentRect.m_min.x + m_contentRect.m_max.x);
    size.y = size.y - (m_contentRect.m_min.y + m_contentRect.m_max.y);
    m_content->SetSize(size);
    Vector2 actual(m_content->GetSize());
    if (actual != size) {
        actual.x = m_contentRect.m_min.x + m_contentRect.m_max.x + actual.x;
        actual.y = (m_minimized ? 0.0f : m_contentRect.m_min.y + m_contentRect.m_max.y) + actual.y;
        GetTargetControl()->SetSize(actual);
    }
    Real32 x, y;
    if (!m_minimized) {
        x = -m_contentRect.m_min.x;
        y = -m_contentRect.m_min.y;
    } else {
        x = 10.0f - GetTargetControl()->GetSize().x;
        y = 0.0f;
    }
    m_content->SetAlignment(0x11, 0x11, x, y, 0);
}

void LuaWindowControl::LoadPreference()                                     // Mac 0x2c8160, Win 0x463890
{
    String value;
    bool found;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        found = preferences->GetValue(m_preference, value);
    }
    if (!found) {
        SavePreference();
        return;
    }
    Vector2 position, size;
    bool minimized = false;
    UInt32 at = 0;
    FromString(position, value, at);
    ++at;
    FromString(size, value, at);
    ++at;
    FromString(minimized, value, at);
    if (m_savePosition)
        GetTargetControl()->SetPosition(position);
    if (m_saveSize)
        GetTargetControl()->SetSize(size);
    SetMinimized(minimized);
    GetTargetControl()->MoveInsideParent();
}

void LuaWindowControl::SavePreference()                                     // Mac 0x2c83c0, Win 0x463a00
{
    String value;
    ToString(GetTargetControl()->GetPosition(), value);
    value << " ";
    ToString(m_restoredSize, value);
    value << " ";
    ToString(m_minimized, value);
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    preferences->SetValue(m_preference, value);
}

// self:OnMinimized(minimized) (CallLuaMethod<bool>, Mac 0x196870).
void LuaWindowControl::SetMinimized(bool minimized)                         // Mac 0x2c82e0, Win 0x4636f0
{
    if (m_minimized == minimized)
        return;
    m_minimized = minimized;
    {
        LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
        CallLuaMethod(self, "OnMinimized", false, 0, minimized);
    }
    if (!m_minimized) {
        GetTargetControl()->SetSize(m_restoredSize);
    } else {
        Vector2 size(GetTargetControl()->GetSize());
        size.y = 0.0f;
        GetTargetControl()->SetSize(size);
    }
    Layout();
}

void LuaWindowControl::DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2c84c0, Win 0x463b40
{
    LuaControl::DefineCallbacks(layout, args);
    DefineLayoutFunction(layout, "OnMinimized", "minimized", args);
}

bool LuaWindowControl::Attach(Control* control, const String& name)        // Mac 0x2c8510, Win 0x463b70
{
    if (!LuaControl::Attach(control, name))
        return false;
    if (!m_preference.IsEmpty())
        LoadPreference();
    return true;
}

void LuaWindowControl::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                          const Vector2& /*oldSize*/)      // Mac 0x2c85b0, Win 0x463bb0
{
    if (ControlAugmentation::GetControl() == control) {
        if (!m_minimized)
            m_restoredSize = control->GetSize();
        if (!m_preference.IsEmpty())
            SavePreference();
        if (m_recursion > 2)
            return;
        ++m_recursion;
        Layout();
    } else {
        if (m_content.Get() != control)
            return;
        if (m_recursion > 2)
            return;
        ++m_recursion;
        Vector2 size(control->GetSize());
        size.x = m_contentRect.m_min.x + m_contentRect.m_max.x + size.x;
        size.y = m_contentRect.m_min.y + m_contentRect.m_max.y + size.y;
        if (m_minimized) {
            m_restoredSize = size;
            size.y = 0.0f;
        }
        GetTargetControl()->SetSize(size);
    }
    --m_recursion;
}

void RegisterLuaWindowControl(LuaState* state, ControlManager* manager)    // Mac 0x2c7200, Win 0x463250
{
    LuaObject luaClass = CreateLuaClass(state, "WindowControl", "Control");
    luaClass.Register(kFunctions, 0);
    lua_checkstack(state->GetCState(), 1);
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register("Create", LuaWindowControlCreate, 1);
}

} // namespace LEGO
