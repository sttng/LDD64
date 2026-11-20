// LEGO Digital Designer — restored source
// LEGO::LuaTextInputControl — Lua class "TextInputControl" (base Control), the LuaControl of a
// TextInputControl (native edit field); forwards OnTextEntered / OnTextChanged to Lua.
// TU 390 "LuaTextInputControl.cpp": Mac 0x37ad70-0x37c54b, Win 0x45de70-0x45ed0f; also emits the
// ObserverN<TextInputControl, TextInputControlObserver> dtor thunks.
//
// RTTI vmi {LuaControl +0, TextInputControlObserver +0x4c}, virtual base Object.
// Mac primary vtable 0x75346c (26 entries: LuaControl's 24 with D1 0x37b420, D0 0x37b780,
// OnLayoutApplied 0x37bb00, OnFinalRelease 0x37b8c0, Attach 0x37beb0, Detach 0x37bfb0;
// Mac-only entries 24/25 of the overriders OnTextEntered 0x37c020, OnTextChanged 0x37c1e0),
// TextInputControlObserver-in 0x7535b8 {…, 0x37c150, 0x37c270}.
// Win 0xa21fd4 {…, OnLayoutApplied 0x45e520, …}, ControlAugmentation-in 0xa21ffc {…, Attach
// 0x45e770, Detach 0x45e7b0}, TextInputControlObserver-in 0xa2209c {0x40be80, 0x40be80,
// OnTextEntered 0x45e7e0, OnTextChanged 0x45e860}, Object-in 0xa220b0 {sdd 0x45f020 / 0x45de70,
// …}.
// Layout Mac (ctor inlined in Create 0x37b170): LuaControl +0x00..+0x4c, TextInputControlObserver
// +0x4c (subjects +0x50), m_textInput +0x60, Object +0x64 (sizeof 0x6c). Names inferred unless
// they are Lua / layout strings.
#pragma once
#include "LEGO/Types.h"
#include "TextInputControl.h"
#include "LuaControl.h"
#include "LuaPlus.h"

namespace LEGO {

class XMLDOMNode;

#if defined(__GNUC__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Woverloaded-virtual"
#endif
class LuaTextInputControl : public LuaControl, private TextInputControlObserver {
public:
    LuaTextInputControl() : m_textInput(0) {}                               // inlined in Create (Mac 0x37b170)
    virtual ~LuaTextInputControl();                                         // Mac 0x37b420/0x37b780, Win 0x45e440 / sdd 0x45de70

    TextInputControl* GetTextInput() const { return m_textInput; }

protected:
    // LuaControl's, then font, text, resizeHeightToFitLines, mode (number type), disabledColor,
    // <OnTextEntered>, <OnTextChanged> ("sender").
    virtual void OnLayoutApplied(const XMLDOMNode* layout, LuaLayoutArgs& args); // Mac 0x37bb00, Win 0x45e520
    // Leaves the text inputs, then LuaControl's.
    virtual void OnFinalRelease();                                          // Mac 0x37b8c0 (thunks 0x37b930 …)
    virtual bool Attach(Control* control, const String& name);              // Mac 0x37beb0 (thunk 0x37bf30), Win 0x45e770
    virtual void Detach();                                                  // Mac 0x37bfb0 (thunk 0x37bfe0), Win 0x45e7b0

    // ---- TextInputControlObserver: self:OnTextEntered(self) / self:OnTextChanged(self) ----
    // (helper Mac 0x37c0b0 / Win 0x45ec60 -> CallLuaMethod<LuaObject>, Mac 0x37c3d0)
    virtual void OnTextEntered(TextInputControl* control);                  // Mac 0x37c020 (thunk 0x37c150), Win 0x45e7e0
    virtual void OnTextChanged(TextInputControl* control);                  // Mac 0x37c1e0 (thunk 0x37c270), Win 0x45e860

private:
    TextInputControl* m_textInput;   // Mac +0x60 (dynamic_cast of the control)
};
#if defined(__GNUC__)
#pragma GCC diagnostic pop
#endif

// The TextInputControl bound to argument `narg`, or a Lua type error naming TextInputControl.
// Header inline in the original (copy Mac 0x37c300 in TU 390; Win 0x45e940); out of line here.
// (name inferred)
TextInputControl* CheckLuaTextInputControl(LuaPlus::LuaState* state, int narg);

// Registers TextInputControl (base Control) {SetText, GetText, ResizeHeightToFitLines, SetFont
// (upvalue `fonts`), Create (upvalue `manager`)}. Mac 0x37ad70, Win 0x45e330 (name inferred)
void RegisterLuaTextInputControl(LuaPlus::LuaState* state, ControlManager* manager,
                                 const XMLDOMNode* fonts);

} // namespace LEGO
