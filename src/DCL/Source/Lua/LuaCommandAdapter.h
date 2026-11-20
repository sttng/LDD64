// LEGO Digital Designer — restored source
// LEGO::LuaCommandAdapter — the Lua twin of a C++ Command (AdapterBindableLuaObject<Command>),
// Lua class "CommandAdapter" (base "Object"); CommandAdapter:New / Create do nothing (adapters
// come from CreateLuaObject only). Its functions also accept a LuaCommand.
// LDDApplication::ShowAboutBox (Mac 0x177ed0) finds "ldd.commands.about" through it.
// Home: TU 306 "LuaCommand.cpp" (Mac 0x2b45c0-0x2b5ecf, Win 0x449cb0-0x44ad6f).
// RTTI: LuaCommandAdapter si AdapterBindableLuaObject<Command> si
// AdapterBindableLuaObjectBase<Command, BindableLuaObject>; Mac vtables 0x745470 {D1 0x2b5450,
// D0 0x2b5540, Bind 0x2b59c0, Unbind 0x2b59e0} / 0x745498 (Object-in); base <Command> vtables
// 0x74552c / 0x775a18 {D1 0x2b5a00, D0 0x2b5a70, …}. Win: dtor 0x44a630, sdd 0x44a600; base
// ctor 0x44ac10, dtor 0x44a9a0 (sdd 0x44ab30).
// Layout (Mac): vptr, state +4, Command* +8, Object +0xc (sizeof 0x14).
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "Command.h"

namespace LEGO {

class LuaCommandAdapter : public AdapterBindableLuaObject<Command> {
public:
    explicit LuaCommandAdapter(Command* command) : AdapterBindableLuaObject<Command>(command) {}
    virtual ~LuaCommandAdapter() {}

    // (additive) A new Lua instance table bound to a new LuaCommandAdapter of
    // `command` (BrickModeGlobalObject Mac 0x181cb0 registers its commands with it). Mac 0x2b52c0
    static LuaPlus::LuaObject CreateLuaObject(Command* command, LuaPlus::LuaState* state);
};

// (additive; for LuaButtonControl's `command` attribute) Connects `observer` to the
// Command at Lua path `path` (from `table`, LookupLuaObject): nil -> Detach; a bound
// AdapterBindableLuaObject<Command> or bound Command -> Attach (no change when there is none or
// it is already the subject). (name inferred)
// Mac 0x2b5650, Win 0x44a690
void ConnectLuaCommandObserver(LuaPlus::LuaObject& table, const String& path,
                               CommandObserver* observer);

// The Command of a Lua value: the adapted command of a bound LuaCommandAdapter, or
// a bound object that is a Command (LuaCommand); otherwise a Lua type error "Command expected,
// got …" for argument `narg`. Mac 0x2b5cd0, Win 0x44b0c0 (name inferred)
Command* CheckLuaCommand(LuaPlus::LuaObject& object, int narg);
// The same for argument `narg` of the Lua stack. Win 0x44ade0 (Mac inlined, e.g. 0x3826b0)
Command* CheckLuaCommand(LuaPlus::LuaState* state, int narg);

// Registers CommandAdapter (base Object) {Do, UpdateCommandEnable, SetEnable, GetEnable,
// GetToggleable, GetToggled, Toggle; New / Create = LuaReturnNothing}.
// Mac 0x2b4de0, Win 0x44a2d0. (name inferred)
void RegisterLuaCommandAdapter(LuaPlus::LuaState* state);

} // namespace LEGO
