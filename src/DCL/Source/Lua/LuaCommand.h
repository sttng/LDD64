// LEGO Digital Designer — restored source
// LEGO::LuaCommand — a Command made in Lua (Lua class "Command", base "Object"; scripts derive
// their own, e.g. ShowControlCommand). Command:New / Do / UpdateEnable are Lua (the chunk run by
// RegisterLuaCommand); C++ Do / UpdateEnable call the table's Do(toggled) / UpdateEnable(toggled)
// (toggled as the number 0 / 1, looked up through the class chain).
// TU 306 "LuaCommand.cpp": Mac 0x2b45c0-0x2b5ecf, Win 0x449cb0-0x44ad6f (+ Win 0x44ade0,
// 0x44b0c0); also emits AdapterBindableLuaObjectBase<Command, BindableLuaObject> and
// LuaCommandAdapter (LuaCommandAdapter.h).
// RTTI vmi {Command +0, BindableLuaObject +0x28}, virtual base Object. Mac vtable 0x74536c {D1
// 0x2b4a90, D0 0x2b4bf0, OnFinalRelease 0x1414d0 (NotifierT), Do 0x2b45c0, SetEnable 0x372e30,
// GetEnable 0x3730b0, UpdateEnable 0x2b4650, UnbindLuaObject 0x2b4da0 (Mac-only primary entry)};
// BindableLuaObject-in 0x745398 {…, BindLuaObject 0x291a70, UnbindLuaObject thunk 0x2b4dc0};
// Object-in 0x7453c0. Win: Do 0x449ce0, UpdateEnable 0x449d20, Unbind 0x44a110, dtor 0x44a080
// (sdd 0x449cb0).
// Layout (Mac): Command +0x00..+0x28 (enabled +0x24, toggled +0x25, toggleable +0x26),
// BindableLuaObject +0x28 (state +0x2c), Object +0x30 (sizeof 0x38). Method names are the Lua
// names; the rest inferred.
#pragma once
#include "LEGO/Types.h"
#include "Command.h"
#include "BindableLuaObject.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaCommand : public Command, public BindableLuaObject {
public:
    LuaCommand(bool enabled, bool toggleable) : Command(enabled, toggleable) {} // (inlined in Create, Mac 0x2b4780)
    virtual ~LuaCommand() {}                                                // Mac 0x2b4a90/0x2b4bf0, Win 0x44a080 (sdd 0x449cb0)

    // self:Do(toggled as 0 / 1). Mac 0x2b45c0, Win 0x449ce0
    virtual void Do();
    // self:UpdateEnable(toggled as 0 / 1). Mac 0x2b4650, Win 0x449d20
    virtual void UpdateEnable();
    // The Lua table is gone: SetEnable(false). Mac 0x2b4da0 (thunk 0x2b4dc0), Win 0x44a110
    virtual void UnbindLuaObject();

    // Argument `narg` bound to a LuaCommand, a Lua type error otherwise. Mac 0x2b5c00, Win 0x44ab60
    static LuaCommand* Check(LuaPlus::LuaState* state, int narg);
};

// Registers Command (base Object) {Create, SetEnable, GetEnable, GetToggleable, GetToggled,
// Toggle} and runs the Lua part (Command:New, Command:Do, Command:UpdateEnable). Must run before
// Scripts/LDD.xml is loaded. Mac 0x2b46e0, Win 0x449f90. (name inferred)
void RegisterLuaCommand(LuaPlus::LuaState* state);

} // namespace LEGO
