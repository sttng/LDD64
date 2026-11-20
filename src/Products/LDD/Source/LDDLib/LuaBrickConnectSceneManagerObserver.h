// LEGO Digital Designer — restored source
// LEGO::LuaSceneManager — Lua class "SceneManager" (base Object): the Lua twin of the
//   BrickConnectSceneManager (an AdapterBindableLuaObject<BrickConnectSceneManager>). Lua:
//   self:GetCurrentTool() -> Tool table (or nil), self:SetCurrentTool(tool | nil),
//   self:SetDefaultTool(tool | nil); New / Create do nothing.
// LEGO::LuaBrickConnectSceneManagerObserver — Lua class "SceneManagerObserver" (base Object):
//   a BrickConnectSceneManagerObserver bound to a Lua table. Lua: SceneManagerObserver.Create
//   (self), self:Connect() (observes the scene manager given to the registration, an upvalue),
//   self:Disconnect(); callbacks self:OnSceneChanged(), self:OnToolChanged(previous, current)
//   (the tools' Lua tables or nil).
//
// TU 279 "LuaBrickConnectSceneManagerObserver.cpp" (+ LuaSceneManager): Mac 0x291ca0-0x29304b,
// Win 0x604f38-0x605c46 (+ 0x462850 CallLuaMethod<LuaObject, LuaObject>, 0x633a48).
// LuaSceneManager: RTTI si AdapterBindableLuaObject<BrickConnectSceneManager>; Mac vtable
//   0x73f69c {D1 0x292030, D0 0x292120, BindLuaObject 0x292b00, UnbindLuaObject 0x292b20},
//   Object-in 0x73f6c4; Win Object-in 0x9ce448 {sdd 0x6059fb}. Mac sizeof 0x14, Win 0x18.
// LuaBrickConnectSceneManagerObserver: RTTI vmi {BrickConnectSceneManagerObserver +0,
//   BindableLuaObject +8 (Win +0xc)}, virtual base Object. Mac vtables 0x73f6ec {D1 0x292690,
//   D0 0x292740, OnFinalRelease 0x193680, OnAttached 0x1936d0, OnDetached 0x1936e0,
//   OnSceneControlChanged 0x292850, OnActiveToolChanged 0x292880, UnbindLuaObject 0x292a70},
//   BindableLuaObject-in 0x73f718 {0x2926c0, 0x292790, 0x291a70, 0x292ab0}; Win 0x9ce3f4
//   {…, 0x604f38, 0x6055be}, 0x9ce3e8 {0x40be80, 0x605b87}, Object-in 0x9ce3d4 {sdd 0x605252?}.
//   Layout Mac: observer {vptr, subject +4}, BindableLuaObject {vptr +8, state +0xc}, Object
//   +0x10 (sizeof 0x18).
// Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "AdapterBindableLuaObject.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class LuaSceneManager : public AdapterBindableLuaObject<BrickConnectSceneManager> {
public:
    // inlined in CreateLuaObject (Mac 0x291ea0)
    explicit LuaSceneManager(BrickConnectSceneManager* manager)
        : AdapterBindableLuaObject<BrickConnectSceneManager>(manager) {}
    virtual ~LuaSceneManager() {}                                           // Mac 0x292030/0x292120, Win sdd 0x6059fb

    // A new SceneManager instance table bound to a new LuaSceneManager of `manager`.
    // NOTE: nil when `manager` already has a Lua table (both builds).
    static LuaPlus::LuaObject CreateLuaObject(BrickConnectSceneManager* manager,
                                              LuaPlus::LuaState* state);     // Mac 0x291ea0, Win 0x60590e
};

class LuaBrickConnectSceneManagerObserver : public BrickConnectSceneManagerObserver,
                                            public BindableLuaObject {
public:
    LuaBrickConnectSceneManagerObserver() {}                               // inlined (Mac 0x2922d0)
    virtual ~LuaBrickConnectSceneManagerObserver() {}                      // Mac 0x292690/0x292740, Win sdd 0x60559f

    // self:OnSceneChanged() (raw lookup in the instance table).
    virtual void OnSceneControlChanged(BrickConnectSceneControl* previous,
                                       BrickConnectSceneControl* current);  // Mac 0x292850, Win 0x604f38
    // self:OnToolChanged(objectMap[previous], objectMap[current]).
    virtual void OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                     BrickConnectSceneControlTool* current); // Mac 0x292880, Win 0x6055be
    // The Lua table went away: stop observing.
    virtual void UnbindLuaObject();                                         // Mac 0x292a70 (thunk 0x292ab0), Win 0x633a48 (via 0x605b87)
};

// Registers the Lua class SceneManager {GetCurrentTool, SetCurrentTool, SetDefaultTool,
// New = Create = no-op}.
void RegisterLuaSceneManager(LuaPlus::LuaState* state);                     // Mac 0x291ca0, Win 0x605ac2
// Registers the Lua class SceneManagerObserver {Create, Connect, Disconnect}; the functions get
// `manager` as upvalue 1 (light userdata).
void RegisterLuaSceneManagerObserver(LuaPlus::LuaState* state,
                                     BrickConnectSceneManager* manager);    // Mac 0x292230, Win 0x605bc0

} // namespace LEGO
