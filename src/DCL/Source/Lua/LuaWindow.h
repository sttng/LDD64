// LEGO Digital Designer — restored source
// LEGO::LuaWindow — the Lua twin of a Window (AdapterBindableLuaObjectBase<Window, LuaLayout>,
// Lua class "Window", base Layout); New / Create do nothing (bound from C++, CreateLuaWindow).
// A layout applied to it may carry a <Menu> (the menu bar). LEGO::LuaModalUpdateReciever calls
// the Lua function of Window:ModalRun on every modal update.
// TU 377 "LuaWindow.cpp": Mac 0x366b40-0x368a6f, Win 0x447000-0x44853f.
//
// RTTI: LuaWindow si AdapterBindableLuaObjectBase<Window, LuaLayout> si LuaLayout. Mac vtables
// LuaWindow 0x750574 {D1 0x368020, D0 0x3680c0, BindLuaObject 0x367f80, UnbindLuaObject
// 0x367fa0, OnLayoutApplied 0x367da0}, adapter 0x7505ec {0x367ea0, 0x367f40, 0x367f80, 0x367fa0,
// 0x2cc720}; Win LuaWindow 0xa209e0 {0x448480, 0x4484a0, OnLayoutApplied 0x448290}, Object-in
// 0xa209f0 {sdd 0x447060, …}. Layout Mac: LuaLayout +0x00..+0x0c, Window* +0x0c, Object +0x10
// (sizeof 0x18); Win Window* +0x10.
// LuaModalUpdateReciever: RTTI vmi {ModalUpdateReciever +0, Object +4 (non-virtual)}; Mac
// vtables 0x7783b0 {D1 0x368230, D0 0x3682a0, ModalUpdate 0x3682e0}, Object-in 0x7783c4;
// layout vptr, Object +4 (vptr, refcount), LuaObject +0x0c (sizeof 0x24); Win sizeof 0x30
// (ctor 0x447200, LuaObject +0xc). Names inferred unless they are Lua strings.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Window.h"
#include "LuaLayout.h"
#include "LuaBindings.h"
#include "AdapterBindableLuaObject.h"
#include "LuaPlus.h"

namespace LEGO {

class XMLDOMNode;
class Menu;

class LuaWindow : public AdapterBindableLuaObjectBase<Window, LuaLayout> {
public:
    explicit LuaWindow(Window* window);                                    // Mac 0x366c80, Win 0x448200
    virtual ~LuaWindow();                                                   // Mac 0x368020/0x3680c0, Win 0x447000 (sdd 0x447060)

    // <Menu>: builds the menu (BuildLuaMenu) and gives it to the window (Window::SetMenu); then
    // LuaLayout's <OnLoad>. Mac 0x367da0, Win 0x448290
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args);
};

class LuaModalUpdateReciever : public ModalUpdateReciever, public Object {
public:
    // Win 0x447200 (inlined on Mac)
    explicit LuaModalUpdateReciever(const LuaPlus::LuaObject& function) : m_function(function) {}
    virtual ~LuaModalUpdateReciever();                                      // Mac 0x368230/0x3682a0, Win 0x447280 (sdd 0x447320)

    // Calls the function (no arguments, through LuaState::PCall). Mac 0x3682e0, Win 0x4472f0
    virtual void ModalUpdate();

private:
    LuaPlus::LuaObject m_function;   // Mac +0x0c
};

// A new Lua "Window" instance bound to `window` through a new LuaWindow.
// NOTE: both builds return nil when the window already has a Lua table (objectMap entry);
// callers look the table up first (LuaControlManager GetWindow). Mac 0x366b40, Win 0x447090
LuaPlus::LuaObject CreateLuaWindow(Window* window, LuaPlus::LuaState* state);

// The Window of argument `narg` (else a Lua type error naming Window).
// NOTE: both builds cross-cast the *bound object* to Window (Mac 0x368160 dynamic_cast to
// N4LEGO6WindowE, Win 0x448ac0 / 0x448c90); a LuaWindow adapter is not a Window, so Window's Lua
// methods raise that error for every table from CreateLuaWindow (the shipped scripts never call
// them).
Window* CheckLuaWindow(LuaPlus::LuaState* state, int narg);

// Fills `menu` from the children of `node`:
//   <MenuItem text="…" | locText="…" command="path" keyCommand="path" key="Ctrl, Shift, X">:
//     with children a submenu; without, a command item (short-cut when key and command are
//     given); without text a separator;
//   <Access script="…"> (or the element's text): the children are added when the Lua chunk,
//     called with `table` (nil when null), returns true.
// Command paths are looked up from `table` (null: the globals). False when a submenu failed.
// Mac 0x3670f0 (state in ECX, node in EDX), Win 0x447930. (name inferred)
bool BuildLuaMenu(LuaPlus::LuaState* state, const XMLDOMNode* node, Menu* menu,
                  LuaPlus::LuaObject* table);
// `menu` = a new Menu filled by BuildLuaMenu; null (false) when that failed.
// Mac 0x367040, Win 0x448110. (name inferred)
bool CreateLuaMenu(LuaPlus::LuaState* state, const XMLDOMNode* node, RefPointer<Menu>& menu,
                   LuaPlus::LuaObject* table);

// Registers Window (base Layout) {ModalRun(func), ModalStop, SetModal([modal]), IsModal,
// New/Create = LuaReturnNothing}. Mac 0x366d30, Win 0x447500. (name inferred)
void RegisterLuaWindow(LuaPlus::LuaState* state);

} // namespace LEGO
