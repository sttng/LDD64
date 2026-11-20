// LEGO Digital Designer — restored source
// LEGO::LuaTool — Lua class "Tool" (base Object): the Lua twin of a BrickConnectSceneControlTool
//   (an AdapterBindableLuaObject<BrickConnectSceneControlTool>); base of LuaHingeTool. No Lua
//   functions of its own (empty function table); New / Create do nothing. SceneManager:
//   Get/SetCurrentTool and SceneManagerObserver:OnToolChanged pass these tables.
//
// TU 280 "LuaTool.cpp": Mac 0x293050-0x293a1b, Win 0x64bf3b-0x64c88f. The TU also emits
// AdapterBindableLuaObject<BrickConnectSceneControlTool> and its base (Mac 0x293560-0x293910).
// RTTI si AdapterBindableLuaObject<BrickConnectSceneControlTool>, virtual base Object (VTT:
// the base-object ctor / dtor are out of line for LuaHingeTool). Mac vtable 0x73f87c {D1
// 0x293360, D0 0x293450, BindLuaObject 0x293650, UnbindLuaObject 0x293670}, Object-in 0x73f8a4
// {0x2933d0, 0x2934d0, …}. Win 0x9d5580 {0x64bf4e, 0x604a66}, Object-in 0x9d556c {sdd 0x64c834}.
// Mac sizeof 0x14, Win 0x18. Function names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "BrickConnectSceneControlTool.h"

namespace LEGO {

class LuaTool : public AdapterBindableLuaObject<BrickConnectSceneControlTool> {
public:
    explicit LuaTool(BrickConnectSceneControlTool* tool);                    // Mac C2 0x2932b0 (C1 inlined in 0x293120), Win 0x64c6cf
    virtual ~LuaTool();                                                     // Mac D2 0x293320, D1/D0 0x293360/0x293450, Win 0x64bf3b / sdd 0x64c834

    // A new Tool instance table bound to a new LuaTool of `tool`.
    // NOTE: nil when `tool` already has a Lua table (both builds).
    static LuaPlus::LuaObject CreateLuaObject(BrickConnectSceneControlTool* tool,
                                              LuaPlus::LuaState* state);     // Mac 0x293120, Win 0x64c747
};

// Registers the Lua class Tool {New = Create = no-op}.
void RegisterLuaTool(LuaPlus::LuaState* state);                             // Mac 0x293050, Win 0x64c470

} // namespace LEGO
