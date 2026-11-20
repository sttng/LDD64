// LEGO Digital Designer — restored source
// LEGO::LuaMatrixLayoutControl — Lua class "MatrixControl" (base Control), the LuaControl of a
// MatrixLayoutControl (a grid of equal cells). Not used by the LDD scripts.
// TU 316 "LuaMatrixLayoutControl.cpp": Mac 0x2c47c0-0x2c5c7b, Win 0x464c90-0x465a8f.
//
// RTTI si LuaControl, virtual base Object. Mac primary vtable 0x747d4c (LuaControl's 24 with
// D1 0x2c5110, D0 0x2c52f0, OnLayoutApplied 0x2c5530, Attach 0x2c5a10, Detach 0x2c5af0).
// Win 0xa22a3c {…, OnLayoutApplied 0x4655c0, …}, ControlAugmentation-in 0xa22a64 {…, Attach
// 0x4658c0, Detach 0x4649d0 (ICF with LuaArrayLayoutControl's)}, Object-in 0xa22b04 {sdd
// 0x465cb0 / 0x464d60, …}.
// Layout Mac (ctor inlined in Create 0x2c4c80 / 0x2c4f90): LuaControl +0x00..+0x4c, m_matrix
// +0x4c, Object +0x50 (sizeof 0x58). Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "MatrixLayoutControl.h"
#include "LuaControl.h"

namespace LEGO {

class LuaMatrixLayoutControl : public LuaControl {
public:
    LuaMatrixLayoutControl() : m_matrix(0) {}                               // inlined (Mac 0x2c4c80, 0x2c4f90)
    virtual ~LuaMatrixLayoutControl();                                      // Mac 0x2c5110/0x2c52f0, Win 0x465540 / sdd 0x464d60

    MatrixLayoutControl* GetMatrix() const { return m_matrix; }

    // A new Lua "MatrixControl" instance bound to `control` through a new LuaMatrixLayoutControl
    // (CreateLuaControlTable<LuaMatrixLayoutControl>, Mac 0x2c4f90, Win 0x464c90).
    // Mac 0x2c4ef0, Win 0x465970
    static LuaPlus::LuaObject CreateControlTable(LuaPlus::LuaState* state, Control* control);

protected:
    // autoSizeMode, LuaControl's, matrixSize, leftToRight, topToBottom, spacing, <Content>.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2c5530, Win 0x4655c0
    virtual bool Attach(Control* control, const String& name);              // Mac 0x2c5a10 (thunk 0x2c5a80), Win 0x4658c0
    virtual void Detach();                                                  // Mac 0x2c5af0 (thunk 0x2c5b10), Win 0x4649d0

private:
    MatrixLayoutControl* m_matrix;   // Mac +0x4c (dynamic_cast of the control)
};

// The MatrixLayoutControl bound to argument `narg`, or a Lua type error naming
// MatrixLayoutControl. Mac 0x2c5b30, Win 0x465900 (name inferred)
MatrixLayoutControl* CheckLuaMatrixLayoutControl(LuaPlus::LuaState* state, int narg);

// Registers MatrixControl (base Control) {GetMatrixSize … SetSpacing, Create (upvalue
// `manager`)}. Mac 0x2c47c0, Win 0x465320 (name inferred)
void RegisterLuaMatrixLayoutControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
