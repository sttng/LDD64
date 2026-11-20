// LEGO Digital Designer — restored source
// TU 379 "LuaBrickMode.cpp": Mac 0x3696f0-0x36a1eb, Win 0x64b5ef-0x64b627 (+ 0x64b820).
// See LuaBrickMode.h. The Mac static initialiser 0x36a170 is not code.
#include "LuaBrickMode.h"
#include "BrickMode.h"
#include "LuaBindings.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// The Lua table of `control` (LuaControl::GetControlTable), a new Control table when it has
// none; pushed. (inlined twice: Mac 0x3697c0 / 0x369890)
int PushControlTable(LuaState* state, Control* control)
{
    LuaObject table = LuaControl::GetControlTable(state, control);
    if (table.IsNil())
        table = LuaControl::CreateControlTable(state, control);
    table.Push();
    return 1;
}

// self:GetClientControl(). Mac 0x3697c0
int LuaBrickModeGetClientControl(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    BrickMode* mode = LuaBrickMode::CheckBrickMode(state, 1);
    return PushControlTable(state, mode->GetClientControl());
}

// self:GetToolbarControl(). Mac 0x369890
int LuaBrickModeGetToolbarControl(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    BrickMode* mode = LuaBrickMode::CheckBrickMode(state, 1);
    return PushControlTable(state, mode->GetToolbarControl());
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x7507d0
    { "GetClientControl", LuaBrickModeGetClientControl },
    { "GetToolbarControl", LuaBrickModeGetToolbarControl },
    { 0, 0 }
};

} // namespace

// NOTE (restoration): raises only after the LuaObject went out of scope (see CheckBoundObject).
BrickMode* LuaBrickMode::CheckBrickMode(LuaState* state, int narg)           // Mac 0x36a0a0
{
    BrickMode* mode = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaBrickMode* luaMode = bound ? dynamic_cast<LuaBrickMode*>(bound) : 0;
        if (luaMode)
            mode = luaMode->m_brickMode;
        if (!mode) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!mode)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(BrickMode).name());
    return mode;
}

void LuaBrickMode::OnFinalRelease()                                          // Mac 0x369ef0
{
    LuaMode::OnFinalRelease();
}

bool LuaBrickMode::Attach(Control* control, const String& name)             // Mac 0x369f80, Win 0x64b5ef
{
    if (!LuaMode::Attach(control, name))
        return false;
    Control* target = GetTargetControl();
    m_brickMode = target ? dynamic_cast<BrickMode*>(target) : 0;
    return true;
}

void LuaBrickMode::Detach()                                                  // Mac 0x36a060
{
    m_brickMode = 0;
    LuaMode::Detach();
}

void RegisterLuaBrickMode(LuaState* state)                                   // Mac 0x3696f0, Win 0x64b820
{
    LuaObject luaClass = CreateLuaClass(state, "BrickMode", "Mode");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
