// LEGO Digital Designer — restored source
// TU 307 "LuaButtonSoundAugmentation.cpp": Mac 0x2b5ed0-0x2b6b6b, Win 0x45bf70-0x45c35f.
// The Mac static initialiser 0x2b6af0 is not code.
#include "LuaButtonSoundAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// ButtonSoundAugmentation.Create(self [, control]) (upvalue 1: the SimpleSound).
// Mac 0x2b6580, Win 0x45c080
int LuaButtonSoundAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaButtonSoundAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        SimpleSound* sound = (SimpleSound*)lua_touserdata(L, lua_upvalueindex(1));
        object = new LuaButtonSoundAugmentation(sound);
        AttachLuaObject(self, object);
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("buttonSound"));
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x63bb40 (empty)
    { 0, 0 }
};

} // namespace

void LuaButtonSoundAugmentation::UnbindLuaObject()                           // Mac 0x2b6ad0
{
    Detach();
}

void LuaButtonSoundAugmentation::OnFinalRelease()                            // Mac 0x2b69d0
{
    ButtonSoundAugmentation::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

void RegisterLuaButtonSoundAugmentation(LuaState* state, SimpleSound* sound)  // Mac 0x2b6440, Win 0x45c1c0
{
    LuaObject luaClass = CreateLuaClass(state, "ButtonSoundAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
    if (!sound) {
        luaClass.Register("Create", LuaReturnNothing, 0);
        luaClass.Register("Layout", LuaReturnNothing, 0);
        luaClass.Register("Connect", LuaReturnNothing, 0);
        luaClass.Register("Disconnect", LuaReturnNothing, 0);
    } else {
        lua_pushlightuserdata(state->GetCState(), sound);
        luaClass.Register("Create", LuaButtonSoundAugmentationCreate, 1);
    }
}

} // namespace LEGO
