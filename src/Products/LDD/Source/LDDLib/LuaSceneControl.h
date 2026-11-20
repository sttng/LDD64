// LEGO Digital Designer — restored source
// LEGO::LuaSceneControl — Lua class "SceneControl" (base Control): the LuaControl of the
//   BrickConnectSceneControl (the 3D view). Lua: self:ResetCamera(), self:RotateLeftRight(a),
//   self:RotateUpDown(a), self:ZoomInOut(f) (scaled by the camera distance); New / Create do
//   nothing (the table comes from CreateLuaObject).
//
// TU 278 "LuaSceneControl.cpp": Mac 0x291050-0x291c9b, Win 0x605c47-0x60625f (+ 0x448dd0, the
// BindableLuaObject deleting dtor emitted here). The TU also emits BindableLuaObject's inline
// virtuals (Mac 0x291a70-0x291b00, see BindableLuaObject.h).
// RTTI si LuaControl. Mac vtable 0x73f084 (24 entries: LuaControl's with D1 0x291530, D0
// 0x291710, Attach 0x291950, Detach 0x291a30), ControlAugmentation-in 0x73f0f0 {…, 0x2919c0,
// 0x291a50, …}, Object-in 0x73f1d4; VTT 0x73f1f4 (virtual base Object). Win vtables
// 0x9ce5a4 / 0x9ce594 {GetControl, Attach 0x605ddd, Detach …}, Object-in 0x9ce4ec {sdd 0x605dc2}.
// Layout: LuaControl (Mac +0x00..+0x4c), m_sceneControl Mac +0x4c / Win +0x50 (Win Attach writes
// ControlAugmentation-this +0x40), Object Mac +0x50 (operator new Mac 0x58, Win 0x60).
// Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "LuaControl.h"

namespace LEGO {

class BrickConnectSceneControl;

class LuaSceneControl : public LuaControl {
public:
    LuaSceneControl() : m_sceneControl(0) {}                                 // inlined (Mac 0x2913b0, Win 0x605c47)
    virtual ~LuaSceneControl() {}                                            // Mac 0x291530/0x291710, Win sdd 0x605e15

    BrickConnectSceneControl* GetSceneControl() const { return m_sceneControl; }

    // The table of the Lua class SceneControl for `control` (CreateLuaControlTable<
    // LuaSceneControl>, Mac 0x2913b0 / Win 0x605e49); nil when the control refused it.
    static LuaPlus::LuaObject CreateLuaObject(Control* control, LuaPlus::LuaState* state); // Mac 0x291310, Win 0x605ee9

protected:
    // LuaControl::Attach, then remember the control as a BrickConnectSceneControl.
    virtual bool Attach(Control* control, const String& name);              // Mac 0x291950 (thunk 0x2919c0), Win 0x605ddd
    virtual void Detach();                                                  // Mac 0x291a30 (thunk 0x291a50)

private:
    BrickConnectSceneControl* m_sceneControl;   // Mac +0x4c, Win +0x50 (dynamic_cast of the control)
};

// The BrickConnectSceneControl of the LuaSceneControl bound to argument `narg`; a Lua type
// error naming BrickConnectSceneControl otherwise. Header inline in the original (copies Mac
// 0x291b50 in this TU, inline in LuaLDDApplication Import 0x382740; Win 0x605f94).
BrickConnectSceneControl* CheckLuaSceneControl(LuaPlus::LuaState* state, int narg);

// Registers the Lua class SceneControl (base Control) {ResetCamera, RotateLeftRight,
// RotateUpDown, ZoomInOut, New = Create = no-op}.
void RegisterLuaSceneControl(LuaPlus::LuaState* state);                    // Mac 0x291050, Win 0x606146

} // namespace LEGO
