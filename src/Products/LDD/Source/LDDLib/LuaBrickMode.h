// LEGO Digital Designer — restored source
// LEGO::LuaBrickMode — the Lua twin of a BrickMode (build / view modes): a LuaMode that also
//   keeps the control as a BrickMode. Lua class "BrickMode" (base "Mode"):
//   self:GetClientControl(), self:GetToolbarControl() (the Lua tables of those controls,
//   created as plain Control tables when they have none); New / Create do nothing.
//   EditMode.lua: self.palette:SetParent(self:GetClientControl()).
//
// TU 379 "LuaBrickMode.cpp": Mac 0x3696f0-0x36a1eb, Win 0x64b5ef-0x64b627 (+ 0x64b820).
// RTTI si LuaMode. Mac primary vtable 0x7507f4 (LuaMode's with OnFinalRelease 0x369ef0, Attach
// 0x369f80, Detach 0x36a060); constructor Mac 0x369960 (C1), D1 0x369a00 / D0 0x369c40.
// Win: Attach 0x64b5ef, sdd 0x64b627, registration 0x64b820.
// Layout (Mac): LuaMode +0x00..+0x58, BrickMode* +0x58, Object +0x5c (sizeof 0x64).
// Method and member names are inferred unless Lua names.
#pragma once
#include "LEGO/Types.h"
#include "LuaMode.h"

namespace LEGO {

class BrickMode;

class LuaBrickMode : public LuaMode {
public:
    LuaBrickMode() : m_brickMode(0) {}                                      // Mac 0x369960
    virtual ~LuaBrickMode() {}                                              // Mac 0x369a00/0x369c40, Win sdd 0x64b627

    BrickMode* GetBrickMode() const { return m_brickMode; }

    // Argument `narg` bound to a LuaBrickMode with a brick mode; a Lua type error naming
    // BrickMode otherwise. Mac 0x36a0a0
    static BrickMode* CheckBrickMode(LuaPlus::LuaState* state, int narg);

protected:
    // LuaMode's (the original overrides it without adding anything). Mac 0x369ef0
    virtual void OnFinalRelease();
    // LuaMode's; then the brick mode = the control as a BrickMode. Mac 0x369f80 (thunk
    // 0x369ff0), Win 0x64b5ef
    virtual bool Attach(Control* control, const String& name);
    // Forgets the brick mode, then LuaMode's. Mac 0x36a060 (thunk 0x36a080)
    virtual void Detach();

private:
    BrickMode* m_brickMode;   // Mac +0x58
};

// Registers the Lua class BrickMode (base Mode) {GetClientControl, GetToolbarControl; New /
// Create = LuaReturnNothing}. Mac 0x3696f0, Win 0x64b820. (name inferred)
void RegisterLuaBrickMode(LuaPlus::LuaState* state);

} // namespace LEGO
