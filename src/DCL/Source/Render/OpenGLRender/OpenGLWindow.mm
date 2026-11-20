// LEGO Digital Designer — restored source
// Mac implementation of LEGO::OpenGLWindow and LEGOOpenGLView : NSOpenGLView (TU 052, Mac
// 0x118cb0-0x11991f). Windows: OpenGLWindow.cpp.
//
// LDD64 Linux (LDD_LINUX, not restored code): the view is the GtkGLArea of the GTK shell
// (Platform/Linux/LinuxShell.h); m_view is then the native window holding it. Other builds
// without Objective-C use a HOST-ONLY GLX window + legacy context (not restored code, marked
// below) that gives OpenGLRender a current context.
#if !defined(_WIN32)
#include "OpenGLWindow.h"

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#import <OpenGL/OpenGL.h>

#if defined(__LP64__)
// PORT64: apps built with the macOS 10.14 SDK or later always get layer-backed views. A
// layer-backed NSOpenGLView shows only what is drawn inside its own display cycle (drawRect:),
// but LDD renders and flushes from its update loop (OpenGLRender::Present), so the window only
// changed when a resize made AppKit redraw the view. 64-bit builds therefore use a plain NSView
// that owns its NSOpenGLContext (setView:, update on every size / window / backing change and in
// updateLayer): a context attached that way presents every flush. The rendering itself is the
// original's; updateLayer takes the place of drawRect: (OnPaint).
@interface LEGOOpenGLView : NSView {
    LEGO::Window* window;
    NSOpenGLContext* context;
}
- (id)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat*)format window:(LEGO::Window*)owner;
- (NSOpenGLContext*)openGLContext;
@end
#else
// Mac objc class LEGOOpenGLView (ivar `window` at +100 = LEGO::Window*).
@interface LEGOOpenGLView : NSOpenGLView {
    LEGO::Window* window;
}
- (id)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat*)format window:(LEGO::Window*)owner;
@end
#endif

// The LEGOMacWindow content view (WindowMac.mm) answers getDrawingView.
@interface NSView (LEGODrawingView)
- (NSView*)getDrawingView;
@end

@implementation LEGOOpenGLView

#if defined(__LP64__)
- (id)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat*)format window:(LEGO::Window*)owner
{
    self = [super initWithFrame:frame];
    if (self) {
        window = owner;
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        [self setWantsLayer:YES];
        context = [[NSOpenGLContext alloc] initWithFormat:format shareContext:nil];
        GLint value = 1;
        [context setValues:&value forParameter:NSOpenGLCPSwapInterval];
        value = 0;
        [context setValues:&value forParameter:NSOpenGLCPSurfaceOpacity];
    }
    return self;
}

- (void)dealloc
{
    [context clearDrawable];
    [context release];
    [super dealloc];
}

- (NSOpenGLContext*)openGLContext
{
    return context;
}

- (void)viewDidMoveToWindow
{
    [super viewDidMoveToWindow];
    if ([self window]) {
        [context setView:self];
        [context update];
    }
}

- (void)setFrameSize:(NSSize)size
{
    [super setFrameSize:size];
    [context update];
}

- (void)viewDidChangeBackingProperties
{
    [super viewDidChangeBackingProperties];
    [context update];
}

- (BOOL)wantsUpdateLayer
{
    return YES;
}

// AppKit's redraw of the (layer-backed) view: the context's surface is brought up to date, then
// the window paints as in drawRect: below
- (void)updateLayer
{
    [context update];
    if (window)
        window->OnPaint((LEGO::UInt32)(size_t)self);
}
#else
// Mac 0x1191f0
- (id)initWithFrame:(NSRect)frame pixelFormat:(NSOpenGLPixelFormat*)format window:(LEGO::Window*)owner
{
    self = [super initWithFrame:frame pixelFormat:format];
    if (self) {
        window = owner;
        [self setAutoresizingMask:NSViewWidthSizable | NSViewHeightSizable];
        GLint value = 1;
        [[self openGLContext] setValues:&value forParameter:NSOpenGLCPSwapInterval];
        value = 0;
        [[self openGLContext] setValues:&value forParameter:NSOpenGLCPSurfaceOpacity];
    }
    return self;
}
#endif

// Mac 0x1192d0
- (void)rightMouseDown:(NSEvent*)event
{
    [[self nextResponder] rightMouseDown:event];
}

// Mac 0x119310
- (BOOL)isOpaque
{
    return YES;
}

// Mac 0x119320
- (BOOL)wantsDefaultClipping
{
    return NO;
}

// Mac 0x119330: OnPaint (WindowInput slot 18) with the address of the dirty rectangle
- (void)drawRect:(NSRect)rect
{
    // PORT64: the address only identifies the paint (no paint observer reads it; RenderControl
    // Manager ignores it), so on 64-bit it is passed truncated like Window.cpp's WM_PAINT; the
    // cast through size_t is what makes it compile there (the same code on i386)
    if (window)
        window->OnPaint((LEGO::UInt32)(size_t)&rect);
}

@end

#elif defined(LDD_LINUX)
#include "LinuxShell.h"
#else
// HOST-ONLY (not restored code): GLX stand-in for the LEGOOpenGLView.
#include <GL/glx.h>
#include <X11/Xlib.h>
#include <string.h>

namespace {
struct HostGLView {
    Display*   display;
    ::Window   window;
    Colormap   colormap;
    GLXContext context;
};

HostGLView* CreateHostGLView(int width, int height)
{
    Display* display = XOpenDisplay(NULL);
    if (!display)
        return NULL;
    int attributes[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_RED_SIZE, 8, GLX_GREEN_SIZE, 8,
                        GLX_BLUE_SIZE, 8, GLX_ALPHA_SIZE, 8, GLX_DEPTH_SIZE, 24, None};
    XVisualInfo* visual = glXChooseVisual(display, DefaultScreen(display), attributes);
    if (!visual) {
        XCloseDisplay(display);
        return NULL;
    }
    HostGLView* view = new HostGLView;
    view->display = display;
    ::Window root = RootWindow(display, visual->screen);
    view->colormap = XCreateColormap(display, root, visual->visual, AllocNone);
    XSetWindowAttributes swa;
    memset(&swa, 0, sizeof(swa));
    swa.colormap = view->colormap;
    view->window = XCreateWindow(display, root, 0, 0, width > 0 ? width : 1, height > 0 ? height : 1,
                                 0, visual->depth, InputOutput, visual->visual, CWColormap, &swa);
    view->context = glXCreateContext(display, visual, NULL, True);
    XFree(visual);
    if (!view->context || !glXMakeCurrent(display, view->window, view->context)) {
        if (view->context)
            glXDestroyContext(display, view->context);
        XDestroyWindow(display, view->window);
        XFreeColormap(display, view->colormap);
        XCloseDisplay(display);
        delete view;
        return NULL;
    }
    return view;
}

void DestroyHostGLView(HostGLView* view)
{
    if (glXGetCurrentContext() == view->context)
        glXMakeCurrent(view->display, None, NULL);
    glXDestroyContext(view->display, view->context);
    XDestroyWindow(view->display, view->window);
    XFreeColormap(view->display, view->colormap);
    XCloseDisplay(view->display);
    delete view;
}
} // namespace
#endif

namespace LEGO {

// Mac 0x118cb0 (C2 with VTT: Window 0x205c0, then the OpenGLWindow vtables)
OpenGLWindow::OpenGLWindow(bool flag95, UInt32 level)
    : Window(flag95), m_level(level), m_view(0)
{
}

// Mac 0x119360 (D1) / 0x1193c0 (D0) and thunks 0x119430-0x119860
OpenGLWindow::~OpenGLWindow()
{
}

// Mac: inline [view removeFromSuperview]; [view release]
void OpenGLWindow::DestroyContext()
{
    if (m_view) {
#if defined(__OBJC__)
        [(LEGOOpenGLView*)m_view removeFromSuperview];
        [(LEGOOpenGLView*)m_view release];
#elif defined(LDD_LINUX)
        // (a native window destroyed since took its view along)
        if (m_view == m_nativeWindow)
            LinuxShell::DestroyGLView(m_view);
#else
        DestroyHostGLView((HostGLView*)m_view);       // HOST-ONLY
#endif
        m_view = 0;
    }
}

// Mac 0x118d30 (thunks 0x118d80, 0x118df0, 0x118e60, 0x118ed0)
void OpenGLWindow::OnFinalRelease()
{
    DestroyContext();
    Window::OnFinalRelease();
}

// Mac 0x118f40
bool OpenGLWindow::NotifyInput17()
{
    DestroyContext();
    if (!CreateContext())
        return false;
    return Window::NotifyInput17();
}

// Mac 0x118fc0
bool OpenGLWindow::ChangeDisplayMode(const WindowStyle& style, const WindowStyle& previous)
{
    if (!Window::ChangeDisplayMode(style, previous))
        return false;
    if (style.flag28 != previous.flag28 || (style.flag28 && previous.value2c != style.value2c))
        DestroyNativeWindow();
    return true;
}

// Mac 0x119020
// NOTE: the style's full-screen depth is passed as NSOpenGLPFADepthSize (the Windows build uses
// it as the colour depth); the pixel format object is never released.
bool OpenGLWindow::CreateContext()
{
#if defined(__OBJC__)
    NSOpenGLPixelFormatAttribute attributes[] = {
        NSOpenGLPFADoubleBuffer,
        NSOpenGLPFADepthSize, (NSOpenGLPixelFormatAttribute)m_style.value2c,
        NSOpenGLPFAAlphaSize, 8,
        0};
    NSOpenGLPixelFormat* format = [[NSOpenGLPixelFormat alloc] initWithAttributes:attributes];
    if (!format)
        return false;
    NSWindow* window = (NSWindow*)m_nativeWindow;
    NSView* content = [window contentView];
    NSRect bounds = content ? [content bounds] : NSZeroRect;
    LEGOOpenGLView* view = [[LEGOOpenGLView alloc] initWithFrame:bounds pixelFormat:format window:this];
    m_view = view;
    [[[window contentView] getDrawingView] addSubview:view];
    [view setNextResponder:[[window contentView] getDrawingView]];
    [[view openGLContext] makeCurrentContext];
    return true;
#elif defined(LDD_LINUX)
    if (!LinuxShell::CreateGLView(m_nativeWindow, this))
        return false;
    m_view = m_nativeWindow;
    return true;
#else
    // HOST-ONLY: an unmapped GLX window of the client size, its context made current.
    m_view = CreateHostGLView((int)m_style.size.x, (int)m_style.size.y);
    return m_view != 0;
#endif
}

} // namespace LEGO
#endif // !_WIN32
