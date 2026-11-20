// LEGO Digital Designer — restored source
// TU 383 "LuaBrickDatabase.cpp": Mac 0x371b30-0x37233f, Win 0x604acc-0x604f37.
#include "LuaBrickDatabase.h"
#include "LuaBindings.h"
#include "Version.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:GetBrickSetVersion() -> the db version (info.xml <Bricks version>) as text. (name inferred)
int LuaBrickDatabaseGetBrickSetVersion(lua_State* L)                         // Mac 0x371c00, Win 0x604d46
{
    LuaState* state = LuaState::CastState(L);
    BrickDatabase* database = CheckAdaptedLuaObject<LuaBrickDatabase>(state, 1); // (Win 0x604d04)
    Version version = database->GetVersion();
    String text;
    ToString(version, text);
    PushLuaString(state, text);
    return 1;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x7517b0
    { "GetBrickSetVersion", LuaBrickDatabaseGetBrickSetVersion },
    { 0, 0 }
};

} // namespace

LuaObject LuaBrickDatabase::CreateLuaObject(BrickDatabase* database, LuaState* state) // Mac 0x371d30, Win 0x604e2c
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, database);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "BrickDatabase");
    RefPointer<LuaBrickDatabase> adapter(new LuaBrickDatabase(database));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void RegisterLuaBrickDatabase(LuaState* state)                               // Mac 0x371b30, Win 0x604dac
{
    LuaObject luaClass = CreateLuaClass(state, "BrickDatabase", "Object");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
