// LEGO Digital Designer — restored source
// TU 402 "LuaPreferencesObserver.cpp": Mac 0x38b270-0x38bacf, Win 0x46b310-0x46b6bf.
#include "LuaPreferencesObserver.h"
#include "LuaBindings.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

// PreferencesObserver.Create(self): binds a new observer to `self`. (name inferred)
int LuaPreferencesObserverCreate(lua_State* L)                             // Mac 0x38b2f0, Win 0x46b340
{
    LuaObject self = GetUnboundSelf(LuaState::CastState(L));
    AttachLuaObject(self, new LuaPreferencesObserver);
    return 0;
}

// self:Connect(): observes the application preferences. (name inferred)
int LuaPreferencesObserverConnect(lua_State* L)                            // Mac 0x38b3b0, Win 0x46b3f0
{
    LuaPreferencesObserver* observer = CheckBoundObject<LuaPreferencesObserver>(LuaState::CastState(L), 1);
    RefPointer<Preferences> preferences = Preferences::GetInstance();
    observer->Attach(preferences);
    return 0;
}

// self:Disconnect(). (name inferred)
int LuaPreferencesObserverDisconnect(lua_State* L)                         // Mac 0x38b690, Win 0x46b480
{
    LuaPreferencesObserver* observer = CheckBoundObject<LuaPreferencesObserver>(LuaState::CastState(L), 1);
    observer->Detach();
    return 0;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x756124
    { "Create", LuaPreferencesObserverCreate },
    { "Connect", LuaPreferencesObserverConnect },
    { "Disconnect", LuaPreferencesObserverDisconnect },
    { 0, 0 }
};

} // namespace

void LuaPreferencesObserver::OnPreferenceChanged(const String& key)       // Mac 0x38b8b0, Win 0x46b610
{
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnPreferenceChanged", false, 0, String(key));
}

void LuaPreferencesObserver::UnbindLuaObject()                             // Mac 0x38b930, Win 0x46b690
{
    Detach();
}

void RegisterLuaPreferencesObserver(LuaState* state)                       // Mac 0x38b270, Win 0x46b4c0
{
    LuaObject luaClass = CreateLuaClass(state, "PreferencesObserver", "Object");
    luaClass.Register(kFunctions, 0);
}

} // namespace LEGO
