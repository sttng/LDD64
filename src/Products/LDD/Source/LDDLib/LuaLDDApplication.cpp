// LEGO Digital Designer — restored source
// TU 397 "LuaLDDApplication.cpp": Mac 0x3824a0-0x3830ab, Win 0x6062fa-0x6068c8.
// The Mac static initialiser 0x383030 also zeroes a 24-byte static of an included header
// (0x77f340: words 0, 0, 0, 0x1000000, 0, 0; the same in TUs 278, 279, 392, 404) — not code.
#include "LuaLDDApplication.h"
#include "LuaBindings.h"
#include "LuaSceneControl.h"
#include "BrickConnectSceneControl.h"
#include "ModelLoaderHandler.h"
#include "Brand.h"
#include "Path.h"
#include "Platform.h"

using namespace LuaPlus;

namespace LEGO {

class Command;

// The Command of argument `narg`: the adapted command of a LuaCommandAdapter, or a bound object
// that is a Command (LuaCommand); a Lua type error naming Command otherwise. Home LuaCommand.cpp
// (TU 306): Mac 0x2b5cd0 (takes the LuaObject), Win 0x44ade0. (name inferred)
Command* CheckLuaCommand(LuaPlus::LuaState* state, int narg);

namespace {

// self:SetBrand(name): unknown names are ignored.
int LuaLDDApplicationSetBrand(lua_State* L)                                  // Mac 0x382570, Win 0x606574
{
    LuaState* state = LuaState::CastState(L);
    LDDApplication* application = CheckAdaptedLuaObject<LuaLDDApplication>(state, 1); // (Mac 0x382f60, Win 0x606532)
    Brand brand;
    bool parsed;
    {
        String name = CheckLuaString(state, 2);
        UInt32 position = 0;
        parsed = FromString(brand, name, position);
    }
    if (parsed)
        application->SetBrand(brand);
    return 0;
}

// self:GetBrand() -> name
int LuaLDDApplicationGetBrand(lua_State* L)                                  // Mac 0x382620, Win 0x6065d5
{
    LuaState* state = LuaState::CastState(L);
    LDDApplication* application = CheckAdaptedLuaObject<LuaLDDApplication>(state, 1);
    Brand brand = application->GetBrand();
    String name;
    ToString(brand, name);
    PushLuaString(state, name);
    return 1;
}

// self:SetEditCommand(command)
int LuaLDDApplicationSetEditCommand(lua_State* L)                            // Mac 0x3826b0, Win 0x606637
{
    LuaState* state = LuaState::CastState(L);
    LDDApplication* application = CheckAdaptedLuaObject<LuaLDDApplication>(state, 1);
    Command* command = CheckLuaCommand(state, 2);
    application->SetEditCommand(command);
    return 0;
}

// self:Import(fileName, sceneControl): imports the file `fileName` of the system directory 9
// into the scene control (its ModelLoaderHandler).
int LuaLDDApplicationImport(lua_State* L)                                    // Mac 0x382740, Win 0x606666
{
    LuaState* state = LuaState::CastState(L);
    LDDApplication* application = CheckAdaptedLuaObject<LuaLDDApplication>(state, 1);
    String name = CheckLuaString(state, 2);
    BrickConnectSceneControl* control = CheckLuaSceneControl(state, 3);
    Path path(name, Platform::GetInstance()->GetSystemDirectory(9, true).Get());
    application->Import(path, control);                                      // (Win: null-checked base conversion)
    return 0;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x755260
    { "SetBrand", LuaLDDApplicationSetBrand },
    { "GetBrand", LuaLDDApplicationGetBrand },
    { "SetEditCommand", LuaLDDApplicationSetEditCommand },
    { "Import", LuaLDDApplicationImport },
    { 0, 0 }
};

} // namespace

LuaObject LuaLDDApplication::CreateLuaObject(LDDApplication* application, LuaState* state) // Mac 0x382990, Win 0x6067bd
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, application);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "LDDApplication");
    RefPointer<LuaLDDApplication> adapter(new LuaLDDApplication(application));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void RegisterLuaLDDApplication(LuaState* state)                              // Mac 0x3824a0, Win 0x606713
{
    LuaObject luaClass = CreateLuaClass(state, "LDDApplication", "Object");
    luaClass.Register(kFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

} // namespace LEGO
