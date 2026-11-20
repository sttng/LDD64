// LEGO Digital Designer — restored source
// TU 312 "LuaStandardTextTooltipAugmentation.cpp": Mac 0x2ba740-0x2bb02b, Win 0x459260-0x45990f.
// See LuaStandardTextTooltipAugmentation.h. The Mac static initialiser 0x2bafb0 is not code.
#include "LuaStandardTextTooltipAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"
#include "TextLocale.h"

using namespace LuaPlus;

namespace LEGO {

int LuaStandardTextTooltipAugmentationCreate(lua_State* L)                  // Mac 0x2ba7c0
{
    LuaState* state = LuaState::CastState(L);
    Control* control;
    RefPointer<LuaStandardTextTooltipAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaStandardTextTooltipAugmentation;
        AttachLuaObject(self, object);
    }
    {
        RefPointer<StandardTextTooltipAugmentation> tooltip =
            new StandardTextTooltipAugmentation(0, String());
        object->m_textTooltip = tooltip;
        object->SetTooltipAugmentation(tooltip);
    }
    control = OptLuaControl(state, 2, 0);
    if (control)
        object->AttachTooltip(control);
    return 0;
}

namespace {

// self:SetText(text). Mac 0x2baa60
int LuaStandardTextTooltipAugmentationSetText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaStandardTextTooltipAugmentation* object = LuaStandardTextTooltipAugmentation::Check(state, 1);
    String text = CheckLuaString(state, 2);
    if (object->GetTextTooltip())
        object->GetTextTooltip()->SetText(text);
    return 0;
}

// self:SetLocText(key): the translation of `key`. Mac 0x2baaf0
int LuaStandardTextTooltipAugmentationSetLocText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaStandardTextTooltipAugmentation* object = LuaStandardTextTooltipAugmentation::Check(state, 1);
    String key = CheckLuaString(state, 2);
    LocalizedString text(key);
    if (object->GetTextTooltip())
        object->GetTextTooltip()->SetText(text);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x746df8
    { "Create", LuaStandardTextTooltipAugmentationCreate },
    { "SetText", LuaStandardTextTooltipAugmentationSetText },
    { "SetLocText", LuaStandardTextTooltipAugmentationSetLocText },
    { 0, 0 }
};

} // namespace

void RegisterLuaStandardTextTooltipAugmentation(LuaState* state)             // Mac 0x2ba740, Win 0x459530
{
    LuaObject luaClass = CreateLuaClass(state, "StandardTextTooltipAugmentation", "TooltipAugmentation");
    luaClass.Register(kFunctions, 0);
}

LuaStandardTextTooltipAugmentation* LuaStandardTextTooltipAugmentation::Check(LuaState* state, int narg) // Mac 0x2baee0
{
    return CheckBoundObject<LuaStandardTextTooltipAugmentation>(state, narg);
}

void LuaStandardTextTooltipAugmentation::UnbindLuaObject()                   // Mac 0x2bad30, Win 0x4597a0
{
    m_textTooltip = 0;
    m_tooltip = 0;
}

void LuaStandardTextTooltipAugmentation::OnFinalRelease()                    // Mac 0x2bace0
{
    LuaControlAugmentation::OnFinalRelease();
}

void LuaStandardTextTooltipAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2bad80, Win 0x4597f0
{
    LuaTooltipAugmentation::OnLayoutApplied(layout, args);
    String value;
    if (layout->GetAttribute(String("text"), value) && m_textTooltip)
        m_textTooltip->SetText(value);
    if (layout->GetAttribute(String("locText"), value)) {
        LocalizedString text(value);
        if (m_textTooltip)
            m_textTooltip->SetText(text);
    }
}

} // namespace LEGO
