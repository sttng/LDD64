// LEGO Digital Designer — restored source
// LEGO::LuaLDDWebCommunicator — Lua class "LDDWebCommunicator" (base Object): the Lua twin of
//   the LDDWebCommunicator (an AdapterBindableLuaObject<LDDWebCommunicator>). It has no Lua
//   functions of its own (empty function table); New / Create do nothing.
//
// TU 399 "LuaLDDWebCommunicator.cpp": Mac 0x383c50-0x38432f, Win 0x6068c9-0x606cab.
// RTTI si AdapterBindableLuaObject<LDDWebCommunicator>. Mac vtable 0x75547c {D1 0x383eb0, D0
// 0x383fa0, BindLuaObject 0x3840b0, UnbindLuaObject 0x3840d0}, Object-in 0x7554a4 {0x383f20,
// 0x384020, …}; AdapterBindableLuaObjectBase<LDDWebCommunicator, …> 0x7554cc {0x3840f0,
// 0x384160, …}. Win 0x9ce754 {0x64bf4e, 0x604a66}, Object-in 0x9ce740 {sdd 0x606c8d, …}.
// Function names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "LDDWebCommunicator.h"

namespace LEGO {

class LuaLDDWebCommunicator : public AdapterBindableLuaObject<LDDWebCommunicator> {
public:
    // inlined in CreateLuaObject (Mac 0x383d20; Win 0x606b28)
    explicit LuaLDDWebCommunicator(LDDWebCommunicator* communicator)
        : AdapterBindableLuaObject<LDDWebCommunicator>(communicator) {}
    virtual ~LuaLDDWebCommunicator() {}                                     // Mac 0x383eb0/0x383fa0, Win 0x606a71 / sdd 0x606c8d

    // A new LDDWebCommunicator instance table bound to a new adapter of `communicator`.
    // NOTE: nil when `communicator` already has a Lua table (both builds).
    static LuaPlus::LuaObject CreateLuaObject(LDDWebCommunicator* communicator,
                                              LuaPlus::LuaState* state);     // Mac 0x383d20, Win 0x606ba0
};

// Registers the Lua class LDDWebCommunicator {New = Create = no-op}.
void RegisterLuaLDDWebCommunicator(LuaPlus::LuaState* state);               // Mac 0x383c50, Win 0x6068c9

} // namespace LEGO
