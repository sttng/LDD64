// LEGO Digital Designer — restored source
// LEGO::LuaLDDApplication — Lua class "LDDApplication" (base Object): the Lua twin of the
//   LDDApplication (an AdapterBindableLuaObject<LDDApplication>). Lua: self:SetBrand(name),
//   self:GetBrand() -> name, self:SetEditCommand(command), self:Import(fileName, sceneControl)
//   (a file of the system directory 9); New / Create do nothing.
//
// TU 397 "LuaLDDApplication.cpp": Mac 0x3824a0-0x3830ab, Win 0x6062fa-0x6068c8.
// RTTI si AdapterBindableLuaObject<LDDApplication>. Mac vtable 0x755294 {D1 0x382b20, D0
// 0x382c10, BindLuaObject 0x382d20, UnbindLuaObject 0x382d40}, Object-in 0x7552bc {0x382b90,
// 0x382c90, …}; AdapterBindableLuaObjectBase<LDDApplication, …> 0x7552dc {0x382d60, 0x382dd0,
// …}. Win 0x9ce688 {0x64bf4e, 0x604a66}, Object-in 0x9ce674 {sdd 0x6068aa}.
// Mac sizeof 0x14, Win 0x18. Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "LDDApplication.h"

namespace LEGO {

class LuaLDDApplication : public AdapterBindableLuaObject<LDDApplication> {
public:
    // inlined in CreateLuaObject (Mac 0x382990; Win 0x6064ba)
    explicit LuaLDDApplication(LDDApplication* application)
        : AdapterBindableLuaObject<LDDApplication>(application) {}
    virtual ~LuaLDDApplication() {}                                         // Mac 0x382b20/0x382c10, Win 0x6063ae / sdd 0x6068aa

    // A new LDDApplication instance table bound to a new LuaLDDApplication of `application`.
    // NOTE: nil when `application` already has a Lua table (both builds).
    static LuaPlus::LuaObject CreateLuaObject(LDDApplication* application,
                                              LuaPlus::LuaState* state);     // Mac 0x382990, Win 0x6067bd
};

// Registers the Lua class LDDApplication {SetBrand, GetBrand, SetEditCommand, Import,
// New = Create = no-op}.
void RegisterLuaLDDApplication(LuaPlus::LuaState* state);                   // Mac 0x3824a0, Win 0x606713

} // namespace LEGO
