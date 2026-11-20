// LEGO Digital Designer — restored source
// LEGO::LuaControlManagerObserver — Lua class "ControlManagerObserver" (base Object): a
// ControlManagerObserver that forwards the manager's key, modal, focus and menu-loop events to
// its Lua table. Create(self), :Connect(), :Disconnect(), :IsModal() use the application's
// ControlManager (upvalue).
// TU 319 "LuaControlManagerObserver.cpp": Mac 0x2c8810-0x2c98bb, Win 0x46a310-0x46a85f.
// RTTI vmi {ControlManagerObserver +0, BindableLuaObject +8 (Win +0xc)}, virtual base Object.
// Mac vtables 0x74937c (22 entries: ControlManagerObserver's 21 with OnManagerKeyPressed
// 0x2c8e70, OnManagerKeyReleased 0x2c8f10, OnManagerModalControl 0x2c8fb0,
// OnManagerWindowKeyboardFocusChanged 0x2c91a0, …EnterMenuLoop 0x2c9230, …ExitMenuLoop 0x2c9260,
// + UnbindLuaObject 0x2c9290), BindableLuaObject-in 0x7493e0 {0x2c8ce0, 0x2c8db0, 0x291a70,
// UnbindLuaObject 0x2c92d0}; Win 0xa234ac {…, 0x46a600, 0x46a640, 0x520a60, 0x46a680,
// 0x46a790, 0x46a7d0, 0x46a800}, 0xa234f8 {0x40be80, UnbindLuaObject 0x46a830}, Object-in
// 0xa23504 {sdd 0x46b230, …}.
// Layout Mac: ControlManagerObserver {vptr, subject +4}, BindableLuaObject {vptr +8, state +0xc},
// Object +0x10 (sizeof 0x18); Win: Object +0x1c (sizeof 0x24).
#pragma once
#include "LEGO/Types.h"
#include "ControlManager.h"
#include "BindableLuaObject.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaControlManagerObserver : public ControlManagerObserver, public BindableLuaObject {
public:
    LuaControlManagerObserver() {}                                          // inlined (Mac 0x2c88b0, Win 0x46a240)
    virtual ~LuaControlManagerObserver() {}                                 // Mac 0x2c8cb0/0x2c8d60, Win 0x46a570 (sdd 0x46a310)

    // self:OnManagerKeyPressed(key name (StringToKey::GetName), modifiers, repeat).
    virtual void OnManagerKeyPressed(UInt32 key, UInt32 modifiers, bool repeat);     // Mac 0x2c8e70, Win 0x46a600
    // self:OnManagerKeyReleased(key name, modifiers).
    virtual void OnManagerKeyReleased(UInt32 key, UInt32 modifiers);                 // Mac 0x2c8f10, Win 0x46a640
    // self:OnManagerModalControl(modal, the Lua table of `control` or nil).
    virtual void OnManagerModalControl(bool modal, Control* control);                // Mac 0x2c8fb0, Win 0x46a680
    virtual void OnManagerWindowKeyboardFocusChanged(bool focused, UInt32 modifiers); // Mac 0x2c91a0, Win 0x46a790
    virtual void OnManagerWindowEnterMenuLoop();                                     // Mac 0x2c9230, Win 0x46a7d0
    virtual void OnManagerWindowExitMenuLoop();                                      // Mac 0x2c9260, Win 0x46a800
    // The Lua table went away: stop observing.
    virtual void UnbindLuaObject();                                                  // Mac 0x2c9290 (thunk 0x2c92d0), Win 0x46a830
};

// Registers ControlManagerObserver {Create, Connect, Disconnect, IsModal; upvalue `manager`}.
// Mac 0x2c8810, Win 0x46a490. (name inferred)
void RegisterLuaControlManagerObserver(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
