// LEGO Digital Designer — restored source
// TU 280 "LuaTool.cpp": Mac 0x293050-0x293a1b, Win 0x64bf3b-0x64c88f.
#include "LuaTool.h"
#include "LuaBindings.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

const luaL_Reg kFunctions[] = {                                             // Mac 0x63a870 (empty)
    { 0, 0 }
};

} // namespace

LuaTool::LuaTool(BrickConnectSceneControlTool* tool)                         // Mac 0x2932b0, Win 0x64c6cf
    : AdapterBindableLuaObject<BrickConnectSceneControlTool>(tool)
{
}

LuaTool::~LuaTool()                                                          // Mac 0x293320, Win 0x64bf3b
{
}

LuaObject LuaTool::CreateLuaObject(BrickConnectSceneControlTool* tool, LuaState* state) // Mac 0x293120, Win 0x64c747
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, tool);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "Tool");
    RefPointer<LuaTool> adapter(new LuaTool(tool));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void RegisterLuaTool(LuaState* state)                                        // Mac 0x293050, Win 0x64c470
{
    LuaObject luaClass = CreateLuaClass(state, "Tool", "Object");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
