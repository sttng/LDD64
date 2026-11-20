// LEGO Digital Designer — restored source
// The Cocoa window shell of LEGO::Window on the Mac: TU 154 "MacWindow.mm" (file name inferred),
// Mac 0x216180-0x21fa4d; Mac only (the Win natives are in Window.cpp).
//
//   C++ (also on the Linux-hosted Mac branch, e.g. for an X11 shell):
//     FunctionKeyMapClass / EnglishCharMapClass / NumpadEnglishCharMapClass — character →
//     LEGO::Key maps of LEGOMacDrawingView keyDown:/keyUp: (BasicMap<unsigned short, Key>,
//     globals built by the TU's static initialiser Mac 0x21f9a0).
//     LEGOMenuWrapperObserver — CommandObserver of a LEGOMenuItemWrapper.
//   Objective-C (behind __OBJC__):
//     LEGOMacWindow (NSWindow, its own delegate) → Window move/resize/activate/close
//     LEGOMacContentView (NSView) = LEGOMacNativeControlsView (Cocoa controls) +
//       LEGOMacDrawingView (mouse / keyboard → the Window's HumanInput slots; holds
//       OpenGLWindow.mm's LEGOOpenGLView)
//     LEGOMenuItemWrapper (NSMenuItem) — a menu item running a LEGO::Command (Window::BuildMenu)
//
// Instance layouts from the Mac binary's Objective-C metadata (ivar offsets, i386): LEGOMacWindow 136
// (window +132); LEGOMacNativeControlsView 80 (no ivars); LEGOMacDrawingView 92 (window +80,
// oldModifierFlags +84, hitTester +88); LEGOMacContentView 92 (window +80, nativeView +84,
// drawingView +88); LEGOMenuItemWrapper 76 (command +64, window +68, menuWrapperObserver +72).
// Ivar names are original (ObjC metadata); method names are the selectors.
#pragma once
#if !defined(_WIN32)
#include "LEGO/Types.h"
#include "BasicMap.h"
#include "Key.h"
#include "Command.h"

namespace LEGO {

class Window;
class HitTester;

// ===========================================================================================
// Key maps. Mac vtables FunctionKeyMapClass 0x76fe5c {D1 0x216180, D0 0x21b260},
// EnglishCharMapClass 0x76fe94 {0x2161b0, 0x21d4d0}, NumpadEnglishCharMapClass 0x76feb4
// {0x2161e0, 0x21f670}; base BasicMap<unsigned short, Key, Less<unsigned short> > 0x76fe84
// {0x21b2b0, 0x21b2e0}. Constructors are straight-line inserts (operator[] = value).
// ===========================================================================================

// NS*FunctionKey characters (0xf700-0xf746) + Escape / Delete → keys; function-key characters
// without a key (F16-F35, Begin, Print, …) are inserted with kKeyUndefined.
class FunctionKeyMapClass : public BasicMap<unsigned short, Key, Less<unsigned short> > {
public:
    FunctionKeyMapClass();                                            // Mac 0x218870
    virtual ~FunctionKeyMapClass() {}                                 // Mac 0x216180/0x21b260
};

// Upper-case characters of the US keyboard (main block) → keys.
class EnglishCharMapClass : public BasicMap<unsigned short, Key, Less<unsigned short> > {
public:
    EnglishCharMapClass();                                            // Mac 0x21b380
    virtual ~EnglishCharMapClass() {}                                 // Mac 0x2161b0/0x21d4d0
};

// The same with the numeric-keypad keys for digits and operators (NSNumericPadKeyMask set).
class NumpadEnglishCharMapClass : public BasicMap<unsigned short, Key, Less<unsigned short> > {
public:
    NumpadEnglishCharMapClass();                                      // Mac 0x21d520
    virtual ~NumpadEnglishCharMapClass() {}                           // Mac 0x2161e0/0x21f670
};

// The TU's global map objects (static initialiser Mac 0x21f9a0). (names inferred)
extern FunctionKeyMapClass       g_functionKeyMap;                    // Mac 0x77dfac
extern EnglishCharMapClass       g_englishCharMap;                    // Mac 0x77dfc8
extern NumpadEnglishCharMapClass g_numpadEnglishCharMap;              // Mac 0x77dfe4

// NSEvent modifier flags (NSShiftKeyMask 1<<17, NSAlternateKeyMask 1<<19, NSCommandKeyMask
// 1<<20) → the KeyModifier bits of HumanInput: Command 1 (= Control on Win), Shift 2, Option 4.
// Body of -[LEGOMacDrawingView getModifiers:] (Mac 0x216dc0) and -[LEGOMacContentView
// getModifiers:] (Mac 0x218160). (name inferred)
inline UInt32 MacModifierFlagsToModifiers(UInt32 flags)
{
    return ((flags >> 17) & 4) | ((flags >> 16) & 2) | ((flags >> 20) & 1);
}

// The key of an upper-cased character typed on the main block or (numericPad) the keypad;
// kKeyUndefined (inserted, like the original's operator[]) when the map has none. (name inferred)
inline Key MacCharacterToKey(unsigned short upper, bool numericPad)
{
    return numericPad ? g_numpadEnglishCharMap[upper] : g_englishCharMap[upper];
}

// ===========================================================================================
// LEGOMenuWrapperObserver — Mac vtable 0x72cd2c {D1 0x218750, D0 0x218780, OnFinalRelease,
// OnAttached, OnDetached, OnEnableChanged (base), OnToggleChanged 0x2184c0}, Object-in
// 0x72cd60. Layout vptr, Command* +0x04, LEGOMenuItemWrapper* +0x08, Object +0x0c (operator
// new 0x14 in -[LEGOMenuItemWrapper init…], Mac 0x218500). Mac only.
// ===========================================================================================
class LEGOMenuWrapperObserver : public CommandObserver {
public:
    // `menuItem`: the LEGOMenuItemWrapper (id). (constructor inlined in Mac 0x218500)
    explicit LEGOMenuWrapperObserver(void* menuItem) : m_menuItem(menuItem) {}
    virtual ~LEGOMenuWrapperObserver() {}                             // Mac 0x218750/0x218780
    // [menuItem onCommandToggled:toggled] (the check mark of the item).
    virtual void OnToggleChanged(bool toggled);                       // Mac 0x2184c0
    void* GetMenuItem() const { return m_menuItem; }                  // (inline, name inferred)
private:
    void* m_menuItem;                                                 // +0x08 LEGOMenuItemWrapper*
};

} // namespace LEGO

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>

// Mac 0x216210-0x216a00. Its own delegate (Window::CreateNativeWindow, WindowMac.mm).
@interface LEGOMacWindow : NSWindow <NSWindowDelegate> {   // (10.6+ SDKs type the delegate)
    LEGO::Window* window;                       // +132
}
+ (NSRect)GetVirtualDesktopRect;
- (BOOL)canBecomeKeyWindow;
- (void)setLEGOWindow:(LEGO::Window*)aWindow;
- (LEGO::Window*)GetLEGOWindow;
- (BOOL)isModal;
- (BOOL)IsKeyPressed:(int)key;
- (void)setHitTester:(LEGO::HitTester*)tester;
- (void)windowDidMove:(NSNotification*)notification;
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize;
- (void)windowDidResize:(NSNotification*)notification;
- (BOOL)windowShouldClose:(id)sender;
- (void)windowWillClose:(NSNotification*)notification;
- (void)windowDidBecomeKey:(NSNotification*)notification;
- (void)windowDidResignKey:(NSNotification*)notification;
@end

// Mac 0x216a10-0x216c5f: parent of the native (Cocoa) controls, behind the drawing view.
@interface LEGOMacNativeControlsView : NSView
- (void)superMouseMoved:(NSEvent*)event;
@end

// Mac 0x216c60-0x217f2f: LDD's own drawing / input view.
@interface LEGOMacDrawingView : NSView {
    LEGO::Window*                    window;            // +80
    unsigned int                     oldModifierFlags;  // +84
    LEGO::RefPointer<LEGO::HitTester> hitTester;         // +88 (.cxx_construct / .cxx_destruct)
}
- (id)initWithFrame:(NSRect)frame Manager:(LEGO::Window*)aWindow;
- (void)setManager:(LEGO::Window*)aWindow;
- (unsigned long)getModifiers:(unsigned int)flags;
- (void)setHitTester:(LEGO::HitTester*)tester;
- (void)windowMouseMoved:(NSEvent*)event andViewLocation:(NSPoint)location;
@end

// Mac 0x217f30-0x21826f: the window's content view.
@interface LEGOMacContentView : NSView {
    LEGO::Window*              window;       // +80
    LEGOMacNativeControlsView* nativeView;   // +84
    LEGOMacDrawingView*        drawingView;  // +88
}
- (id)initWithFrame:(NSRect)frame Manager:(LEGO::Window*)aWindow;
- (void)setManager:(LEGO::Window*)aWindow;
- (unsigned long)getModifiers:(unsigned int)flags;
- (LEGOMacNativeControlsView*)getNativeView;
- (LEGOMacDrawingView*)getDrawingView;
- (void)toggleFirstResponder:(BOOL)toNativeView;
- (void)updateInitialFirstResponder;
@end

// Mac 0x218500-0x21874f
@interface LEGOMenuItemWrapper : NSMenuItem {
    LEGO::Command*                 command;              // +64
    LEGO::Window*                  window;               // +68
    LEGO::LEGOMenuWrapperObserver* menuWrapperObserver;  // +72
}
- (id)initWithTitle:(NSString*)title keyEquivalent:(NSString*)key
    KeyEquivalentModifierMask:(unsigned int)mask command:(LEGO::Command*)aCommand
    window:(LEGO::Window*)aWindow;
- (void)Clicked:(id)sender;
- (BOOL)validateMenuItem:(NSMenuItem*)item;
- (void)onCommandToggled:(bool)toggled;
- (LEGO::Command*)getCommand;
@end
#endif // __OBJC__
#endif // !_WIN32
