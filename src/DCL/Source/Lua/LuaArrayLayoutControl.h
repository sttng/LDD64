// LEGO Digital Designer — restored source
// LEGO::LuaArrayLayoutControl — Lua class "ArrayControl" (base Control), the LuaControl of an
// ArrayLayoutControl (one row / column of controls).
// TU 344 "LuaArrayLayoutControl.cpp": Mac 0x328320-0x32956b, Win 0x463ef0-0x464a4f.
//
// RTTI si LuaControl, virtual base Object. Mac primary vtable 0x74bafc (LuaControl's 24 with
// D1 0x328ac0, D0 0x328ca0, OnLayoutApplied 0x328ee0, Attach 0x329300, Detach 0x3293e0).
// Win 0xa228e8 {…, OnLayoutApplied 0x4646f0, …}, ControlAugmentation-in 0xa22910 {…, Attach
// 0x464990, Detach 0x4649d0}, Object-in 0xa229b4 {sdd 0x464c70 / 0x463ef0, …}.
// Layout Mac (ctor inlined in Create 0x328850): LuaControl +0x00..+0x4c, m_array +0x4c,
// Object +0x50 (sizeof 0x58). Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "ArrayLayoutControl.h"
#include "LuaControl.h"

namespace LEGO {

class LuaArrayLayoutControl : public LuaControl {
public:
    LuaArrayLayoutControl() : m_array(0) {}                                 // inlined in Create (Mac 0x328850)
    virtual ~LuaArrayLayoutControl();                                       // Mac 0x328ac0/0x328ca0, Win 0x464670 / sdd 0x463ef0

    ArrayLayoutControl* GetArray() const { return m_array; }

protected:
    // autoSizeMode (before LuaControl's attributes), columnMode, reverseMode, spacing, the controls
    // of the <Content> children.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x328ee0, Win 0x4646f0
    virtual bool Attach(Control* control, const String& name);              // Mac 0x329300 (thunk 0x329370), Win 0x464990
    virtual void Detach();                                                  // Mac 0x3293e0 (thunk 0x329400), Win 0x4649d0

private:
    ArrayLayoutControl* m_array;   // Mac +0x4c (dynamic_cast of the control)
};

// The ArrayLayoutControl bound to argument `narg`, or a Lua type error naming
// ArrayLayoutControl. Mac 0x329420, Win 0x4649e0 (name inferred)
ArrayLayoutControl* CheckLuaArrayLayoutControl(LuaPlus::LuaState* state, int narg);

// Registers ArrayControl {GetColumnMode … RemoveControls, Create (upvalue `manager`)}.
// Mac 0x328320, Win 0x4644d0 (name inferred)
void RegisterLuaArrayLayoutControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
