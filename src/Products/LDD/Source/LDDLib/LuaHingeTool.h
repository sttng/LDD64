// LEGO Digital Designer — restored source
// LEGO::LuaHingeTool — Lua class "HingeTool" (base Tool): the LuaTool of the HingeTool. Lua:
//   self:SetMinusButton(button), SetPlusButton(button), SetTextInputControl(input) and the
//   same with suffix 2 and 3 (the hinge toolbar controls, Lua tables of LuaButtonControl /
//   LuaTextInputControl); New / Create do nothing.
//
// TU 404 "LuaHingeTool.cpp": Mac 0x38c080-0x38cb9b, Win 0x64c05e-0x64c46f.
// RTTI si LuaTool. Mac vtable 0x7563ac {D1 0x38c7e0, D0 0x38c8a0, 0x293650, 0x293670},
// Object-in 0x7563d4 {0x38c840, 0x38c910, …}; VTT 0x7563f4. Win 0x9d5448 {0x64bf4e, 0x604a66},
// Object-in 0x9d5434 {sdd 0x64c05e}. Function names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LuaTool.h"

namespace LEGO {

class HingeTool;

class LuaHingeTool : public LuaTool {
public:
    explicit LuaHingeTool(HingeTool* tool);                                  // inlined in 0x38c660
    virtual ~LuaHingeTool() {}                                              // Mac 0x38c7e0/0x38c8a0, Win sdd 0x64c05e

    // A new HingeTool instance table bound to a new LuaHingeTool of `tool`.
    // NOTE: nil when `tool` already has a Lua table.
    static LuaPlus::LuaObject CreateLuaObject(HingeTool* tool, LuaPlus::LuaState* state); // Mac 0x38c660
};

// Registers the Lua class HingeTool (base Tool) {SetMinusButton … SetTextInputControl3,
// New = Create = no-op}.
void RegisterLuaHingeTool(LuaPlus::LuaState* state);                        // Mac 0x38c080, Win 0x64c380

} // namespace LEGO
