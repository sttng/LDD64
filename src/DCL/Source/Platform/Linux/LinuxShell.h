// LDD64 Linux platform layer (not restored code): the GTK 3 shell of the Mac branch, doing what
// Cocoa does for the Mac build (windows, GL view, main menu, run-loop events). Wayland or X11.
//
// A window is a GtkWindow with the menu bar above a GtkOverlay: the GL view (GtkGLArea) with the
// native text entries in front. Input follows Windows: buttons 0 left, 1 right, 2 middle; wheel
// steps of 1 per notch; Key.h codes with Ctrl 1, Shift 2, Alt 4. Menu short-cuts run through
// Window::ExecuteMenuShortcut.
//
// LDD draws into an offscreen framebuffer of the view's size (RGBA8, depth 24, stencil 8, as the
// Mac pixel format) that stands in for the default one (OpenGL/gl.h); Present copies it into the
// GtkGLArea. The context is a compatibility profile (GDK_GL=legacy) for the fixed-function
// pipeline.
#pragma once
#if defined(LDD_LINUX)
#include "LEGO/Types.h"
#include "LEGO/String.h"
#include "Geometry.h"
#include "BasicArray.h"

typedef struct _GtkWidget GtkWidget;

namespace LEGO {

class Window;
class Menu;
struct WindowStyle;

namespace LinuxShell {

// Before AppMain: GTK (with the legacy GL profile), the application name and icon.
void Initialize(int* argc, char*** argv);
// Handles the pending events (the run and modal loops call it once per pass).
void Pump();

// ---- native windows (Window, WindowMac.mm) ----
// The GtkWindow of `owner` with its style (not shown yet); `menuBar`: room for SetMenu.
void*   CreateWindow(Window* owner, const WindowStyle& style, bool menuBar);
void    DestroyWindow(void* native);
// Applies size, position, minimum size, resizability, decoration and full screen of `style`.
void    ApplyStyle(void* native, const WindowStyle& style);
void    SetTitle(void* native, const String& title);
void    SetModified(void* native, bool modified);
void    SetVisible(void* native, bool visible);
// The size of the GL view in points (the content view of the Mac window).
Vector2 GetClientSize(void* native);
// The view repaints (Window::OnPaint) on the next frame.
void    Invalidate(void* native);
// While modal the window cannot be closed and its menus are disabled.
void    SetModal(void* native, bool modal);
void    SetMenu(void* native, const Menu* menu, Window* window);
bool    IsKeyPressed(UInt32 key);
// `cursor`: a GdkCursor (0: the default arrow) for every window.
void    SetCursor(void* cursor);
// A theme cursor by CSS name ("default", "pointer", ...), or the Windows cursor resource `id`
// (Cursors/cursor_<id>.cur next to the executable). 0 when there is none.
void*   LoadSystemCursor(const char* name);
void*   LoadResourceCursor(UInt16 id);
void    ReleaseCursor(void* cursor);
Rect    GetVirtualDesktopRect();
// The GtkWindow (dialog parents).
GtkWidget* GetGtkWindow(void* native);
// The GtkWindow of the frontmost LDD window (0 without).
GtkWidget* GetMainGtkWindow();

// ---- the GL view (OpenGLWindow.mm) ----
// Creates the view of the window, its context current.
bool    CreateGLView(void* native, Window* owner);
void    DestroyGLView(void* native);

// ---- native controls (TextInputControlMac.mm) ----
// Puts `widget` in front of the GL view at `frame` (points, top-left origin of the view).
void    AddControl(void* native, GtkWidget* widget);
void    PlaceControl(GtkWidget* widget, const Rect& frame);
// Keyboard focus: to the control (true) or back to the view (false).
void    FocusControl(void* native, GtkWidget* widget, bool focus);

// ---- dialogs and the desktop (Platform.mm; LinuxDialogs.cpp) ----
// NSRunAlertPanel: `title` over `message`; buttons default (button1, "OK" if none), alternate,
// other. Returns 1, 0, -1 (NSAlert*Return).
long    Alert(const String* title, const String& message, const String* button1,
              const String* button2, const String* button3);
// The file chooser (the XDG portal's where available). `folder` / `name`: start; `names` /
// `extensions`: filters (save adds the chosen extension; *typeIndex gets its index). Returns the
// native path.
enum ChooserMode { kChooseOpen, kChooseSave, kChooseFolder };
bool    ChoosePath(ChooserMode mode, const String& title, const String& message, const String& folder,
                   const String& name, const BasicArray<String>& names,
                   const BasicArray<String>& extensions, UInt32* typeIndex, String& result);
// Opens a URL (or a file:// URI) with the desktop's handler.
bool    OpenURI(const String& uri);

// ---- printing (PrintJob.cpp; LinuxPrint.cpp): the PMPrintSession part of the Mac build ----
// A session holds the print dialog's printer, settings and page setup and, between PrintBegin and
// PrintEnd, the job (cairo, points, origin at the printable area's top left).
void*   PrintCreate();
void    PrintRelease(void* session);
bool    PrintDialog(void* session, bool& accepted);
bool    PrintBegin(void* session, const String& name);
void    PrintEnd(void* session);
void    PrintAbort(void* session);
void    PrintBeginPage(void* session);
void    PrintEndPage(void* session);
// The printable area in points.
void    PrintPageSize(void* session, Int32& width, Int32& height);
// A 24-bit RGB (or 32-bit BGRA) top-down bitmap scaled into the rectangle (points).
bool    PrintDrawBitmap(void* session, Int32 x, Int32 y, Int32 width, Int32 height, const UInt8* data,
                        UInt32 bitmapWidth, UInt32 bitmapHeight, Int32 pitch, UInt32 bitsPerPixel);

} // namespace LinuxShell
} // namespace LEGO
#endif
