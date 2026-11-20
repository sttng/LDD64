// LEGO Digital Designer — restored source
// The Cocoa window shell of LEGO::Window: TU 154 "MacWindow.mm" (Mac 0x216180-0x21fa4d; file name
// inferred). Mac only; without Objective-C (Linux-hosted Mac branch) only the C++
// part (key maps, LEGOMenuWrapperObserver) is compiled.
#if !defined(_WIN32)
#include "MacWindow.h"
#include "Window.h"
#include "ControlManager.h"   // HitTester
#include "Geometry.h"

namespace LEGO {

// ===========================================================================================
// Key maps (constructors: straight-line `(*this)[c] = key;`, listed here in insertion order)
// ===========================================================================================
namespace {
const unsigned short kFunctionKeyMap[][2] = {   // 73 entries, insertion order
    {0xf700, 0xc8}, {0xf701, 0xd0}, {0xf702, 0xcb}, {0xf703, 0xcd}, {0xf704, 0x3b},
    {0xf705, 0x3c}, {0xf706, 0x3d}, {0xf707, 0x3e}, {0xf708, 0x3f}, {0xf709, 0x40},
    {0xf70a, 0x41}, {0xf70b, 0x42}, {0xf70c, 0x43}, {0xf70d, 0x44}, {0xf70e, 0x57},
    {0xf70f, 0x58}, {0xf710, 0x64}, {0xf711, 0x65}, {0xf712, 0x66}, {0xf713, 0x00},
    {0xf714, 0x00}, {0xf715, 0x00}, {0xf716, 0x00}, {0xf717, 0x00}, {0xf718, 0x00},
    {0xf719, 0x00}, {0xf71a, 0x00}, {0xf71b, 0x00}, {0xf71c, 0x00}, {0xf71d, 0x00},
    {0xf71e, 0x00}, {0xf71f, 0x00}, {0xf720, 0x00}, {0xf721, 0x00}, {0xf722, 0x00},
    {0xf723, 0x00}, {0xf724, 0x00}, {0xf725, 0x00}, {0xf726, 0x00}, {0xf727, 0xd2},
    {0xf728, 0xd3}, {0xf729, 0xc7}, {0xf72a, 0x00}, {0xf72b, 0xcf}, {0xf72c, 0xc9},
    {0xf72d, 0xd1}, {0xf72e, 0x00}, {0xf72f, 0x46}, {0xf730, 0x00}, {0xf731, 0xb7},
    {0xf732, 0x00}, {0xf733, 0x00}, {0xf734, 0x95}, {0xf735, 0x00}, {0xf736, 0x00},
    {0xf737, 0x00}, {0xf738, 0x00}, {0xf739, 0x00}, {0xf73a, 0x00}, {0xf73b, 0x00},
    {0xf73c, 0x00}, {0xf73d, 0x00}, {0xf73e, 0x00}, {0xf73f, 0x00}, {0xf740, 0x00},
    {0xf741, 0x00}, {0xf742, 0x00}, {0xf743, 0x00}, {0xf744, 0x00}, {0xf745, 0x00},
    {0xf746, 0x00}, {0x001b, 0x01}, {0x007f, 0x0e},
};
const unsigned short kEnglishCharMap[][2] = {   // 58 entries, insertion order
    {0xf73e, 0x0e}, {0x0009, 0x0f}, {0x002d, 0x0c}, {0x002b, 0x4e}, {0x003d, 0x0d},
    {0x005b, 0x1a}, {0x005d, 0x1b}, {0x000d, 0x1c}, {0x0003, 0x9c}, {0x003b, 0x27},
    {0x0027, 0x28}, {0x0060, 0x29}, {0x005c, 0x2b}, {0x002c, 0x33}, {0x002e, 0x34},
    {0x002f, 0x35}, {0x002a, 0x37}, {0x0020, 0x39}, {0x005e, 0x90}, {0x0040, 0x91},
    {0x003a, 0x92}, {0x005f, 0x93}, {0x0030, 0x0b}, {0x0031, 0x02}, {0x0032, 0x03},
    {0x0033, 0x04}, {0x0034, 0x05}, {0x0035, 0x06}, {0x0036, 0x07}, {0x0037, 0x08},
    {0x0038, 0x09}, {0x0039, 0x0a}, {0x0041, 0x1e}, {0x0042, 0x30}, {0x0043, 0x2e},
    {0x0044, 0x20}, {0x0045, 0x12}, {0x0046, 0x21}, {0x0047, 0x22}, {0x0048, 0x23},
    {0x0049, 0x17}, {0x004a, 0x24}, {0x004b, 0x25}, {0x004c, 0x26}, {0x004d, 0x32},
    {0x004e, 0x31}, {0x004f, 0x18}, {0x0050, 0x19}, {0x0051, 0x10}, {0x0052, 0x13},
    {0x0053, 0x1f}, {0x0054, 0x14}, {0x0055, 0x16}, {0x0056, 0x2f}, {0x0057, 0x11},
    {0x0058, 0x2d}, {0x0059, 0x15}, {0x005a, 0x2c},
};
const unsigned short kNumpadEnglishCharMap[][2] = {   // 58 entries, insertion order
    {0xf73e, 0x0e}, {0x0009, 0x0f}, {0x002d, 0x4a}, {0x002b, 0x4e}, {0x003d, 0x8d},
    {0x005b, 0x1a}, {0x005d, 0x1b}, {0x000d, 0x1c}, {0x0003, 0x9c}, {0x003b, 0x27},
    {0x0027, 0x28}, {0x0060, 0x29}, {0x005c, 0x2b}, {0x002c, 0xb3}, {0x002e, 0x53},
    {0x002f, 0xb5}, {0x002a, 0x37}, {0x0020, 0x39}, {0x005e, 0x90}, {0x0040, 0x91},
    {0x003a, 0x92}, {0x005f, 0x93}, {0x0030, 0x52}, {0x0031, 0x4f}, {0x0032, 0x50},
    {0x0033, 0x51}, {0x0034, 0x4b}, {0x0035, 0x4c}, {0x0036, 0x4d}, {0x0037, 0x47},
    {0x0038, 0x48}, {0x0039, 0x49}, {0x0041, 0x1e}, {0x0042, 0x30}, {0x0043, 0x2e},
    {0x0044, 0x20}, {0x0045, 0x12}, {0x0046, 0x21}, {0x0047, 0x22}, {0x0048, 0x23},
    {0x0049, 0x17}, {0x004a, 0x24}, {0x004b, 0x25}, {0x004c, 0x26}, {0x004d, 0x32},
    {0x004e, 0x31}, {0x004f, 0x18}, {0x0050, 0x19}, {0x0051, 0x10}, {0x0052, 0x13},
    {0x0053, 0x1f}, {0x0054, 0x14}, {0x0055, 0x16}, {0x0056, 0x2f}, {0x0057, 0x11},
    {0x0058, 0x2d}, {0x0059, 0x15}, {0x005a, 0x2c},
};

} // namespace

// Mac 0x218870
FunctionKeyMapClass::FunctionKeyMapClass()
{
    for (UInt32 i = 0; i < sizeof(kFunctionKeyMap) / sizeof(kFunctionKeyMap[0]); ++i)
        (*this)[kFunctionKeyMap[i][0]] = (Key)kFunctionKeyMap[i][1];
}

// Mac 0x21b380
EnglishCharMapClass::EnglishCharMapClass()
{
    for (UInt32 i = 0; i < sizeof(kEnglishCharMap) / sizeof(kEnglishCharMap[0]); ++i)
        (*this)[kEnglishCharMap[i][0]] = (Key)kEnglishCharMap[i][1];
}

// Mac 0x21d520
NumpadEnglishCharMapClass::NumpadEnglishCharMapClass()
{
    for (UInt32 i = 0; i < sizeof(kNumpadEnglishCharMap) / sizeof(kNumpadEnglishCharMap[0]); ++i)
        (*this)[kNumpadEnglishCharMap[i][0]] = (Key)kNumpadEnglishCharMap[i][1];
}

// Static initialiser Mac 0x21f9a0 (after the <iostream> init object 0x77dfa8).
FunctionKeyMapClass       g_functionKeyMap;
EnglishCharMapClass       g_englishCharMap;
NumpadEnglishCharMapClass g_numpadEnglishCharMap;

// ===========================================================================================
// LEGOMenuWrapperObserver
// ===========================================================================================

// Mac 0x2184c0
void LEGOMenuWrapperObserver::OnToggleChanged(bool toggled)
{
#if defined(__OBJC__)
    [(LEGOMenuItemWrapper*)m_menuItem onCommandToggled:toggled];
#else
    (void)toggled;
#endif
}

} // namespace LEGO

#if defined(__OBJC__)
using LEGO::Vector2;

// ===========================================================================================
// LEGOMacWindow — Window slots used (Mac vtable): 0 OnMouseMove, 10 OnMove, 11 OnResize,
// 12 OnActivate, 13 OnInput13 (keyboard focus), 15 OnCloseRequest, 16 OnClose.
// ===========================================================================================
@implementation LEGOMacWindow

// Mac 0x216210
- (BOOL)canBecomeKeyWindow
{
    return YES;
}

// Mac 0x216220
- (void)setLEGOWindow:(LEGO::Window*)aWindow
{
    window = aWindow;
    [self setAcceptsMouseMovedEvents:YES];
}

// Mac 0x216250
- (LEGO::Window*)GetLEGOWindow
{
    return window;
}

// Mac 0x216260
- (BOOL)isModal
{
    return window ? window->IsModal() : NO;
}

// Mac 0x216290: NOTE: always NO (Window::IsKeyPressed is never true on the Mac).
- (BOOL)IsKeyPressed:(int)key
{
    (void)key;
    return NO;
}

// Mac 0x2162a0
- (void)setHitTester:(LEGO::HitTester*)tester
{
    [[(LEGOMacContentView*)[self contentView] getDrawingView] setHitTester:tester];
}

// Mac 0x2162f0: the union of the frames of all screens.
+ (NSRect)GetVirtualDesktopRect
{
    NSScreen* first = [[NSScreen screens] objectAtIndex:0];
    NSRect rect = first ? [first frame] : NSZeroRect;
    for (NSUInteger i = 1; i < [[NSScreen screens] count]; ++i) {
        NSScreen* screen = [[NSScreen screens] objectAtIndex:i];
        rect = NSUnionRect(rect, screen ? [screen frame] : NSZeroRect);
    }
    return rect;
}

// Mac 0x2164a0: top-left corner of the frame in top-down screen coordinates.
- (void)windowDidMove:(NSNotification*)notification
{
    NSWindow* sender = [notification object];
    NSRect frame = sender ? [sender frame] : NSZeroRect;
    NSScreen* screen = [sender screen];
    NSRect screenFrame = screen ? [screen frame] : NSZeroRect;
    NSRect visible = screen ? [screen visibleFrame] : NSZeroRect;
    NSRect full = screen ? [screen frame] : NSZeroRect;
    if (NSEqualRects(full, visible))
        screenFrame = [LEGOMacWindow GetVirtualDesktopRect];
    window->OnMove(Vector2(frame.origin.x,
                           screenFrame.size.height - frame.size.height - frame.origin.y));
}

// Mac 0x216650
- (NSSize)windowWillResize:(NSWindow*)sender toSize:(NSSize)frameSize
{
    (void)sender;
    return frameSize;
}

// Mac 0x216660: frame size + content size, then the position (the top left moves).
- (void)windowDidResize:(NSNotification*)notification
{
    NSWindow* sender = [notification object];
    NSSize size = sender ? [sender frame].size : NSZeroSize;
    NSView* content = [sender contentView];
    NSSize clientSize = content ? [content frame].size : NSZeroSize;
    window->OnResize(Vector2(size.width, size.height), Vector2(clientSize.width, clientSize.height));
    [self windowDidMove:notification];
}

// Mac 0x2167b0
- (BOOL)windowShouldClose:(id)sender
{
    (void)sender;
    return window->OnCloseRequest();
}

// Mac 0x2167d0
- (void)windowWillClose:(NSNotification*)notification
{
    (void)notification;
    window->OnClose();
}

// Mac 0x2167f0: keyboard focus with the current modifiers, the mouse position, activation.
- (void)windowDidBecomeKey:(NSNotification*)notification
{
    (void)notification;
    if (!window)
        return;
    LEGOMacContentView* content = (LEGOMacContentView*)[self contentView];
#if defined(__LP64__)
    window->Invalidate(0);   // PORT64: also marks the subviews (layer-backed views, WindowMac.mm)
#else
    [content setNeedsDisplay:YES];
#endif
    LEGOMacDrawingView* drawingView = [content getDrawingView];
    NSEvent* event = [self currentEvent];
    NSPoint location = [[self contentView] convertPoint:[self mouseLocationOutsideOfEventStream]
                                               fromView:nil];
    window->OnInput13(true, (LEGO::UInt32)[drawingView getModifiers:(unsigned int)[event modifierFlags]], 0);
    window->OnMouseMove(Vector2(location.x, location.y));
    window->OnActivate(true);
    window->Invalidate(0);
}

// Mac 0x216940
- (void)windowDidResignKey:(NSNotification*)notification
{
    (void)notification;
    if (!window)
        return;
    LEGOMacDrawingView* drawingView = [(LEGOMacContentView*)[self contentView] getDrawingView];
    NSEvent* event = [self currentEvent];
    window->OnInput13(false, (LEGO::UInt32)[drawingView getModifiers:(unsigned int)[event modifierFlags]], 0);
    window->OnActivate(false);
    window->Invalidate(0);
}
@end

// ===========================================================================================
// LEGOMacNativeControlsView
// ===========================================================================================
@implementation LEGOMacNativeControlsView

// Mac 0x216a10
- (id)initWithFrame:(NSRect)frame
{
    self = [super initWithFrame:frame];
    if (self)
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    return self;
}

// Mac 0x216a90
- (void)dealloc
{
    [super dealloc];
}

- (BOOL)isFlipped { return YES; }                                     // Mac 0x216ad0
- (BOOL)isOpaque { return YES; }                                      // Mac 0x216ae0
- (BOOL)acceptsFirstResponder { return YES; }                         // Mac 0x216af0
- (BOOL)canBecomeKeyView { return YES; }                              // Mac 0x216b00

// Mac 0x216b10: the drawing view (in front) sees the movement too.
- (void)mouseMoved:(NSEvent*)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    [super mouseMoved:event];
    [[(LEGOMacContentView*)[self superview] getDrawingView] windowMouseMoved:event
                                                               andViewLocation:location];
}

- (BOOL)wantsDefaultClipping { return YES; }                          // Mac 0x216bb0

// Mac 0x216bc0
- (void)drawRect:(NSRect)rect
{
    [[NSColor blackColor] set];
    NSRectFill(rect);
}

// Mac 0x216c20
- (void)superMouseMoved:(NSEvent*)event
{
    [super mouseMoved:event];
}
@end

// ===========================================================================================
// LEGOMacDrawingView — Window slots used: 0 OnMouseMove, 1 OnMouseDown, 2 OnMouseUp,
// 3 OnMouseDoubleClick, 4 OnMouseWheel, 5 OnKeyDown, 6 OnKeyUp, 7 OnCharacter.
// ===========================================================================================
@implementation LEGOMacDrawingView

// Mac 0x216c60
- (id)initWithFrame:(NSRect)frame
{
    return [self initWithFrame:frame Manager:0];
}

// Mac 0x216cb0
- (id)initWithFrame:(NSRect)frame Manager:(LEGO::Window*)aWindow
{
    self = [super initWithFrame:frame];
    if (self) {
        window = aWindow;
        oldModifierFlags = 0;
        hitTester = 0;
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
    }
    return self;
}

// Mac 0x216d50
- (void)dealloc
{
    hitTester = 0;
    window = 0;
    [super dealloc];
}

// Mac 0x216db0
- (void)setManager:(LEGO::Window*)aWindow
{
    window = aWindow;
}

// Mac 0x216dc0
- (unsigned long)getModifiers:(unsigned int)flags
{
    return LEGO::MacModifierFlagsToModifiers(flags);
}

- (BOOL)isFlipped { return YES; }                                     // Mac 0x216df0
- (BOOL)isOpaque { return NO; }                                       // Mac 0x216e00
- (BOOL)acceptsFirstResponder { return YES; }                         // Mac 0x216e10
- (BOOL)canBecomeKeyView { return YES; }                              // Mac 0x216e20
- (BOOL)wantsDefaultClipping { return NO; }                           // Mac 0x216e30

// Mac 0x216e40: a native control may refuse to give the focus away.
- (BOOL)resignFirstResponder
{
    if (!hitTester)
        return YES;
    return hitTester->Activate();
}

// Mac 0x216e70: clicks on native controls fall through to the views behind (nil).
- (NSView*)hitTest:(NSPoint)point
{
    if (hitTester && !hitTester->HitTest(Vector2(point.x, point.y)))
        return nil;
    return self;
}

// Mac 0x216ed0: tracks the left button until it goes up (drags = mouse moves). Control-click
// is the right button (1).
- (void)mouseDown:(NSEvent*)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    LEGO::UInt32 button = ([event modifierFlags] >> 18) & 1;   // NSControlKeyMask
    [self flagsChanged:event];
    if ([event clickCount] == 2)
        window->OnMouseDoubleClick(Vector2(location.x, location.y), button);
    else
        window->OnMouseDown(Vector2(location.x, location.y), button);
    NSEvent* next;
    NSEventType type;
    do {
        next = [[self window] nextEventMatchingMask:NSLeftMouseUpMask | NSLeftMouseDraggedMask];
        location = [self convertPoint:[next locationInWindow] fromView:nil];
        type = [next type];
        if (type == NSLeftMouseDragged) {
            [self flagsChanged:next];
            window->OnMouseMove(Vector2(location.x, location.y));
        }
    } while (type != NSLeftMouseUp);
    [self flagsChanged:next];
    window->OnMouseUp(Vector2(location.x, location.y), button);
    [[self window] discardEventsMatchingMask:NSAnyEventMask beforeEvent:next];
}

// Mac 0x2170b0: the native-controls view (behind) sees the movement too.
- (void)mouseMoved:(NSEvent*)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    [[(LEGOMacContentView*)[self superview] getNativeView] superMouseMoved:event];
    [self windowMouseMoved:event andViewLocation:location];
}

// Mac 0x217140: as mouseDown: for the right button (1).
- (void)rightMouseDown:(NSEvent*)event
{
    NSPoint location = [self convertPoint:[event locationInWindow] fromView:nil];
    [self flagsChanged:event];
    if ([event clickCount] == 2)
        window->OnMouseDoubleClick(Vector2(location.x, location.y), 1);
    else
        window->OnMouseDown(Vector2(location.x, location.y), 1);
    NSEvent* next;
    NSEventType type;
    do {
        next = [[self window] nextEventMatchingMask:NSRightMouseUpMask | NSRightMouseDraggedMask];
        location = [self convertPoint:[next locationInWindow] fromView:nil];
        type = [next type];
        if (type == NSRightMouseDragged) {
            [self flagsChanged:next];
            window->OnMouseMove(Vector2(location.x, location.y));
        }
    } while (type != NSRightMouseUp);
    [self flagsChanged:next];
    window->OnMouseUp(Vector2(location.x, location.y), 1);
    [[self window] discardEventsMatchingMask:NSAnyEventMask beforeEvent:next];
}

// Mac 0x217310: NOTE: deltaY goes to axis 0 and deltaX to axis 1.
- (void)scrollWheel:(NSEvent*)event
{
    if (fabsf((float)[event deltaY]) >= 1e-05f)
        window->OnMouseWheel(0, (LEGO::Real32)[event deltaY]);
    if (fabsf((float)[event deltaX]) >= 1e-05f)
        window->OnMouseWheel(1, (LEGO::Real32)[event deltaX]);
    [self flagsChanged:event];
}

// Mac 0x217410: Command-W closes the window (there is no File > Close menu item for it).
- (BOOL)performKeyEquivalent:(NSEvent*)event
{
    if ([event modifierFlags] & NSCommandKeyMask) {
        NSString* characters = [[event charactersIgnoringModifiers] uppercaseString];
        NSUInteger length = [characters length];
        for (NSUInteger i = 0; i != length; ++i) {
            if ([characters characterAtIndex:i] == 'W')
                [[self window] performClose:self];
        }
    }
    return [super performKeyEquivalent:event];
}

// Mac 0x217520: per typed character a key-down (function keys, or main-block / keypad keys)
// and, except for function keys, the character itself.
- (void)keyDown:(NSEvent*)event
{
    if (![[self window] isKeyWindow])
        return;
    NSString* characters = [event charactersIgnoringModifiers];
    NSString* upper = [[event charactersIgnoringModifiers] uppercaseString];
    oldModifierFlags = (unsigned int)[event modifierFlags];
    LEGO::UInt32 modifiers = (LEGO::UInt32)[self getModifiers:oldModifierFlags];
    NSUInteger length = [characters length];
    for (NSUInteger i = 0; i != length; ++i) {
        unichar c = [characters characterAtIndex:i];
        unichar u = [upper characterAtIndex:i];
        if (LEGO::g_functionKeyMap.Contains(u)) {
            window->OnKeyDown(LEGO::g_functionKeyMap[u], modifiers, [event isARepeat] != NO);
            continue;
        }
        if (LEGO::g_englishCharMap.Contains(u)) {
            LEGO::Key key = LEGO::MacCharacterToKey(u, ([event modifierFlags] & NSNumericPadKeyMask) != 0);
            window->OnKeyDown(key, modifiers, [event isARepeat] != NO);
        }
        window->OnCharacter(c, modifiers, [event isARepeat] != NO);
    }
}

// Mac 0x217930: NOTE: unlike keyDown: both maps are tried for every character.
- (void)keyUp:(NSEvent*)event
{
    if (![[self window] isKeyWindow])
        return;
    NSString* upper = [[event charactersIgnoringModifiers] uppercaseString];
    oldModifierFlags = (unsigned int)[event modifierFlags];
    LEGO::UInt32 modifiers = (LEGO::UInt32)[self getModifiers:oldModifierFlags];
    NSUInteger length = [upper length];
    for (NSUInteger i = 0; i != length; ++i) {
        unichar u = [upper characterAtIndex:i];
        if (LEGO::g_functionKeyMap.Contains(u))
            window->OnKeyUp(LEGO::g_functionKeyMap[u], modifiers);
        if (LEGO::g_englishCharMap.Contains(u))
            window->OnKeyUp(LEGO::MacCharacterToKey(u, ([event modifierFlags] & NSNumericPadKeyMask) != 0),
                            modifiers);
    }
}

// Mac 0x217c90: modifier keys as key-downs / key-ups of their keys.
- (void)flagsChanged:(NSEvent*)event
{
    if (!window)
        return;
    unsigned int flags = (unsigned int)[event modifierFlags];
    LEGO::UInt32 modifiers = (LEGO::UInt32)[self getModifiers:flags];
    static const struct { unsigned int mask; LEGO::Key key; } kModifierKeys[] = {
        {NSAlphaShiftKeyMask, LEGO::kKeyCapsLock}, {NSShiftKeyMask, LEGO::kKeyShift},
        {NSControlKeyMask, LEGO::kKeyControl},     {NSAlternateKeyMask, LEGO::kKeyAlt},
        {NSCommandKeyMask, LEGO::kKeyApps},        {NSNumericPadKeyMask, LEGO::kKeyNumLock},
    };
    for (unsigned int i = 0; i < sizeof(kModifierKeys) / sizeof(kModifierKeys[0]); ++i) {
        unsigned int mask = kModifierKeys[i].mask;
        if ((flags & mask) == (oldModifierFlags & mask))
            continue;
        if (flags & mask)
            window->OnKeyDown(kModifierKeys[i].key, modifiers, false);
        else
            window->OnKeyUp(kModifierKeys[i].key, modifiers);
    }
    oldModifierFlags = flags;
}

// Mac 0x217e30
- (void)setHitTester:(LEGO::HitTester*)tester
{
    hitTester = tester;
}

// Mac 0x217e80
- (void)windowMouseMoved:(NSEvent*)event andViewLocation:(NSPoint)location
{
    [self flagsChanged:event];
    window->OnMouseMove(Vector2(location.x, location.y));
}
// .cxx_destruct Mac 0x217ef0 / .cxx_construct Mac 0x217f20: the hitTester RefPointer.
@end

// ===========================================================================================
// LEGOMacContentView
// ===========================================================================================
@implementation LEGOMacContentView

// Mac 0x217f30
- (id)initWithFrame:(NSRect)frame
{
    return [self initWithFrame:frame Manager:0];
}

// Mac 0x217f80: the native-controls view first (behind), the drawing view in front.
- (id)initWithFrame:(NSRect)frame Manager:(LEGO::Window*)aWindow
{
    self = [super initWithFrame:frame];
    if (self) {
        window = aWindow;
        nativeView = [[LEGOMacNativeControlsView alloc] initWithFrame:frame];
        [self addSubview:nativeView];
        drawingView = [[LEGOMacDrawingView alloc] initWithFrame:frame Manager:window];
        [self addSubview:drawingView];
    }
    return self;
}

// Mac 0x2180c0
- (void)dealloc
{
    window = 0;
    [drawingView release];
    [nativeView release];
    [super dealloc];
}

// Mac 0x218130
- (void)setManager:(LEGO::Window*)aWindow
{
    window = aWindow;
    [drawingView setManager:aWindow];
}

// Mac 0x218160
- (unsigned long)getModifiers:(unsigned int)flags
{
    return LEGO::MacModifierFlagsToModifiers(flags);
}

- (BOOL)isFlipped { return YES; }                                     // Mac 0x218190
- (BOOL)isOpaque { return YES; }                                      // Mac 0x2181a0
- (BOOL)acceptsFirstResponder { return NO; }                          // Mac 0x2181b0
- (LEGOMacNativeControlsView*)getNativeView { return nativeView; }   // Mac 0x2181c0
- (LEGOMacDrawingView*)getDrawingView { return drawingView; }        // Mac 0x2181d0

// Mac 0x2181e0 (NativeControlKeyboardFocusObserver::ToggleFirstResponder)
- (void)toggleFirstResponder:(BOOL)toNativeView
{
    [[self window] makeFirstResponder:toNativeView ? (NSView*)nativeView : (NSView*)drawingView];
}

// Mac 0x218230
- (void)updateInitialFirstResponder
{
    [[self window] setInitialFirstResponder:drawingView];
}
@end

// ===========================================================================================
// LEGOMenuItemWrapper — a menu item that runs a Command (slots: Do 3, GetEnable 5).
// ===========================================================================================
@implementation LEGOMenuItemWrapper

// Mac 0x218500 (Observer<Command, CommandObserver>::Attach is the out-of-line copy 0x218270)
- (id)initWithTitle:(NSString*)title keyEquivalent:(NSString*)key
    KeyEquivalentModifierMask:(unsigned int)mask command:(LEGO::Command*)aCommand
    window:(LEGO::Window*)aWindow
{
    self = [super initWithTitle:title action:@selector(Clicked:) keyEquivalent:key];
    if (self) {
        [self setKeyEquivalentModifierMask:mask];
        [self setTarget:self];
        command = aCommand;
        window = aWindow;
        LEGO::LEGOMenuWrapperObserver* observer = new LEGO::LEGOMenuWrapperObserver(self);
        observer->Attach([self getCommand]);
        menuWrapperObserver = observer;
    }
    return self;
}

// Mac 0x218640
- (void)Clicked:(id)sender
{
    (void)sender;
    command->Do();
}

// Mac 0x218660: disabled while the window is modal or miniaturised.
- (BOOL)validateMenuItem:(NSMenuItem*)item
{
    (void)item;
    if (window->IsModal())
        return NO;
    if ([(NSWindow*)window->GetNativeWindow() isMiniaturized])
        return NO;
    return command->GetEnable();
}

// Mac 0x2186c0
- (void)onCommandToggled:(bool)toggled
{
    [self setState:toggled];
}

// Mac 0x2186f0
- (LEGO::Command*)getCommand
{
    return command;
}

// Mac 0x218700: NOTE: the observer is released without a NULL check.
- (void)dealloc
{
    menuWrapperObserver->Release();
    [super dealloc];
}
@end
#endif // __OBJC__
#endif // !_WIN32
