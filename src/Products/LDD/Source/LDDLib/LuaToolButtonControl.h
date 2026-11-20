// LEGO Digital Designer — restored source
// LEGO::LuaToolButtonControl — Lua class "ToolButtonControl" (base ButtonControl): a button
//   that selects a scene tool. Its layout attribute `tool` names (Lua path) the Tool table of
//   the tool; clicking the button makes that tool the scene manager's active tool, and the
//   button shows state 2 while its tool is active (state mapping 0/1 -> 2) and the normal
//   states otherwise. Observes the BrickConnectSceneManager: callback
//   self:OnToolChanged(oldTool, newTool) (also definable as the layout's <OnToolChanged>).
//   Lua: ToolButtonControl.Create(self, parent) (upvalues: control manager, scene manager),
//   self:GetTool().
//
// TU 281 "LuaToolButtonControl.cpp": Mac 0x293a20-0x294ffb, Win 0x635965-0x635e8f.
// RTTI vmi {LuaButtonControl +0, BrickConnectSceneManagerObserver +0x58}, virtual base Object.
// Mac primary vtable 0x73f93c (28 entries: LuaButtonControl's with D1 0x294180, D0 0x294420,
// OnLayoutApplied 0x294970, OnFinalRelease 0x294740, OnMouseClicked 0x294bd0; Mac-only primary
// entries 26/27 of the BrickConnectSceneManagerObserver overriders OnAttached 0x294c50,
// OnActiveToolChanged 0x294d10), BrickConnectSceneManagerObserver-in 0x73fab8 {…, 0x294cb0,
// 0x1936e0, 0x1936f0, 0x294e90}; VTT 0x73fb00. Win sdd 0x635a1b, OnLayoutApplied 0x635a3a.
// Layout Mac: LuaButtonControl +0x00..+0x58, BrickConnectSceneManagerObserver +0x58 (subject
// +0x5c), m_tool +0x60, Object +0x64 (sizeof 0x6c).
// Function names are inferred (Lua names are the strings).
#pragma once
#include "LEGO/Types.h"
#include "LuaButtonControl.h"
#include "BrickConnectSceneManager.h"

namespace LEGO {

class ControlManager;

// The observer overrides OnAttached(Command* / BrickConnectSceneManager*) hide the OnAttached
// overloads of the other (private) observer bases; they cannot be re-exposed with a using-
// declaration (inaccessible / ambiguous), so the warning is silenced here.
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaToolButtonControl : public LuaButtonControl, public BrickConnectSceneManagerObserver {
public:
    LuaToolButtonControl() : m_tool(0) {}                                   // inlined in Create (Mac 0x293a20)
    virtual ~LuaToolButtonControl();                                        // Mac 0x294180/0x294420 (D2 0x2940b0), Win sdd 0x635a1b

    BrickConnectSceneControlTool* GetTool() const { return m_tool; }

protected:
    // LuaButtonControl's, then <OnToolChanged> ("oldTool, newTool") and the `tool` attribute.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x294970, Win 0x635a3a
    // Leaves the scene manager, then LuaButtonControl's.
    virtual void OnFinalRelease();                                          // Mac 0x294740 (thunks 0x294790 …)
    // LuaButtonControl's, then activates the tool.
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // Mac 0x294bd0 (thunk 0x294c10), Win 0x635965

    // ---- BrickConnectSceneManagerObserver ----
    virtual void OnAttached(BrickConnectSceneManager* manager);             // Mac 0x294c50 (thunk 0x294cb0), Win 0x635b2a
    // Button state, then self:OnToolChanged(objectMap[previous], objectMap[current]).
    virtual void OnActiveToolChanged(BrickConnectSceneControlTool* previous,
                                     BrickConnectSceneControlTool* current); // Mac 0x294d10 (thunk 0x294e90), Win 0x635b48

private:
    // State mapping of the button for `active` being / not being this button's tool: 0 -> 2,
    // 1 -> 2, or 0 -> 0, 1 -> 1. (inline on Mac; Win 0x635991; name inferred)
    void UpdateButtonStates(BrickConnectSceneControlTool* active);

    friend int LuaToolButtonControlCreate(lua_State* L);

    BrickConnectSceneControlTool* m_tool;   // Mac +0x60 (referenced)
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// ToolButtonControl.Create(self, parent): a new ButtonControl of the control manager (upvalue
// 1) bound to `self` through a new LuaToolButtonControl that observes the scene manager
// (upvalue 2). (name inferred)
int LuaToolButtonControlCreate(lua_State* L);                               // Mac 0x293a20, Win 0x635c88

// Registers the Lua class ToolButtonControl (base ButtonControl) {GetTool, Create (upvalues
// `controlManager`, `sceneManager`)}.
void RegisterLuaToolButtonControl(LuaPlus::LuaState* state, ControlManager* controlManager,
                                  BrickConnectSceneManager* sceneManager);  // Mac 0x293f50, Win 0x635df1

} // namespace LEGO
