// LEGO Digital Designer — restored source
// LEGO::LuaToggleControl — Lua class "ToggleControl" (base Control), the LuaButtonControl of a
// ToggleControl: self:OnToggled(toggled) on toggle; control and command toggle states follow
// each other. <ToggledUp>, <ToggledOver>, <ToggledDown>, <ToggledDisabled> describe the toggled
// states (descriptions 4-7).
// TU 317 "LuaToggleControl.cpp": Mac 0x2c5c80-0x2c70db, Win 0x465dc0-0x4665af; also emits the
// ObserverN<ToggleControl, ToggleControlObserver> dtor thunks.
//
// RTTI vmi {LuaButtonControl +0, ToggleControlObserver +0x58}, virtual base Object.
// Mac primary vtable 0x74839c (30 entries: LuaButtonControl's 26 with D1 0x2c6100, D0 0x2c64f0,
// OnLayoutApplied 0x2c6900, OnFinalRelease 0x2c6650, DefineCallbacks 0x2c6f40, Attach 0x2c6a80,
// Detach 0x2c6bd0, OnAttached(Command*) 0x2c6c40; Mac-only primary entries 26-29 of the
// overriders OnToggleChanged 0x2c6ce0, OnAttached(ToggleControl*) 0x2c6d40, OnToggled 0x2c6dc0,
// OnUntoggled 0x2c6e80), CommandObserver-in 0x7484f8 {…, 0x2c6c90, 0x14ea80, 0x2bd8d0,
// 0x2c6d10}, ToggleControlObserver-in 0x748520 {…, 0x2c6d80, 0x6ecb0, 0x2c6e60, 0x2c6f20}.
// Win vtables 0xa22b70 {…, OnLayoutApplied 0x466280, …, DefineCallbacks 0x466580, …},
// ControlAugmentation-in 0xa22b98 {…, Attach 0x4663b0, Detach 0x466410}, CommandObserver-in
// 0xa22c3c {OnAttached 0x466440, 0x40be80, 0x467d00, OnToggleChanged 0x4664b0},
// ToggleControlObserver-in 0xa22c50 {OnAttached 0x4664d0, 0x40be80, OnToggled 0x466500,
// OnUntoggled 0x466540}, Object-in 0xa22c64 {sdd 0x466790 / 0x465dc0, …}.
// Layout Mac: LuaButtonControl +0x00..+0x58, ToggleControlObserver +0x58 (subjects +0x5c),
// m_toggle +0x6c, Object +0x70 (sizeof 0x78). Win: m_toggle +0x78 (ControlAugmentation-relative
// +0x68), Object +0x80. Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "ToggleControl.h"
#include "LuaButtonControl.h"

namespace LEGO {

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaToggleControl : public LuaButtonControl, private ToggleControlObserver {
public:
    LuaToggleControl() : m_toggle(0) {}                                     // inlined in Create (Mac 0x2c5e20)
    virtual ~LuaToggleControl();                                            // Mac 0x2c6100/0x2c64f0, Win sdd 0x465dc0

    // The ToggleControl of the bound control (dynamic_cast in Attach). (name inferred)
    ToggleControl* GetToggle() const { return m_toggle; }

protected:
    // LuaButtonControl's, then <ToggledUp> … <ToggledDisabled> (descriptions 4-7).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2c6900, Win 0x466280
    // Leaves the toggles, then LuaButtonControl's.
    virtual void OnFinalRelease();                                          // Mac 0x2c6650 (thunks 0x2c66c0 …)
    // LuaControl's (not LuaButtonControl's: it has none), then <OnToggled> ("toggled").
    virtual void DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2c6f40, Win 0x466580
    // LuaButtonControl's; then observes the toggle and sets it to the command's toggle state.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x2c6a80 (thunk 0x2c6b20), Win 0x4663b0
    virtual void Detach();                                                  // Mac 0x2c6bd0 (thunk 0x2c6c00), Win 0x466410

    // ---- CommandObserver ----
    // LuaButtonControl's, then the toggle takes the command's toggle state.
    virtual void OnAttached(Command* command);                              // Mac 0x2c6c40 (thunk 0x2c6c90), Win 0x466440
    virtual void OnToggleChanged(bool toggled);                             // Mac 0x2c6ce0 (thunk 0x2c6d10), Win 0x4664b0

    // ---- ToggleControlObserver ----
    // The toggle takes the command's toggle state.
    virtual void OnAttached(ToggleControl* control);                        // Mac 0x2c6d40 (thunk 0x2c6d80), Win 0x4664d0
    // self:OnToggled(true / false), then the command's toggle state.
    virtual void OnToggled(ToggleControl* control);                         // Mac 0x2c6dc0 (thunk 0x2c6e60), Win 0x466500
    virtual void OnUntoggled(ToggleControl* control);                       // Mac 0x2c6e80 (thunk 0x2c6f20), Win 0x466540

private:
    ToggleControl* m_toggle;   // Mac +0x6c
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// The ToggleControl bound to argument `narg`, or a Lua type error naming ToggleControl.
// Mac 0x2c6f90 (name inferred)
ToggleControl* CheckLuaToggleControl(LuaPlus::LuaState* state, int narg);

// Registers ToggleControl (base Control) {GetToggled, Toggle, Create (upvalue `manager`)}.
// Mac 0x2c5c80 (name inferred)
void RegisterLuaToggleControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
