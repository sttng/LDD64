// LEGO Digital Designer — restored source
// TU 319 "LuaControlManagerObserver.cpp": Mac 0x2c8810-0x2c98bb, Win 0x46a310-0x46a85f.
#include "LuaControlManagerObserver.h"
#include "LuaBindings.h"
#include "LuaControl.h"
#include "KeyNames.h"

using namespace LuaPlus;

namespace LEGO {

namespace {

ControlManager* UpvalueManager(lua_State* L)
{
    return (ControlManager*)lua_touserdata(L, lua_upvalueindex(1));
}

// ControlManagerObserver.Create(self). Mac 0x2c88b0, Win 0x46a340
int LuaControlManagerObserverCreate(lua_State* L)
{
    LuaObject self = GetUnboundSelf(LuaState::CastState(L));
    AttachLuaObject(self, new LuaControlManagerObserver);
    return 0;
}

// self:Connect(): observes the manager. Mac 0x2c8970, Win 0x46a3f0
int LuaControlManagerObserverConnect(lua_State* L)
{
    LuaControlManagerObserver* observer =
        CheckBoundObject<LuaControlManagerObserver>(LuaState::CastState(L), 1);   // Mac 0x2c9320
    observer->Attach(UpvalueManager(L));
    return 0;
}

// self:Disconnect(). Mac 0x2c8c10, Win 0x46a420
int LuaControlManagerObserverDisconnect(lua_State* L)
{
    LuaControlManagerObserver* observer =
        CheckBoundObject<LuaControlManagerObserver>(LuaState::CastState(L), 1);
    observer->Detach();
    return 0;
}

// self:IsModal(): the manager has a modal control. Mac 0x2c8c70, Win 0x46a460
int LuaControlManagerObserverIsModal(lua_State* L)
{
    lua_pushboolean(L, UpvalueManager(L)->GetModalControl() != 0);
    return 1;
}

const luaL_Reg kFunctions[] = {                                            // Mac 0x749424
    { "Create", LuaControlManagerObserverCreate },
    { "Connect", LuaControlManagerObserverConnect },
    { "Disconnect", LuaControlManagerObserverDisconnect },
    { "IsModal", LuaControlManagerObserverIsModal },
    { 0, 0 }
};

} // namespace

// CallLuaMethod<const char*, UInt32, bool> (Mac 0x2c93f0).
void LuaControlManagerObserver::OnManagerKeyPressed(UInt32 key, UInt32 modifiers, bool repeat) // Mac 0x2c8e70, Win 0x46a600
{
    const char* name = StringToKey::GetName((Key)key);
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnManagerKeyPressed", false, 0, name, (unsigned int)modifiers, repeat);
}

// CallLuaMethod<const char*, UInt32> (Mac 0x2c9510).
void LuaControlManagerObserver::OnManagerKeyReleased(UInt32 key, UInt32 modifiers) // Mac 0x2c8f10, Win 0x46a640
{
    const char* name = StringToKey::GetName((Key)key);
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnManagerKeyReleased", false, 0, name, (unsigned int)modifiers);
}

// Helper Mac 0x2c90f0 -> CallLuaMethod<bool, LuaObject> (Mac 0x2c9620).
void LuaControlManagerObserver::OnManagerModalControl(bool modal, Control* control) // Mac 0x2c8fb0, Win 0x46a680
{
    RefPointer<LuaControl> object = LuaControl::FromControl(control);
    LuaObject table(m_luaState);
    if (object)
        table = GetLuaObject(static_cast<BindableLuaObject*>(object.Get()));
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    if (!self.IsTable())
        return;
    LuaObject function = self.GetByName("OnManagerModalControl");   // (GetByName, see CallLuaMethod)
    if (function.IsNil())
        return;
    LuaCall call(function);
    call << self << modal << table;
    call.m_state->PCall(call.m_numArgs, 0);
}

// CallLuaMethod<bool, UInt32> (Mac 0x2c9730).
void LuaControlManagerObserver::OnManagerWindowKeyboardFocusChanged(bool focused, UInt32 modifiers) // Mac 0x2c91a0, Win 0x46a790
{
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "OnManagerWindowKeyboardFocusChanged", false, 0, focused, (unsigned int)modifiers);
}

void LuaControlManagerObserver::OnManagerWindowEnterMenuLoop()             // Mac 0x2c9230, Win 0x46a7d0
{
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnManagerWindowEnterMenuLoop", false, 0);
}

void LuaControlManagerObserver::OnManagerWindowExitMenuLoop()              // Mac 0x2c9260, Win 0x46a800
{
    CallLuaMethod(static_cast<BindableLuaObject*>(this), "OnManagerWindowExitMenuLoop", false, 0);
}

void LuaControlManagerObserver::UnbindLuaObject()                          // Mac 0x2c9290, Win 0x46a830
{
    Detach();
}

void RegisterLuaControlManagerObserver(LuaState* state, ControlManager* manager) // Mac 0x2c8810, Win 0x46a490
{
    LuaObject luaClass = CreateLuaClass(state, "ControlManagerObserver", "Object");
    lua_pushlightuserdata(state->GetCState(), manager);
    luaClass.Register(kFunctions, 1);
}

} // namespace LEGO
