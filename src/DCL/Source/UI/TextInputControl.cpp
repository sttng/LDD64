// LEGO Digital Designer — restored source
// TU 325 (Mac 0x2d80e0-0x2daadb; Win 0x514380-0x51551e): the platform-independent and Windows
// parts (native EDIT control, 0x5144da-0x5150d6); Mac: TextInputControlMac.mm.
#include "TextInputControl.h"
#include "ControlManager.h"
#include "StringStream.h"
#if defined(_WIN32)
#include "Window.h"
#include "TextLayout.h"
#include "FontPlatformFactory.h"
#include "ObjectFactory.h"
#include "Render.h"
#include "Win32Unicode.h"
#include <imm.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#elif defined(LDD_LINUX)
#include "TextLayout.h"   // LDD64 Linux: m_textLayout (TextInputControlMac.mm)
#endif

namespace LEGO {

typedef NotifierT<TextInputControl, TextInputControlObserver> TextInputNotifier;

// Mac 0x2d89f0, Win 0x514380
TextInputControl::TextInputControl(ControlManager* manager)
    : Control(manager), m_multiLine(false), m_numbersOnly(false), m_autoHScroll(true),
      m_hScrollBar(false), m_autoVScroll(false), m_vScrollBar(false), m_maxLength(0x400),
      m_numberType(FreeText), m_disabledColor(0.5f, 0.5f, 0.5f, 0.5f),
#if defined(_WIN32)
      m_fractionDigits(3), m_window(0), m_inputContext(0)
#else
      m_field(0), m_target(0), m_formatter(0)   // NOTE: the original leaves m_target unset
#endif
{
    m_status.m_bit11 = 1;
}

// Mac 0x2d9740 (D1), 0x2d9540 (D2 body), 0x2d98f0 (D0); Win 0x5149cb, sdd 0x5144aa
TextInputControl::~TextInputControl()
{
    DestroyNative();
}

// Mac 0x2da7b0, Win 0x51611f
bool TextInputControl::IsInside(const Vector2&) const
{
    return true;
}

// Mac 0x2d8140 (called by -[LEGOTextInputControlTarget textEntered:] 0x2d8120); Win inlined
void TextInputControl::NotifyTextEntered()
{
    TextInputNotifier::Iterator it(*this);
    while (TextInputControlObserver* observer = it.Next(TextInputNotifier::m_observers))
        observer->OnTextEntered(this);
}

// Mac 0x2d83f0 (called by -[LEGOTextInputControlTarget controlTextDidChange:] 0x2d83d0); Win inlined
void TextInputControl::NotifyTextChanged()
{
    TextInputNotifier::Iterator it(*this);
    while (TextInputControlObserver* observer = it.Next(TextInputNotifier::m_observers))
        observer->OnTextChanged(this);
}

// Win 0x5150b5 (Mac inlined in 0x2d8c30 / 0x2d9c60 / 0x2d9f50)
void TextInputControl::AttachToAncestors()
{
    for (Control* ancestor = m_parent; ancestor; ancestor = ancestor->GetParent())
        ControlObserver::Attach(ancestor);
}

// Mac 0x2d9ae0 (thunks 0x2d9be0, 0x2d9c00, 0x2d9c20), Win 0x514a74
void TextInputControl::OnFinalRelease()
{
    ControlObserver::OnFinalRelease();
    TextInputNotifier::OnFinalRelease();
    Control::OnFinalRelease();
}

// Mac 0x2d9c60, Win 0x514f16
void TextInputControl::OnDestroyed()
{
    ControlObserver::DetachAll();
    AttachToAncestors();
    UpdateNativeFrame();
    Control::OnDestroyed();
}

// Mac 0x2d9d60, Win 0x514f38
void TextInputControl::OnRectangleChanged(const Vector2& oldPosition, const Vector2& oldSize)
{
    UpdateNativeFrame();
    Control::OnRectangleChanged(oldPosition, oldSize);
}

// Mac 0x2d9f50 (thunk 0x2da050), Win 0x515087
void TextInputControl::OnParentChanged(Control*)
{
    ControlObserver::DetachAll();
    AttachToAncestors();
    UpdateNativeFrame();
}

// Mac 0x2da070 (thunk 0x2da100), Win 0x5150a5
void TextInputControl::OnRectangleChanged(Control*, const Vector2&, const Vector2&)
{
    UpdateNativeFrame();
}

// Mac 0x2da190, Win 0x51511a
bool FromString(TextInputControl::NumberType& value, const String& text, UInt32& position)
{
    if (ParseKeyword(String("FreeText"), text, position, ',')) {
        value = TextInputControl::FreeText;
        return true;
    }
    if (ParseKeyword(String("Int"), text, position, ',')) {
        value = TextInputControl::IntNumber;
        return true;
    }
    if (ParseKeyword(String("UInt"), text, position, ',')) {
        value = TextInputControl::UIntNumber;
        return true;
    }
    if (ParseKeyword(String("Real"), text, position, ',')) {
        value = TextInputControl::RealNumber;
        return true;
    }
    return false;
}

#if defined(_WIN32)
// ===========================================================================================
// Windows: native EDIT control
// ===========================================================================================

namespace {

// The subclassed edit windows (Win .data 0xae1924, a BasicArray<OSWinInfo>; PushBack instance
// 0x515414). (name inferred)
BasicArray<TextInputControl::OSWinInfo>& EditWindows()
{
    static BasicArray<TextInputControl::OSWinInfo> windows;
    return windows;
}

// "%.<precision>G" as the VS2008 CRT prints it (_cftog / _fptostr): 17 significant digits,
// rounded half up to `precision` digits, trailing zeros cropped, 3-digit exponent (the emulation
// of String.cpp's file-local FormatRealVS2008).
void FormatGVS2008(char* out, double value, int precision)
{
    char buffer[40];
    sprintf(buffer, "%.16e", value);              // d.dddddddddddddddde±x: 17 digits
    bool negative = buffer[0] == '-';
    const char* p = buffer + (negative ? 1 : 0);
    char digits[20];
    int count = 0;
    for (; *p && *p != 'e'; ++p)
        if (*p >= '0' && *p <= '9')
            digits[count++] = *p;
    int decpt = atoi(p + 1) + 1;
    bool zero = true;
    for (int i = 0; i < count; ++i)
        if (digits[i] != '0')
            zero = false;
    if (precision <= 0)
        precision = 1;
    char r[40];
    int keep = precision < 32 ? precision : 32;
    for (int i = 0; i < keep; ++i)
        r[i] = i < count ? digits[i] : '0';
    if (keep < count && digits[keep] >= '5') {
        int i = keep - 1;
        while (i >= 0 && r[i] == '9')
            r[i--] = '0';
        if (i < 0) {
            memmove(r + 1, r, keep - 1);
            r[0] = '1';
            ++decpt;
        } else {
            ++r[i];
        }
    }
    int length = keep;
    while (length > 0 && r[length - 1] == '0')
        --length;
    char* o = out;
    if (negative)
        *o++ = '-';
    int magnitude = decpt - 1;
    if (zero || length == 0) {
        *o++ = '0';
    } else if (magnitude < -4 || magnitude >= precision) {
        *o++ = r[0];
        if (length > 1) {
            *o++ = '.';
            memcpy(o, r + 1, length - 1);
            o += length - 1;
        }
        o += sprintf(o, "E%c%03d", magnitude < 0 ? '-' : '+', magnitude < 0 ? -magnitude : magnitude);
    } else if (decpt <= 0) {
        *o++ = '0';
        *o++ = '.';
        for (int i = 0; i < -decpt; ++i)
            *o++ = '0';
        memcpy(o, r, length);
        o += length;
    } else {
        for (int i = 0; i < decpt; ++i)
            *o++ = i < length ? r[i] : '0';
        if (length > decpt) {
            *o++ = '.';
            memcpy(o, r + decpt, length - decpt);
            o += length - decpt;
        }
    }
    *o = 0;
}

} // namespace

// `value` with `fractionDigits` digits after the point (Win 0x402100, a String helper of TU 273
// that has no Mac twin; not restored with String.cpp): "%.<n>G" (Win 0x431c50) with n = the
// integer digits (trunc(log10 |v|) + min((int)|v|, 1)) + fractionDigits; n <= 0 gives "0"; |v|
// below 1e-5 prints as 0.
String TextInputControl::FormatFloat(Real32 value, UInt32 fractionDigits)
{
    Real32 magnitude = (Real32)fabs((double)value);
    Real32 logarithm = (Real32)log10((double)magnitude);
    int integerPart = (int)magnitude;
    int leading = integerPart > 1 ? 1 : integerPart;
    int precision = (int)logarithm + leading + (int)fractionDigits;
    if (precision <= 0)
        return String("0");
    double printed = magnitude < 1e-05f ? 0.0 : (double)value;
    char buffer[64];
    FormatGVS2008(buffer, printed, precision);
    return String(buffer);
}

// Win 0x5144da
bool TextInputControl::Create()
{
    SetInputFlags(m_inputFlags | 3);
    ControlManager* manager = m_manager;
    TextLayout::CreateArgs args;
    args.text = String();
    m_textLayout = manager->GetObjectFactory()->Create<TextLayoutRender>(args);   // Win 0x50faa8
    if (!m_textLayout.Get())
        return false;
    Vector2 position = GetAbsolutePosition();
    String className("edit");
    String windowName;
    HWND parent = manager->GetWindow()->GetHWND();
    DWORD style = (m_numbersOnly ? 0x2000 : 0) | (m_multiLine ? 0x1004 : 0) |
                  (m_autoHScroll ? 0x80 : 0) | (m_autoVScroll ? 0x40 : 0) |
                  (m_vScrollBar ? 0x200000 : 0) | (m_hScrollBar ? 0x100000 : 0) | 0x46000000;
    m_window = PlatformCreateWindowEx(0, className, windowName, style, (int)position.x,
                                      (int)position.y, (int)m_size.x, (int)m_size.y, parent, 0,
                                      GetModuleHandleW(0), this);
    if (!m_window)
        return false;
    OSWinInfo info;
    info.window = m_window;
    // PORT64: the pointer-sized window long (on Win32 SetWindowLongPtrW is SetWindowLongW and
    // GWLP_WNDPROC is GWL_WNDPROC: the original call)
    info.previousProc = (WNDPROC)SetWindowLongPtrW(m_window, GWLP_WNDPROC, (LONG_PTR)EditWindowProc);
    info.control = this;
    EditWindows().PushBack(info);
    if (m_maxLength)
        SendMessageW(m_window, EM_LIMITTEXT, m_maxLength, 0);
    m_inputContext = ImmGetContext(m_window);
    AttachToAncestors();
    return true;
}

// Win 0x5149cb (destructor part)
void TextInputControl::DestroyNative()
{
    if (m_inputContext)
        ImmReleaseContext(m_window, (HIMC)m_inputContext);
    if (m_window) {
        DestroyWindow(m_window);
        m_window = 0;
    }
}

// Win 0x5146ca
bool TextInputControl::SetText(const String& text)
{
    m_textLayout->SetText(text);
    PlatformSetWindowText(m_window, text);
    UpdateWindow(m_window);
    NotifyTextChanged();
    return true;
}

// Win 0x514748
String TextInputControl::GetText() const
{
    String text;
    PlatformGetWindowText(m_window, text);
    return text;
}

// Win 0x51478c
bool TextInputControl::SetFont(Font* font)
{
    FontPlatform* platformFont = dynamic_cast<FontPlatform*>(font);   // Win 0x4f1950
    if (platformFont) {
        m_textLayout->SetFont(platformFont);
        SendMessageW(m_window, WM_SETFONT, (WPARAM)platformFont->GetNativeFont(), 0);
        SendMessageW(m_window, EM_SETMARGINS, EC_LEFTMARGIN | EC_RIGHTMARGIN, 0);
        UpdateWindow(m_window);
    }
    return platformFont != 0;
}

// Win 0x5147e5: the height of "Yy" in the layout's font (no wrapping) per line, + 2.
bool TextInputControl::SetLineCount(UInt32 lines)
{
    String text = GetText();
    m_textLayout->SetText(String("Yy"));
    m_textLayout->SetMaxWidth(0);
    Vector2 lineSize = m_textLayout->GetSize();
    m_textLayout->SetText(text);
    Vector2 size(m_size.x, 0.0f);
    Real32 height = (Real32)((double)lines * (double)lineSize.y);
    size.y = (Real32)((double)height + 2.0);
    return SetSize(size);
}

// Win 0x5148a0
void TextInputControl::SetNumberType(NumberType type)
{
    if (m_numberType == (UInt32)type)
        return;
    m_numberType = type;
    SetText(String());
}

// Win 0x5148e5
void TextInputControl::SetFractionDigits(UInt32 digits)
{
    m_fractionDigits = digits;
}

// Win 0x5148f2: a decimal comma is read as a point.
Real32 TextInputControl::GetFloat() const
{
    String text = GetText();
    text.ReplaceAll(String(","), String("."));
    Real32 value;
    UInt32 position = 0;
    if (!text.Parse(value, position))   // Win 0x4179e0
        value = 0.0f;
    return value;
}

// Win 0x51497f
void TextInputControl::SetFloat(Real32 value)
{
    SetText(FormatFloat(value, m_fractionDigits));
}

// Win 0x5150d6
void TextInputControl::UpdateNativeFrame()
{
    Vector2 position = GetAbsolutePosition();
    MoveWindow(m_window, (int)position.x, (int)position.y, (int)m_size.x, (int)m_size.y, TRUE);
}

// Win 0x514f53
void TextInputControl::OnShown()
{
    Control::OnShown();
}

// Win 0x514f58
void TextInputControl::OnHidden()
{
    SetKeyboardFocus(false);
    Control::OnHidden();
}

// Win 0x514f6a
void TextInputControl::OnEnabled()
{
    Control::OnEnabled();
}

// Win 0x514f6f
void TextInputControl::OnDisabled()
{
    Control::OnDisabled();
}

// Win 0x514e4d
void TextInputControl::OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers)
{
    Control::OnKeyboardFocusGained(flag, previous, modifiers);
    ShowWindow(m_window, SW_SHOW);
    SetFocus(m_window);
    if (flag) {
        // click where the mouse is, so the caret lands there
        Vector2 mouse = m_manager->GetMousePosition(this);
        WORD y = (WORD)(Int32)mouse.y;
        WORD x = (WORD)(Int32)mouse.x;
        SendMessageW(m_window, WM_LBUTTONDOWN, MK_LBUTTON, ((LPARAM)y << 16) | x);
    }
    if (m_inputContext)
        ImmSetOpenStatus((HIMC)m_inputContext, TRUE);
    UpdateWindow(m_window);
}

// Win 0x514ee4
void TextInputControl::OnKeyboardFocusLost(Control* next)
{
    if (m_inputContext)
        ImmSetOpenStatus((HIMC)m_inputContext, FALSE);
    ShowWindow(m_window, SW_HIDE);
    Control::OnKeyboardFocusLost(next);
}

// Win 0x514f74: white background, the text layout's texture in black where it is visible,
// the disabled colour over a disabled field.
void TextInputControl::Draw(ControlDrawArgs& args)
{
    args.Setup(false);
    args.DrawRect(Rect(m_size), Color(1.0f, 1.0f, 1.0f, 1.0f), false);
    Rect visible(m_textLayout->GetSize());
    visible &= args.GetLocalClip();
    if (visible.IsValid()) {
        Texture* texture = m_textLayout->GetTexture();
        if (texture) {
            Rect textureRect = m_textLayout->GetTexCoords();
            args.DrawTexture(Rect(m_textLayout->GetSize()), texture, textureRect,
                             Color(0.0f, 0.0f, 0.0f, 1.0f), true);
        }
    }
    if (!m_status.m_enabledInTree) {
        args.m_render->SetClearColor(m_disabledColor);
        args.m_render->Clear(1);
    }
    m_status.m_invalid = 0;
}

// Win 0x5142aa
LRESULT CALLBACK TextInputControl::EditWindowProc(HWND window, UINT message, WPARAM wParam,
                                                  LPARAM lParam)
{
    BasicArray<OSWinInfo>& windows = EditWindows();
    UInt32 index = 0;
    while (index < windows.GetSize() && windows[index].window != window)
        ++index;
    if (index == windows.GetSize())
        return 0;
    OSWinInfo info = windows[index];
    if (message == WM_NCDESTROY)
        windows.Erase(index);
    return info.control->HandleEditMessage(info.previousProc, message, wParam, lParam);
}

// Win 0x514a90
LRESULT TextInputControl::HandleEditMessage(WNDPROC previousProc, UINT message, WPARAM wParam,
                                            LPARAM lParam)
{
    DWORD selection = 0xffffffff;
    LRESULT result = 0;
    bool changed = false;
    bool edits = message == WM_SETTEXT || message == WM_CHAR || message == WM_PASTE ||
                 message == WM_UNDO;
    if (m_numberType != FreeText && edits)
        selection = (DWORD)SendMessageA(m_window, EM_GETSEL, 0, 0);
    bool enterInSingleLine = !m_multiLine && message == WM_CHAR && wParam == '\r';
    if (!enterInSingleLine)
        result = CallWindowProcW(previousProc, m_window, message, wParam, lParam);
    bool keyboard = message >= WM_KEYFIRST && message <= 0x109;
    bool readText;
    if (m_numberType != FreeText) {
        if (edits) {
            String text = GetText();
            bool rejected = false;
            bool check = true;
            if (m_numberType == UIntNumber) {
                rejected = text.FindFirstNotOf(String("0123456789")) != String::npos;
            } else if (m_numberType == IntNumber) {
                rejected = text.FindFirstNotOf(String("-0123456789")) != String::npos;
                UInt32 minus = text.FindLastOf((UniChar16)'-', String::npos);
                if (minus != String::npos && minus != 0)
                    rejected = true;
            } else if (m_numberType == RealNumber) {
                rejected = text.FindFirstNotOf(String(".,-0123456789")) != String::npos;
                UInt32 minus = text.FindLastOf((UniChar16)'-', String::npos);
                if (minus != String::npos && minus != 0)
                    rejected = true;
                // at most one decimal separator
                String separators(",.");
                UInt32 last = text.FindLastOf(String(",."), String::npos);
                rejected = rejected || text.FindFirstOf(separators, 0) != last;
            } else {
                check = false;
            }
            if (check && rejected) {
                PlatformSetWindowText(m_window, m_textLayout->GetText());
                SendMessageA(m_window, EM_SETSEL, selection & 0xffff, selection >> 16);
                UpdateWindow(m_window);
            } else {
                changed = text != m_textLayout->GetText();
                if (changed)
                    m_textLayout->SetText(text);
            }
        }
        readText = keyboard || message == WM_CUT || message == WM_CLEAR;
    } else {
        readText = keyboard || message == WM_CUT || message == WM_PASTE || message == WM_CLEAR ||
                   message == WM_UNDO || message == WM_SETTEXT;
    }
    if (readText) {
        // NOTE: original quirk: in a number field a typed character was already copied to the layout
        // above, so this comparison finds no change and OnTextChanged is not sent.
        String text = GetText();
        changed = text != m_textLayout->GetText();
        if (changed)
            m_textLayout->SetText(text);
    }
    if (enterInSingleLine)
        NotifyTextEntered();
    else if (changed)
        NotifyTextChanged();
    return result;
}
#endif // _WIN32

} // namespace LEGO
