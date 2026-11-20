// LEGO Digital Designer — restored source
// TU 311 "LuaSaveRectAugmentation.cpp": Mac 0x2b97c0-0x2ba73b, Win 0x45c620-0x45cc7f.
// The Mac static initialiser 0x2ba6c0 is not code.
#include "LuaSaveRectAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"
#include "Preferences.h"
#include "StringStream.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// SaveRectAugmentation.Create(self [, control]). Mac 0x2b9950, Win 0x45c740
int LuaSaveRectAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaSaveRectAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaSaveRectAugmentation;
        AttachLuaObject(self, static_cast<LuaControlAugmentation*>(object.Get()));
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("saveRect"));
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x746a0c
    { "Create", LuaSaveRectAugmentationCreate },
    { 0, 0 }
};

} // namespace

void LuaSaveRectAugmentation::UnbindLuaObject()                              // Mac 0x2ba380
{
    Detach();
}

void LuaSaveRectAugmentation::OnFinalRelease()                               // Mac 0x2ba1b0, Win 0x45ca00
{
    ControlAugmentationControlObserver::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

bool LuaSaveRectAugmentation::Attach(Control* control, const String& name)  // Mac 0x2b9b30, Win 0x45c910
{
    if (!ControlAugmentationControlObserver::Attach(control, name))
        return false;
    if (!m_preference.IsEmpty())
        Load();
    return true;
}

void LuaSaveRectAugmentation::Load()                                         // Mac 0x2b9bc0, Win 0x45cb70
{
    Rect rectangle(0.0f, 0.0f, 0.0f, 0.0f);
    bool loaded = false;
    {
        RefPointer<Preferences> preferences = Preferences::GetInstance();
        String text;
        if (preferences->GetValue(m_preference, text)) {
            UInt32 position = 0;
            loaded = FromString(rectangle, text, position);
        }
    }
    if (!loaded) {
        Save();
        return;
    }
    Control* control = GetControl();
    if (m_savePosition)
        control->SetPosition(rectangle.GetMin());
    if (m_saveSize)
        control->SetSize(rectangle.GetSize());
    control->MoveInsideParent();
}

void LuaSaveRectAugmentation::Save()                                         // Mac 0x2ba5a0
{
    Control* control = GetControl();
    Rect rectangle(control->GetPosition(), control->GetSize());
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    String text;
    ToString(rectangle, text);
    preferences->SetValue(m_preference, text);
}

void LuaSaveRectAugmentation::OnRectangleChanged(Control* control, const Vector2& /*oldPosition*/,
                                                 const Vector2& /*oldSize*/) // Mac 0x2ba560
{
    if (GetControl() == control && !m_preference.IsEmpty())
        Save();
}

void LuaSaveRectAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2ba3a0, Win 0x45ca10
{
    LuaLayout::OnLayoutApplied(layout, args);
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
    String preference;
    if (layout->GetAttribute(String("preference"), preference)) {
        m_preference = preference;
        if (GetControl())
            Load();
    }
}

void RegisterLuaSaveRectAugmentation(LuaState* state)                        // Mac 0x2b98d0, Win 0x45c870
{
    LuaObject luaClass = CreateLuaClass(state, "SaveRectAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
