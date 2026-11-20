// LEGO Digital Designer — restored source
// TU 313 "LuaTooltipAugmentation.cpp": Mac 0x2bb030-0x2bb79b, Win 0x459b30-0x459fcf.
// The Mac static initialiser 0x2bb720 is not code.
#include "LuaTooltipAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "XMLDOMNode.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:SetTooltip(control): the tooltip becomes `control`'s. Mac 0x2bb0f0
int LuaTooltipAugmentationSetTooltip(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaTooltipAugmentation* object = LuaTooltipAugmentation::Check(state, 1);
    Control* control = CheckLuaControl(state, 2);
    if (object->GetTooltipAugmentation())
        object->GetTooltipAugmentation()->AttachTo(control);
    return 0;
}

// self:RemoveTooltip(). Mac 0x2bb200
int LuaTooltipAugmentationRemoveTooltip(lua_State* L)
{
    LuaTooltipAugmentation* object = LuaTooltipAugmentation::Check(LuaState::CastState(L), 1);
    if (object->GetTooltipAugmentation())
        object->GetTooltipAugmentation()->Remove();
    return 0;
}

// self:TooltipChanged(): the shown tooltip is updated. Mac 0x2bb240
int LuaTooltipAugmentationTooltipChanged(lua_State* L)
{
    LuaTooltipAugmentation* object = LuaTooltipAugmentation::Check(LuaState::CastState(L), 1);
    if (object->GetTooltipAugmentation())
        object->GetTooltipAugmentation()->Refresh();
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x746f70
    { "SetTooltip", LuaTooltipAugmentationSetTooltip },
    { "RemoveTooltip", LuaTooltipAugmentationRemoveTooltip },
    { "TooltipChanged", LuaTooltipAugmentationTooltipChanged },
    { 0, 0 }
};

} // namespace

void RegisterLuaTooltipAugmentation(LuaState* state)                         // Mac 0x2bb070, Win 0x459c00
{
    LuaObject luaClass = CreateLuaClass(state, "TooltipAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
}

LuaTooltipAugmentation* LuaTooltipAugmentation::Check(LuaState* state, int narg) // Mac 0x2bb650
{
    return CheckBoundObject<LuaTooltipAugmentation>(state, narg);
}

void LuaTooltipAugmentation::SetTooltipAugmentation(TooltipAugmentation* tooltip) // Mac 0x2bb2b0
{
    m_tooltip = tooltip;
}

bool LuaTooltipAugmentation::AttachTooltip(Control* control)                 // Mac 0x2bb280
{
    if (m_tooltip)
        return m_tooltip->AttachTo(control);
    return false;
}

void LuaTooltipAugmentation::OnFinalRelease()                               // Mac 0x2bb4c0
{
    LuaControlAugmentation::OnFinalRelease();
}

void LuaTooltipAugmentation::OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args) // Mac 0x2bb510, Win 0x459ec0
{
    LuaLayout::OnLayoutApplied(layout, args);
    String path;
    if (!layout->GetAttribute(String("clipper"), path))
        return;
    Control* anchor = 0;
    {
        LuaObject object = LookupLuaObject(*args.m_table, path);
        if (!object.IsNil()) {
            BindableLuaObject* bound = GetBoundObject(object);
            if (bound)
                if (LuaControl* control = dynamic_cast<LuaControl*>(bound))
                    anchor = control->GetTargetControl();
        }
    }
    m_tooltip->SetAnchor(anchor);
}

} // namespace LEGO
