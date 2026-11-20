// LEGO Digital Designer — restored source
// TU 392 "LuaBrickPaletteControl.cpp": Mac 0x37d080-0x37daab, Win 0x64afd5-0x64b13a.
#include "LuaBrickPaletteControl.h"
#include "LuaBindings.h"
#include "BrickPaletteControl.h"
#include "BrickPalette.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:GetNumberOfBags() -> number of bags of the palette's current brick set (0 without one).
int LuaBrickPaletteControlGetNumberOfBags(lua_State* L)                      // Mac 0x37d150, Win 0x64b318
{
    LuaState* state = LuaState::CastState(L);
    BrickPaletteControl* control = CheckLuaBrickPaletteControl(state, 1);
    BrickPalette* palette = control->GetBrickPalette();
    double count = palette ? (double)palette->GetBagCount() : 0.0;
    state->PushNumber(count);
    return 1;
}

// self:SetBrickFilterDescription(text)
// NOTE: checks only the binding (LuaBrickPaletteControl), not that it has a palette control
// (a null control is used as is; both builds).
int LuaBrickPaletteControlSetBrickFilterDescription(lua_State* L)            // Mac 0x37d1f0, Win 0x64b231
{
    LuaBrickPaletteControl* luaControl =
        CheckBoundObject<LuaBrickPaletteControl>(LuaState::CastState(L), 1); // (Mac 0x37d960)
    BrickPaletteControl* control = luaControl->GetPaletteControl();
    String description = CheckLuaString(L, 2);
    control->SetBrickFilterDescription(description);
    return 0;
}

// self:ClearFilter()
int LuaBrickPaletteControlClearFilter(lua_State* L)                          // Mac 0x37d270, Win 0x64b36d
{
    BrickPaletteControl* control = CheckLuaBrickPaletteControl(LuaState::CastState(L), 1);
    control->ClearFilter();
    return 0;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x753cc0
    { "GetNumberOfBags", LuaBrickPaletteControlGetNumberOfBags },
    { "SetBrickFilterDescription", LuaBrickPaletteControlSetBrickFilterDescription },
    { "ClearFilter", LuaBrickPaletteControlClearFilter },
    { 0, 0 }
};

} // namespace

BrickPaletteControl* CheckLuaBrickPaletteControl(LuaState* state, int narg)   // Mac 0x37d890, Win 0x64b0c6 (form on a LuaObject)
{
    BrickPaletteControl* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaBrickPaletteControl* luaControl = bound ? dynamic_cast<LuaBrickPaletteControl*>(bound) : 0;
        if (luaControl)
            result = luaControl->GetPaletteControl();
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(BrickPaletteControl).name());
    return result;
}

LuaBrickPaletteControl::LuaBrickPaletteControl()                             // Mac 0x37d2b0
    : m_paletteControl(0), m_value50(0)
{
}

bool LuaBrickPaletteControl::Attach(Control* control, const String& name)   // Mac 0x37d770, Win 0x64b08e
{
    if (!LuaControl::Attach(control, name))
        return false;
    Control* target = GetTargetControl();
    m_paletteControl = target ? dynamic_cast<BrickPaletteControl*>(target) : 0;
    return true;
}

void LuaBrickPaletteControl::Detach()                                        // Mac 0x37d850
{
    m_paletteControl = 0;
    LuaControl::Detach();
}

void RegisterLuaBrickPaletteControl(LuaState* state)                         // Mac 0x37d080, Win 0x64b38f
{
    LuaObject luaClass = CreateLuaClass(state, "BrickPaletteControl", "Control");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
