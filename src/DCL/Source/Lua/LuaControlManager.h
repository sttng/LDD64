// LEGO Digital Designer — restored source
// LEGO::LuaControlManager — the Lua twin of the ControlManager (AdapterBindableLuaObject<
// ControlManager>, Lua class "ControlManager", base Object); New / Create do nothing, the table
// is bound from C++ (CreateLuaControlManager).
// TU 378 "LuaControlManager.cpp": Mac 0x368a70-0x3696eb, Win 0x448e10-0x44984f.
//
// RTTI: LuaControlManager si AdapterBindableLuaObject<ControlManager> si
// AdapterBindableLuaObjectBase<ControlManager, BindableLuaObject>. Mac vtables 0x75071c {D1
// 0x369160, D0 0x369250, BindLuaObject 0x369360, UnbindLuaObject 0x369380}, adapter base
// 0x75076c {0x3693a0, 0x369410, 0x369360, 0x369380}; Win 0xa20aa8 {0x44aa30, 0x44aa50},
// Object-in 0xa20ab4 {sdd 0x449590, …}. Layout Mac vptr, state +4, ControlManager* +8, Object
// +0xc (sizeof 0x14); Win ControlManager* +0xc, Object +0x10. Names inferred unless Lua strings.
#pragma once
#include "LEGO/Types.h"
#include "ControlManager.h"
#include "AdapterBindableLuaObject.h"
#include "LuaBindings.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaControlManager : public AdapterBindableLuaObject<ControlManager> {
public:
    // (inlined in CreateLuaControlManager on both builds)
    explicit LuaControlManager(ControlManager* manager) : AdapterBindableLuaObject<ControlManager>(manager) {}
    virtual ~LuaControlManager() {}                                         // Mac 0x369160/0x369250, Win 0x4495c0 (sdd 0x449590)
};

// A new Lua "ControlManager" instance bound to `manager` through a new LuaControlManager.
// NOTE: both builds return nil when the manager already has a Lua table (objectMap entry).
// Mac 0x368a70, Win 0x448e10. (name inferred)
LuaPlus::LuaObject CreateLuaControlManager(ControlManager* manager, LuaPlus::LuaState* state);

// Registers ControlManager {GetBackground (layer 1), GetKeyboardFocus, GetWindow,
// IsKeyPressed(keyName), GetModifiers -> ctrl, shift, alt; New/Create = LuaReturnNothing}.
// Mac 0x368c00, Win 0x4493f0. (name inferred)
void RegisterLuaControlManager(LuaPlus::LuaState* state);

} // namespace LEGO
