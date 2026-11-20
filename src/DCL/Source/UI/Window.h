// LEGO Digital Designer — restored source
// LEGO::Window — platform window: a Notifier<WindowObserver> that queues the input it receives as
// HumanInput and dispatches it to its observers from Update(). Also HumanInput (input interface,
// empty handlers), WindowInput (+ window events), WindowObserver, ModalUpdateReciever (modal-loop
// callback; RTTI spelling), WindowStyle (creation parameters).
// TU 020 (file name inferred "Window.cpp"): Mac 0x20470-0x2d5b9, Win 0x40be60-0x4124c0. Layout and
// slots. Window.cpp = shared members + Win32 natives;
// WindowMac.mm = Cocoa natives with HOST-ONLY fallbacks. Directory UI/ is a guess.
//
// Layout evidence: the ObjC ivar type encoding of LEGOOpenGLView.window / LEGOMacWindow.window
// spells out the whole Mac object:
//   ^^? ^^? {BasicArray<WindowObserver*>} {BasicArray<Iterator*>} ^^? {BasicArray<UpdateObserver*>}
//   {BasicArray<Iterator*>} ^^? ^{Updater} ^^? {WindowStyle={String}{Vector2}{Vector2}{Vector2}BBBIB}
//   {Vector2} B B {RefPointer<const Menu>} {String} I ^{ModalUpdateReciever}
//   {RefPointer<NativeControlKeyboardFocusObserver>} ^v B ^{__CFDictionary} B
//   {BasicQueue<Window::Message, BasicDeque<Window::Message> >} ^^? i      (Window, 0xfc bytes)
//   OpenGLWindow adds  I ^v  before the Object base (0x104 bytes).
// Win layout from the ctor 0x40c3c0 and the accessors (offsets per member).
//
// Virtual slots (Mac primary vtable 0x704034, 60 entries; Win 0xa1f16c, 53 entries):
//   Mac  0- 7  HumanInput handlers (Window overrides: forward to 38-42)     Win  0- 7
//   Mac  8/ 9  D1/D0 (Win: destructor in the Object vtable)                 —
//   Mac 10-23  WindowInput handlers (Window overrides 10-21 → 43-54)        Win  8-21
//   Mac 24-34  Window: GetStyle … GetSize                                   Win 22-32
//   Mac 35-37  overriders of secondary-base virtuals (ModalUpdate, OnFinalRelease, Update):
//              Itanium gives them primary entries, MSVC does not            —
//   Mac 38-56  Window: input handling, notifications, display mode          Win 33-51
//   Mac 57-59  Mac-only display capture virtuals                            —
//   —          Win-only window procedure                                    Win 52
// All member names inferred; "(signature inferred)" = from argument counts / uses only.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "Notifier.h"
#include "Updater.h"
#include "BasicQueue.h"

#if defined(_WIN32)
#include <windows.h>
#else
#include <CoreFoundation/CoreFoundation.h>
#endif
#include "Menu.h"
#include "Key.h"

namespace LEGO {

class Window;
class Command;
class Menu;                                   // UI menu (Menu.h, TU 047)
class Path;
class HitTester;                              // Mac: native-control hit test (ControlManager's NativeControlHitTester)

// ===========================================================================================
// NativeControlKeyboardFocusObserver — lets native (Cocoa) controls hand the keyboard focus
// back to the window. Mac vtable 0x706fcc {D1 0x204f0, D0 0x20530}, Object-in 0x706fec;
// layout vptr, Window* +0x04, Object +0x08. Window::SetKeyboardFocusObserver (Mac 0x23210)
// stores it and sets its window. (Mac 0x20490, Cocoa toggleFirstResponder:; in WindowMac.mm.)
// ===========================================================================================
class NativeControlKeyboardFocusObserver : public virtual Object {
public:
    NativeControlKeyboardFocusObserver() : m_window(0) {}
    virtual ~NativeControlKeyboardFocusObserver() {}                  // Mac 0x204f0/0x20530
#if !defined(_WIN32)
    // [[window contentView] toggleFirstResponder:nil]: moves the keyboard focus between the
    // native (Cocoa) controls and the LDD drawing view. Caller: ControlManager's observer
    // (Mac 0xd7620). (name inferred from the selector)
    void ToggleFirstResponder();                                      // Mac 0x20490
#endif

    Window* m_window;                                                 // Mac +0x04
};

// ===========================================================================================
// HumanInput — Mac vtable 0x7042ec {0x271b0..0x27220 (empty), D1 0x273e0, D0 0x27400};
// Win 0x9cec90 {0x40be80, 0x424ec0 ×4, 0x520a60, 0x424ec0, 0x520a60} (ICF'd empty bodies:
// ret 4 / ret 8 / ret 0xc give the argument counts). Virtual base Object (Mac +0x04 alone).
// Callers: LEGOMacDrawingView mouseDown:/scrollWheel:/keyDown:/flagsChanged: (Mac 0x216ed0 …).
// ===========================================================================================
class HumanInput : public virtual Object {
public:
    // (signatures inferred)
    virtual void OnMouseMove(const Vector2& /*position*/) {}                          // slot 0
    virtual void OnMouseDown(const Vector2& /*position*/, UInt32 /*button*/) {}       // slot 1
    virtual void OnMouseUp(const Vector2& /*position*/, UInt32 /*button*/) {}         // slot 2
    virtual void OnMouseDoubleClick(const Vector2& /*position*/, UInt32 /*button*/) {} // slot 3
    // axis 0 = horizontal, 1 = vertical (scrollWheel: deltaX / deltaY, Mac 0x217310)
    virtual void OnMouseWheel(UInt32 /*axis*/, Real32 /*delta*/) {}                   // slot 4
    virtual void OnKeyDown(UInt32 /*key*/, UInt32 /*modifiers*/, bool /*repeat*/) {}   // slot 5
    virtual void OnKeyUp(UInt32 /*key*/, UInt32 /*modifiers*/) {}                      // slot 6
    virtual void OnCharacter(UniChar16 /*c*/, UInt32 /*modifiers*/, bool /*repeat*/) {} // slot 7
    virtual ~HumanInput() {}                                                           // Mac 8/9
};

// ===========================================================================================
// WindowInput — Mac vtable 0x70424c (24 slots), Win 0x9ceccc (22 slots). Defaults: empty,
// except slots 15 and 17 which return true (Mac 0x272e0 / 0x27300, Win 0x4f5b70).
// ===========================================================================================
class WindowInput : public HumanInput {
public:
    virtual ~WindowInput() {}                                         // Mac 0x27230/0x27250

    // (signatures inferred; senders: LEGOMacWindow windowDidMove: 0x2164a0 → 10,
    // windowDidBecomeKey:/windowDidResignKey: → 12 (+13), windowShouldClose: → 15,
    // windowWillClose: → 16, LEGOOpenGLView drawRect: → 18)
    virtual void OnMove(const Vector2& /*position*/) {}               // slot 10, Win 8
    // OpenGLRender overrides it to keep the viewport size (Mac 0xf7270, +0x11c).
    virtual void OnResize(const Vector2& /*size*/, const Vector2& /*clientSize*/) {} // slot 11, Win 9
    virtual void OnActivate(bool /*active*/) {}                       // slot 12, Win 10
    // (name unknown) Window re-posts a key-up message with `modifiers` when `flag` is set.
    // ControlManager (Mac 0xdad90) forwards it as OnManagerWindowKeyboardFocusChanged(flag,
    // modifiers): the window gained / lost the keyboard focus.
    // Win: WM_SETFOCUS → (true, modifiers, 0), WM_KILLFOCUS → (false, 0, the window getting the
    // focus if it is a child of this window, else 0).
    virtual void OnInput13(bool /*flag*/, UInt32 /*modifiers*/, UInt32 /*arg*/) {} // slot 13, Win 11
    // Win WM_SHOWWINDOW: arg = shown (wParam != 0). (name unknown)
    virtual void OnInput14(UInt32 /*arg*/) {}                         // slot 14, Win 12 (name unknown)
    // Return false to keep the window open.
    virtual bool OnCloseRequest() { return true; }                    // slot 15, Win 13
    virtual void OnClose() {}                                         // slot 16, Win 14
    // The native window has just been created (Create); false makes Create fail.
    virtual bool OnInput17() { return true; }                         // slot 17, Win 15 (name unknown)
    virtual void OnPaint(UInt32 /*arg*/) {}                           // slot 18, Win 16
    // Win WM_DISPLAYCHANGE: the display mode changed. (name unknown)
    virtual void OnInput19() {}                                       // slot 19, Win 17 (name unknown)
    // Win WM_POWERBROADCAST: 0 = suspend (PBT_APMSUSPEND), 1 = resume. (name unknown)
    virtual void OnInput20(UInt32 /*arg*/) {}                         // slot 20, Win 18 (name unknown)
    // Win TrackMouse (only when the window does not manage the cursor): 1 = the mouse is over
    // a child window (native control), 0 = over the window itself. (name unknown)
    virtual void OnInput21(UInt32 /*arg*/) {}                         // slot 21, Win 19 (name unknown)
    // ControlManager (Mac 0xdb260 / 0xdb520) forwards 22 / 23 as OnManagerWindowEnterMenuLoop /
    // OnManagerWindowExitMenuLoop: a menu tracking loop starts / ends.
    virtual void OnInput22() {}                                       // slot 22, Win 20 (name unknown)
    virtual void OnInput23() {}                                       // slot 23, Win 21 (name unknown)
};

// ===========================================================================================
// WindowObserver — Mac vtable 0x70710c (26 slots): WindowInput + slot 24 (returns true,
// Mac 0xe8bb0, Win 0x4f5b70) + Mac-only slot 25 (OnFinalRelease overrider 0xdc480).
// Win: WindowInput vfptr (23 slots) + Observer vfptr at +8 {OnAttached, OnDetached}.
// ===========================================================================================
class WindowObserver : public WindowInput, public Observer<Window, WindowObserver> {
public:
    virtual ~WindowObserver() {}
    virtual bool OnObserverQuery24() { return true; }                 // slot 24, Win 22 (name unknown)

protected:
    // Both bases inherit Object::OnFinalRelease (virtual base): the observer's one wins.
    virtual void OnFinalRelease() { Observer<Window, WindowObserver>::OnFinalRelease(); } // Mac 0xdc480
};

// ===========================================================================================
// ModalUpdateReciever — Mac vtable 0x7601f8 {D1 0x27ca0, D0 0x27cb0, ModalUpdate = pure};
// Win 0x9d63ac {sdd 0x40be60, _purecall}. No data. (RTTI spelling "Reciever".)
// ===========================================================================================
class ModalUpdateReciever {
public:
    virtual ~ModalUpdateReciever() {}                                 // Mac 0x27ca0/0x27cb0, Win 0x40be60
    // Called repeatedly while a modal loop runs (LEGOApplicationDelegate updateModal: 0x3060).
    virtual void ModalUpdate() = 0;                                   // (name inferred)
};

// ===========================================================================================
// WindowStyle — 0x34 bytes on Mac (ivar encoding {String, Vector2 ×3, B B B I B}), 0x2c on Win
// (String 4 bytes, + icon); default {"", 0, 0, 0, false, true, true, 0, false} (Mac 0x205c0, Win
// ctor 0x40bf30, copy 0x40c9c0). Field meaning from Window::Create / native window creation (Mac
// 0x20ba0 / 0x212c0, Win 0x40c660 / 0x40e400) and the display-mode code (Mac 0x26340 / 0x26450,
// Win 0x40e1a0 / 0x40e2b0); field names kept from R0.2.
// ===========================================================================================
struct WindowStyle {
    WindowStyle()
        : flag28(false), flag29(true), flag2a(true), value2c(0), hidden(false)
#if defined(_WIN32)
          , icon(0)
#endif
    {}

    String  title;          // Mac +0x00 (Window +0x58), Win +0x00 (Window +0x64)
    // Top-left corner of the window frame on the screen (Mac: top-left origin, flipped).
    Vector2 position;       // Mac +0x10 (Window +0x68), Win +0x04
    // Outer size of the window frame (CheckDisplayMode raises it to minimumSize).
    Vector2 size;           // Mac +0x18 (Window +0x70), Win +0x0c
    // Minimum frame size while resizing (Mac setMinSize:, Win WM_GETMINMAXINFO) if non-zero.
    Vector2 minimumSize;    // Mac +0x20 (Window +0x78), Win +0x14
    // Full screen: switches the display mode to `size` × `value2c` bits, Mac shielding window
    // level / Win WS_EX_TOPMOST; forces flag29 / flag2a off and the position to 0.
    bool    flag28;         // Mac +0x28, Win +0x1c (fullScreen; name kept)
    // Decorated: title bar + close / minimise box (Win WS_OVERLAPPED…, else WS_POPUP).
    bool    flag29;         // Mac +0x29, Win +0x1d (titled; default true)
    // Resizable (Mac NSResizableWindowMask, Win WS_THICKFRAME | WS_MAXIMIZEBOX).
    bool    flag2a;         // Mac +0x2a, Win +0x1e (resizable; default true)
    // Colour depth of the full-screen mode in bits (0: the current one, filled in by
    // CheckDisplayMode); OpenGLWindow passes it to the pixel format.
    UInt32  value2c;        // Mac +0x2c, Win +0x20 (colorBits)
    bool    hidden;         // Mac +0x30 (Window +0x88), Win +0x24 (Window +0x88)
#if defined(_WIN32)
    // Icon of "DCLWindowClass" (used by the first window only): a resource id (high word 0,
    // LoadIcon) or an HICON. (name inferred)
    UInt32  icon;           // Win +0x28 (Window +0x8c)
#endif
};

// ===========================================================================================
// Window
// Mac sizeof 0xfc (Object at +0xf4); Win: WindowInput vfptr +0, vbptr +4, Updater +0x08,
// UpdateObserver +0x30, ModalUpdateReciever +0x3c, Notifier<WindowObserver> +0x40, members
// from +0x64, vtordisp +0xf0, Object +0xf4.
// ===========================================================================================
class Window : public WindowInput,
               public Notifier<WindowObserver>,
               public Updater,
               private UpdateObserver,
               public ModalUpdateReciever {
public:
    // One queued input event (0x28 bytes; built by slots 39-42, dispatched by Update()).
    struct Message {
        enum Type { kMouseButton = 0, kMouseWheel = 1, kKey = 2, kCharacter = 3 };

        Message()
            : type(0), button(0), down(false), doubleClick(false), wheelAxis(0), wheelDelta(0.0f),
              key(0), modifiers(0), repeat(false), character(0) {}

        UInt32    type;          // +0x00
        Vector2   position;      // +0x04 (mouse button)
        UInt32    button;        // +0x0c
        bool      down;          // +0x10 (mouse button / key)
        bool      doubleClick;   // +0x11
        UInt32    wheelAxis;     // +0x14
        Real32    wheelDelta;    // +0x18
        UInt32    key;           // +0x1c
        UInt32    modifiers;     // +0x20
        bool      repeat;        // +0x24
        UniChar16 character;     // +0x26
    };

    // `flag95` is stored at Mac +0x95 / Win +0x99 (meaning unknown).
    explicit Window(bool flag95);                                     // Mac 0x205c0 (C2), Win 0x40c3c0
    virtual ~Window();                                                // Mac 0x234f0/0x23780, Win 0x412420 (sdd)

    // --- HumanInput: queue the event (Mac 0x27950-0x27a90, Win 0x40c050-0x40c120) ---
    virtual void OnMouseMove(const Vector2& position);                // → HandleMouseMove
    virtual void OnMouseDown(const Vector2& position, UInt32 button); // → HandleMouseButton(…, true, false)
    virtual void OnMouseUp(const Vector2& position, UInt32 button);   // → HandleMouseButton(…, false, false)
    virtual void OnMouseDoubleClick(const Vector2& position, UInt32 button); // → (…, true, true)
    virtual void OnMouseWheel(UInt32 axis, Real32 delta);             // → HandleMouseWheel
    virtual void OnKeyDown(UInt32 key, UInt32 modifiers, bool repeat); // → HandleKey(…, true, repeat)
    virtual void OnKeyUp(UInt32 key, UInt32 modifiers);               // → HandleKey(…, false, false)
    virtual void OnCharacter(UniChar16 c, UInt32 modifiers, bool repeat); // → HandleCharacter

    // --- WindowInput: notify the observers (Mac 0x27ab0-0x27c10, Win 0x40c130-0x40c1e0) ---
    virtual void OnMove(const Vector2& position);
    virtual void OnResize(const Vector2& size, const Vector2& clientSize);
    virtual void OnActivate(bool active);
    virtual void OnInput13(bool flag, UInt32 modifiers, UInt32 arg);
    virtual void OnInput14(UInt32 arg);
    virtual bool OnCloseRequest();
    virtual void OnClose();
    virtual bool OnInput17();
    virtual void OnPaint(UInt32 arg);
    virtual void OnInput19();
    virtual void OnInput20(UInt32 arg);
    virtual void OnInput21(UInt32 arg);

    // --- Window (Mac slot / Win slot) ---
    virtual const WindowStyle& GetStyle() const;                      // 24/22 Mac 0x27c30, Win 0x40bfe0
    // Creates the native window, or applies `style` to the existing one. (signature inferred)
    virtual bool Create(const WindowStyle& style);                    // 25/23 Mac 0x20ba0, Win 0x40c660
    virtual bool SetTitle(const String& title);                       // 26/24 Mac 0x218b0, Win 0x40cad0
    // Document file shown in the title bar (Mac setRepresentedFilename:). (name inferred)
    virtual bool SetRepresentedFile(const Path& path);                // 27/25 Mac 0x21910, Win 0x40cb10
    virtual const String& GetRepresentedFile() const;                 // 28/26 Mac 0x27c40, Win 0x40bff0
    // Mac setDocumentEdited:. (name inferred)
    virtual void SetModified(bool modified);                          // 29/27 Mac 0x21be0, Win 0x40cc30
    virtual bool IsHidden() const;                                    // 30/28 Mac 0x27c50, Win 0x40c000
    virtual void SetHidden(bool hidden);                              // 31/29 Mac 0x21c20, Win 0x40cd40
    // Lets the window manage the cursor (Win WM_SETCURSOR checks +0x98). (name inferred)
    virtual void ShowCursor(bool show);                               // 32/30 Mac 0x21c80, Win 0x40cd90
    // style.size - GetSize() (frame decoration). (name inferred)
    virtual Vector2 GetBorderSize() const;                            // 33/31 Mac 0x27c60, Win 0x40c010
    // Client area size in pixels (Win GetClientRect, Mac contentView frame). OpenGLRender's
    // constructor reads it (Win +0x80).
    virtual Vector2 GetSize() const;                                  // 34/32 Mac 0x21ca0, Win 0x40cda0

    // ModalUpdateReciever: dispatches queued input, then forwards to the registered
    // receiver (m_modalUpdateReciever).
    virtual void ModalUpdate();                                       // Mac 35 0x21e70 (thunk 0x231b0), Win 0x40d1f0

protected:
    // Restores the display mode, closes the native window, detaches all observers.
    virtual void OnFinalRelease();                                    // Mac 36 0x23a70 (thunks 0x23d60 …)

public:
    // Updater + UpdateObserver: dispatches the queued input, then updates the observers.
    virtual void Update();                                            // Mac 37 0x23de0 (thunks 0x23e10, 0x23e40)

    // --- input handling (Mac 38-42 / Win 33-37) ---
    // Stores the mouse position and notifies OnMouseMove at once (if it changed).
    virtual void HandleMouseMove(const Vector2& position);            // 38/33 Mac 0x23e70, Win 0x40d5c0
    virtual void HandleMouseButton(const Vector2& position, UInt32 button, bool down, bool doubleClick); // 39/34 Mac 0x24130, Win 0x40d6b0
    virtual void HandleMouseWheel(UInt32 axis, Real32 delta);         // 40/35 Mac 0x24210, Win 0x40d720
    virtual void HandleKey(UInt32 key, UInt32 modifiers, bool down, bool repeat); // 41/36 Mac 0x242e0, Win 0x40d780
    virtual void HandleCharacter(UniChar16 c, UInt32 modifiers, bool repeat); // 42/37 Mac 0x243b0, Win 0x40d7f0

    // --- observer notification (Mac 43-54 / Win 38-49), one per WindowInput slot 10-21 ---
    virtual void NotifyMove(const Vector2& position);                 // 43/38 Mac 0x24480 (stores position)
    virtual void NotifyResize(const Vector2& size, const Vector2& clientSize); // 44/39 Mac 0x24720 (stores size)
    virtual void NotifyActivate(bool active);                         // 45/40 Mac 0x249d0
    virtual void NotifyInput13(bool flag, UInt32 modifiers, UInt32 arg); // 46/41 Mac 0x24c50
    virtual void NotifyInput14(UInt32 arg);                           // 47/42 Mac 0x24f40
    virtual bool NotifyCloseRequest();                                // 48/43 Mac 0x251c0
    virtual void NotifyClose();                                       // 49/44 Mac 0x25430
    virtual bool NotifyInput17();                                     // 50/45 Mac 0x256d0
    virtual void NotifyPaint(UInt32 arg);                             // 51/46 Mac 0x25940
    virtual void NotifyInput19();                                     // 52/47 Mac 0x25bd0
    virtual void NotifyInput20(UInt32 arg);                           // 53/48 Mac 0x25e40
    virtual void NotifyInput21(UInt32 arg);                           // 54/49 Mac 0x260c0

    // --- full-screen display mode (signatures inferred) ---
    virtual bool CheckDisplayMode(WindowStyle& style);                   // 55/50 Mac 0x26340, Win 0x40e1a0
    virtual bool ChangeDisplayMode(const WindowStyle& style, const WindowStyle& previous); // 56/51 Mac 0x26450, Win 0x40e2b0
#if defined(_WIN32)
    // Window procedure body (WM_PAINT, WM_SETCURSOR, WM_COMMAND, menus …). Called by WindowProc
    // (Win 0x40f800) with the HWND first (signature for OpenGLWindow, R2.2).
    virtual LRESULT HandleMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // Win 52 0x40f930
#else
    virtual bool SwitchDisplayMode(CFDictionaryRef mode);             // Mac 57 0x26cd0
    virtual bool CaptureDisplays();                                   // Mac 58 0x26d20
    virtual bool ReleaseDisplays();                                   // Mac 59 0x26d50
#endif

    // (non-virtual)
    const Vector2& GetMousePosition() const { return m_mousePosition; }
    void SetModalUpdateReciever(ModalUpdateReciever* r) { m_modalUpdateReciever = r; }
    // Native window, read directly (+0xb0 / +0xb8) by Platform's dialogs and Application's
    // modal loops. (accessors, names inferred)
#if defined(_WIN32)
    HWND  GetHWND() const { return m_hwnd; }
#else
    void* GetNativeWindow() const { return m_nativeWindow; }
#endif
    // Counts modal states (Mac +0xac / Win +0xa4: +1 / -1); on 0 <-> 1 the close box (Mac) / the
    // system-menu close item and the menu bar (Win) are disabled / enabled again.
    // ControlManager::SetModalControl calls it. (name inferred)
    void SetModalState(bool modal);                                   // Mac 0x21de0, Win 0x40d0e0
    bool IsModal() const { return m_modalCount != 0; }                // (inlined reads of +0xac / +0xa4)
    // Mac 0x23210 (name inferred)
    void SetKeyboardFocusObserver(NativeControlKeyboardFocusObserver* observer)
    {
        m_keyboardFocusObserver = observer;
        m_keyboardFocusObserver->m_window = this;
    }
#if !defined(_WIN32)
    // Hands the native window the object that decides whether a click belongs to a native
    // (Cocoa) control: [LEGOMacWindow setHitTester:]. Called by ControlManager::Initialize
    // (Mac 0xd8640). (name inferred)
    void SetHitTester(HitTester* tester);                             // Mac 0x231e0
#endif

    // Installs the menu bar: keeps `menu` and builds the native menu from it (Win only when the
    // window was constructed with a menu bar; Mac replaces all but the application menu of [NSApp
    // mainMenu]). ProcessMessages handles its key short-cuts. Callers: LDDApplication, Mode,
    // LuaWindow. (name inferred)
    void SetMenu(const Menu* menu);                                   // Mac 0x26bb0, Win 0x40f4f0
    const Menu* GetMenu() const { return m_menu.Get(); }
    // Whether `key` (LEGO::Key) is held down right now (Win GetAsyncKeyState of the key's
    // virtual key; Mac [LEGOMacWindow IsKeyPressed:]). Callers: tools, LuaControlManager.
    // (name from the Mac selector)
    bool IsKeyPressed(UInt32 key) const;                              // Mac 0x265c0, Win 0x40eb50
    // Redraws `rect` (whole window when null). (name inferred)
    void Invalidate(const Rect* rect);                                // Mac 0x21d30 (Win: inlined InvalidateRect)
#if defined(_WIN32)
    typedef HCURSOR NativeCursor;
#else
    typedef void* NativeCursor;                                       // NSCursor*
#endif
    // Shows `cursor` (0: the arrow) while the mouse is over the client area. Caller: LDDCursor.
    // (name inferred)
    void SetCursor(NativeCursor cursor);                              // Mac 0x26c90, Win 0x40cdf0
#if !defined(_WIN32)
    // Union of all screens ([LEGOMacWindow GetVirtualDesktopRect]); start-up 0x139e20.
    static Rect GetVirtualDesktopRect();                              // Mac 0x27140
#endif

protected:
    // Native window helpers (added for OpenGLWindow, R2.2; names inferred).
#if defined(_WIN32)
    // Registers "DCLWindowClass" once, creates the HWND for m_style and sends NotifyInput17
    // (OpenGLWindow creates its GL context there); on failure the window is destroyed again.
    static bool CreateNativeWindow(Window* window);                   // Win 0x40e400
    void ShowNativeWindow();                                          // Win 0x40e750 (ESI = this)
    // Window procedure of "DCLWindowClass": WM_CREATE stores the Window* (GWLP_USERDATA =
    // GWL_USERDATA on Win32) and m_hwnd, every message goes to HandleMessage, WM_DESTROY clears both.
    static LRESULT CALLBACK WindowProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // Win 0x40f800

    // ---- HandleMessage helpers (/LTCG register conventions noted; names inferred) ----
    void OnNativeMove();                                              // Win 0x40c210 (ESI = hwnd, EDI = this)
    void OnNativeSize();                                              // Win 0x40c2c0 (ESI = hwnd, EDI = this)
    // Mouse button `buttonBit` (1 left, 2 right, 4 middle) went down: SetCapture on the first.
    void CaptureMouseButton(UInt32 buttonBit);                        // Win 0x40c1f0 (ESI = this, EDI = bit)
    // Current modifiers as KeyModifier bits (GetKeyState of Ctrl / Shift / Alt).
    static UInt32 GetKeyModifiers();                                  // Win 0x40ce90
    // Sends a key-down / key-up for `vk` when its asynchronous state differs from the
    // message-queue state (after the application was re-activated).
    static void SyncKeyState(HWND hwnd, WPARAM vk);                   // Win 0x40cec0 (EBX = hwnd, EDI = vk)
    // Keyboard messages -> HandleKey / HandleCharacter (called first for every message).
    void TranslateKeyMessage(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam); // Win 0x40cf10 (EAX/EDX/ECX, ESI = this)
    // Menu item state (MFS_*) of a command: grayed when disabled, checked when toggled.
    static UINT GetCommandMenuState(const Command* command);         // Win 0x40eb80 (ESI = command)
    // Mouse enter / leave tracking and the "over a child window" state (Update, ModalUpdate).
    void TrackMouse();                                                // Win 0x40d410 (ESI = this)
    // Opens a dropped / handed-over file (WM_DROPFILES, message 0x8000 from a second LDD
    // instance) through Application::OpenDocumentFile, or flashes the window when busy.
    void OpenFile(const UniChar16* nativePath);                       // Win 0x40f610
    // Adds the items of `menu` to `nativeMenu`; command ids count up from *nextId.
    static void BuildMenu(const Menu* menu, HMENU* nativeMenu, UINT* nextId); // Win 0x40ebc0
#else
    // Creates the LEGOMacWindow for m_style (re-using the content view of an old one) and sends
    // NotifyInput17. (name inferred)
    bool CreateNativeWindow();                                        // Mac 0x212c0
    // Adds the items of `menu` to the NSMenu `nativeMenu` (LEGOMenuItemWrapper items calling
    // their command for `window`).
    static void BuildMenu(const Menu* menu, void* nativeMenu, Window* window); // Mac 0x26600
#endif
    // Destroys the native window (re-created by the next Create) and notifies OnClose (Mac:
    // [[window contentView] setManager:nil], orderOut:, release; inlined in 0x212c0, 0x23a70).
    void DestroyNativeWindow();                                       // Mac 0x23c70, Win 0x40e720 (ESI = this)

    // Dispatches every queued Message to the observers' HumanInput slots; a key-down first
    // goes to the menu short-cuts. (name inferred)
    void ProcessMessages();                                           // Mac 0x21ea0 (4.7 KB), Win 0x4104a0
    // Runs the menu short-cut of a key-down unless the window is modal or an observer refuses
    // (WindowObserver slot 24). (name inferred)
    void HandleMenuShortcut(UInt32 key, UInt32 modifiers, bool repeat); // Mac 0x26d80, Win 0x410a00 (EDI = this)
    // Searches `menu` (recursively) for an enabled item with the short-cut and runs its command.
    static bool ExecuteMenuShortcut(const Menu* menu, UInt32 key, UInt32 modifiers, bool repeat); // Mac 0x27050, Win 0x410af0

    WindowStyle                      m_style;               // Mac +0x58, Win +0x64 (Win: icon +0x8c)
    Vector2                          m_mousePosition;       // Mac +0x8c, Win +0x90; (-30000,-30000) initially
    // The window sets its own cursor (Win WM_SETCURSOR); when false, TrackMouse reports
    // whether the mouse is over a child window (NotifyInput21). (name kept from R0.2)
    bool                             m_showCursor;          // Mac +0x94, Win +0x98; true initially
    bool                             m_hasMenuBar;          // Mac +0x95, Win +0x99 (ctor argument)
    RefPointer<const Menu>           m_menu;                // Mac +0x98, Win +0x9c
    String                           m_representedFile;     // Mac +0x9c, Win +0xa0
    UInt32                           m_modalCount;          // Mac +0xac, Win +0xa4 (SetModalState)
    ModalUpdateReciever*             m_modalUpdateReciever; // Mac +0xb0, Win +0xa8
    RefPointer<NativeControlKeyboardFocusObserver> m_keyboardFocusObserver; // Mac +0xb4, Win +0xac
#if defined(_WIN32)
    HWND                             m_hwnd;                // Win +0xb0
    int                              m_clientLeft;          // Win +0xb4 client origin on the screen (WM_MOVE)
    int                              m_clientTop;           // Win +0xb8
    HCURSOR                          m_cursor;              // Win +0xbc LoadCursor(IDC_ARROW)
    HBRUSH                           m_background;          // Win +0xc0 CreateSolidBrush(white)
    UInt32                           m_capturedButtons;     // Win +0xc4 (bits 1/2/4: SetCapture)
    bool                             m_destroying;          // Win +0xc8 (no NotifyClose from WM_DESTROY)
    bool                             m_mouseInside;         // Win +0xc9 (TrackMouse)
#else
    void*                            m_nativeWindow;        // Mac +0xb8 LEGOMacWindow* (NSWindow)
    bool                             m_flagBC;              // Mac +0xbc (never written in TU 020)
    CFDictionaryRef                  m_originalDisplayMode; // Mac +0xc0 CGDisplayCurrentMode at construction
    bool                             m_displaysCaptured;    // Mac +0xc4 (CaptureDisplays)
#endif
    BasicQueue<Message>              m_messages;            // Mac +0xc8 (0x2c), Win +0xcc

    friend class NativeControlKeyboardFocusObserver;

private:
    Window(const Window&);
    Window& operator=(const Window&);
};

} // namespace LEGO
