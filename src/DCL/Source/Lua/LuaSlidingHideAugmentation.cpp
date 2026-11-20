// LEGO Digital Designer — restored source
// TU 389 "LuaSlidingHideAugmentation.cpp": Mac 0x37a630-0x37ad6b, Win 0x458df0-0x45915f.
// The Mac static initialiser 0x37acf0 is not code.
#include "LuaSlidingHideAugmentation.h"
#include "LuaBindings.h"
#include "LuaControl.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// SlidingHideAugmentation.Create(self [, control]). Mac 0x37a790, Win 0x458ef0
int LuaSlidingHideAugmentationCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    RefPointer<LuaSlidingHideAugmentation> object;
    {
        LuaObject self = GetUnboundSelf(state);
        object = new LuaSlidingHideAugmentation;
        AttachLuaObject(self, object);
    }
    Control* control = OptLuaControl(state, 2, 0);
    if (control)
        object->Attach(control, String("SlidingHideAugmentation"));
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x63e9f0 (empty)
    { 0, 0 }
};

} // namespace

void LuaSlidingHideAugmentation::UnbindLuaObject()                           // Mac 0x37acd0
{
    Detach();
}

void LuaSlidingHideAugmentation::OnFinalRelease()                            // Mac 0x37abd0
{
    SlidingHideAugmentation::OnFinalRelease();
    LuaControlAugmentation::OnFinalRelease();
}

void RegisterLuaSlidingHideAugmentation(LuaState* state)                     // Mac 0x37a6e0, Win 0x459020
{
    LuaObject luaClass = CreateLuaClass(state, "SlidingHideAugmentation", "ControlAugmentation");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("Create", LuaSlidingHideAugmentationCreate, 0);
}

} // namespace LEGO
