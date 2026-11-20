// LEGO Digital Designer — restored source
// TU 308 "LuaControlAugmentation.cpp": Mac 0x2b6b70-0x2b6e6b, Win 0x45dc60-0x45dd7f.
// The Mac static initialiser 0x2b6df0 (iostream + a zeroed 24-byte header static) is not code.
#include "LuaControlAugmentation.h"
#include "LuaBindings.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

const luaL_Reg kFunctions[] = {                                            // Mac 0x63bb80 (empty)
    { 0, 0 }
};

} // namespace

void RegisterLuaControlAugmentation(LuaState* state)                         // Mac 0x2b6ba0, Win 0x45dc90
{
    LuaObject luaClass = CreateLuaClass(state, "ControlAugmentation", "Layout");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
