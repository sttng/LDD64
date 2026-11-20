// LEGO Digital Designer — restored source
// The Mac natives of LEGO::Window (TU 020, Mac 0x20470-0x2d5b9: the original's Objective-C part):
// LEGOMacWindow (TU 154 MacWindow.mm) with a LEGOMacContentView; menus = [NSApp mainMenu] with
// LEGOMenuItemWrapper items; full screen through CoreGraphics display capture / mode switching.
//
// LDD64 Linux (LDD_LINUX, not restored code, marked below): the Cocoa calls go to the GTK shell of
// Platform/Linux/LinuxShell.h (a GtkWindow with the menu bar and the GL view); no display modes.
// Other builds without Objective-C (the Linux-hosted Mac branch of the tests) keep HOST-ONLY
// stand-ins: no NSWindow (m_nativeWindow is a non-null placeholder while the window "exists").
#if !defined(_WIN32)
#include "Window.h"
#include "Command.h"
#include "Menu.h"
#include "BasicMap.h"
#include "Platform.h"
#include "Path.h"
#include "StringConversion.h"
#if defined(LDD_LINUX)
#include "LinuxShell.h"
#endif

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#import <ApplicationServices/ApplicationServices.h>

// TU 154 (MacWindow.mm): the selectors Window uses.
@interface LEGOMacWindow : NSWindow <NSWindowDelegate>   // (its own delegate: 10.6+ SDKs type it)
+ (NSRect)GetVirtualDesktopRect;
- (void)setLEGOWindow:(LEGO::Window*)window;
- (void)setHitTester:(LEGO::HitTester*)tester;
- (BOOL)IsKeyPressed:(unsigned int)key;
@end
@interface LEGOMacContentView : NSView
- (id)initWithFrame:(NSRect)frame Manager:(LEGO::Window*)window;
- (void)setManager:(LEGO::Window*)window;
- (void)updateInitialFirstResponder;
- (void)toggleFirstResponder:(id)sender;
@end
@interface LEGOMenuItemWrapper : NSMenuItem
- (id)initWithTitle:(NSString*)title keyEquivalent:(NSString*)key
    KeyEquivalentModifierMask:(unsigned int)mask command:(LEGO::Command*)command
    window:(LEGO::Window*)window;
@end

#if defined(__LP64__)
// PORT64: 64-bit builds use the CGDisplayMode API (10.6) instead of the deprecated mode
// dictionaries of CGDisplayCurrentMode / CGDisplayBestModeForParameters / CGDisplaySwitchToMode
// the 32-bit code calls. A mode still travels as a dictionary (Width, Height, BitsPerPixel,
// RefreshRate, IODisplayModeID), compared with isEqualToDictionary:, not owned by the caller
// (autoreleased). Every mode of a current Mac has 32 bits per pixel.
namespace {
CFDictionaryRef LEGODescribeDisplayMode(CGDisplayModeRef mode)
{
    if (mode == 0)
        return 0;
    return (CFDictionaryRef)[NSDictionary dictionaryWithObjectsAndKeys:
        [NSNumber numberWithUnsignedLongLong:CGDisplayModeGetWidth(mode)], @"Width",
        [NSNumber numberWithUnsignedLongLong:CGDisplayModeGetHeight(mode)], @"Height",
        [NSNumber numberWithInt:32], @"BitsPerPixel",
        [NSNumber numberWithDouble:CGDisplayModeGetRefreshRate(mode)], @"RefreshRate",
        [NSNumber numberWithInt:CGDisplayModeGetIODisplayModeID(mode)], @"IODisplayModeID",
        nil];
}

CFDictionaryRef LEGODisplayCurrentMode(CGDirectDisplayID display)
{
    CGDisplayModeRef mode = CGDisplayCopyDisplayMode(display);
    CFDictionaryRef description = LEGODescribeDisplayMode(mode);
    CGDisplayModeRelease(mode);
    return description;
}

// The mode of that size (preferring the current refresh rate); exact = 0 and the current mode
// when there is none, or when another depth than 32 bits is asked for.
CFDictionaryRef LEGODisplayBestModeForParameters(CGDirectDisplayID display, size_t bitsPerPixel,
                                                 size_t width, size_t height, boolean_t* exact)
{
    if (exact)
        *exact = 0;
    CGDisplayModeRef current = CGDisplayCopyDisplayMode(display);
    double rate = current ? CGDisplayModeGetRefreshRate(current) : 0.0;
    CFDictionaryRef result = 0;
    NSArray* modes = (NSArray*)CGDisplayCopyAllDisplayModes(display, 0);
    if (bitsPerPixel == 32 || bitsPerPixel == 0) {
        CGDisplayModeRef best = 0;
        for (NSUInteger i = 0; i < [modes count]; ++i) {
            CGDisplayModeRef mode = (CGDisplayModeRef)[modes objectAtIndex:i];
            if (CGDisplayModeGetWidth(mode) != width || CGDisplayModeGetHeight(mode) != height)
                continue;
            if (best == 0 || CGDisplayModeGetRefreshRate(mode) == rate)
                best = mode;
        }
        if (best != 0) {
            result = LEGODescribeDisplayMode(best);
            if (exact)
                *exact = 1;
        }
    }
    if (result == 0)
        result = LEGODescribeDisplayMode(current);
    [modes release];
    CGDisplayModeRelease(current);
    return result;
}

CGError LEGODisplaySwitchToMode(CGDirectDisplayID display, CFDictionaryRef description)
{
    int modeID = [[(NSDictionary*)description objectForKey:@"IODisplayModeID"] intValue];
    NSArray* modes = (NSArray*)CGDisplayCopyAllDisplayModes(display, 0);
    CGError error = kCGErrorFailure;
    for (NSUInteger i = 0; i < [modes count]; ++i) {
        CGDisplayModeRef mode = (CGDisplayModeRef)[modes objectAtIndex:i];
        if (CGDisplayModeGetIODisplayModeID(mode) == modeID) {
            error = CGDisplaySetDisplayMode(display, mode, 0);
            break;
        }
    }
    [modes release];
    return error;
}
} // namespace

#define CGDisplayCurrentMode LEGODisplayCurrentMode
#define CGDisplayBestModeForParameters LEGODisplayBestModeForParameters
#define CGDisplaySwitchToMode LEGODisplaySwitchToMode
#endif
#endif

namespace LEGO {

// ===========================================================================================
// Key → key-equivalent character maps (menu short-cuts): two global BasicMap<Key, UInt16> built
// by the TU's static initialiser (Mac 0x2d530): Mac 0x77d140 (unshifted) and 0x77d15c (with
// Shift, US layout). Function keys map to the NS*FunctionKey values. The original's constructors
// are straight-line inserts; the pairs are listed here.
namespace {
const UInt16 kKeyToUniChar[][2] = {
    {0x01, 0x001b}, {0x02, 0x0031}, {0x03, 0x0032}, {0x04, 0x0033}, {0x05, 0x0034},
    {0x06, 0x0035}, {0x07, 0x0036}, {0x08, 0x0037}, {0x09, 0x0038}, {0x0a, 0x0039},
    {0x0b, 0x0030}, {0x0c, 0x002d}, {0x0d, 0x003d}, {0x0e, 0xf73e}, {0x0f, 0x0009},
    {0x10, 0x0071}, {0x11, 0x0077}, {0x12, 0x0065}, {0x13, 0x0072}, {0x14, 0x0074},
    {0x15, 0x0079}, {0x16, 0x0075}, {0x17, 0x0069}, {0x18, 0x006f}, {0x19, 0x0070},
    {0x1a, 0x005b}, {0x1b, 0x005d}, {0x1c, 0x000d}, {0x1e, 0x0061}, {0x1f, 0x0073},
    {0x20, 0x0064}, {0x21, 0x0066}, {0x22, 0x0067}, {0x23, 0x0068}, {0x24, 0x006a},
    {0x25, 0x006b}, {0x26, 0x006c}, {0x27, 0x003b}, {0x28, 0x0027}, {0x29, 0x0060},
    {0x2b, 0x005c}, {0x2c, 0x007a}, {0x2d, 0x0078}, {0x2e, 0x0063}, {0x2f, 0x0076},
    {0x30, 0x0062}, {0x31, 0x006e}, {0x32, 0x006d}, {0x33, 0x002c}, {0x34, 0x002e},
    {0x35, 0x002f}, {0x37, 0x002a}, {0x39, 0x0020}, {0x3b, 0xf704}, {0x3c, 0xf705},
    {0x3d, 0xf706}, {0x3e, 0xf707}, {0x3f, 0xf708}, {0x40, 0xf709}, {0x41, 0xf70a},
    {0x42, 0xf70b}, {0x43, 0xf70c}, {0x44, 0xf70d}, {0x47, 0x0037}, {0x48, 0x0038},
    {0x49, 0x0039}, {0x4a, 0x002d}, {0x4b, 0x0034}, {0x4c, 0x0035}, {0x4d, 0x0036},
    {0x4e, 0x002b}, {0x4f, 0x0031}, {0x50, 0x0032}, {0x51, 0x0033}, {0x52, 0x0030},
    {0x53, 0x002e}, {0x57, 0xf70e}, {0x58, 0xf70f}, {0x64, 0xf710}, {0x65, 0xf711},
    {0x66, 0xf712}, {0x8d, 0x003d}, {0x90, 0x005e}, {0x91, 0x0040}, {0x92, 0x003a},
    {0x93, 0x005f}, {0x9c, 0x0003}, {0xb3, 0x002c}, {0xb5, 0x002f}, {0xc7, 0xf729},
    {0xc8, 0xf700}, {0xc9, 0xf72c}, {0xcb, 0xf702}, {0xcd, 0xf703}, {0xcf, 0xf72b},
    {0xd0, 0xf701}, {0xd1, 0xf72d}, {0xd2, 0xf727}, {0xd3, 0xf728},
};
const UInt16 kKeyAndShiftToUniChar[][2] = {
    {0x02, 0x0021}, {0x03, 0x0040}, {0x04, 0x0023}, {0x05, 0x0024}, {0x06, 0x0025},
    {0x07, 0x005e}, {0x08, 0x0026}, {0x09, 0x002a}, {0x0a, 0x0028}, {0x0b, 0x0029},
    {0x10, 0x0051}, {0x11, 0x0057}, {0x12, 0x0045}, {0x13, 0x0052}, {0x14, 0x0054},
    {0x15, 0x0059}, {0x16, 0x0055}, {0x17, 0x0049}, {0x18, 0x004f}, {0x19, 0x0050},
    {0x1a, 0x007b}, {0x1b, 0x007d}, {0x1e, 0x0041}, {0x1f, 0x0053}, {0x20, 0x0044},
    {0x21, 0x0046}, {0x22, 0x0047}, {0x23, 0x0048}, {0x24, 0x004a}, {0x25, 0x004b},
    {0x26, 0x004c}, {0x27, 0x003a}, {0x28, 0x0022}, {0x29, 0x007e}, {0x2b, 0x007c},
    {0x2c, 0x005a}, {0x2d, 0x0058}, {0x2e, 0x0043}, {0x2f, 0x0056}, {0x30, 0x0042},
    {0x31, 0x004e}, {0x32, 0x004d}, {0x33, 0x003c}, {0x34, 0x003e}, {0x35, 0x003f},
};
} // namespace

// Mac vtable 0x76020c {D1 0x26560, D0 0x2aff0}
class KeyToUniCharMapClass : public BasicMap<Key, UInt16, Less<Key> > {
public:
    KeyToUniCharMapClass()                                            // Mac 0x27cc0
    {
        for (UInt32 i = 0; i < sizeof(kKeyToUniChar) / sizeof(kKeyToUniChar[0]); ++i)
            (*this)[(Key)kKeyToUniChar[i][0]] = kKeyToUniChar[i][1];
    }
    virtual ~KeyToUniCharMapClass() {}                                // Mac 0x26560/0x2aff0
};

// Mac vtable 0x760244 {D1 0x26590, D0 0x2ca10}
class KeyAndShiftToUniCharMapClass : public BasicMap<Key, UInt16, Less<Key> > {
public:
    KeyAndShiftToUniCharMapClass()                                    // Mac 0x2b3d0
    {
        for (UInt32 i = 0; i < sizeof(kKeyAndShiftToUniChar) / sizeof(kKeyAndShiftToUniChar[0]); ++i)
            (*this)[(Key)kKeyAndShiftToUniChar[i][0]] = kKeyAndShiftToUniChar[i][1];
    }
    virtual ~KeyAndShiftToUniCharMapClass() {}                        // Mac 0x26590/0x2ca10
};

namespace {
KeyToUniCharMapClass         s_keyToUniChar;           // Mac 0x77d140
KeyAndShiftToUniCharMapClass s_keyAndShiftToUniChar;   // Mac 0x77d15c

#if !defined(__OBJC__)
// HOST-ONLY: marks "the native window exists" without Cocoa.
char s_hostNativeWindow;
#endif
} // namespace

// ===========================================================================================
// NativeControlKeyboardFocusObserver
// ===========================================================================================

// Mac 0x20490
void NativeControlKeyboardFocusObserver::ToggleFirstResponder()
{
    if (!m_window)
        return;
#if defined(__OBJC__)
    [[(NSWindow*)m_window->m_nativeWindow contentView] toggleFirstResponder:nil];
#endif
}

// ===========================================================================================
// Construction
// ===========================================================================================

// Mac 0x205c0 (C2 with VTT; the C1 copy is inlined in OpenGLWindow)
Window::Window(bool flag95)
    : m_mousePosition(-30000.0f, -30000.0f),
      m_showCursor(true),
      m_hasMenuBar(flag95),
      m_menu(0),
      m_modalCount(0),
      m_modalUpdateReciever(0),
      m_keyboardFocusObserver(0),
      m_nativeWindow(0),
      m_flagBC(false),
      m_originalDisplayMode(0),
      m_displaysCaptured(false)
{
#if defined(__OBJC__)
    m_originalDisplayMode = CGDisplayCurrentMode(CGMainDisplayID());
#if defined(__LP64__)
    // PORT64: the 64-bit description is autoreleased; the window keeps it (as long as the
    // original keeps the dictionary CoreGraphics returns)
    if (m_originalDisplayMode)
        CFRetain(m_originalDisplayMode);
#endif
#endif
}

// Mac 0x234f0 (D1) / 0x23780 (D0), base body 0x23270
Window::~Window()
{
}

// Mac 0x23a70 (thunks 0x23d60, 0x23d80, 0x23da0, 0x23dc0): closes the window, restores the
// display mode of the start, releases captured displays, then the three base bodies.
void Window::OnFinalRelease()
{
    if (m_nativeWindow)
        DestroyNativeWindow();
#if defined(__OBJC__)
    if (![(NSDictionary*)CGDisplayCurrentMode(CGMainDisplayID())
            isEqualToDictionary:(NSDictionary*)m_originalDisplayMode]) {
        if (!SwitchDisplayMode(m_originalDisplayMode)) {
            // NOTE: the message String is built and discarded (an assertion of a debug build).
            String message("Couldn't change display mode");
        }
    }
#endif
    if (!ReleaseDisplays()) {
        String message("Couldn't release the display(s)!");   // NOTE: discarded as above
    }
    NotifierT<Window, WindowObserver>::OnFinalRelease();
    UpdateObserver::OnFinalRelease();
    NotifierT<Updater, UpdateObserver>::OnFinalRelease();
}

// Mac 0x23de0 (thunks 0x23e10, 0x23e40)
void Window::Update()
{
    ProcessMessages();
    Updater::Update();
}

// Mac 0x21e70 (thunk 0x231b0)
void Window::ModalUpdate()
{
    ProcessMessages();
    if (m_modalUpdateReciever)
        m_modalUpdateReciever->ModalUpdate();
}

// ===========================================================================================
// Native window
// ===========================================================================================

// Mac 0x20ba0
bool Window::Create(const WindowStyle& style)
{
    WindowStyle previous = m_style;
    WindowStyle wanted = style;
    if (!CheckDisplayMode(wanted))
        return false;
    if (!ChangeDisplayMode(wanted, previous))
        return false;
    m_style = wanted;
    if (!m_nativeWindow) {
        if (!CreateNativeWindow())
            return false;
#if defined(__OBJC__)
        if (!m_style.hidden)
            [(NSWindow*)m_nativeWindow makeKeyAndOrderFront:nil];
#elif defined(LDD_LINUX)
        if (!m_style.hidden)
            LinuxShell::SetVisible(m_nativeWindow, true);
#endif
    } else {
        if (!SetTitle(wanted.title))
            return false;
        bool recreate = m_style.flag28 != previous.flag28 || m_style.flag29 != previous.flag29 ||
                        m_style.flag2a != previous.flag2a ||
                        (m_style.flag28 && m_style.size != previous.size);
        if (recreate) {
            if (!CreateNativeWindow())
                return false;
#if defined(__OBJC__)
            if (!m_style.hidden)
                [(NSWindow*)m_nativeWindow makeKeyAndOrderFront:nil];
#elif defined(LDD_LINUX)
            if (!m_style.hidden)
                LinuxShell::SetVisible(m_nativeWindow, true);
#endif
        } else if (!m_style.flag28) {
#if defined(__OBJC__)
            NSWindow* window = (NSWindow*)m_nativeWindow;
            if (m_style.minimumSize.x > 0.0f || m_style.minimumSize.y > 0.0f)
                [window setMinSize:NSMakeSize(m_style.minimumSize.x, m_style.minimumSize.y)];
            NSScreen* screen = [window screen];
            NSRect visible = screen ? [screen visibleFrame] : NSZeroRect;
            // NOTE: the height of the visible frame, not its top, flips the y coordinate.
            NSRect frame = NSMakeRect(m_style.position.x,
                                      visible.size.height - m_style.position.y - m_style.size.y,
                                      m_style.size.x, m_style.size.y);
            [window setFrame:frame display:YES];
#elif defined(LDD_LINUX)
            LinuxShell::ApplyStyle(m_nativeWindow, m_style);
#endif
        }
#if defined(__OBJC__)
        if (m_style.hidden != previous.hidden) {
            if (m_style.hidden)
                [(NSWindow*)m_nativeWindow orderOut:nil];
            else
                [(NSWindow*)m_nativeWindow makeKeyAndOrderFront:nil];
        }
#elif defined(LDD_LINUX)
        if (m_style.hidden != previous.hidden)
            LinuxShell::SetVisible(m_nativeWindow, !m_style.hidden);
#endif
    }
    RefPointer<Updater> updater = MainUpdater::GetInstance();
    UpdateObserver::Attach(updater.Get());
    return true;
}

// Mac 0x212c0
bool Window::CreateNativeWindow()
{
#if defined(__OBJC__)
    unsigned int styleMask = 0;
    if (m_style.flag29)       // titled | closable | miniaturizable (+ resizable)
        styleMask = (m_style.flag2a ? 8 : 0) + 7;
    NSView* content = nil;
    if (m_nativeWindow) {
        // keep the content view (with its native controls) for the new window
        NSWindow* old = (NSWindow*)m_nativeWindow;
        content = [[old contentView] retain];
        [content removeFromSuperviewWithoutNeedingDisplay];
        [old orderOut:nil];
        [old setDelegate:nil];                                    // (M19, see below)
        [old release];
        m_nativeWindow = 0;
    }
    NSRect rect = [NSWindow contentRectForFrameRect:NSMakeRect(0, 0, m_style.size.x, m_style.size.y)
                                          styleMask:styleMask];
    LEGOMacWindow* window = [[LEGOMacWindow alloc] initWithContentRect:rect styleMask:styleMask
                                                               backing:NSBackingStoreBuffered
                                                                 defer:NO];
    m_nativeWindow = window;
    if (!window)
        return false;
    [window setLEGOWindow:this];
    [window useOptimizedDrawing:YES];
    [window setDelegate:window];
    // NOTE: the original lets AppKit release the window when it is closed (close button,
    // Command-W) although m_nativeWindow keeps the alloc reference that DestroyNativeWindow
    // releases: closing frees the window and DestroyNativeWindow then messages and releases
    // it a second time. fixed: one owner, the LEGO Window (not released when closed; the
    // delegate is cleared before the release) (M19).
    [window setReleasedWhenClosed:NO];
    if (!content)
        content = [[LEGOMacContentView alloc] initWithFrame:rect Manager:this];
    else
        [content setFrame:rect];
    [window setContentView:content];
    [(LEGOMacContentView*)content updateInitialFirstResponder];
    [content release];
    SetTitle(m_style.title);
    [window setBackgroundColor:[NSColor blackColor]];
    if (m_style.flag28) {
        [window setLevel:CGShieldingWindowLevel()];
    } else {
        if (m_style.minimumSize.x > 0.0f || m_style.minimumSize.y > 0.0f)
            [window setMinSize:NSMakeSize(m_style.minimumSize.x, m_style.minimumSize.y)];
        // the screen containing the window position (default: the window's screen)
        NSScreen* screen = [window screen];
        for (NSUInteger i = 0; i < [[NSScreen screens] count]; ++i) {
            NSScreen* candidate = [[NSScreen screens] objectAtIndex:i];
            NSRect frame = candidate ? [candidate frame] : NSZeroRect;
            if (NSPointInRect(NSMakePoint(m_style.position.x, m_style.position.y), frame))
                screen = [[NSScreen screens] objectAtIndex:i];
        }
        NSRect frame = screen ? [screen frame] : NSZeroRect;
        NSRect fullFrame = screen ? [screen frame] : NSZeroRect;
        NSRect visible = screen ? [screen visibleFrame] : NSZeroRect;
        if (NSEqualRects(fullFrame, visible))
            frame = [LEGOMacWindow GetVirtualDesktopRect];
        [window setFrameTopLeftPoint:NSMakePoint(m_style.position.x,
                                                 frame.size.height + frame.origin.y - m_style.position.y)];
        [window setShowsResizeIndicator:NO];
    }
    if (NotifyInput17())
        return true;
    if (m_nativeWindow)
        DestroyNativeWindow();
    return false;
#else
#if defined(LDD_LINUX)
    // Linux: a new GtkWindow (the old one goes, with its GL view); NotifyInput17 lets an
    // OpenGLWindow create its view
    if (m_nativeWindow) {
        LinuxShell::DestroyWindow(m_nativeWindow);
        m_nativeWindow = 0;
    }
    m_nativeWindow = LinuxShell::CreateWindow(this, m_style, m_hasMenuBar);
    if (m_menu.Get())
        LinuxShell::SetMenu(m_nativeWindow, m_menu.Get(), this);
#else
    // HOST-ONLY: no NSWindow; NotifyInput17 lets an OpenGLWindow create its GLX stand-in.
    m_nativeWindow = &s_hostNativeWindow;
#endif
    if (NotifyInput17())
        return true;
    DestroyNativeWindow();
    return false;
#endif
}

// Mac 0x23c70
void Window::DestroyNativeWindow()
{
#if defined(__OBJC__)
    NSWindow* window = (NSWindow*)m_nativeWindow;
    [(LEGOMacContentView*)[window contentView] setManager:0];
    [window orderOut:nil];
    [window setDelegate:nil];                                     // (M19)
    [window release];
#elif defined(LDD_LINUX)
    LinuxShell::DestroyWindow(m_nativeWindow);
#endif
    m_nativeWindow = 0;
    NotifyClose();
}

// Mac 0x218b0
bool Window::SetTitle(const String& title)
{
    m_style.title = title;
    if (!m_nativeWindow)
        return false;
#if defined(__OBJC__)
    [(NSWindow*)m_nativeWindow setTitle:(NSString*)title.GetCFString()];
#elif defined(LDD_LINUX)
    LinuxShell::SetTitle(m_nativeWindow, title);
#endif
    return true;
}

// Mac 0x21910: an existing file gets the proxy icon and its name as the title; otherwise only
// the last path component. The title the window then shows is copied back into the style.
bool Window::SetRepresentedFile(const Path& path)
{
    if (!m_nativeWindow)
        return false;
    m_representedFile = path.GetName();
    if (!path.GetParentDirectory()) {
#if defined(__OBJC__)
        [(NSWindow*)m_nativeWindow setRepresentedFilename:[NSString string]];
#endif
        if (m_representedFile.IsEmpty()) {
            // NOTE: clears the remembered title only; the window keeps showing the old one.
            m_style.title.Clear();
            return true;
        }
        SetTitle(String(path.GetName()));
        return true;
    }
    RefPointer<Directory> directory(path.GetParentDirectory());
    String native = Platform::GetInstance()->VFSToNativePath(path.GetPath(true));
#if defined(__OBJC__)
    CFStringRef file = CreateCFSubstring(native, 0, String::npos);
    NSWindow* window = (NSWindow*)m_nativeWindow;
    if (path.Exists()) {
        [window setTitleWithRepresentedFilename:(NSString*)file];
    } else {
        [window setRepresentedFilename:@""];
        [window setTitle:[(NSString*)file lastPathComponent]];
    }
    CFRelease(file);
    m_style.title = String((CFStringRef)[window title]);
#else
    // Linux / HOST-ONLY: the title is the file name.
    (void)native;
    m_style.title = path.GetName();
#if defined(LDD_LINUX)
    LinuxShell::SetTitle(m_nativeWindow, m_style.title);
#endif
#endif
    return true;
}

// Mac 0x21be0
void Window::SetModified(bool modified)
{
#if defined(__OBJC__)
    [(NSWindow*)m_nativeWindow setDocumentEdited:modified];
#elif defined(LDD_LINUX)
    if (m_nativeWindow)
        LinuxShell::SetModified(m_nativeWindow, modified);
#else
    (void)modified;
#endif
}

// Mac 0x21c20
void Window::SetHidden(bool hidden)
{
    if (hidden == m_style.hidden)
        return;
    m_style.hidden = hidden;
#if defined(__OBJC__)
    if (hidden)
        [(NSWindow*)m_nativeWindow orderOut:nil];
    else
        [(NSWindow*)m_nativeWindow makeKeyAndOrderFront:nil];
#elif defined(LDD_LINUX)
    if (m_nativeWindow)
        LinuxShell::SetVisible(m_nativeWindow, !hidden);
#endif
}

// Mac 0x21ca0: the content view size (0 without a window).
Vector2 Window::GetSize() const
{
#if defined(__OBJC__)
    NSView* content = [(NSWindow*)m_nativeWindow contentView];
    if (!content)
        return Vector2(0.0f, 0.0f);
    NSRect frame = [content frame];
    return Vector2(frame.size.width, frame.size.height);
#elif defined(LDD_LINUX)
    if (!m_nativeWindow)
        return Vector2(0.0f, 0.0f);
    return LinuxShell::GetClientSize(m_nativeWindow);
#else
    // HOST-ONLY: the GLX stand-in has the size of the style.
    if (!m_nativeWindow)
        return Vector2(0.0f, 0.0f);
    return m_style.size;
#endif
}

#if defined(__OBJC__) && defined(__LP64__)
namespace {
// PORT64: with the 10.14+ SDKs every view is layer-backed and marking the content view redraws
// only its own layer, not the subviews in front; the 32-bit window redraws them too, and that
// redraw paints LDD (the GL view's OnPaint). So the whole subview tree is marked.
void LEGOSetNeedsDisplayOfSubviews(NSView* view)
{
    NSArray* subviews = [view subviews];
    for (NSUInteger i = 0; i < [subviews count]; ++i) {
        NSView* subview = (NSView*)[subviews objectAtIndex:i];
        [subview setNeedsDisplay:YES];
        LEGOSetNeedsDisplayOfSubviews(subview);
    }
}
} // namespace
#endif

// Mac 0x21d30
void Window::Invalidate(const Rect* rect)
{
#if defined(__OBJC__)
    NSView* content = [(NSWindow*)m_nativeWindow contentView];
    if (!rect)
        [content setNeedsDisplay:YES];
    else
        [content setNeedsDisplayInRect:NSMakeRect(rect->m_min.x, rect->m_min.y, rect->GetWidth(),
                                                  rect->GetHeight())];
#if defined(__LP64__)
    LEGOSetNeedsDisplayOfSubviews(content);
#endif
#else
    (void)rect;
#if defined(LDD_LINUX)
    if (m_nativeWindow)
        LinuxShell::Invalidate(m_nativeWindow);
#endif
#endif
}

// Mac 0x21de0
void Window::SetModalState(bool modal)
{
    UInt32 before = m_modalCount;
    m_modalCount = before + (modal ? 1 : 0) * 2 - 1;
    if ((before != 0) == (m_modalCount != 0))
        return;
#if defined(__OBJC__)
    NSWindow* window = (NSWindow*)m_nativeWindow;
    NSButton* close = [window standardWindowButton:NSWindowCloseButton];
    if (close)
        [close setEnabled:m_modalCount == 0];
    [window deminiaturize:window];
#elif defined(LDD_LINUX)
    if (m_nativeWindow)
        LinuxShell::SetModal(m_nativeWindow, m_modalCount != 0);
#endif
}

// Mac 0x231e0
void Window::SetHitTester(HitTester* tester)
{
#if defined(__OBJC__)
    [(LEGOMacWindow*)m_nativeWindow setHitTester:tester];
#else
    (void)tester;
#endif
}

// Mac 0x265c0
bool Window::IsKeyPressed(UInt32 key) const
{
#if defined(__OBJC__)
    return [(LEGOMacWindow*)m_nativeWindow IsKeyPressed:key] != NO;
#elif defined(LDD_LINUX)
    return LinuxShell::IsKeyPressed(key);   // as on Windows (GetAsyncKeyState)
#else
    (void)key;
    return false;   // HOST-ONLY
#endif
}

// Mac 0x26c90 (no use of `this`)
void Window::SetCursor(NativeCursor cursor)
{
#if defined(__OBJC__)
    if (!cursor)
        cursor = [NSCursor arrowCursor];
    [(NSCursor*)cursor set];
#elif defined(LDD_LINUX)
    LinuxShell::SetCursor(cursor);
#else
    (void)cursor;
#endif
}

// Mac 0x27140
Rect Window::GetVirtualDesktopRect()
{
#if defined(__OBJC__)
    NSRect r = [LEGOMacWindow GetVirtualDesktopRect];
    return Rect(r.origin.x, r.origin.y, r.size.width, r.size.height);
#elif defined(LDD_LINUX)
    return LinuxShell::GetVirtualDesktopRect();
#else
    return Rect(0.0f, 0.0f, 0.0f, 0.0f);   // HOST-ONLY
#endif
}

// ===========================================================================================
// Menus
// ===========================================================================================

// Mac 0x26600
// Submenu items get an NSMenu of their own; an item without command or title becomes a
// separator; command items become LEGOMenuItemWrappers with the key equivalent of their
// short-cut (Shift short-cuts use the shifted character when the US layout has one).
void Window::BuildMenu(const Menu* menu, void* nativeMenu, Window* window)
{
#if defined(__OBJC__)
    NSMenu* target = (NSMenu*)nativeMenu;
    const BasicArray<RefPointer<MenuItem> >& items = menu->GetItems();
    for (UInt32 i = 0; i < items.GetSize(); ++i) {
        const MenuItem* item = items[i].Get();
        if (item->GetSubMenu() && !item->GetTitle().IsEmpty()) {
            NSMenuItem* entry = [[NSMenuItem alloc] initWithTitle:(NSString*)item->GetTitle().GetCFString()
                                                           action:0
                                                    keyEquivalent:@""];
            NSMenu* submenu = [[NSMenu alloc] initWithTitle:(NSString*)item->GetTitle().GetCFString()];
            BuildMenu(item->GetSubMenu(), submenu, window);
            [entry setSubmenu:submenu];
            [target addItem:entry];
            [submenu release];
            [entry release];
            continue;
        }
        if (!item->GetCommand() || item->GetTitle().IsEmpty()) {
            [target addItem:[NSMenuItem separatorItem]];
            continue;
        }
        Key key = (Key)item->GetKey();
        UInt32 modifiers = item->GetModifiers();
        // NOTE: without a character for the key the original passes an uninitialised mask.
        unsigned int mask = 0;
        NSString* keyEquivalent = nil;
        UInt16 c = 0;
        if (!(modifiers & kModifierShift)) {
            if (s_keyToUniChar.Contains(key)) {
                mask = ((modifiers & kModifierControl) << 20) | ((modifiers & kModifierAlt) << 17);
                c = s_keyToUniChar[key];
            }
        } else if (s_keyAndShiftToUniChar.Contains(key)) {
            mask = ((modifiers & kModifierControl) << 20) | ((modifiers & kModifierAlt) << 17);
            c = s_keyAndShiftToUniChar[key];
        } else if (s_keyToUniChar.Contains(key)) {
            mask = ((modifiers & kModifierControl) << 20) + 0x20000 + ((modifiers & kModifierAlt) << 17);
            c = s_keyToUniChar[key];
        }
        if (c)
            keyEquivalent = [NSString stringWithCharacters:&c length:1];
        else
            keyEquivalent = [NSString string];
        LEGOMenuItemWrapper* entry =
            [[LEGOMenuItemWrapper alloc] initWithTitle:(NSString*)item->GetTitle().GetCFString()
                                         keyEquivalent:keyEquivalent
                             KeyEquivalentModifierMask:mask
                                               command:item->GetCommand()
                                                window:window];
        [target addItem:entry];
        [entry release];
    }
#else
    (void)menu;
    (void)nativeMenu;
    (void)window;
#endif
}

// Mac 0x26bb0: keeps the application menu (item 0) of the main menu.
void Window::SetMenu(const Menu* menu)
{
    m_menu = menu;
#if defined(__OBJC__)
    NSMenu* mainMenu = [NSApp mainMenu];
    while ([mainMenu numberOfItems] > 1)
        [mainMenu removeItemAtIndex:[mainMenu numberOfItems] - 1];
    BuildMenu(menu, mainMenu, this);
#elif defined(LDD_LINUX)
    if (m_nativeWindow)
        LinuxShell::SetMenu(m_nativeWindow, menu, this);
#endif
}

// ===========================================================================================
// Display modes
// ===========================================================================================

// Mac 0x26340
bool Window::CheckDisplayMode(WindowStyle& style)
{
    if (style.size.x < 0.0f || style.size.y < 0.0f || style.minimumSize.x < 0.0f ||
        style.minimumSize.y < 0.0f)
        return false;
    if (style.size.x <= style.minimumSize.x)
        style.size.x = style.minimumSize.x;
    if (style.size.y <= style.minimumSize.y)
        style.size.y = style.minimumSize.y;
    if (!style.flag28) {
        style.value2c = 0;
        if (!style.flag29)
            style.flag2a = false;
    } else {
        style.position = Vector2(0.0f, 0.0f);
        style.flag29 = false;
        style.flag2a = false;
        if (style.value2c != 0)
            return true;
    }
#if defined(__OBJC__)
    NSDictionary* mode = (NSDictionary*)CGDisplayCurrentMode(CGMainDisplayID());
    style.value2c = [[mode objectForKey:@"BitsPerPixel"] intValue];
#else
    style.value2c = 32;   // HOST-ONLY
#endif
    return true;
}

// Mac 0x26450
bool Window::ChangeDisplayMode(const WindowStyle& style, const WindowStyle& /*previous*/)
{
#if defined(__OBJC__)
    NSDictionary* current = (NSDictionary*)CGDisplayCurrentMode(CGMainDisplayID());
    if (!style.flag28) {
        if (![current isEqualToDictionary:(NSDictionary*)m_originalDisplayMode] &&
            !SwitchDisplayMode(m_originalDisplayMode))
            return false;
        return ReleaseDisplays();
    }
    boolean_t exact = 0;
    CFDictionaryRef best = CGDisplayBestModeForParameters(CGMainDisplayID(), style.value2c,
                                                          (size_t)style.size.x, (size_t)style.size.y,
                                                          &exact);
    if (!exact)
        return false;
    CaptureDisplays();
    if ([current isEqualToDictionary:(NSDictionary*)best])
        return true;
    return SwitchDisplayMode(best);
#else
    // HOST-ONLY: no display modes; leaving full screen releases the (fake) capture.
    if (!style.flag28)
        return ReleaseDisplays();
    return CaptureDisplays();
#endif
}

// Mac 0x26cd0
bool Window::SwitchDisplayMode(CFDictionaryRef mode)
{
#if defined(__OBJC__)
    if (!m_displaysCaptured) {
        if (CGCaptureAllDisplays() != kCGErrorSuccess)
            return false;
        m_displaysCaptured = true;
    }
    return CGDisplaySwitchToMode(CGMainDisplayID(), mode) == kCGErrorSuccess;
#else
    (void)mode;
    m_displaysCaptured = true;   // HOST-ONLY
    return true;
#endif
}

// Mac 0x26d20
bool Window::CaptureDisplays()
{
    if (m_displaysCaptured)
        return true;
#if defined(__OBJC__)
    if (CGCaptureAllDisplays() != kCGErrorSuccess)
        return false;
#endif
    m_displaysCaptured = true;
    return true;
}

// Mac 0x26d50
bool Window::ReleaseDisplays()
{
    if (!m_displaysCaptured)
        return true;
#if defined(__OBJC__)
    if (CGReleaseAllDisplays() != kCGErrorSuccess)
        return false;
#endif
    m_displaysCaptured = false;
    return true;
}

} // namespace LEGO
#endif // !_WIN32
