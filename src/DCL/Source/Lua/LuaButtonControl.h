// LEGO Digital Designer — restored source
// LEGO::LuaButtonControl — Lua class "ButtonControl" (base Control, no own Lua functions), the
// LuaControl of a ButtonControl; `command="lua.path"` connects it to a Command. Base of
// LuaToggleControl, LuaSliderControl (TU 317, 387) and LuaToolButtonControl (TU 281).
// TU 314 "LuaButtonControl.cpp": Mac 0x2bb7a0-0x2bda2b, Win 0x4661c0-0x467caf (+ 0x509ebf); also
// emits the inlines ParseCursorShapeOptional (Mac 0x2bd550, LuaControl.h) and
// FromStringOptional(String&) (Mac 0x2bd900, StringStream.h).
//
// RTTI vmi {LuaControl +0, CommandObserver +0x4c private}, virtual base Object.
// Mac primary vtable 0x73fbbc (26 entries: LuaControl's 24 with D1 0x2bbbe0, D0 0x2bbf50,
// OnLayoutApplied 0x2bc530, OnFinalRelease 0x2bc360, ApplyRenderAttributes 0x2bcbf0, Attach
// 0x2bd600, Detach 0x2bd740, OnMouseClicked 0x2bd780; Mac-only primary entries 24/25 of the
// CommandObserver overriders OnAttached 0x2bd820, OnEnableChanged 0x2bd8a0), CommandObserver-in
// 0x73fd08 {…, 0x2bd860, 0x14ea80, 0x2bd8d0, 0x14eaa0}; VTT (base-object ctor Mac 0x2bb7a0 and
// dtor 0x2bbb80, used by LuaToggleControl / LuaSliderControl / LuaToolButtonControl).
// Win vtables 0xa22cb8 {0x44e240, 0x44e260, OnLayoutApplied 0x466e10, 0x44bb90, 0x44d270,
// 0x44d980, ApplyRenderAttributes 0x466fb0, 0x44e0d0, 0x44e1d0}, ControlAugmentation-in 0xa22ce0
// {0x52f4b0, Attach 0x467bd0, Detach 0x467c40}, ControlObserver-in 0xa22cf4 {…, OnMouseClicked
// 0x467c50, …}, CommandObserver-in 0xa22d84 {OnAttached 0x467cb0, 0x40be80, OnEnableChanged
// 0x467d00, 0x40be80}, Object-in 0xa22d98 {sdd 0x4680b0, …}.
// Layout Mac: LuaControl +0x00..+0x4c, CommandObserver +0x4c (subject +0x50), m_button +0x54,
// Object +0x58 (sizeof 0x60). Win: CommandObserver +0x50, m_button +0x5c, Object +0x64 (sizeof
// 0x6c, ctor 0x466920). Names inferred unless they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Command.h"
#include "LuaControl.h"

namespace LEGO {

class ButtonControl;
class ControlManager;

// The OnAttached(Command* / BrickConnectSceneManager*) overrides hide the other private observer
// bases' overloads, which a using-declaration cannot re-expose (inaccessible / ambiguous).
#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaButtonControl : public LuaControl, private CommandObserver {
public:
    LuaButtonControl();                                                     // Mac C2 0x2bb7a0 (C1 inlined in Create), Win 0x466920
    virtual ~LuaButtonControl();                                            // Mac D2 0x2bbb80, D1/D0 0x2bbbe0/0x2bbf50, Win 0x4661c0 / sdd 0x4680b0

    // The ButtonControl of the bound control (dynamic_cast in Attach). (name inferred)
    ButtonControl* GetButton() const { return m_button; }

protected:
    // LuaControl's, then the `command` attribute and the state children <Up>, <Over>, <Down>,
    // <Disabled> (state descriptions 0-3).
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 4 0x2bc530, Win 0x466e10
    // Leaves the command, then LuaControl's.
    virtual void OnFinalRelease();                                          // Mac 6 0x2bc360 (thunks 0x2bc3b0 …), Win inlined
    // LuaControl's, then cursor / blendColor / text / locText / font / image of the node into every
    // state description (the values the control now has; the cursor parsed here).
    virtual void ApplyRenderAttributes(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 9 0x2bcbf0, Win 0x466fb0
    // LuaControl's; then the button (dynamic_cast) follows the command's enable state.
    virtual bool Attach(Control* control, const String& name);              // Mac 12 0x2bd600 (thunk 0x2bd6a0), Win 0x467bd0
    virtual void Detach();                                                  // Mac 13 0x2bd740 (thunk 0x2bd760), Win 0x467c40
    // LuaControl's; a left click on the control runs the command.
    virtual void OnMouseClicked(Control* control, const Vector2& position, UInt32 button); // Mac 21 0x2bd780 (thunk 0x2bd7d0), Win 0x467c50

    // ---- CommandObserver: the button's enable state follows the command ----
    virtual void OnAttached(Command* command);                              // Mac 24 0x2bd820 (thunk 0x2bd860), Win 0x467cb0
    virtual void OnEnableChanged(bool enabled);                             // Mac 25 0x2bd8a0 (thunk 0x2bd8d0), Win 0x467d00

    // Applies the layout child `name` of `layout` (when present) to state description `index`:
    // blendColor, image (subRect / subPosSize), text, locText, font. Also used by
    // LuaToggleControl (descriptions 4-7). Mac 0x2bc740, Win 0x4677a0 (name inferred)
    void ApplyStateLayout(UInt32 index, const String& name, const XMLDOMNode* layout,
                          LuaLayoutArgs& args);

    // The observed command (private CommandObserver's subject), for the derived Lua controls.
    // (restoration accessor)
    Command* GetCommand() const { return CommandObserver::GetSubject(); }

private:
    ButtonControl* m_button;   // Mac +0x54, Win +0x5c (dynamic_cast of the control)
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// The ButtonControl bound to argument `narg`, or a Lua type error naming ButtonControl. Header
// inline in the original (copies e.g. Mac 0x38c990 in LuaHingeTool; Win 0x64c197); out of line
// in LuaButtonControl.cpp here. (name inferred)
ButtonControl* CheckLuaButtonControl(LuaPlus::LuaState* state, int narg);

// ButtonControl.Create(self, parent): a new, initialized ButtonControl of the control manager
// (upvalue 1) bound to `self` through a new LuaButtonControl (flags 7, empty rectangle).
// Mac 0x2bb8f0, Win 0x466a40 (name inferred)
int LuaButtonControlCreate(lua_State* L);

// Registers the Lua class ButtonControl (base Control) {Create (upvalue `manager`)}.
// Mac 0x2bb820, Win 0x466c50 (name inferred)
void RegisterLuaButtonControl(LuaPlus::LuaState* state, ControlManager* manager);

} // namespace LEGO
