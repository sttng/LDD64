// LEGO Digital Designer — restored source
// TU 020 "Window.cpp": Mac 0x20470-0x2d5b9, Win 0x40be60-0x4124c0 (+ the Win32 natives
// 0x40c1f0-0x410482). Platform-independent members (input queue, notifications,
// dispatcher with menu short-cuts, accessors) and the Win32 natives ("DCLWindowClass",
// HandleMessage, menus, display modes, VK → LEGO::Key table); Mac natives: WindowMac.mm.
// Compiler-generated / template code, not written out: the TU's NotifierT / Notifier /
// BasicArray / BasicDeque instances (Mac 0x274b0-0x278b0, 0x2ca60-0x2d400, Win
// 0x410c00-0x411f6f), the virtual-base thunks (Mac 0x23550-0x239f0, 0x23d60-0x23dc0, 0x23e10,
// 0x23e40, 0x231b0) and LocalizedString's inline dtor (Win 0x40c350/0x40c3a0).
#include "Window.h"
#include "Command.h"
#include "Menu.h"
#include "LEGO/Assert.h"
#if defined(_WIN32)
#include "Application.h"
#include "Platform.h"
#include "Path.h"
#include "Directory.h"
#include "TextLocale.h"
#include <shellapi.h>
#endif
#include <string.h>

namespace LEGO {

typedef NotifierT<Window, WindowObserver>::Iterator WindowObserverIterator;

// Iterates over the window observers with the usual NotifierT pass (safe against observers
// detaching themselves while being called).
#define LEGO_WINDOW_NOTIFY(call)                                                        \
    WindowObserverIterator it(*this);                                                  \
    while (WindowObserver* o = it.Next(NotifierT<Window, WindowObserver>::m_observers)) \
        o->call;

#if defined(_WIN32)
// ===========================================================================================
// Win: virtual key <-> LEGO::Key tables (Win 0x40e770: dynamic initialiser of a static object at
// Win 0xb42798 — 256 Key values indexed by VK, then at 0xb42b98 the inverse, 256 UInt16 VK values
// indexed by Key). (type and member names inferred)
// ===========================================================================================
namespace {
struct VirtualKeyMap {
    UInt32 keyOfVirtualKey[256];   // Win 0xb42798
    UInt16 virtualKeyOfKey[256];   // Win 0xb42b98

    // Win 0x40e770
    VirtualKeyMap()
    {
        memset(keyOfVirtualKey, 0, sizeof(keyOfVirtualKey));
        keyOfVirtualKey[VK_BACK]      = kKeyBackspace;
        keyOfVirtualKey[VK_TAB]       = kKeyTab;
        keyOfVirtualKey[VK_RETURN]    = kKeyReturn;
        keyOfVirtualKey[VK_SHIFT]     = kKeyShift;
        keyOfVirtualKey[VK_CONTROL]   = kKeyControl;
        keyOfVirtualKey[VK_MENU]      = kKeyAlt;
        keyOfVirtualKey[0x15]         = kKeyKana;        // VK_KANA
        keyOfVirtualKey[0x19]         = kKeyKanji;       // VK_KANJI
        keyOfVirtualKey[VK_ESCAPE]    = kKeyEscape;
        keyOfVirtualKey[0x1c]         = kKeyConvert;     // VK_CONVERT
        keyOfVirtualKey[0x1d]         = kKeyNoConvert;   // VK_NONCONVERT
        keyOfVirtualKey[VK_SPACE]     = kKeySpace;
        keyOfVirtualKey[VK_PRIOR]     = kKeyPageUp;
        keyOfVirtualKey[VK_NEXT]      = kKeyPageDown;
        keyOfVirtualKey[VK_END]       = kKeyEnd;
        keyOfVirtualKey[VK_HOME]      = kKeyHome;
        keyOfVirtualKey[VK_LEFT]      = kKeyLeftArrow;
        keyOfVirtualKey[VK_UP]        = kKeyUpArrow;
        keyOfVirtualKey[VK_RIGHT]     = kKeyRightArrow;
        keyOfVirtualKey[VK_DOWN]      = kKeyDownArrow;
        keyOfVirtualKey[VK_INSERT]    = kKeyInsert;
        keyOfVirtualKey[VK_DELETE]    = kKeyDelete;
        keyOfVirtualKey['0'] = kKey0;
        for (int i = 1; i <= 9; ++i)
            keyOfVirtualKey['0' + i] = kKey1 + (i - 1);
        static const UInt8 kLetters[26] = {
            kKeyA, kKeyB, kKeyC, kKeyD, kKeyE, kKeyF, kKeyG, kKeyH, kKeyI, kKeyJ, kKeyK, kKeyL,
            kKeyM, kKeyN, kKeyO, kKeyP, kKeyQ, kKeyR, kKeyS, kKeyT, kKeyU, kKeyV, kKeyW, kKeyX,
            kKeyY, kKeyZ};
        for (int i = 0; i < 26; ++i)
            keyOfVirtualKey['A' + i] = kLetters[i];
        keyOfVirtualKey[VK_LWIN]      = kKeyLeftWindows;
        keyOfVirtualKey[VK_RWIN]      = kKeyRightWindows;
        keyOfVirtualKey[VK_APPS]      = kKeyApps;
        static const UInt8 kNumpad[10] = {
            kKeyNumpad0, kKeyNumpad1, kKeyNumpad2, kKeyNumpad3, kKeyNumpad4,
            kKeyNumpad5, kKeyNumpad6, kKeyNumpad7, kKeyNumpad8, kKeyNumpad9};
        for (int i = 0; i < 10; ++i)
            keyOfVirtualKey[VK_NUMPAD0 + i] = kNumpad[i];
        keyOfVirtualKey[VK_MULTIPLY]  = kKeyNumpadMultiply;
        keyOfVirtualKey[VK_ADD]       = kKeyNumpadPlus;
        keyOfVirtualKey[VK_SUBTRACT]  = kKeyNumpadMinus;
        keyOfVirtualKey[VK_DECIMAL]   = kKeyNumpadPeriod;
        keyOfVirtualKey[VK_DIVIDE]    = kKeyNumpadDivide;
        static const UInt8 kFunction[12] = {
            kKeyF1, kKeyF2, kKeyF3, kKeyF4, kKeyF5, kKeyF6, kKeyF7, kKeyF8, kKeyF9, kKeyF10,
            kKeyF11, kKeyF12};
        for (int i = 0; i < 12; ++i)
            keyOfVirtualKey[VK_F1 + i] = kFunction[i];
        keyOfVirtualKey[VK_NUMLOCK]   = kKeyNumLock;
        keyOfVirtualKey[VK_SCROLL]    = kKeyScrollLock;

        memset(virtualKeyOfKey, 0, sizeof(virtualKeyOfKey));
        for (UInt16 vk = 0; vk < 0x100; ++vk)
            if (keyOfVirtualKey[vk])
                virtualKeyOfKey[keyOfVirtualKey[vk]] = vk;
    }
};

VirtualKeyMap s_virtualKeyMap;

String s_windowClassName;              // Win 0xb44c50 "DCLWindowClass" (guard bit 0xb44c54)
bool   s_windowClassNameSet = false;
ATOM   s_windowClass = 0;              // Win 0xb3ffdc
} // namespace
#endif

// ===========================================================================================
// Construction (Win; the Mac constructor / destructor / OnFinalRelease are in WindowMac.mm)
// ===========================================================================================
#if defined(_WIN32)
// Win 0x40c3c0
Window::Window(bool flag95)
    : m_mousePosition(-30000.0f, -30000.0f),
      m_showCursor(true),
      m_hasMenuBar(flag95),
      m_menu(0),
      m_modalCount(0),
      m_modalUpdateReciever(0),
      m_keyboardFocusObserver(0),
      m_hwnd(0),
      m_clientLeft(0),
      m_clientTop(0),
      m_capturedButtons(0),
      m_destroying(false),
      m_mouseInside(false)
{
    m_cursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    m_background = CreateSolidBrush(RGB(255, 255, 255));
}

// Win 0x40d250 (sdd 0x40c5e0 / 0x412420)
// NOTE: the original neither destroys the HWND here (OnFinalRelease does) nor deletes the
// background brush (a GDI object leaked per window); fixed: deleted.
Window::~Window()
{
    if (m_background)
        DeleteObject(m_background);
}

// Win 0x40d390 (thunk on the Object sub-object)
void Window::OnFinalRelease()
{
    if (m_hwnd) {
        m_destroying = true;
        DestroyWindow(m_hwnd);
        NotifyClose();
    }
    if (m_style.flag28)
        ChangeDisplaySettingsA(NULL, 0);
    NotifierT<Window, WindowObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    NotifierT<Updater, UpdateObserver>::OnFinalRelease();
}
#endif

// ===========================================================================================
// HumanInput → queue (Mac 0x27950-0x27a90, Win 0x40c050-0x40c120)
// ===========================================================================================
void Window::OnMouseMove(const Vector2& position)                    { HandleMouseMove(position); } // Mac 0x27950, Win 0x40c050
void Window::OnMouseDown(const Vector2& position, UInt32 button)     { HandleMouseButton(position, button, true, false); } // Mac 0x27970, Win 0x40c060
void Window::OnMouseUp(const Vector2& position, UInt32 button)       { HandleMouseButton(position, button, false, false); } // Mac 0x279a0, Win 0x40c080
void Window::OnMouseDoubleClick(const Vector2& position, UInt32 button) { HandleMouseButton(position, button, true, true); } // Mac 0x279d0, Win 0x40c0a0
void Window::OnMouseWheel(UInt32 axis, Real32 delta)                 { HandleMouseWheel(axis, delta); } // Mac 0x27a00, Win 0x40c0c0
void Window::OnKeyDown(UInt32 key, UInt32 modifiers, bool repeat)    { HandleKey(key, modifiers, true, repeat); } // Mac 0x27a20, Win 0x40c0e0
void Window::OnKeyUp(UInt32 key, UInt32 modifiers)                   { HandleKey(key, modifiers, false, false); } // Mac 0x27a60, Win 0x40c100
void Window::OnCharacter(UniChar16 c, UInt32 modifiers, bool repeat) { HandleCharacter(c, modifiers, repeat); } // Mac 0x27a90, Win 0x40c120

// ===========================================================================================
// WindowInput → notifications (Mac 0x27ab0-0x27c10, Win 0x40c130-0x40c1e0)
// ===========================================================================================
void Window::OnMove(const Vector2& position)                         { NotifyMove(position); } // Mac 0x27ab0, Win 0x40c130
void Window::OnResize(const Vector2& size, const Vector2& clientSize) { NotifyResize(size, clientSize); } // Mac 0x27ad0, Win 0x40c140
void Window::OnActivate(bool active)                                 { NotifyActivate(active); } // Mac 0x27af0, Win 0x40c150
void Window::OnInput13(bool flag, UInt32 modifiers, UInt32 arg)      { NotifyInput13(flag, modifiers, arg); } // Mac 0x27b10, Win 0x40c160
void Window::OnInput14(UInt32 arg)                                   { NotifyInput14(arg); } // Mac 0x27b30, Win 0x40c170
bool Window::OnCloseRequest()                                        { return NotifyCloseRequest(); } // Mac 0x27b50, Win 0x40c180
void Window::OnClose()                                               { NotifyClose(); } // Mac 0x27b70, Win 0x40c190
bool Window::OnInput17()                                             { return NotifyInput17(); } // Mac 0x27b90, Win 0x40c1a0
void Window::OnPaint(UInt32 arg)                                     { NotifyPaint(arg); } // Mac 0x27bb0, Win 0x40c1b0
void Window::OnInput19()                                             { NotifyInput19(); } // Mac 0x27bd0, Win 0x40c1c0
void Window::OnInput20(UInt32 arg)                                   { NotifyInput20(arg); } // Mac 0x27bf0, Win 0x40c1d0
void Window::OnInput21(UInt32 arg)                                   { NotifyInput21(arg); } // Mac 0x27c10, Win 0x40c1e0

// ===========================================================================================
// Accessors
// ===========================================================================================

// Mac 0x27c30, Win 0x40bfe0
const WindowStyle& Window::GetStyle() const { return m_style; }

// Mac 0x27c40, Win 0x40bff0
const String& Window::GetRepresentedFile() const { return m_representedFile; }

// Mac 0x27c50, Win 0x40c000
bool Window::IsHidden() const { return m_style.hidden; }

// Mac 0x21c80, Win 0x40cd90
void Window::ShowCursor(bool show) { m_showCursor = show; }

// Mac 0x27c60, Win 0x40c010
Vector2 Window::GetBorderSize() const
{
    return m_style.size - GetSize();
}

// ===========================================================================================
// Input queue
// ===========================================================================================

// Mac 0x23e70, Win 0x40d5c0: stores the position and notifies at once (not queued).
// NOTE: Win compares with an epsilon of 1e-5 (Vector2::IsEqual inlined), Mac with operator!=.
void Window::HandleMouseMove(const Vector2& position)
{
    if (!(m_mousePosition != position))
        return;
    m_mousePosition = position;
    LEGO_WINDOW_NOTIFY(OnMouseMove(position))
}

// Mac 0x24130, Win 0x40d6b0
void Window::HandleMouseButton(const Vector2& position, UInt32 button, bool down, bool doubleClick)
{
    Message m;
    m.type = Message::kMouseButton;
    m.position = position;
    m.button = button;
    m.down = down;
    m.doubleClick = doubleClick;
    m_messages.Push(m);
}

// Mac 0x24210, Win 0x40d720
void Window::HandleMouseWheel(UInt32 axis, Real32 delta)
{
    Message m;
    m.type = Message::kMouseWheel;
    m.wheelAxis = axis;
    m.wheelDelta = delta;
    m_messages.Push(m);
}

// Mac 0x242e0, Win 0x40d780
void Window::HandleKey(UInt32 key, UInt32 modifiers, bool down, bool repeat)
{
    Message m;
    m.type = Message::kKey;
    m.down = down;
    m.key = key;
    m.modifiers = modifiers;
    m.repeat = repeat;
    m_messages.Push(m);
}

// Mac 0x243b0, Win 0x40d7f0
void Window::HandleCharacter(UniChar16 c, UInt32 modifiers, bool repeat)
{
    Message m;
    m.type = Message::kCharacter;
    m.modifiers = modifiers;
    m.repeat = repeat;
    m.character = c;
    m_messages.Push(m);
}

// Mac 0x21ea0, Win 0x4104a0: pops every queued message and hands it to the observers. A
// key-down is first offered to the menu short-cuts (and still delivered afterwards).
void Window::ProcessMessages()
{
    while (!m_messages.IsEmpty()) {
        Message m = m_messages.Front();
        m_messages.Pop();
        switch (m.type) {
        case Message::kMouseButton:
            if (!m.down) {
                LEGO_WINDOW_NOTIFY(OnMouseUp(m.position, m.button))
            } else if (!m.doubleClick) {
                LEGO_WINDOW_NOTIFY(OnMouseDown(m.position, m.button))
            } else {
                LEGO_WINDOW_NOTIFY(OnMouseDoubleClick(m.position, m.button))
            }
            break;
        case Message::kMouseWheel: {
            LEGO_WINDOW_NOTIFY(OnMouseWheel(m.wheelAxis, m.wheelDelta))
            break;
        }
        case Message::kKey:
            if (!m.down) {
                LEGO_WINDOW_NOTIFY(OnKeyUp(m.key, m.modifiers))
            } else {
                HandleMenuShortcut(m.key, m.modifiers, m.repeat);
                LEGO_WINDOW_NOTIFY(OnKeyDown(m.key, m.modifiers, m.repeat))
            }
            break;
        case Message::kCharacter: {
            LEGO_WINDOW_NOTIFY(OnCharacter(m.character, m.modifiers, m.repeat))
            break;
        }
        default:
            break;
        }
    }
}

// Mac 0x26d80, Win 0x410a00
void Window::HandleMenuShortcut(UInt32 key, UInt32 modifiers, bool repeat)
{
    if (!m_menu.Get() || key == 0 || m_modalCount != 0)
        return;
    bool allowed = true;
    {
        WindowObserverIterator it(*this);
        while (allowed) {
            WindowObserver* o = it.Next(NotifierT<Window, WindowObserver>::m_observers);
            if (!o)
                break;
            allowed = allowed && o->OnObserverQuery24();
        }
    }
    if (allowed)
        ExecuteMenuShortcut(m_menu.Get(), key, modifiers, repeat);
}

// Mac 0x27050, Win 0x410af0
// The Mac build lets Cocoa run Command short-cuts and F1-F15 itself (NSMenuItem key
// equivalents); those are only reported as handled.
bool Window::ExecuteMenuShortcut(const Menu* menu, UInt32 key, UInt32 modifiers, bool repeat)
{
    const BasicArray<RefPointer<MenuItem> >& items = menu->GetItems();
    for (UInt32 i = 0; i < items.GetSize(); ++i) {
        MenuItem* item = items[i].Get();
        if (item->GetKey() == key && item->GetModifiers() == modifiers) {
            Command* command = item->GetShortcutCommand();
            if (command && command->GetEnable()) {
#if !defined(_WIN32) && !defined(LDD_LINUX)   // (Linux: the menu bar only shows short-cuts)
                if (item->GetModifiers() & kModifierControl)
                    return true;
                if (key - kKeyF1 < 10)
                    return true;
                if (key - kKeyF11 < 0x10 && ((0xe003u >> (key - kKeyF11)) & 1))
                    return true;          // F11, F12, F13-F15
#endif
                item->GetShortcutCommand()->Do();
                return true;
            }
        }
        if (item->GetSubMenu() && ExecuteMenuShortcut(item->GetSubMenu(), key, modifiers, repeat))
            return true;
    }
    return false;
}

// ===========================================================================================
// Notifications (Mac 0x24480-0x260c0, Win 0x40d860-0x40e0e0)
// ===========================================================================================

// Mac 0x24480, Win 0x40d860: remembers the new position, then notifies.
void Window::NotifyMove(const Vector2& position)
{
    m_style.position = position;
    LEGO_WINDOW_NOTIFY(OnMove(position))
}

// Mac 0x24720, Win 0x40d930: remembers the new outer size, then notifies.
void Window::NotifyResize(const Vector2& size, const Vector2& clientSize)
{
    m_style.size = size;
    LEGO_WINDOW_NOTIFY(OnResize(size, clientSize))
}

void Window::NotifyActivate(bool active)                             { LEGO_WINDOW_NOTIFY(OnActivate(active)) }   // Mac 0x249d0, Win 0x40da10
void Window::NotifyInput14(UInt32 arg)                               { LEGO_WINDOW_NOTIFY(OnInput14(arg)) }       // Mac 0x24f40, Win 0x40dbb0
void Window::NotifyInput19()                                         { LEGO_WINDOW_NOTIFY(OnInput19()) }          // Mac 0x25bd0, Win 0x40df70
void Window::NotifyInput20(UInt32 arg)                               { LEGO_WINDOW_NOTIFY(OnInput20(arg)) }       // Mac 0x25e40, Win 0x40e020
void Window::NotifyInput21(UInt32 arg)                               { LEGO_WINDOW_NOTIFY(OnInput21(arg)) }       // Mac 0x260c0, Win 0x40e0e0

// Mac 0x25940, Win 0x40deb0: not while an assertion box is up.
void Window::NotifyPaint(UInt32 arg)
{
    if (IsInsideAssert())
        return;
    LEGO_WINDOW_NOTIFY(OnPaint(arg))
}

// Mac 0x24c50, Win 0x40dad0: drops the queued input (deque cleared, Win 0x411660), notifies, then
// (focus gained) queues a key-up of key 0 with the current modifiers to re-sync the modifier state.
void Window::NotifyInput13(bool flag, UInt32 modifiers, UInt32 arg)
{
    while (!m_messages.IsEmpty())
        m_messages.Pop();
    {
        LEGO_WINDOW_NOTIFY(OnInput13(flag, modifiers, arg))
    }
    if (flag)
        HandleKey(0, modifiers, false, false);
}

// Mac 0x251c0, Win 0x40dc70: false as soon as one observer refuses.
bool Window::NotifyCloseRequest()
{
    WindowObserverIterator it(*this);
    while (WindowObserver* o = it.Next(NotifierT<Window, WindowObserver>::m_observers))
        if (!o->OnCloseRequest())
            return false;
    return true;
}

// Mac 0x25430, Win 0x40dd20: notifies, leaves the updater, forgets the native window.
void Window::NotifyClose()
{
    {
        LEGO_WINDOW_NOTIFY(OnClose())
    }
    UpdateObserver::Detach();
#if defined(_WIN32)
    m_hwnd = 0;
#else
    m_nativeWindow = 0;
#endif
}

// Mac 0x256d0, Win 0x40de00: false as soon as one observer refuses.
bool Window::NotifyInput17()
{
    WindowObserverIterator it(*this);
    while (WindowObserver* o = it.Next(NotifierT<Window, WindowObserver>::m_observers))
        if (!o->OnInput17())
            return false;
    return true;
}

#if defined(_WIN32)
// ===========================================================================================
// Win32 natives
// ===========================================================================================

// Win 0x40d560 (Updater sub-object): re-enables a window a modal loop left disabled, dispatches
// the queued input, tracks the mouse, then updates the observers.
void Window::Update()
{
    if (m_modalCount != 0 && !IsWindowEnabled(m_hwnd)) {
        EnableWindow(m_hwnd, TRUE);
        SetFocus(m_hwnd);
    }
    ProcessMessages();
    TrackMouse();
    Updater::Update();
}

// Win 0x40d1f0 (ModalUpdateReciever sub-object)
void Window::ModalUpdate()
{
    if (!IsWindowEnabled(m_hwnd)) {
        EnableWindow(m_hwnd, TRUE);
        SetFocus(m_hwnd);
    }
    ProcessMessages();
    TrackMouse();
    if (m_modalUpdateReciever)
        m_modalUpdateReciever->ModalUpdate();
}

// Win 0x40c660
bool Window::Create(const WindowStyle& style)
{
    WindowStyle previous = m_style;
    WindowStyle wanted = style;
    if (!CheckDisplayMode(wanted))
        return false;
    if (!ChangeDisplayMode(wanted, previous))
        return false;
    m_style = wanted;
    if (!m_hwnd) {
        if (!CreateNativeWindow(this))
            return false;
        if (!m_style.hidden)
            ShowNativeWindow();
    } else {
        if (!SetTitle(wanted.title))
            return false;
        int x = (int)m_style.position.x;
        int y = (int)m_style.position.y;
        int width = (int)m_style.size.x;
        int height = (int)m_style.size.y;
        if (m_style.flag28 != previous.flag28) {
            LONG exStyle = GetWindowLongW(m_hwnd, GWL_EXSTYLE);
            exStyle = (exStyle & ~WS_EX_TOPMOST) | (m_style.flag28 ? WS_EX_TOPMOST : 0);
            SetWindowLongW(m_hwnd, GWL_EXSTYLE, exStyle);
        }
        if (m_style.flag29 != previous.flag29 || m_style.flag2a != previous.flag2a) {
            DWORD frame;
            if (m_style.flag29)
                frame = m_style.flag2a ? 0x06cf0000 : 0x06c80000;
            else
                frame = 0x86000000;
            LONG windowStyle = GetWindowLongW(m_hwnd, GWL_STYLE);
            windowStyle = (windowStyle & 0xff30ffff) | frame;
            SetWindowLongW(m_hwnd, GWL_STYLE, windowStyle);
        }
        if (!MoveWindow(m_hwnd, x, y, width, height, TRUE))
            return false;
        if (m_style.hidden != previous.hidden) {
            if (m_style.hidden)
                ::ShowWindow(m_hwnd, SW_HIDE);
            else
                ShowNativeWindow();
        }
    }
    // Win 0x40c777: attach to the main updater (input dispatch, observers' Update).
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    UpdateObserver::Attach(updater.Get());
    return true;
}

// Win 0x40e400 (stack: this)
bool Window::CreateNativeWindow(Window* window)
{
    const WindowStyle& style = window->m_style;
    DWORD windowStyle;
    if (style.flag29)
        windowStyle = style.flag2a ? 0x06cf0000 : 0x06c80000;   // overlapped (+ size box, maximize)
    else
        windowStyle = 0x86000000;                              // WS_POPUP | clip children/siblings
    if (!s_windowClassNameSet) {
        s_windowClassNameSet = true;
        s_windowClassName = String("DCLWindowClass");
    }
    if (!s_windowClass) {
        // transparent class cursor (AND mask all ones, XOR mask zero); the window sets m_cursor
        int w = GetSystemMetrics(SM_CXCURSOR);
        int h = GetSystemMetrics(SM_CYCURSOR);
        UInt8* andMask = new UInt8[w * h];
        UInt8* xorMask = new UInt8[w * h];
        memset(andMask, 0xff, w * h);
        memset(xorMask, 0, w * h);
        HCURSOR cursor = CreateCursor(GetModuleHandleA(NULL), 0, 0, w, h, andMask, xorMask);
        delete[] andMask;
        delete[] xorMask;
        WNDCLASSEXW wc;
        wc.cbSize = sizeof(wc);
        wc.style = CS_VREDRAW | CS_HREDRAW | CS_DBLCLKS | CS_OWNDC;
        wc.lpfnWndProc = WindowProc;
        wc.cbClsExtra = 0;
        wc.cbWndExtra = 4;
        wc.hInstance = GetModuleHandleA(NULL);
        if (!style.icon)
            wc.hIcon = 0;
        else if (!(style.icon & 0xffff0000))
            wc.hIcon = LoadIconA(GetModuleHandleA(NULL), MAKEINTRESOURCEA(style.icon & 0xffff));
        else
            wc.hIcon = (HICON)(UINT_PTR)style.icon;
        wc.hCursor = cursor;
        wc.hbrBackground = 0;
        wc.lpszMenuName = 0;
        wc.lpszClassName = (LPCWSTR)s_windowClassName.GetBuffer();
        wc.hIconSm = 0;
        s_windowClass = RegisterClassExW(&wc);
        if (!s_windowClass)
            return false;
        DestroyCursor(cursor);
    }
    HMENU menu = 0;
    if (window->m_hasMenuBar) {
        // a menu bar with one empty item, until SetMenu installs the real one
        menu = CreateMenu();
        MENUITEMINFOW mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_FTYPE | MIIM_STRING;
        mi.dwTypeData = (LPWSTR)L"";
        InsertMenuItemW(menu, 0, TRUE, &mi);
    }
    DWORD exStyle = (style.flag28 ? WS_EX_TOPMOST : 0) | WS_EX_APPWINDOW | WS_EX_ACCEPTFILES;
    window->m_hwnd = CreateWindowExW(exStyle, (LPCWSTR)s_windowClassName.GetBuffer(),
                                     (LPCWSTR)style.title.GetBuffer(), windowStyle,
                                     (int)style.position.x, (int)style.position.y,
                                     (int)style.size.x, (int)style.size.y,
                                     NULL, menu, GetModuleHandleA(NULL), window);
    if (!window->m_hwnd)
        return false;
    if (window->NotifyInput17())
        return true;
    if (window->m_hwnd) {
        window->m_destroying = true;
        DestroyWindow(window->m_hwnd);
        window->NotifyClose();
    }
    return false;
}

// Win 0x40e720
void Window::DestroyNativeWindow()
{
    if (m_hwnd) {
        m_destroying = true;
        DestroyWindow(m_hwnd);
        NotifyClose();
    }
}

// Win 0x40e750
void Window::ShowNativeWindow()
{
    ::ShowWindow(m_hwnd, SW_SHOW);
    SetForegroundWindow(m_hwnd);
}

// Win 0x40cad0: remembers the title even without a window.
bool Window::SetTitle(const String& title)
{
    m_style.title = title;
    if (!m_hwnd)
        return false;
    return SetWindowTextW(m_hwnd, (LPCWSTR)title.GetBuffer()) != FALSE;
}

// Win 0x40cb10: "<file> - <application name>", or the application name alone.
bool Window::SetRepresentedFile(const Path& path)
{
    if (!m_hwnd)
        return false;
    m_representedFile = path.GetPath(false);
    const String& applicationName = Application::GetInstance()->GetName();
    if (m_representedFile.IsEmpty())
        return SetTitle(applicationName);
    String title(m_representedFile);
    title.Append(" - ");
    title << applicationName;
    return SetTitle(title);
}

// Win 0x40cc30: "<file>* - <application name>" while the document is modified.
void Window::SetModified(bool modified)
{
    if (!m_hwnd)
        return;
    String mark;
    if (modified)
        mark.Append("*");
    const String& applicationName = Application::GetInstance()->GetName();
    String title;
    if (m_representedFile.IsEmpty()) {
        title = mark;
        title << applicationName;
    } else {
        title = m_representedFile;
        title << mark;
        title.Append(" - ");
        title << applicationName;
    }
    SetTitle(title);
}

// Win 0x40cd40
void Window::SetHidden(bool hidden)
{
    if (hidden == m_style.hidden)
        return;
    m_style.hidden = hidden;
    if (!hidden) {
        ::ShowWindow(m_hwnd, SW_SHOW);
        SetForegroundWindow(m_hwnd);
        return;
    }
    ::ShowWindow(m_hwnd, SW_HIDE);
}

// Win 0x40cda0: the client area.
Vector2 Window::GetSize() const
{
    RECT r;
    GetClientRect(m_hwnd, &r);
    return Vector2((Real32)(r.right - r.left), (Real32)(r.bottom - r.top));
}

// Win 0x40cdf0
void Window::SetCursor(NativeCursor cursor)
{
    if (!cursor)
        cursor = LoadCursorA(NULL, (LPCSTR)IDC_ARROW);
    m_cursor = cursor;
    POINT p;
    if (!GetCursorPos(&p))
        return;
    RECT r;
    GetClientRect(m_hwnd, &r);
    int width = r.right - r.left;
    int height = r.bottom - r.top;
    if (m_clientLeft <= p.x && p.x < m_clientLeft + width && m_clientTop <= p.y &&
        p.y < m_clientTop + height)
        ::SetCursor(m_cursor);
}

// (Win: inlined into the callers, e.g. RenderControlManager; address unknown)
void Window::Invalidate(const Rect* rect)
{
    if (!m_hwnd)
        return;
    if (!rect) {
        InvalidateRect(m_hwnd, NULL, FALSE);
        return;
    }
    RECT r;
    r.left = (LONG)rect->m_min.x;
    r.top = (LONG)rect->m_min.y;
    r.right = (LONG)(rect->m_min.x + rect->GetWidth());
    r.bottom = (LONG)(rect->m_min.y + rect->GetHeight());
    InvalidateRect(m_hwnd, &r, FALSE);
}

// Win 0x40d0e0
void Window::SetModalState(bool modal)
{
    UInt32 before = m_modalCount;
    if (modal)
        ++m_modalCount;
    else
        --m_modalCount;
    if ((before != 0) == (m_modalCount != 0))
        return;
    EnableMenuItem(GetSystemMenu(m_hwnd, FALSE), SC_CLOSE, m_modalCount != 0 ? MF_GRAYED : MF_ENABLED);
    HMENU menuBar = ::GetMenu(m_hwnd);
    if (menuBar) {
        UINT flags = (m_modalCount != 0 ? MF_GRAYED : MF_ENABLED) | MF_BYPOSITION;
        if (EnableMenuItem(menuBar, 0, flags) != (BOOL)-1) {
            UINT position = 0;
            do {
                ++position;
            } while (EnableMenuItem(menuBar, position, flags) != (BOOL)-1);
        }
        DrawMenuBar(m_hwnd);
    }
    if (IsIconic(m_hwnd))
        ::ShowWindow(m_hwnd, SW_RESTORE);
    EnableWindow(m_hwnd, m_modalCount == 0);
    InvalidateRect(m_hwnd, NULL, FALSE);
}

// Win 0x40eb50 (no `this` after /LTCG)
bool Window::IsKeyPressed(UInt32 key) const
{
    if (!key)
        return false;
    return (GetAsyncKeyState(s_virtualKeyMap.virtualKeyOfKey[key & 0xff]) & 0x8000) != 0;
}

// Win 0x40c1f0 (ESI = this, EDI = bit)
void Window::CaptureMouseButton(UInt32 buttonBit)
{
    if (m_capturedButtons == 0)
        SetCapture(m_hwnd);
    m_capturedButtons |= buttonBit;
}

// Win 0x40c210: WM_MOVE / SC_MOVE — remembers the client origin, notifies the frame position.
void Window::OnNativeMove()
{
    if (IsIconic(m_hwnd))
        return;
    RECT frame;
    GetWindowRect(m_hwnd, &frame);
    RECT border = {0, 0, 0, 0};
    DWORD windowStyle = (DWORD)GetWindowLongW(m_hwnd, GWL_STYLE);
    AdjustWindowRect(&border, windowStyle, ::GetMenu(m_hwnd) != 0);
    Vector2 position((Real32)frame.left, (Real32)frame.top);
    m_clientLeft = frame.left - border.left;
    m_clientTop = frame.top - border.top;
    NotifyMove(position);
}

// Win 0x40c2c0: WM_SIZE / SC_SIZE — notifies the frame and client sizes.
void Window::OnNativeSize()
{
    if (IsIconic(m_hwnd))
        return;
    RECT frame, client;
    GetWindowRect(m_hwnd, &frame);
    GetClientRect(m_hwnd, &client);
    Vector2 clientSize((Real32)(client.right - client.left), (Real32)(client.bottom - client.top));
    Vector2 size((Real32)(frame.right - frame.left), (Real32)(frame.bottom - frame.top));
    NotifyResize(size, clientSize);
}

// Win 0x40ce90
UInt32 Window::GetKeyModifiers()
{
    UInt16 control = (UInt16)GetKeyState(VK_CONTROL);
    UInt16 shift = (UInt16)GetKeyState(VK_SHIFT);
    UInt16 alt = (UInt16)GetKeyState(VK_MENU);
    return (UInt32)(control >> 15) | ((shift & 0x8000) >> 14) | ((alt & 0x8000) >> 13);
}

// Win 0x40cec0 (EBX = hwnd, EDI = vk)
void Window::SyncKeyState(HWND hwnd, WPARAM vk)
{
    bool down = (GetAsyncKeyState((int)vk) & 0x8000) != 0;
    bool queued = (GetKeyState((int)vk) & 0x8000) != 0;
    if (down) {
        if (!queued)
            SendMessageA(hwnd, WM_KEYDOWN, vk, 0);
    } else if (queued) {
        SendMessageA(hwnd, WM_KEYUP, vk, (LPARAM)0xc0000001);
    }
}

// Win 0x40cf10 (EAX = message, EDX = wParam, ECX = lParam, ESI = this, stack hwnd)
void Window::TranslateKeyMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    bool repeat = ((UInt32)lParam >> 30) & 1;
    switch (message) {
    case WM_ACTIVATEAPP:
        if (wParam) {
            SyncKeyState(hwnd, VK_CONTROL);
            SyncKeyState(hwnd, VK_SHIFT);
            SyncKeyState(hwnd, VK_MENU);
        }
        break;
    case WM_KEYDOWN:
    case WM_SYSKEYDOWN:
        if (wParam < 0x100 && s_virtualKeyMap.keyOfVirtualKey[wParam])
            HandleKey(s_virtualKeyMap.keyOfVirtualKey[wParam], GetKeyModifiers(), true, repeat);
        break;
    case WM_KEYUP:
    case WM_SYSKEYUP:
        if (wParam < 0x100 && s_virtualKeyMap.keyOfVirtualKey[wParam])
            HandleKey(s_virtualKeyMap.keyOfVirtualKey[wParam], GetKeyModifiers(), false, repeat);
        break;
    case WM_CHAR:
    case WM_SYSCHAR:
        HandleCharacter((UniChar16)wParam, GetKeyModifiers(), repeat);
        break;
    default:
        break;
    }
}

// Win 0x40d410
void Window::TrackMouse()
{
    if (IsIconic(m_hwnd) || !IsWindowEnabled(m_hwnd))
        return;
    POINT p;
    if (!GetCursorPos(&p))
        return;
    HWND under = WindowFromPoint(p);
    if (m_capturedButtons == 0 && under != m_hwnd) {
        if (m_mouseInside) {
            HandleMouseMove(Vector2((Real32)(p.x - m_clientLeft), (Real32)(p.y - m_clientTop)));
            m_mouseInside = false;
        }
    } else {
        HandleMouseMove(Vector2((Real32)(p.x - m_clientLeft), (Real32)(p.y - m_clientTop)));
        m_mouseInside = true;
    }
    if (!m_showCursor) {
        POINT client;
        client.x = (LONG)m_mousePosition.x;
        client.y = (LONG)m_mousePosition.y;
        HWND child = ChildWindowFromPoint(under, client);
        NotifyInput21(child != m_hwnd);
    }
}

// Win 0x40eb80 (ESI = command)
UINT Window::GetCommandMenuState(const Command* command)
{
    if (!command)
        return MFS_GRAYED;
    UINT state = command->GetEnable() ? MFS_ENABLED : MFS_GRAYED;
    if (command->IsToggleable())
        state |= command->IsToggled() ? MFS_CHECKED : 0;
    return state;
}

// Win 0x40ebc0 (cdecl)
// Item label: the title, then for a short-cut "   (" + localized modifier names + key + ")"; the
// first occurrence of the key's character gets the '&' mnemonic. Submenu titles get '&' in front.
// An item without a title is a separator. The Command* is the item data.
void Window::BuildMenu(const Menu* menu, HMENU* nativeMenu, UINT* nextId)
{
    const BasicArray<RefPointer<MenuItem> >& items = menu->GetItems();
    for (UInt32 i = 0; i < items.GetSize(); ++i) {
        const MenuItem* item = items[i].Get();
        HMENU submenu = 0;
        Menu* subItems = item->GetSubMenu();
        if (subItems && !item->GetTitle().IsEmpty()) {
            submenu = CreateMenu();
            BuildMenu(subItems, &submenu, nextId);
        }
        String label(item->GetTitle());
        if (item->GetSubMenu()) {
            label.Insert(0, 1, (UniChar16)'&');
        } else if ((UInt16)item->GetKey() != 0) {
            UInt32 key = (UInt16)item->GetKey();
            char keyChar = (char)s_virtualKeyMap.virtualKeyOfKey[key & 0xff];
            if (keyChar) {
                label.Append("   (");
                if (item->GetModifiers() & kModifierControl)
                    label << LocalizedString(String("MENUCTRL+"));
                if (item->GetModifiers() & kModifierShift)
                    label << LocalizedString(String("MENUSHIFT+"));
                if (item->GetModifiers() & kModifierAlt)
                    label << LocalizedString(String("MENUALT+"));
                switch (key) {
                case kKeyReturn:    label << LocalizedString(String("MENUENTER")); break;
                case kKeySpace:     label << LocalizedString(String("MENUSSPACEBAR")); break;
                case kKeyF1:        label << "F1"; break;
                case kKeyF2:        label << "F2"; break;
                case kKeyF3:        label << "F3"; break;
                case kKeyF4:        label << "F4"; break;
                case kKeyF5:        label << "F5"; break;
                case kKeyF6:        label << "F6"; break;
                case kKeyF7:        label << "F7"; break;
                case kKeyF8:        label << "F8"; break;
                case kKeyF9:        label << "F9"; break;
                case kKeyF10:       label << "F10"; break;
                case kKeyF11:       label << "F11"; break;
                case kKeyF12:       label << "F12"; break;
                case kKeyHome:      label << LocalizedString(String("MENUHOME")); break;
                case kKeyPageUp:    label << LocalizedString(String("MENUPAGEUP")); break;
                case kKeyEnd:       label << LocalizedString(String("MENUEND")); break;
                case kKeyPageDown:  label << LocalizedString(String("MENUPAGEDN")); break;
                case kKeyDelete:    label << LocalizedString(String("MENUDELETE")); break;
                default: {
                    // NOTE: the mnemonic goes to the first occurrence of the (upper-case) key
                    // character anywhere in the label built so far, the modifier names included.
                    UniChar16 c = (UniChar16)(Int16)keyChar;
                    UInt32 pos = label.Find(c);
                    if (pos != String::npos)
                        label.Insert(pos, 1, (UniChar16)'&');
                    label << String(1, c);
                    break;
                }
                }
                label.Append(")");
            }
        }
        MENUITEMINFOW mi;
        mi.cbSize = sizeof(mi);
        mi.fMask = (submenu ? MIIM_SUBMENU : 0) | MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;
        mi.fType = item->GetTitle().IsEmpty() ? MFT_SEPARATOR : 0;
        mi.fState = GetCommandMenuState(item->GetCommand());
        mi.wID = (*nextId)++;
        mi.hSubMenu = submenu;
        mi.hbmpChecked = 0;
        mi.hbmpUnchecked = 0;
        mi.dwItemData = subItems ? 0 : (ULONG_PTR)item->GetCommand();
        mi.dwTypeData = label.IsEmpty() ? (LPWSTR)L"" : (LPWSTR)label.GetBuffer();
        mi.cch = item->GetTitle().GetLength();
        mi.hbmpItem = 0;
        InsertMenuItemW(*nativeMenu, GetMenuItemCount(*nativeMenu), TRUE, &mi);
    }
}

// Win 0x40f4f0
// NOTE: the previous HMENU is not destroyed (as in the original).
void Window::SetMenu(const Menu* menu)
{
    if (!m_hasMenuBar)
        return;
    m_menu = menu;
    HMENU bar = CreateMenu();
    if (menu->GetItems().GetSize() != 0) {
        UINT nextId = 0;
        BuildMenu(menu, &bar, &nextId);
    } else {
        MENUITEMINFOW mi;
        memset(&mi, 0, sizeof(mi));
        mi.cbSize = sizeof(mi);
        mi.fMask = MIIM_ID | MIIM_DATA | MIIM_STRING | MIIM_FTYPE;
        mi.fState = MFS_GRAYED;
        mi.dwTypeData = (LPWSTR)L"";
        InsertMenuItemW(bar, GetMenuItemCount(bar), TRUE, &mi);
    }
    ::SetMenu(m_hwnd, bar);
}

// Win 0x40f610 (ECX = this, cdecl argument)
void Window::OpenFile(const UniChar16* nativePath)
{
    if (IsWindowEnabled(m_hwnd) && m_modalCount == 0) {
        Path path(String(), 0);
        String native(nativePath);
        bool exists = false;
        if (path.Append(Platform::GetInstance()->NativeToVFSPath(native)) && path.GetParentDirectory())
            exists = path.GetParentDirectory()->FileExists(path.GetName());
        if (exists) {
            Application::GetInstance()->OpenDocumentFile(path);
            if (IsIconic(m_hwnd))
                ::ShowWindow(m_hwnd, SW_RESTORE);
            SetForegroundWindow(m_hwnd);
            return;
        }
    }
    FLASHWINFO flash;
    flash.cbSize = sizeof(flash);
    flash.hwnd = m_hwnd;
    flash.dwFlags = FLASHW_CAPTION;
    flash.uCount = 2;
    flash.dwTimeout = 0;
    FlashWindowEx(&flash);
}

// Win 0x40e1a0: validates the style; the size is raised to the minimum size, a full-screen
// style loses position and decoration, a missing colour depth becomes the current one.
bool Window::CheckDisplayMode(WindowStyle& style)
{
    if (style.size.x < 0.0f || style.size.y < 0.0f || style.minimumSize.x < 0.0f ||
        style.minimumSize.y < 0.0f)
        return false;
    style.size.x = style.size.x > style.minimumSize.x ? style.size.x : style.minimumSize.x;
    style.size.y = style.size.y > style.minimumSize.y ? style.size.y : style.minimumSize.y;
    if (!style.flag28) {
        style.value2c = 0;
        if (!style.flag29)
            style.flag2a = false;
    } else {
        style.position = Vector2(0.0f, 0.0f);
        style.flag29 = false;
        style.flag2a = false;
    }
    if (style.value2c == 0) {
        DEVMODEA mode;
        mode.dmSize = sizeof(mode);
        mode.dmDriverExtra = 0;
        if (!EnumDisplaySettingsA(NULL, ENUM_CURRENT_SETTINGS, &mode))
            return false;
        style.value2c = mode.dmBitsPerPel;
    }
    return true;
}

// Win 0x40e2b0: leaving full screen restores the registry mode; entering it (or changing its
// size or depth) switches the display to size × value2c at 75 Hz.
bool Window::ChangeDisplayMode(const WindowStyle& style, const WindowStyle& previous)
{
    if (!style.flag28) {
        if (previous.flag28)
            ChangeDisplaySettingsA(NULL, 0);
        return true;
    }
    if (previous.flag28 && style.size.IsEqual(previous.size, 1e-5f) && previous.value2c == style.value2c)
        return true;
    DEVMODEA mode;
    memset(&mode, 0, sizeof(mode));
    mode.dmSize = sizeof(mode);
    mode.dmDisplayFrequency = 75;
    mode.dmFields = DM_BITSPERPEL | DM_PELSWIDTH | DM_PELSHEIGHT | DM_DISPLAYFREQUENCY;
    mode.dmPelsWidth = (DWORD)style.size.x;
    mode.dmBitsPerPel = style.value2c;
    mode.dmPelsHeight = (DWORD)style.size.y;
    ChangeDisplaySettingsA(&mode, CDS_FULLSCREEN);
    return true;
}

// Win 0x40f800
LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    if (message == WM_CREATE) {
        Window* created = (Window*)((CREATESTRUCTW*)lParam)->lpCreateParams;
        // PORT64: the pointer-sized window long (on Win32 SetWindowLongPtrW is SetWindowLongW and
        // GWLP_USERDATA is GWL_USERDATA: the original call)
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, (LONG_PTR)created);
        created->m_hwnd = hwnd;
    }
    Window* window = (Window*)GetWindowLongPtrW(hwnd, GWLP_USERDATA);   // PORT64 (as above)
    if (!window)
        return DefWindowProcW(hwnd, message, wParam, lParam);
    LRESULT result = window->HandleMessage(hwnd, message, wParam, lParam);
    if (message == WM_DESTROY) {
        window->m_hwnd = 0;
        SetWindowLongPtrW(hwnd, GWLP_USERDATA, 0);   // PORT64 (as above)
    }
    return result;
}

// Win 0x40f930
LRESULT Window::HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
    TranslateKeyMessage(hwnd, message, wParam, lParam);
    switch (message) {
    case WM_DESTROY:
        if (!m_destroying)
            NotifyClose();
        else
            m_destroying = false;
        break;
    case WM_MOVE:
        OnNativeMove();
        break;
    case WM_SIZE:
        OnNativeSize();
        break;
    case WM_SETFOCUS:
        NotifyInput13(true, GetKeyModifiers(), 0);
        break;
    case WM_KILLFOCUS: {
        // the window getting the focus, if it is one of ours (a child, e.g. a text field)
        HWND focus = (HWND)wParam;
        if (focus) {
            HWND parent = GetParent(focus);
            while (parent && parent != hwnd)
                parent = GetParent(parent);
            if (!parent)
                focus = 0;
        }
        // PORT64: window handles keep 32 significant bits on x64 (they are shared with 32-bit
        // processes) and ControlManager only tests the value against 0
        NotifyInput13(false, 0, (UInt32)(UINT_PTR)focus);
        break;
    }
    case WM_ACTIVATE:
        NotifyActivate(wParam != 0);
        InvalidateRect(m_hwnd, NULL, FALSE);
        break;
    case WM_PAINT: {
        PAINTSTRUCT paint;
        if (BeginPaint(hwnd, &paint)) {
            // PORT64: the PAINTSTRUCT's address only identifies the paint (no observer reads
            // it), so the truncation on x64 is harmless
            if (!IsRectEmpty(&paint.rcPaint))
                NotifyPaint((UInt32)(UINT_PTR)&paint);
            EndPaint(hwnd, &paint);
        }
        break;
    }
    case WM_CLOSE:
        if (m_modalCount != 0 || !NotifyCloseRequest())
            return 0;
        break;
    case WM_SHOWWINDOW:
        NotifyInput14(wParam != 0);
        break;
    case WM_SETCURSOR:
        if (m_showCursor) {
            POINT p;
            if (GetCursorPos(&p)) {
                RECT r;
                GetClientRect(hwnd, &r);
                int width = r.right - r.left;
                int height = r.bottom - r.top;
                if (m_clientLeft <= p.x && p.x < m_clientLeft + width && m_clientTop <= p.y &&
                    p.y < m_clientTop + height) {
                    ::SetCursor(m_cursor);
                    return 1;
                }
            }
        }
        break;
    case WM_GETMINMAXINFO:
        if (m_style.minimumSize.x > 0.0f || m_style.minimumSize.y > 0.0f) {
            MINMAXINFO* info = (MINMAXINFO*)lParam;
            info->ptMinTrackSize.x = (LONG)m_style.minimumSize.x;
            info->ptMinTrackSize.y = (LONG)m_style.minimumSize.y;
            return 0;
        }
        break;
    case WM_DISPLAYCHANGE:
        NotifyInput19();
        break;
    case WM_COMMAND:
        if (HIWORD(wParam) == 0) {
            MENUITEMINFOW mi;
            memset(&mi, 0, sizeof(mi));
            mi.cbSize = sizeof(mi);
            mi.fMask = MIIM_DATA;
            GetMenuItemInfoW(::GetMenu(hwnd), LOWORD(wParam), FALSE, &mi);
            if (mi.dwItemData)
                ((Command*)mi.dwItemData)->Do();
            InvalidateRect(hwnd, NULL, FALSE);
        }
        break;
    case WM_SYSCOMMAND:
        if ((wParam & 0xfff0) == SC_SIZE)
            OnNativeSize();
        else if ((wParam & 0xfff0) == SC_MOVE)
            OnNativeMove();
        break;
    case WM_INITMENUPOPUP:
        if (HIWORD(lParam) == 0) {
            HMENU popup = (HMENU)wParam;
            UINT count = (UINT)GetMenuItemCount(popup);
            for (UINT position = 0; position < count; ++position) {
                MENUITEMINFOW mi;
                memset(&mi, 0, sizeof(mi));
                mi.cbSize = sizeof(mi);
                mi.fMask = MIIM_STATE | MIIM_ID | MIIM_DATA;
                UINT id = GetMenuItemID(popup, (int)position);
                GetMenuItemInfoW(popup, id, FALSE, &mi);
                if (mi.dwItemData) {
                    mi.fState = GetCommandMenuState((const Command*)mi.dwItemData);
                    SetMenuItemInfoW(popup, id, FALSE, &mi);
                }
            }
            DrawMenuBar(m_hwnd);
        }
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    case WM_CTLCOLOREDIT:
        SetBkMode((HDC)wParam, OPAQUE);
        SetBkColor((HDC)wParam, RGB(255, 255, 255));
        return (LRESULT)m_background;
    case WM_LBUTTONDOWN:
    case WM_LBUTTONUP:
    case WM_LBUTTONDBLCLK:
    case WM_RBUTTONDOWN:
    case WM_RBUTTONUP:
    case WM_RBUTTONDBLCLK:
    case WM_MBUTTONDOWN:
    case WM_MBUTTONUP:
    case WM_MBUTTONDBLCLK: {
        UInt32 index = (message - WM_LBUTTONDOWN) / 3;       // 0 left, 1 right, 2 middle
        UInt32 kind = (message - WM_LBUTTONDOWN) % 3;        // 0 down, 1 up, 2 double click
        UInt32 bit = 1u << index;
        Vector2 position((Real32)(short)LOWORD(lParam), (Real32)(short)HIWORD(lParam));
        if (kind == 1) {
            m_capturedButtons &= ~bit;
            if (m_capturedButtons == 0)
                ReleaseCapture();
            HandleMouseButton(position, index, false, false);
        } else {
            CaptureMouseButton(bit);
            HandleMouseButton(position, index, true, kind == 2);
        }
        break;
    }
    case WM_MOUSEWHEEL:
        HandleMouseWheel(0, (Real32)((double)(short)HIWORD(wParam) / 120.0));
        break;
    case WM_ENTERMENULOOP: {
        LEGO_WINDOW_NOTIFY(OnInput22())
        break;
    }
    case WM_EXITMENULOOP: {
        {
            LEGO_WINDOW_NOTIFY(OnInput23())
        }
        HandleKey(0, GetKeyModifiers(), false, false);
        InvalidateRect(hwnd, NULL, FALSE);
        break;
    }
    case WM_CAPTURECHANGED:
        if (m_capturedButtons != 0) {
            const Vector2 nowhere(-3.4028235e38f, -3.4028235e38f);
            if (m_capturedButtons & 1)
                HandleMouseButton(nowhere, 0, false, false);
            if (m_capturedButtons & 2)
                HandleMouseButton(nowhere, 1, false, false);
            if (m_capturedButtons & 4)
                HandleMouseButton(nowhere, 2, false, false);
            m_capturedButtons = 0;
        }
        break;
    case WM_POWERBROADCAST:
        switch (wParam) {
        case 4:                 // PBT_APMSUSPEND
            NotifyInput20(0);
            break;
        case 6:                 // PBT_APMRESUMECRITICAL
        case 7:                 // PBT_APMRESUMESUSPEND
        case 0x12:              // PBT_APMRESUMEAUTOMATIC
            NotifyInput20(1);
            break;
        }
        break;
    case WM_DROPFILES: {
        // NOTE: DragFinish is never called (as in the original).
        WCHAR file[0x402];
        UINT length = DragQueryFileW((HDROP)wParam, 0, file, 0x400);
        if (length - 1 < 0x3ff)
            OpenFile((const UniChar16*)file);
        return 0;
    }
    case 0x8000: {
        // file handed over by a second LDD instance as a global atom (TU 060)
        WCHAR file[0x402];
        UINT length = GlobalGetAtomNameW((ATOM)wParam, file, 0x400);
        GlobalDeleteAtom((ATOM)wParam);
        if (length - 1 < 0x3ff)
            OpenFile((const UniChar16*)file);
        return 0;
    }
    default:
        break;
    }
    return DefWindowProcW(hwnd, message, wParam, lParam);
}
#endif // _WIN32

#undef LEGO_WINDOW_NOTIFY

} // namespace LEGO
