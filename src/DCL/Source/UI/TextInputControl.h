// LEGO Digital Designer — restored source
// LEGO::TextInputControl — a text field: a native edit control over the control's rectangle
// (Mac: NSTextField in the content view; Win: an "edit" child window shown only while focused,
// the text drawn through a TextLayoutRender otherwise); optional number input (Int / UInt / Real).
// Watches its ancestors to keep the field in place; TextInputControlObservers hear Return
// (OnTextEntered) and text changes. Lua: LuaTextInputControl (TU 390).
// TU 325 "TextInputControl" (Mac 0x2d80e0-0x2daadb: TextInputControlMac.mm with
// LEGOTextInputControlTarget and the LEGO*NumberFormatter classes; Win 0x514380-0x51551e incl.
// 0x5144da-0x5150d6). Sources: TextInputControl.cpp (shared + Win), TextInputControlMac.mm.
// Names inferred except ObjC names and FromString's keywords.
//
// Layout (Mac, from the type encoding of the LEGOTextInputControlTarget ivar): Control
//   +0x000..+0x158, Notifier<TextInputControlObserver> +0x158 (vptr, observers +0x15c,
//   iterators +0x16c), ControlObserver +0x17c (vptr, subjects +0x180), B×6 +0x190..+0x195,
//   I +0x198, i +0x19c, Color +0x1a0, ^v×3 +0x1b0/+0x1b4/+0x1b8, Object +0x1bc (sizeof 0x1c4).
//   Win: B×6 +0x1a8, max length +0x1b0, number type +0x1b4, colour +0x1b8, fraction digits
//   +0x1c8, HWND +0x1cc, HIMC +0x1d0, text layout +0x1d4, Object +0x1dc.
// Virtual slots: Mac 0x74a01c (38: Control's 36 + the two ControlObserver overriders 36/37);
//   Win 0xa287e4 (33), ControlObserver-in 0xa2886c.
#pragma once
#include "LEGO/Types.h"
#include "Control.h"
#include "Color.h"
#include "Notifier.h"
#include "ObserverN.h"
#include "LEGO/String.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class TextInputControl;
class Font;
class TextLayoutRender;

// Mac vtable 0x70c1ac {D1 0x1283a0, D0 0x1283f0, OnFinalRelease 0x127c90, OnAttached 0x128460,
// OnDetached 0x128470, OnTextEntered 0x128480, OnTextChanged 0x128490} (emitted with HingeTool);
// Win 0x9dbb94 {OnAttached, OnDetached, OnTextEntered, OnTextChanged} (all 0x40be80, empty).
class TextInputControlObserver : public ObserverN<TextInputControl, TextInputControlObserver> {
public:
    TextInputControlObserver() {}
    virtual ~TextInputControlObserver() {}
    // Return in a single-line field (Mac: the NSTextField action). (name inferred)
    virtual void OnTextEntered(TextInputControl* /*control*/) {}   // Mac 5, Win 2
    // The text changed (typing, paste, SetText). (name inferred)
    virtual void OnTextChanged(TextInputControl* /*control*/) {}   // Mac 6, Win 3
};

class TextInputControl : public Control,
                         public Notifier<TextInputControlObserver>,
                         private ControlObserver {
public:
    // What the field accepts (Lua keywords "FreeText", "Int", "UInt", "Real").
    enum NumberType {
        FreeText   = 0,
        IntNumber  = 1,
        UIntNumber = 2,
        RealNumber = 3
    };

    explicit TextInputControl(ControlManager* manager);          // Mac 0x2d89f0, Win 0x514380
    virtual ~TextInputControl();                                 // Mac 0x2d9740/0x2d98f0 (body 0x2d9540), Win 0x5149cb/0x5144aa

    // Creates the native field (mouse + keyboard input on). Mac 0x2d8c30, Win 0x5144da
    bool Create();
    // Sets the text and notifies OnTextChanged; true. Mac 0x2d8e00, Win 0x5146ca
    bool SetText(const String& text);
    String GetText() const;                                      // Mac 0x2d90c0, Win 0x514748
    // A FontPlatform's native font for the field; false for other fonts. Mac 0x2d9100, Win 0x51478c
    bool SetFont(Font* font);
    // Height for `lines` lines of the field's font (width kept). Mac 0x2d9170, Win 0x5147e5
    bool SetLineCount(UInt32 lines);
    NumberType GetNumberType() const { return (NumberType)m_numberType; }
    // Changing it clears the text. Mac 0x2d9240, Win 0x5148a0
    void SetNumberType(NumberType type);
    // Digits after the decimal point for Real fields (Win default 3). Mac 0x2d9470, Win 0x5148e5
    void SetFractionDigits(UInt32 digits);
    // The text as a number (0 when it is none). Mac 0x2d94a0, Win 0x5148f2
    Real32 GetFloat() const;
    void SetFloat(Real32 value);                                 // Mac 0x2d94d0, Win 0x51497f
    // Colour drawn over a disabled field. Mac 0x2d9510 (Win inlined)
    void SetDisabledColor(const Color& color) { m_disabledColor = color; }
    const Color& GetDisabledColor() const { return m_disabledColor; }

    // Native style switches (Win CreateWindowEx styles; read when the field is created); no setters
    // in the binaries (LuaTextInputControl sets them through inlined code).
    bool   m_multiLine;        // +0x190 / +0x1a8 false  ES_MULTILINE | ES_WANTRETURN
    bool   m_numbersOnly;      // +0x191 / +0x1a9 false  ES_NUMBER
    bool   m_autoHScroll;      // +0x192 / +0x1aa true   ES_AUTOHSCROLL
    bool   m_hScrollBar;       // +0x193 / +0x1ab false  WS_HSCROLL
    bool   m_autoVScroll;      // +0x194 / +0x1ac false  ES_AUTOVSCROLL
    bool   m_vScrollBar;       // +0x195 / +0x1ad false  WS_VSCROLL
    UInt32 m_maxLength;        // +0x198 / +0x1b0 1024 (Win EM_LIMITTEXT)

    // Observer notifications (Mac: called by the LEGOTextInputControlTarget delegate / action;
    // Win: inlined in the edit window procedure and SetText). Mac 0x2d8140 / 0x2d83f0
    void NotifyTextEntered();
    void NotifyTextChanged();
#if defined(LDD_LINUX)
    // LDD64 Linux: the GtkEntry's text changed: the GL text follows (Win: HandleEditMessage).
    void SyncTextLayout();
#endif

    // ---- Control overrides (Mac slot / Win slot) ----
    // Always true: the whole rectangle takes the mouse. Mac 0x2da7b0, Win 0x51611f
    virtual bool IsInside(const Vector2& point) const;           // 3/0
    // Moved below another parent: watch the new ancestors, move the native field.
    // (Control's slot 6, see Control.h) Mac 0x2d9c60, Win 0x514f16
    virtual void OnDestroyed();                                  // 6/3
    virtual void OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize); // 8/5 Mac 0x2d9d60, Win 0x514f38
    virtual void OnShown();                                      // 10/7  Mac 0x2d9e00, Win 0x514f53
    virtual void OnHidden();                                     // 11/8  Mac 0x2d9e30, Win 0x514f58
    virtual void OnEnabled();                                    // 12/9  Mac 0x2d9e60, Win 0x514f6a
    virtual void OnDisabled();                                   // 13/10 Mac 0x2d9e90, Win 0x514f6f
    // Win: show the edit window, focus it, place the caret (click at the mouse), IME on.
    virtual void OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers); // 26/23 Mac 0x2d9c40, Win 0x514e4d
    // Win: IME off, hide the edit window.
    virtual void OnKeyboardFocusLost(Control* next);             // 27/24 Mac 0x2d9c50, Win 0x514ee4
    virtual void Draw(ControlDrawArgs& args);                    // 33/30 Mac 0x2d9ec0, Win 0x514f74

protected:
    // Leaves the watched ancestors and the observers, then Control's part. Mac 0x2d9ae0, Win 0x514a74
    virtual void OnFinalRelease();

    // ---- ControlObserver handlers for the ancestors (Mac primary slots 36/37, thunks
    // 0x2da050 / 0x2da100; Win ControlObserver-in slots 4 / 6) ----
    virtual void OnParentChanged(Control* control);              // 36 Mac 0x2d9f50, Win 0x515087
    virtual void OnRectangleChanged(Control* control, const Vector2& oldPosition,
                                    const Vector2& oldSize);     // 37 Mac 0x2da070, Win 0x5150a5
    using Control::OnParentChanged;
    using Control::OnRectangleChanged;

private:
    // The ControlObserver handlers of the same names stay the (empty) ControlObserver ones.
    using ControlObserver::OnDestroyed;
    using ControlObserver::OnShown;
    using ControlObserver::OnHidden;
    using ControlObserver::OnEnabled;
    using ControlObserver::OnDisabled;
    using ControlObserver::OnKeyboardFocusGained;
    using ControlObserver::OnKeyboardFocusLost;

    // Watches every ancestor (ControlObserver). Win 0x5150b5 (Mac inlined)
    void AttachToAncestors();
    // Native field frame = absolute position + size. Win 0x5150d6 (Mac inlined: setFrame:)
    void UpdateNativeFrame();
    // Platform part of the destructor (Mac: release the field, target, formatter; Win: IME
    // context, DestroyWindow).
    void DestroyNative();

#if defined(_WIN32)
public:
    // `value` as SetFloat shows it: "%.<n>G" (VS2008 CRT) with n = integer digits +
    // `fractionDigits`. The original is a String helper (Win 0x402100, append to a String; no
    // Mac twin) that is not restored with String.cpp; kept here. (name inferred)
    static String FormatFloat(Real32 value, UInt32 fractionDigits);

    // One subclassed edit window (RTTI BasicArray<OSWinInfo>; member names inferred).
    struct OSWinInfo {
        HWND                 window;         // +0
        WNDPROC              previousProc;   // +4
        TextInputControl*    control;        // +8
    };
private:
    // The subclassing window procedure: finds the window's entry (dropped on WM_NCDESTROY) and hands
    // the message to HandleEditMessage. Win 0x5142aa
    // NOTE: the original wraps the call in __try/__except (crash-report filter 0x5ce42c, result 0);
    // not reproduced.
    static LRESULT CALLBACK EditWindowProc(HWND window, UINT message, WPARAM wParam, LPARAM lParam);
    // Calls the edit control's own procedure (Return of a single-line field is swallowed), filters
    // number input (reverting text and selection for a disallowed character), keeps the text layout
    // in step and notifies. Win 0x514a90
    LRESULT HandleEditMessage(WNDPROC previousProc, UINT message, WPARAM wParam, LPARAM lParam);
#endif

    // ---- data (Mac / Win offsets) ----
    UInt32 m_numberType;       // +0x19c / +0x1b4 FreeText
    Color  m_disabledColor;    // +0x1a0 / +0x1b8 (0.5, 0.5, 0.5, 0.5)
#if defined(_WIN32)
    UInt32                       m_fractionDigits;   // +0x1c8 3
    HWND                         m_window;           // +0x1cc
    void*                        m_inputContext;     // +0x1d0 HIMC (ImmGetContext)
    RefPointer<TextLayoutRender> m_textLayout;       // +0x1d4 the text when the field is hidden
#else
    // Mac: NSTextField*, LEGOTextInputControlTarget*, NSNumberFormatter*.
    // HOST-ONLY (Mac branch without Objective-C): m_field holds a HostTextField (see .mm).
    void*  m_field;            // +0x1b0
    void*  m_target;           // +0x1b4
    void*  m_formatter;        // +0x1b8
#if defined(LDD_LINUX)
    // LDD64 Linux (not restored code): the field is drawn in GL as on Windows (the text when the
    // GtkEntry is hidden); the entry shows only while the control has the keyboard focus, so what
    // LDD draws over the field (the start screen, menus) covers it.
    RefPointer<TextLayoutRender> m_textLayout;
#endif
#endif
};

// Keyword at `position` ("FreeText", "Int", "UInt", "Real"); false otherwise (value unchanged).
// Mac 0x2da190, Win 0x51511a (name inferred)
bool FromString(TextInputControl::NumberType& value, const String& text, UInt32& position);

} // namespace LEGO
