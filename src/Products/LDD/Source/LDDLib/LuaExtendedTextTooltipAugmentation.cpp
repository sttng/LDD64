// LEGO Digital Designer — restored source
// TU 297 "LuaExtendedTextTooltipAugmentation.cpp": Mac 0x2a6030-0x2a6a4b, Win 0x64c9d9-0x64cdff.
// See LuaExtendedTextTooltipAugmentation.h. The Mac static initialiser 0x2a69d0 is not code.
#include "LuaExtendedTextTooltipAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"
#include "TextLocale.h"
#include "LDDUtilities.h"

using namespace LuaPlus;

namespace LEGO {

int LuaExtendedTextTooltipAugmentationCreate(lua_State* L)                  // Mac 0x2a60b0
{
    LuaState* state = LuaState::CastState(L);
    Control* control;
    RefPointer<LuaExtendedTextTooltipAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaExtendedTextTooltipAugmentation;
        AttachLuaObject(self, object);
    }
    {
        RefPointer<ExtendedTextTooltipAugmentation> tooltip =
            CreateExtendedTextTooltip(0, String(), String());              // Mac 0x14d140 (TU 063)
        object->m_textTooltip = tooltip;
        object->SetTooltipAugmentation(tooltip);
    }
    control = OptLuaControl(state, 2, 0);
    if (control)
        object->AttachTooltip(control);
    return 0;
}

namespace {

// self:SetText(text): standard and extended text. Mac 0x2a6340
int LuaExtendedTextTooltipAugmentationSetText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaExtendedTextTooltipAugmentation* object = LuaExtendedTextTooltipAugmentation::Check(state, 1);
    String text = CheckLuaString(state, 2);
    if (ExtendedTextTooltipAugmentation* tooltip = object->GetTextTooltip())
        tooltip->SetTexts(text, text);
    return 0;
}

// self:SetLocText(key). Mac 0x2a6410
int LuaExtendedTextTooltipAugmentationSetLocText(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaExtendedTextTooltipAugmentation* object = LuaExtendedTextTooltipAugmentation::Check(state, 1);
    String key = CheckLuaString(state, 2);
    object->SetLocText(key);
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x744108
    { "Create", LuaExtendedTextTooltipAugmentationCreate },
    { "SetText", LuaExtendedTextTooltipAugmentationSetText },
    { "SetLocText", LuaExtendedTextTooltipAugmentationSetLocText },
    { 0, 0 }
};

} // namespace

void RegisterLuaExtendedTextTooltipAugmentation(LuaState* state)             // Mac 0x2a6030, Win 0x64cd97
{
    LuaObject luaClass = CreateLuaClass(state, "ExtendedTextTooltipAugmentation", "TooltipAugmentation");
    luaClass.Register(kFunctions, 0);
}

LuaExtendedTextTooltipAugmentation* LuaExtendedTextTooltipAugmentation::Check(LuaState* state, int narg) // Mac 0x2a6900
{
    return CheckBoundObject<LuaExtendedTextTooltipAugmentation>(state, narg);
}

void LuaExtendedTextTooltipAugmentation::SetLocText(const String& key)        // Mac 0x2a6490, Win 0x64c9d9
{
    LocalizedString text(key);
    String extendedKey(key);
    extendedKey << "Extended";
    LocalizedString extendedText(extendedKey);
    if (m_textTooltip)
        m_textTooltip->SetTexts(text, extendedText);
}

void LuaExtendedTextTooltipAugmentation::UnbindLuaObject()                   // Mac 0x2a6740, Win 0x64ca87
{
    m_textTooltip = 0;
    m_tooltip = 0;
}

void LuaExtendedTextTooltipAugmentation::OnFinalRelease()                    // Mac 0x2a66f0
{
    LuaControlAugmentation::OnFinalRelease();
}

void LuaExtendedTextTooltipAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2a6790, Win 0x64caa0
{
    LuaTooltipAugmentation::OnLayoutApplied(layout, args);
    String value;
    if (layout->GetAttribute(String("text"), value) && m_textTooltip)
        m_textTooltip->SetTexts(value, value);
    if (layout->GetAttribute(String("locText"), value))
        SetLocText(value);
}

} // namespace LEGO
