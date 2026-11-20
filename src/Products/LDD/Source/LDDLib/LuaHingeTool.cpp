// LEGO Digital Designer — restored source
// TU 404 "LuaHingeTool.cpp": Mac 0x38c080-0x38cb9b, Win 0x64c05e-0x64c46f.
#include "LuaHingeTool.h"
#include "LuaBindings.h"
#include "HingeTool.h"
#include "LuaButtonControl.h"
#include "LuaTextInputControl.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

namespace {

// The HingeTool of the LuaTool bound to argument `narg`, or a Lua type error naming HingeTool.
// Mac 0x38ca60 (takes the LuaObject), Win 0x64c155. (name inferred)
HingeTool* CheckLuaHingeTool(LuaState* state, int narg)
{
    HingeTool* result = 0;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        BindableLuaObject* bound = GetBoundObject(object);
        LuaTool* luaTool = bound ? dynamic_cast<LuaTool*>(bound) : 0;
        BrickConnectSceneControlTool* tool = luaTool ? luaTool->GetAdaptedObject() : 0;
        result = tool ? dynamic_cast<HingeTool*>(tool) : 0;
        if (!result) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!result)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(HingeTool).name());
    return result;
}

#define LUA_HINGE_SETTER(function, setter, check, mac, win)                                 \
    int function(lua_State* L) /* mac, win */                                              \
    {                                                                                      \
        LuaState* state = LuaState::CastState(L);                                          \
        HingeTool* tool = CheckLuaHingeTool(state, 1);                                     \
        tool->setter(check(state, 2));                                                     \
        return 0;                                                                          \
    }

// self:SetMinusButton(button) …: (names = Lua strings)
LUA_HINGE_SETTER(LuaHingeToolSetMinusButton, SetMinusButton, CheckLuaButtonControl, 0x38c150, 0x64c1d9)
LUA_HINGE_SETTER(LuaHingeToolSetPlusButton, SetPlusButton, CheckLuaButtonControl, 0x38c1e0, 0x64c208)
LUA_HINGE_SETTER(LuaHingeToolSetTextInputControl, SetTextInputControl, CheckLuaTextInputControl, 0x38c270, 0x64c237)
LUA_HINGE_SETTER(LuaHingeToolSetMinusButton2, SetMinusButton2, CheckLuaButtonControl, 0x38c300, 0x64c266)
LUA_HINGE_SETTER(LuaHingeToolSetPlusButton2, SetPlusButton2, CheckLuaButtonControl, 0x38c390, 0x64c295)
LUA_HINGE_SETTER(LuaHingeToolSetTextInputControl2, SetTextInputControl2, CheckLuaTextInputControl, 0x38c420, 0x64c2c4)
LUA_HINGE_SETTER(LuaHingeToolSetMinusButton3, SetMinusButton3, CheckLuaButtonControl, 0x38c4b0, 0x64c2f3)
LUA_HINGE_SETTER(LuaHingeToolSetPlusButton3, SetPlusButton3, CheckLuaButtonControl, 0x38c540, 0x64c322)
LUA_HINGE_SETTER(LuaHingeToolSetTextInputControl3, SetTextInputControl3, CheckLuaTextInputControl, 0x38c5d0, 0x64c351)

#undef LUA_HINGE_SETTER

const luaL_Reg kFunctions[] = {                                             // Mac 0x756350
    { "SetMinusButton", LuaHingeToolSetMinusButton },
    { "SetPlusButton", LuaHingeToolSetPlusButton },
    { "SetTextInputControl", LuaHingeToolSetTextInputControl },
    { "SetMinusButton2", LuaHingeToolSetMinusButton2 },
    { "SetPlusButton2", LuaHingeToolSetPlusButton2 },
    { "SetTextInputControl2", LuaHingeToolSetTextInputControl2 },
    { "SetMinusButton3", LuaHingeToolSetMinusButton3 },
    { "SetPlusButton3", LuaHingeToolSetPlusButton3 },
    { "SetTextInputControl3", LuaHingeToolSetTextInputControl3 },
    { 0, 0 }
};

} // namespace

LuaHingeTool::LuaHingeTool(HingeTool* tool)
    : LuaTool(tool)
{
}

LuaObject LuaHingeTool::CreateLuaObject(HingeTool* tool, LuaState* state)    // Mac 0x38c660
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, tool);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "HingeTool");
    RefPointer<LuaHingeTool> adapter(new LuaHingeTool(tool));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void RegisterLuaHingeTool(LuaState* state)                                   // Mac 0x38c080, Win 0x64c380
{
    LuaObject luaClass = CreateLuaClass(state, "HingeTool", "Tool");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
