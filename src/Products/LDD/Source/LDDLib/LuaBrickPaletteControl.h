// LEGO Digital Designer — restored source
// LEGO::LuaBrickPaletteControl — Lua class "BrickPaletteControl" (base Control): the LuaControl
//   of the BrickPaletteControl. Lua: self:GetNumberOfBags(), self:SetBrickFilterDescription(
//   text), self:ClearFilter(); New / Create do nothing (BrickPaletteControl creates its
//   LuaBrickPaletteControl itself, Mac 0x69430).
//
// TU 392 "LuaBrickPaletteControl.cpp": Mac 0x37d080-0x37daab, Win 0x64afd5-0x64b13a (the Win
// Lua functions are inlined / folded elsewhere).
// RTTI si LuaControl. Mac vtable 0x753cec (24 entries: LuaControl's with D1 0x37d350, D0
// 0x37d530, Attach 0x37d770, Detach 0x37d850), ControlAugmentation-in 0x753d58 {…, 0x37d7e0,
// 0x37d870, …}, Object-in 0x753e3c; VTT 0x753e54. Win Attach 0x64b08e, sdd 0x64b11b.
// Layout: LuaControl, m_paletteControl Mac +0x4c / Win +0x50, Mac +0x50 (0), Object Mac +0x54.
// Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "LuaControl.h"

namespace LEGO {

class BrickPaletteControl;

class LuaBrickPaletteControl : public LuaControl {
public:
    LuaBrickPaletteControl();                                               // Mac 0x37d2b0 (C1)
    virtual ~LuaBrickPaletteControl() {}                                    // Mac 0x37d350/0x37d530, Win sdd 0x64b11b

    BrickPaletteControl* GetPaletteControl() const { return m_paletteControl; }
    // BrickPaletteControl::Create (Mac 0x68cc0) stores the brick database here after binding
    // (never read). PORT64: a pointer member (4 bytes on 32-bit, like the original's UInt32), so
    // the address is not cut down on 64-bit.
    void SetBrickDatabase(const void* database) { m_value50 = database; }

protected:
    // LuaControl::Attach, then remember the control as a BrickPaletteControl.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x37d770 (thunk 0x37d7e0), Win 0x64b08e
    virtual void Detach();                                                  // Mac 0x37d850 (thunk 0x37d870)

private:
    BrickPaletteControl* m_paletteControl;   // Mac +0x4c, Win +0x50 (dynamic_cast of the control)
protected:
    const void*          m_value50;          // Mac +0x50: 0 (not used in this TU; name inferred)
};

// The BrickPaletteControl of the LuaBrickPaletteControl bound to argument `narg`; a Lua type
// error naming BrickPaletteControl otherwise. (header inline in the original; copy Mac 0x37d890)
BrickPaletteControl* CheckLuaBrickPaletteControl(LuaPlus::LuaState* state, int narg);

// Registers the Lua class BrickPaletteControl (base Control) {GetNumberOfBags,
// SetBrickFilterDescription, ClearFilter, New = Create = no-op}.
void RegisterLuaBrickPaletteControl(LuaPlus::LuaState* state);             // Mac 0x37d080, Win 0x64b38f

} // namespace LEGO
