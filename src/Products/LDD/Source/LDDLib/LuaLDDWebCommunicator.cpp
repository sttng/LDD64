// LEGO Digital Designer — restored source
// TU 399 "LuaLDDWebCommunicator.cpp": Mac 0x383c50-0x38432f, Win 0x6068c9-0x606cab.
#include "LuaLDDWebCommunicator.h"
#include "LuaBindings.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

const luaL_Reg kFunctions[] = {                                             // Mac 0x63ed50 (empty)
    { 0, 0 }
};

} // namespace

LuaObject LuaLDDWebCommunicator::CreateLuaObject(LDDWebCommunicator* communicator,
                                                 LuaState* state)          // Mac 0x383d20, Win 0x606ba0
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, communicator);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "LDDWebCommunicator");
    RefPointer<LuaLDDWebCommunicator> adapter(new LuaLDDWebCommunicator(communicator));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void RegisterLuaLDDWebCommunicator(LuaState* state)                         // Mac 0x383c50, Win 0x6068c9
{
    LuaObject luaClass = CreateLuaClass(state, "LDDWebCommunicator", "Object");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
