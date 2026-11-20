// LEGO Digital Designer — restored source
// TU 306 "LuaCommand.cpp": Mac 0x2b45c0-0x2b5ecf, Win 0x449cb0-0x44ad6f (+ 0x44ade0, 0x44b0c0):
// LuaCommand, LuaCommandAdapter and the Command helpers. Mac static initialiser 0x2b5e90
// (iostream) is not code.
#include "LuaCommand.h"
#include "LuaCommandAdapter.h"
#include "LuaBindings.h"
#include "AdapterBindableLuaObject.h"

#include <typeinfo>

using namespace LuaPlus;

namespace LEGO {

typedef AdapterBindableLuaObjectBase<Command, BindableLuaObject> CommandAdapterBase;

namespace {

// The Command of a bound object: an adapter's command, or the object itself (LuaCommand).
// (inlined in Mac 0x2b5650 / 0x2b5cd0)
Command* BoundCommand(LuaObject& object)
{
    BindableLuaObject* bound = GetBoundObject(object);
    if (!bound)
        return 0;
    if (CommandAdapterBase* adapter = dynamic_cast<CommandAdapterBase*>(bound))
        return adapter->GetAdaptedObject();
    return dynamic_cast<Command*>(bound);
}

const char kCommandLua[] =                                                  // Mac 0x2b46e0 (string)
    "function Command:New(enabled,toggleable)\n"
    "\tlocal new_inst = {}\n"
    "\tsetmetatable( new_inst, self.classMetatable )\n"
    "\tnew_inst:Create(enabled, toggleable)\n"
    "\treturn new_inst\n"
    "end\n"
    "function Command:Do()end\n"
    "function Command:UpdateEnable() self:SetEnable(true) end\n";

// ---- Command (LuaCommand) ----

// Command.Create(self, [enabled = false], [toggleable = false]). Mac 0x2b4780
int LuaCommandCreate(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaObject self = GetUnboundSelf(state);
    bool enabled = OptLuaBoolean(L, 2, false);
    bool toggleable = OptLuaBoolean(L, 3, false);
    AttachLuaObject(self, new LuaCommand(enabled, toggleable));
    return 0;
}

// self:SetEnable([enable = true]). Mac 0x2b48c0
int LuaCommandSetEnable(lua_State* L)
{
    LuaCommand* command = LuaCommand::Check(LuaState::CastState(L), 1);
    command->SetEnable(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetEnable(). Mac 0x2b4920
int LuaCommandGetEnable(lua_State* L)
{
    LuaCommand* command = LuaCommand::Check(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->GetEnable());
    return 1;
}

// self:GetToggleable(). Mac 0x2b4970
int LuaCommandGetToggleable(lua_State* L)
{
    LuaCommand* command = LuaCommand::Check(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->IsToggleable());
    return 1;
}

// self:GetToggled(). Mac 0x2b49c0
int LuaCommandGetToggled(lua_State* L)
{
    LuaCommand* command = LuaCommand::Check(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->IsToggled());
    return 1;
}

// self:Toggle([toggled]): no argument (or nil) inverts. Mac 0x2b4a10
int LuaCommandToggle(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    LuaCommand* command = LuaCommand::Check(state, 1);
    bool toggled;
    if (lua_type(L, 2) < 1)
        toggled = !command->IsToggled();
    else
        toggled = CheckLuaBoolean(state, 2);
    command->SetToggled(toggled);
    return 0;
}

const luaL_Reg kCommandFunctions[] = {                                     // Mac 0x7453ec
    { "Create", LuaCommandCreate },
    { "SetEnable", LuaCommandSetEnable },
    { "GetEnable", LuaCommandGetEnable },
    { "GetToggleable", LuaCommandGetToggleable },
    { "GetToggled", LuaCommandGetToggled },
    { "Toggle", LuaCommandToggle },
    { 0, 0 }
};

// ---- CommandAdapter ----

// self:Do(). Mac 0x2b4eb0
int LuaCommandAdapterDo(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    command->Do();
    return 0;
}

// self:UpdateCommandEnable(). Mac 0x2b4f30
int LuaCommandAdapterUpdateCommandEnable(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    command->UpdateEnable();
    return 0;
}

// self:SetEnable([enable = true]). Mac 0x2b4fb0
int LuaCommandAdapterSetEnable(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    command->SetEnable(OptLuaBoolean(L, 2, true));
    return 0;
}

// self:GetEnable(). Mac 0x2b5050
int LuaCommandAdapterGetEnable(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->GetEnable());
    return 1;
}

// self:GetToggleable(). Mac 0x2b50e0
int LuaCommandAdapterGetToggleable(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->IsToggleable());
    return 1;
}

// self:GetToggled(). Mac 0x2b5170
int LuaCommandAdapterGetToggled(lua_State* L)
{
    Command* command = CheckLuaCommand(LuaState::CastState(L), 1);
    lua_pushboolean(L, command->IsToggled());
    return 1;
}

// self:Toggle([toggled]). Mac 0x2b5200
int LuaCommandAdapterToggle(lua_State* L)
{
    LuaState* state = LuaState::CastState(L);
    Command* command = CheckLuaCommand(state, 1);
    bool toggled;
    if (lua_type(L, 2) < 1)
        toggled = !command->IsToggled();
    else
        toggled = CheckLuaBoolean(state, 2);
    command->SetToggled(toggled);
    return 0;
}

const luaL_Reg kAdapterFunctions[] = {                                     // Mac 0x745424
    { "Do", LuaCommandAdapterDo },
    { "UpdateCommandEnable", LuaCommandAdapterUpdateCommandEnable },
    { "SetEnable", LuaCommandAdapterSetEnable },
    { "GetEnable", LuaCommandAdapterGetEnable },
    { "GetToggleable", LuaCommandAdapterGetToggleable },
    { "GetToggled", LuaCommandAdapterGetToggled },
    { "Toggle", LuaCommandAdapterToggle },
    { 0, 0 }
};

} // namespace

// ---- LuaCommand ----

// CallLuaMethod<int> (Mac 0x2b5d90, Win 0x44acf0): the toggle state goes to Lua as a number
// (0 / 1; LuaCall operator<<(int) Mac 0x404a9e -> lua_pushnumber), not a boolean.
void LuaCommand::Do()                                                        // Mac 0x2b45c0, Win 0x449ce0
{
    int toggled = m_toggled;
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "Do", false, 0, toggled);
}

void LuaCommand::UpdateEnable()                                              // Mac 0x2b4650, Win 0x449d20
{
    int toggled = m_toggled;
    LuaObject self = GetLuaObject(static_cast<BindableLuaObject*>(this));
    CallLuaMethod(self, "UpdateEnable", false, 0, toggled);
}

void LuaCommand::UnbindLuaObject()                                           // Mac 0x2b4da0, Win 0x44a110
{
    SetEnable(false);
}

LuaCommand* LuaCommand::Check(LuaState* state, int narg)                     // Mac 0x2b5c00, Win 0x44ab60
{
    return CheckBoundObject<LuaCommand>(state, narg);
}

void RegisterLuaCommand(LuaState* state)                                     // Mac 0x2b46e0, Win 0x449f90
{
    LuaObject luaClass = CreateLuaClass(state, "Command", "Object");
    luaClass.Register(kCommandFunctions, 0);
    state->DoString(kCommandLua);
}

// ---- LuaCommandAdapter and helpers ----

void RegisterLuaCommandAdapter(LuaState* state)                              // Mac 0x2b4de0, Win 0x44a2d0
{
    LuaObject luaClass = CreateLuaClass(state, "CommandAdapter", "Object");
    luaClass.Register(kAdapterFunctions, 0);
    luaClass.Register("New", LuaReturnNothing, 0);
    luaClass.Register("Create", LuaReturnNothing, 0);
}

// NOTE: returns nil when `command` already has a Lua table (both builds).
LuaObject LuaCommandAdapter::CreateLuaObject(Command* command, LuaState* state) // Mac 0x2b52c0, Win 0x44a400
{
    bool exists;
    {
        LuaObject existing = GetObjectMapEntry(state, command);
        exists = !existing.IsNil();
    }
    if (exists)
        return LuaObject(state);
    LuaObject table = CreateLuaInstance(state, "CommandAdapter");
    RefPointer<LuaCommandAdapter> adapter = new LuaCommandAdapter(command);
    AttachLuaObject(table, adapter);
    return table;
}

void ConnectLuaCommandObserver(LuaObject& table, const String& path, CommandObserver* observer) // Mac 0x2b5650, Win 0x44a690
{
    LuaObject object = LookupLuaObject(table, path);
    if (object.IsNil()) {
        observer->Detach();
        return;
    }
    Command* command = BoundCommand(object);
    if (command)
        observer->Attach(command);
}

Command* CheckLuaCommand(LuaObject& object, int narg)                        // Mac 0x2b5cd0, Win 0x44b0c0
{
    Command* command = BoundCommand(object);
    if (command)
        return command;
    BindableLuaObject* other = GetBoundObject(object);
    LuaTypeError(object, narg, other ? typeid(*other).name() : 0, typeid(Command).name());
    return 0;
}

// NOTE (restoration): raises only after the LuaObject went out of scope (see CheckBoundObject).
Command* CheckLuaCommand(LuaState* state, int narg)                          // Win 0x44ade0
{
    Command* command;
    const char* got = 0;
    {
        LuaObject object(state, narg);
        command = BoundCommand(object);
        if (!command) {
            BindableLuaObject* other = GetBoundObject(object);
            got = other ? typeid(*other).name() : 0;
        }
    }
    if (!command)
        RaiseLuaTypeError(state->GetCState(), narg, got, typeid(Command).name());
    return command;
}

} // namespace LEGO
