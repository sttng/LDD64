// LEGO Digital Designer — restored source
// LEGO::LuaBrickDatabase — Lua class "BrickDatabase" (base Object): the Lua twin of the
//   BrickDatabase (an AdapterBindableLuaObject<BrickDatabase>). Lua: BrickDatabase:
//   GetBrickSetVersion() -> version string; New / Create do nothing (instances only come from
//   CreateLuaObject).
//
// TU 383 "LuaBrickDatabase.cpp": Mac 0x371b30-0x37233f, Win 0x604acc-0x604f37. In LDDLib (name
// inferred: the Win code lies with the other application-object bindings).
// RTTI si AdapterBindableLuaObject<BrickDatabase>. Mac vtable 0x7517cc {D1 0x371ec0, D0
// 0x371fb0, BindLuaObject 0x3720c0, UnbindLuaObject 0x3720e0}, Object-in 0x7517f4
// {0x371f30, 0x372030, …}. Win 0x9ce330 {0x64bf4e, 0x604a66}, Object-in 0x9ce31c {sdd 0x604f19 …}.
// Mac sizeof 0x14, Win 0x18 (see AdapterBindableLuaObject.h). Function names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "BrickDatabase.h"

namespace LEGO {

class LuaBrickDatabase : public AdapterBindableLuaObject<BrickDatabase> {
public:
    // inlined in CreateLuaObject (Mac 0x371d30; Win 0x604c8c)
    explicit LuaBrickDatabase(BrickDatabase* database)
        : AdapterBindableLuaObject<BrickDatabase>(database) {}
    virtual ~LuaBrickDatabase() {}                                          // Mac 0x371ec0/0x371fb0, Win 0x604b80 / sdd 0x604f19

    // A new BrickDatabase instance table bound to a new LuaBrickDatabase for `database`.
    // NOTE: when `database` already has a Lua table (objectMap entry) the result is nil, not
    // that table (both builds).
    static LuaPlus::LuaObject CreateLuaObject(BrickDatabase* database,
                                              LuaPlus::LuaState* state);     // Mac 0x371d30, Win 0x604e2c
};

// Registers the Lua class BrickDatabase {GetBrickSetVersion, New = Create = no-op}.
void RegisterLuaBrickDatabase(LuaPlus::LuaState* state);                    // Mac 0x371b30, Win 0x604dac

} // namespace LEGO
