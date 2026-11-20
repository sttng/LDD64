// LEGO Digital Designer — restored source
// TU 398 "LuaLDDApplicationObserver.cpp": Mac 0x3830b0-0x383c4f, Win 0x64aab5-0x64ae5f.
#include "LuaLDDApplicationObserver.h"
#include "LuaBindings.h"
#include "Brand.h"
#include "Path.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// LDDApplicationObserver.Create(self): binds a new observer to `self`.
int LuaLDDApplicationObserverCreate(lua_State* L)                            // Mac 0x383130, Win 0x64ab5c
{
    LuaObject self = GetUnboundSelf(LuaState::CastState(L));
    AttachLuaObject(self, new LuaLDDApplicationObserver);
    return 0;
}

// self:Connect(): observes the application.
int LuaLDDApplicationObserverConnect(lua_State* L)                           // Mac 0x3831f0, Win 0x64ae40
{
    LuaLDDApplicationObserver* observer =
        CheckBoundObject<LuaLDDApplicationObserver>(LuaState::CastState(L), 1); // (Mac 0x383a40, Win 0x64ac57)
    observer->Attach(LDDApplication::GetInstance());
    return 0;
}

// self:Disconnect()
int LuaLDDApplicationObserverDisconnect(lua_State* L)                        // Mac 0x383490, Win 0x64ae1a
{
    LuaLDDApplicationObserver* observer =
        CheckBoundObject<LuaLDDApplicationObserver>(LuaState::CastState(L), 1);
    observer->Detach();
    return 0;
}

const luaL_Reg kFunctions[] = {                                             // Mac 0x7553d4
    { "Create", LuaLDDApplicationObserverCreate },
    { "Connect", LuaLDDApplicationObserverConnect },
    { "Disconnect", LuaLDDApplicationObserverDisconnect },
    { 0, 0 }
};

} // namespace

void LuaLDDApplicationObserver::OnRecentModelsChanged(const Path& path)     // Mac 0x3836b0, Win 0x64accb
{
    String text = path.GetPath(true);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnRecentModelsChanged", false, 0, text); // (Mac 0x383740, Win 0x46b730)
}

void LuaLDDApplicationObserver::OnBrandChanged(Brand brand)                  // Mac 0x3837e0, Win 0x64ad15
{
    String name;
    ToString(brand, name);
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnBrandChanged", false, 0, String(name));
}

void LuaLDDApplicationObserver::OnExtendedTooltipChanged(bool enabled)       // Mac 0x383890, Win 0x64ad85
{
    LuaObject self = GetLuaObject(this);
    CallLuaMethod(self, "OnExtendedTooltipChanged", false, 0, enabled);     // (Mac 0x196870)
}

void LuaLDDApplicationObserver::OnOnlineStatusChanged(bool online)           // Mac 0x383920, Win 0x64ada9
{
    LuaObject self = GetLuaObject(this);
    CallLuaMethod(self, "OnOnlineStatusChanged", false, 0, online);
}

void LuaLDDApplicationObserver::UnbindLuaObject()                            // Mac 0x3839b0, Win 0x62c020
{
    Detach();
}

void RegisterLuaLDDApplicationObserver(LuaState* state)                      // Mac 0x3830b0, Win 0x64ae68
{
    LuaObject luaClass = CreateLuaClass(state, "LDDApplicationObserver", "Object");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
