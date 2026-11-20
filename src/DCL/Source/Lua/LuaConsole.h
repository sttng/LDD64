// LEGO Digital Designer — restored source
// LEGO::LuaConsole — the developer Lua console: a one-line text input under a scrolling
// TextOutputControl (in the given ContainerControl). Return runs the line as a Lua chunk (output
// and errors to the pane, _ERRORREPORTER redirected meanwhile); "/p expr" prints the Lua path
// `expr` (tables one level deep).
// TU 320 "LuaConsole.cpp": Mac 0x2c98c0-0x2cb14b, Win 0x456630-0x4573bf. Created by AppMain
// (Mac 0x13cfd3: new LuaConsole(manager, container, state), then Initialize).
//
// RTTI vmi {BufferedAbleRenderControl +0, TextInputControlObserver +0x194 (Win +0x1a8) private,
// ControlObserver +0x1a8 (Win +0x1c0) private}, virtual base Object.
// Mac vtables 0x7494cc (41 entries: BufferedAbleRenderControl's 38 with OnFinalRelease
// 0x2ca060, + Initialize 0x2c9af0, OnShown 0x2ca180, OnTextEntered 0x2ca1e0),
// TextInputControlObserver-in 0x74957c {…, OnTextEntered thunk 0x2caa90}, ControlObserver-in
// 0x7495a4 {…, OnShown thunk 0x2ca1b0}, Object-in 0x749644.
// Win 0xa21514 (36 entries: …, Initialize 0x4568d0), 0xa215a8 {…, OnTextEntered 0x457150},
// 0xa215bc {…, OnShown 0x457120}, Object-in 0xa2163c {sdd 0x457800, OnFinalRelease 0x4577e0}.
// Layout Mac (ctor 0x2c98c0): BufferedAbleRenderControl +0x000..+0x194, TextInputControlObserver
//   +0x194, ControlObserver +0x1a8, members below, Object +0x1cc (sizeof 0x1d4). Win: input
//   +0x1d8, output +0x1dc, container +0x1e0, state +0x1e4, vtordisp +0x1e8, Object +0x1ec.
// Method and member names are inferred.
#pragma once
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "BufferedAbleRenderControl.h"
#include "TextInputControl.h"
#include "TextOutputControl.h"
#include "ContainerControl.h"
#include "LuaPlus.h"

namespace LEGO {

class LuaConsole : public BufferedAbleRenderControl,
                   private TextInputControlObserver,
                   private ControlObserver {
public:
    // `container` (referenced) shows the output pane; `state` runs the input. Mac 0x2c98c0
    LuaConsole(ControlManager* manager, ContainerControl* container, LuaPlus::LuaState* state);
    virtual ~LuaConsole();                                                  // Mac 0x2caab0/0x2cab10, Win 0x456630 (sdd 0x456740)

    // Creates the input field (system font 15, black; one line, bottom-aligned) and the output pane
    // (in the container, above the input); minimum size 10 x twice the input height; shown, enabled,
    // active. False when the font cannot be created. Mac slot 38 0x2c9af0, Win 35 0x4568d0
    virtual bool Initialize();

    // The output pane's line, scrolled to the bottom. Win 0x456d50 (inlined on Mac)
    void AddOutputLine(const String& text);

    using BufferedAbleRenderControl::OnShown;   // Control::OnShown() stays visible

protected:
    // Leaves the input field (both observer bases), then Control's. Mac 0x2ca060, Win 0x4577e0 → 0x456da0
    virtual void OnFinalRelease();
    // The input field takes the keyboard focus when it is shown. Mac 39 0x2ca180 (thunk 0x2ca1b0), Win 0x457120
    virtual void OnShown(Control* control);
    // Return in the input field (see the header comment). Mac 40 0x2ca1e0 (thunk 0x2caa90), Win 0x457150
    virtual void OnTextEntered(TextInputControl* control);

private:
    RefPointer<TextInputControl>  m_input;      // Mac +0x1bc
    RefPointer<TextOutputControl> m_output;     // Mac +0x1c0
    RefPointer<ContainerControl>  m_container;  // Mac +0x1c4
    LuaPlus::LuaState*            m_state;      // Mac +0x1c8
};

// Appends the printable form of `object` to `out`: nil, true/false, numbers, quoted strings,
// "[Table]" (+ " - bound to [<RTTI name>]"; with `fields`: "\r\n  key = value" per entry),
// "[C Function]" / "[Lua Function]", "[User data]", "[Light user data]", "[Thread]".
// Mac 0x2ca5e0 (object in ECX, out in EDX), Win 0x456e80. (name inferred)
void DumpLuaValue(LuaPlus::LuaObject& object, String& out, bool fields);

} // namespace LEGO
