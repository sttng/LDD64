// LEGO Digital Designer — restored source
// LEGO::LuaMode — the Lua twin of a Mode: a LuaControl bound to the mode (a RenderControl) that
//   also observes it (ModeObserver: OnEnter / OnExit are forwarded to the table). Lua class
//   "Mode" (base "Control"): self:IsCurrent(), self:CanEnter(), self:CanExit(); Mode:New /
//   Mode.Create do nothing (modes are created in C++, the table with
//   CreateLuaControlTable<LuaMode>(mode, state, "Mode", object)). Layout child <Menu> (the
//   LuaWindow menu syntax) becomes the mode's menu.
//
// TU 380 "LuaMode.cpp": Mac 0x36a1f0-0x36b31b, Win 0x64bac9-0x64bde1 (+ 0x64b8ae, 0x64b8cf).
// RTTI vmi {LuaControl +0, ModeObserver +0x4c (non-public)}, virtual base Object. Mac primary
// vtable 0x750a2c (LuaControl's 24 slots with OnFinalRelease 0x36aa50, ApplyControlAttributes
// 0x36ac40, Attach 0x36ad40, Detach 0x36b030, + OnEnter 0x36b0f0, OnExit 0x36b160);
// construction C2 0x36a3d0 (VTT), destructor D1 0x36a480 / D0 0x36a730 (body 0x36a450).
// Win: ApplyControlAttributes 0x64bac9, Attach 0x64bdb3, OnEnter 0x64b8ae, OnExit 0x64b8cf, sdd
// 0x64bb83.
// Layout (Mac): LuaControl +0x00..+0x4c, ModeObserver +0x4c (subject +0x50), Mode* +0x54,
// Object +0x58 (sizeof 0x60). Method and member names are inferred unless Lua names.
#pragma once
#include "LEGO/Types.h"
#include "LuaControl.h"
#include "Mode.h"

namespace LEGO {

class LuaMode : public LuaControl, protected ModeObserver {
public:
    LuaMode() : m_mode(0) {}                                                // Mac 0x36a3d0 (C2)
    virtual ~LuaMode() {}                                                   // Mac 0x36a480/0x36a730, Win sdd 0x64bb83

    Mode* GetMode() const { return m_mode; }

    // Argument `narg` bound to a LuaMode with a mode; a Lua type error naming Mode otherwise.
    // Mac 0x36b1d0
    static Mode* CheckMode(LuaPlus::LuaState* state, int narg);

protected:
    // ModeObserver's (leaves the mode), then LuaControl's. Mac 0x36aa50 (thunks 0x36aab0 …)
    virtual void OnFinalRelease();
    // LuaControl's, then the child <Menu>: CreateLuaMenu (Lua paths from the table being laid
    // out) -> Mode::SetMenu. Mac 0x36ac40, Win 0x64bac9
    virtual void ApplyControlAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // LuaControl's; then the mode = the control as a Mode, observed. Mac 0x36ad40, Win 0x64bdb3
    virtual bool Attach(Control* control, const String& name);
    // Leaves the mode, then LuaControl's. Mac 0x36b030 (thunk 0x36b090)
    virtual void Detach();

    // ---- ModeObserver: self:OnEnter() / self:OnExit() for this object's mode ----
    virtual void OnEnter(Mode* mode);                                       // Mac 0x36b0f0 (thunk 0x36b120), Win 0x64b8ae
    virtual void OnExit(Mode* mode);                                        // Mac 0x36b160 (thunk 0x36b190), Win 0x64b8cf

    Mode* m_mode;   // Mac +0x54
};

// Registers the Lua class Mode (base Control) {IsCurrent, CanEnter, CanExit; New / Create =
// LuaReturnNothing}. Mac 0x36a1f0, Win 0x64bce4. (name inferred)
void RegisterLuaMode(LuaPlus::LuaState* state);

} // namespace LEGO
