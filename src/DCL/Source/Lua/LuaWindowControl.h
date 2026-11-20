// LEGO Digital Designer — restored source
// LEGO::LuaWindowControl — Lua class "WindowControl" (base Control): a framed control whose
// content (first child of <Content>) sits at contentRect margins; frame and content sizes follow
// each other. Minimize() keeps only the title strip (10 units wide; self:OnMinimized(minimized)),
// Restore(); position / size / minimized state go to preference `preference` (savePosition,
// saveSize).
// TU 318 "LuaWindowControl.cpp": Mac 0x2c70e0-0x2c880b, Win 0x462f90-0x463caf.
// RTTI si LuaControl. Mac vtables 0x748d6c (LuaControl's 24 slots with D1 0x2c7670, D0
// 0x2c7890, OnLayoutApplied 0x2c7b00, DefineCallbacks 0x2c84c0, Attach 0x2c8510,
// OnRectangleChanged 0x2c85b0), ControlAugmentation-in 0x748dd8 {…, Attach thunk 0x2c8560},
// ControlObserver-in 0x748df8 {…, OnRectangleChanged thunk 0x2c86a0}, Object-in 0x748ebc.
// Win 0xa226e8 {…, OnLayoutApplied 0x4633e0, …, DefineCallbacks 0x463b40, …},
// ControlAugmentation-in 0xa22710 {…, Attach 0x463b70, …}, ControlObserver-in 0xa22724
// {…, OnRectangleChanged 0x463bb0, …}, Object-in 0xa227b4 {sdd 0x463e00, OnFinalRelease 0x463e10}.
// Layout Mac (ctor 0x2c70e0): LuaControl +0x00..+0x4c, members below, Object +0x84 (sizeof
// 0x8c). Win: content +0x50, minimized +0x54, recursion +0x58, restored size +0x5c, contentRect
// +0x64, preference +0x74, saveSize +0x78, savePosition +0x79, vtordisp +0x7c, Object +0x80.
// Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "LuaControl.h"

namespace LEGO {

class LuaWindowControl : public LuaControl {
public:
    LuaWindowControl();                                                     // Mac 0x2c70e0, Win 0x463330
    virtual ~LuaWindowControl();                                            // Mac 0x2c7670/0x2c7890 (base 0x2c75c0), Win sdd 0x462f90

    // LuaControl's, then contentRect, <Content>, saveSize, savePosition, preference.
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x2c7b00, Win 0x4633e0

    // Minimizes (true) or restores (false): self:OnMinimized(minimized), frame resized, Layout.
    void SetMinimized(bool minimized);                                      // Mac 0x2c82e0, Win 0x4636f0

protected:
    // LuaControl's plus <OnMinimized> (parameter "minimized"). Mac 0x2c84c0, Win 0x463b40
    virtual void DefineCallbacks(const XMLDOMNode* layout, LuaLayoutArgs& args);
    // LuaControl's; then the saved state is loaded when a preference key is set.
    // Mac 0x2c8510 (thunk 0x2c8560), Win 0x463b70
    virtual bool Attach(Control* control, const String& name);
    // Frame resized: remember the size, save, re-layout the content. Content resized: resize
    // the frame. (At most 3 nested calls.) NOTE: the Lua OnRectangleChanged is not called.
    // Mac 0x2c85b0 (thunk 0x2c86a0), Win 0x463bb0
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);

    // Content size = restored size minus the margins (the frame follows when the content keeps
    // another size), content aligned at the top-left margin (minimized: right of the frame,
    // 10 units in). Mac 0x2c8000, Win 0x463760
    void Layout();
    // "x y, w h, minimized" from the preference (or SavePreference without one). Mac 0x2c8160, Win 0x463890
    void LoadPreference();
    // Mac 0x2c83c0, Win 0x463a00
    void SavePreference();

    RefPointer<Control> m_content;        // Mac +0x4c
    bool                m_minimized;      // Mac +0x50
    UInt32              m_recursion;      // Mac +0x54 nesting of OnRectangleChanged
    Vector2             m_restoredSize;   // Mac +0x58 frame size while not minimized
    Rect                m_contentRect;    // Mac +0x60 margins left, top, right, bottom
    String              m_preference;     // Mac +0x70 preference key ("preference")
    bool                m_saveSize;       // Mac +0x80 true ("saveSize")
    bool                m_savePosition;   // Mac +0x81 true ("savePosition")
};

// Registers WindowControl (base Control) {Minimize, Restore, Create (upvalue `manager`)}.
// Mac 0x2c7200, Win 0x463250. (name inferred)
void RegisterLuaWindowControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
