// LEGO Digital Designer — restored source
// TU 279 "LuaBrickConnectSceneManagerObserver.cpp": Mac 0x291ca0-0x29304b, Win 0x604f38-0x605c46.
#include "LuaBrickConnectSceneManagerObserver.h"
#include "LuaBindings.h"
#include "LuaTool.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// self:GetCurrentTool() -> the Lua table of the active tool (nil without one / unbound).
int LuaSceneManagerGetCurrentTool(lua_State* L)                              // Mac 0x291d70, Win 0x6058bb
{
    LuaState* state = LuaState::CastState(L);
    BrickConnectSceneManager* manager = CheckAdaptedLuaObject<LuaSceneManager>(state, 1); // (Mac 0x292d40)
    LuaObject table = GetObjectMapEntry(state, manager->GetActiveTool());
    table.Push();
    return 1;
}

// self:SetCurrentTool(tool | nil)
int LuaSceneManagerSetCurrentTool(lua_State* L)                              // Mac 0x291e00, Win 0x605a60
{
    LuaState* state = LuaState::CastState(L);
    BrickConnectSceneManager* manager = CheckAdaptedLuaObject<LuaSceneManager>(state, 1);
    manager->SetActiveTool(OptAdaptedLuaObject<LuaTool>(state, 2, 0));       // (Mac 0x292e10)
    return 0;
}

// self:SetDefaultTool(tool | nil)
int LuaSceneManagerSetDefaultTool(lua_State* L)                              // Mac 0x291e50, Win 0x605a91
{
    LuaState* state = LuaState::CastState(L);
    BrickConnectSceneManager* manager = CheckAdaptedLuaObject<LuaSceneManager>(state, 1);
    manager->SetDefaultTool(OptAdaptedLuaObject<LuaTool>(state, 2, 0));
    return 0;
}

const luaL_Reg kSceneManagerFunctions[] = {                                 // Mac 0x73f670
    { "GetCurrentTool", LuaSceneManagerGetCurrentTool },
    { "SetCurrentTool", LuaSceneManagerSetCurrentTool },
    { "SetDefaultTool", LuaSceneManagerSetDefaultTool },
    { 0, 0 }
};

// SceneManagerObserver.Create(self): binds a new observer to `self`.
int LuaSceneManagerObserverCreate(lua_State* L)                              // Mac 0x2922d0, Win 0x60525a
{
    LuaObject self = GetUnboundSelf(LuaState::CastState(L));
    AttachLuaObject(self, new LuaBrickConnectSceneManagerObserver);
    return 0;
}

// self:Connect(): observes the scene manager of the registration (upvalue 1).
int LuaSceneManagerObserverConnect(lua_State* L)                             // Mac 0x292390, Win 0x605b8f
{
    LuaBrickConnectSceneManagerObserver* observer =
        CheckBoundObject<LuaBrickConnectSceneManagerObserver>(LuaState::CastState(L), 1); // (Mac 0x292f00)
    BrickConnectSceneManager* manager =
        static_cast<BrickConnectSceneManager*>(lua_touserdata(L, lua_upvalueindex(1)));
    observer->Attach(manager);
    return 0;
}

// self:Disconnect()
int LuaSceneManagerObserverDisconnect(lua_State* L)                          // Mac 0x292630, Win 0x605b69
{
    LuaBrickConnectSceneManagerObserver* observer =
        CheckBoundObject<LuaBrickConnectSceneManagerObserver>(LuaState::CastState(L), 1);
    observer->Detach();
    return 0;
}

const luaL_Reg kObserverFunctions[] = {                                     // Mac 0x73f764
    { "Create", LuaSceneManagerObserverCreate },
    { "Connect", LuaSceneManagerObserverConnect },
    { "Disconnect", LuaSceneManagerObserverDisconnect },
    { 0, 0 }
};

} // namespace

LuaObject LuaSceneManager::CreateLuaObject(BrickConnectSceneManager* manager, LuaState* state) // Mac 0x291ea0, Win 0x60590e
{
    bool unbound;
    {
        LuaObject entry = GetObjectMapEntry(state, manager);
        unbound = entry.IsNil();
    }
    if (!unbound)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "SceneManager");
    RefPointer<LuaSceneManager> adapter(new LuaSceneManager(manager));
    AttachLuaObject(table, adapter.Get());
    return table;
}

void LuaBrickConnectSceneManagerObserver::OnSceneControlChanged(BrickConnectSceneControl* /*previous*/,
                                                                BrickConnectSceneControl* /*current*/) // Mac 0x292850, Win 0x604f38
{
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnSceneChanged", false, 0);
}

void LuaBrickConnectSceneManagerObserver::OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                                              BrickConnectSceneControlTool* current) // Mac 0x292880, Win 0x6055be
{
    LuaObject previousTable = GetObjectMapEntry(m_luaState, previous);
    LuaObject currentTable = GetObjectMapEntry(m_luaState, current);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnToolChanged", false, 0,
                  previousTable, currentTable);                            // (Mac 0x2929a0, Win 0x462850)
}

void LuaBrickConnectSceneManagerObserver::UnbindLuaObject()                  // Mac 0x292a70, Win 0x633a48
{
    Detach();
}

void RegisterLuaSceneManager(LuaState* state)                                // Mac 0x291ca0, Win 0x605ac2
{
    LuaObject luaClass = CreateLuaClass(state, "SceneManager", "Object");
    luaClass.Register(kSceneManagerFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

void RegisterLuaSceneManagerObserver(LuaState* state, BrickConnectSceneManager* manager) // Mac 0x292230, Win 0x605bc0
{
    LuaObject luaClass = CreateLuaClass(state, "SceneManagerObserver", "Object");
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register(kObserverFunctions, 1);
}

} // namespace LEGO
