// LDD64 Linux platform layer (not restored code): LinuxShell.h on GTK 3.
#if defined(LDD_LINUX)
#define GL_GLEXT_PROTOTYPES 1
#include "LinuxShell.h"
#include "Window.h"
#include "Menu.h"
#include "Command.h"
#include "Key.h"
#include "Application.h"
#include "Platform.h"
#include "Path.h"
#include <gtk/gtk.h>
#include "OpenGL/OpenGL.h"
// this file calls GL's own functions (the redirection of OpenGL/gl.h is implemented below)
#undef glBindFramebufferEXT
#undef glBindFramebuffer
#undef glReadBuffer
#undef glDrawBuffer
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <set>
#include <string>
#include <vector>

// The GL view; OpenGL.h's CGLContextObj.
struct _CGLContextObject {
    void*      native;          // LEGO::LinuxShell::NativeWindow*
    GtkWidget* area;
    GLuint     framebuffer;     // "the window" of LDD's frames
    GLuint     color;
    GLuint     depthStencil;
    int        width;           // of the framebuffer (points)
    int        height;
    GLuint     bound;           // LDD's framebuffer binding (0: the window)
    bool       needsPaint;      // Window::Invalidate since the last paint
    bool       inRender;        // inside the GtkGLArea's render signal
    bool       hasFrame;        // a frame was presented since the framebuffer was (re)made
};

namespace LEGO {
namespace LinuxShell {

namespace {

struct NativeWindow {
    Window*        owner;
    GtkWidget*     window;
    GtkWidget*     box;
    GtkWidget*     menuBar;
    GtkWidget*     overlay;
    _CGLContextObject* view;
    String         title;
    bool           modified;
    bool           modal;
    bool           hidden;
    Vector2        styleSize;       // the content size asked for (before the first allocation)
    // double clicks (Windows rules: the second press inside the time and distance)
    guint32        lastPressTime;
    guint          lastPressButton;
    double         lastPressX;
    double         lastPressY;
    UInt32         buttonsDown;
};

std::vector<NativeWindow*> s_windows;
_CGLContextObject*        s_current = 0;
std::set<UInt32>          s_pressedKeys;
GdkCursor*                s_cursor = 0;
bool                      s_initialized = false;

bool IsLive(NativeWindow* w)
{
    for (size_t i = 0; i < s_windows.size(); ++i)
        if (s_windows[i] == w)
            return true;
    return false;
}

// ---- GL ----------------------------------------------------------------------------------------

// The framebuffer of `view` at the size of its area (made or resized; the context is current).
void EnsureFramebuffer(_CGLContextObject* view)
{
    GtkAllocation a;
    gtk_widget_get_allocation(view->area, &a);
    int w = a.width > 1 ? a.width : 1, h = a.height > 1 ? a.height : 1;
    if (!gtk_widget_get_realized(view->area) || a.width <= 1) {
        NativeWindow* nw = (NativeWindow*)view->native;
        w = nw->styleSize.x > 1.0f ? (int)nw->styleSize.x : 1;
        h = nw->styleSize.y > 1.0f ? (int)nw->styleSize.y : 1;
    }
    if (view->framebuffer && view->width == w && view->height == h)
        return;
    GLint previousRenderbuffer = 0;
    glGetIntegerv(GL_RENDERBUFFER_BINDING, &previousRenderbuffer);
    if (!view->framebuffer) {
        glGenFramebuffers(1, &view->framebuffer);
        glGenRenderbuffers(1, &view->color);
        glGenRenderbuffers(1, &view->depthStencil);
    }
    glBindRenderbuffer(GL_RENDERBUFFER, view->color);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_RGBA8, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, view->depthStencil);
    glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, w, h);
    glBindRenderbuffer(GL_RENDERBUFFER, (GLuint)previousRenderbuffer);
    GLint previousDraw = 0, previousRead = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &previousDraw);
    glGetIntegerv(GL_READ_FRAMEBUFFER_BINDING, &previousRead);
    glBindFramebuffer(GL_FRAMEBUFFER, view->framebuffer);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_RENDERBUFFER, view->color);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, view->depthStencil);
    glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_STENCIL_ATTACHMENT, GL_RENDERBUFFER, view->depthStencil);
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)previousDraw);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, (GLuint)previousRead);
    view->width = w;
    view->height = h;
    view->hasFrame = false;
    view->needsPaint = true;
}

// Makes the view's context current with LDD's framebuffer binding (GTK binds its own while it
// draws, and makes other contexts current).
void MakeCurrent(_CGLContextObject* view)
{
    if (!view || !gtk_widget_get_realized(view->area))
        return;
    gtk_gl_area_make_current(GTK_GL_AREA(view->area));
    s_current = view;
    EnsureFramebuffer(view);
    glBindFramebuffer(GL_FRAMEBUFFER, view->bound ? view->bound : view->framebuffer);
}

// The view's framebuffer into the area's (bound by GTK), scaled to the display.
void CopyFrame(_CGLContextObject* view)
{
    GLint target = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &target);
    int scale = gtk_widget_get_scale_factor(view->area);
    GtkAllocation a;
    gtk_widget_get_allocation(view->area, &a);
    GLboolean scissor = glIsEnabled(GL_SCISSOR_TEST);
    glDisable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_READ_FRAMEBUFFER, view->framebuffer);
    glReadBuffer(GL_COLOR_ATTACHMENT0);
    glBlitFramebuffer(0, 0, view->width, view->height, 0, 0, a.width * scale, a.height * scale,
                      GL_COLOR_BUFFER_BIT, scale == 1 ? GL_NEAREST : GL_LINEAR);
    if (scissor)
        glEnable(GL_SCISSOR_TEST);
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)target);
}

gboolean OnRender(GtkGLArea* area, GdkGLContext*, gpointer data)
{
    _CGLContextObject* view = (_CGLContextObject*)data;
    NativeWindow* nw = (NativeWindow*)view->native;
    (void)area;
    GLint gtkFramebuffer = 0;
    glGetIntegerv(GL_DRAW_FRAMEBUFFER_BINDING, &gtkFramebuffer);
    s_current = view;
    EnsureFramebuffer(view);
    if (view->needsPaint || !view->hasFrame) {
        view->needsPaint = false;
        view->inRender = true;
        glBindFramebuffer(GL_FRAMEBUFFER, view->bound ? view->bound : view->framebuffer);
        nw->owner->OnPaint(0);
        view->inRender = false;
        gtk_gl_area_make_current(area);   // (a paint can make another context current)
    }
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, (GLuint)gtkFramebuffer);
    CopyFrame(view);
    glBindFramebuffer(GL_FRAMEBUFFER, (GLuint)gtkFramebuffer);
    return TRUE;
}

// GtkGLArea's default resize handler sets the viewport, which is LDD's.
void OnAreaResize(GtkGLArea* area, gint, gint, gpointer)
{
    g_signal_stop_emission_by_name(area, "resize");
}

// ---- input -------------------------------------------------------------------------------------

UInt32 Modifiers(guint state)
{
    return ((state & GDK_CONTROL_MASK) ? kModifierControl : 0) | ((state & GDK_SHIFT_MASK) ? kModifierShift : 0) |
           ((state & GDK_MOD1_MASK) ? kModifierAlt : 0);
}

// Window.cpp's virtual-key table (Win 0x40e770) by the keysym of the key without modifiers on the
// active layout (letters follow the layout, as Windows' virtual keys do).
UInt32 KeyOf(GdkEventKey* e)
{
    guint keyval = 0;
    GdkKeymap* keymap = gdk_keymap_get_for_display(gdk_window_get_display(e->window));
    if (!gdk_keymap_translate_keyboard_state(keymap, e->hardware_keycode, (GdkModifierType)(e->state & GDK_MOD2_MASK),
                                             e->group, &keyval, 0, 0, 0))
        keyval = e->keyval;
    if (keyval >= GDK_KEY_a && keyval <= GDK_KEY_z)
        keyval = keyval - GDK_KEY_a + GDK_KEY_A;
    static const UInt8 kLetters[26] = {
        kKeyA, kKeyB, kKeyC, kKeyD, kKeyE, kKeyF, kKeyG, kKeyH, kKeyI, kKeyJ, kKeyK, kKeyL, kKeyM,
        kKeyN, kKeyO, kKeyP, kKeyQ, kKeyR, kKeyS, kKeyT, kKeyU, kKeyV, kKeyW, kKeyX, kKeyY, kKeyZ};
    if (keyval >= GDK_KEY_A && keyval <= GDK_KEY_Z)
        return kLetters[keyval - GDK_KEY_A];
    if (keyval >= GDK_KEY_1 && keyval <= GDK_KEY_9)
        return kKey1 + (keyval - GDK_KEY_1);
    if (keyval == GDK_KEY_0)
        return kKey0;
    static const UInt8 kFunction[15] = {kKeyF1, kKeyF2, kKeyF3, kKeyF4, kKeyF5, kKeyF6, kKeyF7, kKeyF8,
                                        kKeyF9, kKeyF10, kKeyF11, kKeyF12, kKeyF13, kKeyF14, kKeyF15};
    if (keyval >= GDK_KEY_F1 && keyval <= GDK_KEY_F12)   // (Windows' table stops at F12)
        return kFunction[keyval - GDK_KEY_F1];
    static const UInt8 kNumpad[10] = {kKeyNumpad0, kKeyNumpad1, kKeyNumpad2, kKeyNumpad3, kKeyNumpad4,
                                      kKeyNumpad5, kKeyNumpad6, kKeyNumpad7, kKeyNumpad8, kKeyNumpad9};
    if (keyval >= GDK_KEY_KP_0 && keyval <= GDK_KEY_KP_9)
        return kNumpad[keyval - GDK_KEY_KP_0];
    switch (keyval) {
    case GDK_KEY_BackSpace:    return kKeyBackspace;
    case GDK_KEY_Tab:
    case GDK_KEY_ISO_Left_Tab: return kKeyTab;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:     return kKeyReturn;
    case GDK_KEY_Shift_L:
    case GDK_KEY_Shift_R:      return kKeyShift;
    case GDK_KEY_Control_L:
    case GDK_KEY_Control_R:    return kKeyControl;
    case GDK_KEY_Alt_L:
    case GDK_KEY_Alt_R:
    case GDK_KEY_Meta_L:
    case GDK_KEY_Meta_R:
    case GDK_KEY_ISO_Level3_Shift: return kKeyAlt;
    case GDK_KEY_Hiragana_Katakana:
    case GDK_KEY_Katakana:     return kKeyKana;
    case GDK_KEY_Kanji:        return kKeyKanji;
    case GDK_KEY_Escape:       return kKeyEscape;
    case GDK_KEY_Henkan:       return kKeyConvert;
    case GDK_KEY_Muhenkan:     return kKeyNoConvert;
    case GDK_KEY_space:        return kKeySpace;
    case GDK_KEY_Page_Up:
    case GDK_KEY_KP_Page_Up:   return kKeyPageUp;
    case GDK_KEY_Page_Down:
    case GDK_KEY_KP_Page_Down: return kKeyPageDown;
    case GDK_KEY_End:
    case GDK_KEY_KP_End:       return kKeyEnd;
    case GDK_KEY_Home:
    case GDK_KEY_KP_Home:      return kKeyHome;
    case GDK_KEY_Left:
    case GDK_KEY_KP_Left:      return kKeyLeftArrow;
    case GDK_KEY_Up:
    case GDK_KEY_KP_Up:        return kKeyUpArrow;
    case GDK_KEY_Right:
    case GDK_KEY_KP_Right:     return kKeyRightArrow;
    case GDK_KEY_Down:
    case GDK_KEY_KP_Down:      return kKeyDownArrow;
    case GDK_KEY_Insert:
    case GDK_KEY_KP_Insert:    return kKeyInsert;
    case GDK_KEY_Delete:
    case GDK_KEY_KP_Delete:    return kKeyDelete;
    case GDK_KEY_Super_L:      return kKeyLeftWindows;
    case GDK_KEY_Super_R:      return kKeyRightWindows;
    case GDK_KEY_Menu:         return kKeyApps;
    case GDK_KEY_KP_Multiply:  return kKeyNumpadMultiply;
    case GDK_KEY_KP_Add:       return kKeyNumpadPlus;
    case GDK_KEY_KP_Subtract:  return kKeyNumpadMinus;
    case GDK_KEY_KP_Decimal:
    case GDK_KEY_KP_Separator: return kKeyNumpadPeriod;
    case GDK_KEY_KP_Divide:    return kKeyNumpadDivide;
    case GDK_KEY_Num_Lock:     return kKeyNumLock;
    case GDK_KEY_Scroll_Lock:  return kKeyScrollLock;
    default:                   return kKeyUndefined;
    }
}

// The character Windows' WM_CHAR / WM_SYSCHAR would carry (0: none): control characters for
// Backspace, Tab, Return, Escape and Ctrl+letter.
UInt32 CharacterOf(GdkEventKey* e)
{
    switch (e->keyval) {
    case GDK_KEY_BackSpace:    return 8;
    case GDK_KEY_Tab:
    case GDK_KEY_ISO_Left_Tab: return 9;
    case GDK_KEY_Return:
    case GDK_KEY_KP_Enter:     return 13;
    case GDK_KEY_Escape:       return 27;
    default:                   break;
    }
    UInt32 c = gdk_keyval_to_unicode(e->keyval);
    if (c == 0 || c == 0x7f)
        return 0;
    if ((e->state & GDK_CONTROL_MASK) && !(e->state & GDK_MOD1_MASK)) {
        if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z'))
            return c & 0x1f;
        return 0;
    }
    return c;
}

void SendCharacter(Window* w, UInt32 c, UInt32 modifiers, bool repeat)
{
    if (c >= 0x10000) {
        c -= 0x10000;
        w->OnCharacter((UniChar16)(0xd800 + (c >> 10)), modifiers, repeat);
        w->OnCharacter((UniChar16)(0xdc00 + (c & 0x3ff)), modifiers, repeat);
    } else {
        w->OnCharacter((UniChar16)c, modifiers, repeat);
    }
}

// A native control (entry) has the keyboard: its keys stay with GTK.
bool ControlHasFocus(NativeWindow* nw)
{
    GtkWidget* focus = gtk_window_get_focus(GTK_WINDOW(nw->window));
    return focus && (GTK_IS_ENTRY(focus) || GTK_IS_TEXT_VIEW(focus));
}

gboolean OnKeyPress(GtkWidget*, GdkEventKey* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (ControlHasFocus(nw))
        return FALSE;
    if (nw->view)
        MakeCurrent(nw->view);
    UInt32 modifiers = Modifiers(e->state);
    UInt32 key = KeyOf(e);
    bool repeat = false;
    if (key != kKeyUndefined) {
        repeat = !s_pressedKeys.insert(key).second;
        nw->owner->OnKeyDown(key, modifiers, repeat);
    }
    if (!IsLive(nw))
        return TRUE;
    UInt32 c = CharacterOf(e);
    if (c)
        SendCharacter(nw->owner, c, modifiers, repeat);
    return TRUE;
}

gboolean OnKeyRelease(GtkWidget*, GdkEventKey* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    UInt32 key = KeyOf(e);
    bool wasDown = s_pressedKeys.erase(key) != 0;
    if (ControlHasFocus(nw) && !wasDown)
        return FALSE;
    if (nw->view)
        MakeCurrent(nw->view);
    if (key != kKeyUndefined)
        nw->owner->OnKeyUp(key, Modifiers(e->state));
    return TRUE;
}

UInt32 ButtonIndex(guint button)
{
    switch (button) {
    case 1:  return 0;
    case 3:  return 1;
    case 2:  return 2;
    default: return 3;
    }
}

gboolean OnButtonPress(GtkWidget* widget, GdkEventButton* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (e->type != GDK_BUTTON_PRESS)
        return TRUE;   // GDK's 2BUTTON / 3BUTTON: double clicks are told apart below
    UInt32 button = ButtonIndex(e->button);
    if (button > 2)
        return FALSE;
    gtk_widget_grab_focus(widget);
    if (nw->view)
        MakeCurrent(nw->view);
    GtkSettings* settings = gtk_widget_get_settings(widget);
    gint time = 400, distance = 5;
    g_object_get(settings, "gtk-double-click-time", &time, "gtk-double-click-distance", &distance, NULL);
    bool doubleClick = e->button == nw->lastPressButton && e->time - nw->lastPressTime <= (guint32)time &&
                       fabs(e->x - nw->lastPressX) <= distance && fabs(e->y - nw->lastPressY) <= distance;
    nw->lastPressButton = doubleClick ? 0 : e->button;
    nw->lastPressTime = e->time;
    nw->lastPressX = e->x;
    nw->lastPressY = e->y;
    nw->buttonsDown |= 1u << button;
    Vector2 position((Real32)e->x, (Real32)e->y);
    if (doubleClick)
        nw->owner->OnMouseDoubleClick(position, button);
    else
        nw->owner->OnMouseDown(position, button);
    return TRUE;
}

gboolean OnButtonRelease(GtkWidget*, GdkEventButton* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    UInt32 button = ButtonIndex(e->button);
    if (button > 2 || !(nw->buttonsDown & (1u << button)))
        return FALSE;
    nw->buttonsDown &= ~(1u << button);
    if (nw->view)
        MakeCurrent(nw->view);
    nw->owner->OnMouseUp(Vector2((Real32)e->x, (Real32)e->y), button);
    return TRUE;
}

gboolean OnMotion(GtkWidget*, GdkEventMotion* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (nw->view)
        MakeCurrent(nw->view);
    nw->owner->OnMouseMove(Vector2((Real32)e->x, (Real32)e->y));
    return TRUE;
}

// Leaving the view: the position far outside, as Window::TrackMouse does on Windows.
gboolean OnLeave(GtkWidget*, GdkEventCrossing* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (nw->buttonsDown || e->mode != GDK_CROSSING_NORMAL)
        return FALSE;
    if (nw->view)
        MakeCurrent(nw->view);
    nw->owner->OnMouseMove(Vector2(-30000.0f, -30000.0f));
    return FALSE;
}

gboolean OnScroll(GtkWidget*, GdkEventScroll* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (nw->view)
        MakeCurrent(nw->view);
    double dx = 0.0, dy = 0.0;
    switch (e->direction) {
    case GDK_SCROLL_UP:     dy = -1.0; break;
    case GDK_SCROLL_DOWN:   dy = 1.0; break;
    case GDK_SCROLL_LEFT:   dx = -1.0; break;
    case GDK_SCROLL_RIGHT:  dx = 1.0; break;
    case GDK_SCROLL_SMOOTH: gdk_event_get_scroll_deltas((GdkEvent*)e, &dx, &dy); break;
    default: break;
    }
    // Windows: +1 per notch away from the user (axis 0); the Mac's horizontal axis is 1
    if (fabs(dy) >= 1e-5)
        nw->owner->OnMouseWheel(0, (Real32)-dy);
    if (IsLive(nw) && fabs(dx) >= 1e-5)
        nw->owner->OnMouseWheel(1, (Real32)-dx);
    return TRUE;
}

gboolean OnFocus(GtkWidget*, GdkEventFocus* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    bool in = e->in != 0;
    if (nw->view)
        MakeCurrent(nw->view);
    if (!in) {
        // keys held while the focus leaves come up (Windows syncs them on activation)
        std::set<UInt32> held;
        held.swap(s_pressedKeys);
        for (std::set<UInt32>::iterator it = held.begin(); it != held.end() && IsLive(nw); ++it)
            nw->owner->OnKeyUp(*it, 0);
        if (!IsLive(nw))
            return FALSE;
    }
    GdkModifierType state = (GdkModifierType)0;
    gtk_get_current_event_state(&state);
    nw->owner->OnInput13(in, Modifiers(state), 0);
    if (!IsLive(nw))
        return FALSE;
    if (in && nw->view) {
        gint x = 0, y = 0;
        GdkWindow* gw = gtk_widget_get_window(nw->view->area);
        GdkSeat* seat = gdk_display_get_default_seat(gtk_widget_get_display(nw->window));
        if (gw && seat) {
            gdk_window_get_device_position(gw, gdk_seat_get_pointer(seat), &x, &y, 0);
            nw->owner->OnMouseMove(Vector2((Real32)x, (Real32)y));
        }
    }
    if (IsLive(nw))
        nw->owner->OnActivate(in);
    if (IsLive(nw))
        Invalidate(nw);
    return FALSE;
}

gboolean OnDelete(GtkWidget*, GdkEvent*, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    if (nw->modal)
        return TRUE;
    if (nw->view)
        MakeCurrent(nw->view);
    if (nw->owner->OnCloseRequest() && IsLive(nw)) {
        gtk_widget_hide(nw->window);
        nw->owner->OnClose();
    }
    return TRUE;   // the LEGO window destroys its native window (DestroyNativeWindow)
}

// A file dropped on the window: the first one, if it ends in .lxfml or .lxf, is opened (LDDView's
// drag operations on the Mac); not while modal.
void OnDragData(GtkWidget*, GdkDragContext* context, gint, gint, GtkSelectionData* data, guint, guint time,
                gpointer user)
{
    NativeWindow* nw = (NativeWindow*)user;
    bool ok = false;
    gchar** uris = gtk_selection_data_get_uris(data);
    Application* application = Application::GetInstance();
    if (uris && uris[0] && !nw->modal && application && !application->IsModal()) {
        gchar* file = g_filename_from_uri(uris[0], 0, 0);
        if (file) {
            size_t n = strlen(file);
            if ((n > 6 && strcmp(file + n - 6, ".lxfml") == 0) || (n > 4 && strcmp(file + n - 4, ".lxf") == 0)) {
                glong length = 0;
                gunichar2* u = g_utf8_to_utf16(file, -1, 0, &length, 0);
                String native((const UniChar16*)u, (UInt32)length);
                g_free(u);
                gtk_drag_finish(context, TRUE, FALSE, time);
                g_strfreev(uris);
                g_free(file);
                if (nw->view)
                    MakeCurrent(nw->view);
                Path path(String(), 0);
                path.Append(Platform::GetInstance()->NativeToVFSPath(native));
                application->OpenDocumentFile(path);
                return;
            }
            g_free(file);
        }
    }
    g_strfreev(uris);
    gtk_drag_finish(context, ok, FALSE, time);
}

void NotifyResize(NativeWindow* nw)
{
    gint w = 0, h = 0;
    gtk_window_get_size(GTK_WINDOW(nw->window), &w, &h);
    Vector2 client = GetClientSize(nw);
    nw->owner->OnResize(Vector2((Real32)w, (Real32)h), client);
}

void OnAllocate(GtkWidget*, GdkRectangle* a, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    nw->styleSize = Vector2((Real32)a->width, (Real32)a->height);
    if (nw->view && gtk_widget_get_realized(nw->view->area)) {
        MakeCurrent(nw->view);
    }
    NotifyResize(nw);
    if (IsLive(nw))
        Invalidate(nw);
}

gboolean OnConfigure(GtkWidget*, GdkEventConfigure* e, gpointer data)
{
    NativeWindow* nw = (NativeWindow*)data;
    nw->owner->OnMove(Vector2((Real32)e->x, (Real32)e->y));
    return FALSE;
}

// ---- menus -------------------------------------------------------------------------------------

guint KeyvalOf(UInt32 key)
{
    if (key >= kKeyQ && key <= kKeyP) {
        static const char kRow[] = "qwertyuiop";
        return GDK_KEY_a + (kRow[key - kKeyQ] - 'a');
    }
    if (key >= kKeyA && key <= kKeyL) {
        static const char kRow[] = "asdfghjkl";
        return GDK_KEY_a + (kRow[key - kKeyA] - 'a');
    }
    if (key >= kKeyZ && key <= kKeyM) {
        static const char kRow[] = "zxcvbnm";
        return GDK_KEY_a + (kRow[key - kKeyZ] - 'a');
    }
    if (key >= kKey1 && key <= kKey9)
        return GDK_KEY_1 + (key - kKey1);
    if (key == kKey0)
        return GDK_KEY_0;
    if (key >= kKeyF1 && key <= kKeyF10)
        return GDK_KEY_F1 + (key - kKeyF1);
    switch (key) {
    case kKeyF11:        return GDK_KEY_F11;
    case kKeyF12:        return GDK_KEY_F12;
    case kKeyDelete:     return GDK_KEY_Delete;
    case kKeyBackspace:  return GDK_KEY_BackSpace;
    case kKeyEscape:     return GDK_KEY_Escape;
    case kKeyReturn:     return GDK_KEY_Return;
    case kKeySpace:      return GDK_KEY_space;
    case kKeyTab:        return GDK_KEY_Tab;
    case kKeyHome:       return GDK_KEY_Home;
    case kKeyEnd:        return GDK_KEY_End;
    case kKeyPageUp:     return GDK_KEY_Page_Up;
    case kKeyPageDown:   return GDK_KEY_Page_Down;
    case kKeyInsert:     return GDK_KEY_Insert;
    case kKeyLeftArrow:  return GDK_KEY_Left;
    case kKeyRightArrow: return GDK_KEY_Right;
    case kKeyUpArrow:    return GDK_KEY_Up;
    case kKeyDownArrow:  return GDK_KEY_Down;
    case kKeyNumpadPlus: return GDK_KEY_KP_Add;
    case kKeyNumpadMinus: return GDK_KEY_KP_Subtract;
    case kKeyNumpadMultiply: return GDK_KEY_KP_Multiply;
    case kKeyNumpadDivide: return GDK_KEY_KP_Divide;
    case kKeyMinus:      return GDK_KEY_minus;
    case kKeyEqual:      return GDK_KEY_equal;
    case kKeyComma:      return GDK_KEY_comma;
    case kKeyPeriod:     return GDK_KEY_period;
    default:             return 0;
    }
}

std::string UTF8(const String& s)
{
    gchar* utf8 = g_utf16_to_utf8((const gunichar2*)s.GetBuffer(), (glong)s.GetLength(), 0, 0, 0);
    std::string r(utf8 ? utf8 : "");
    g_free(utf8);
    return r;
}

// The item's check mark follows its command (LEGOMenuWrapperObserver on the Mac).
class MenuItemObserver : public CommandObserver {
public:
    explicit MenuItemObserver(GtkWidget* item) : m_item(item) {}
    virtual void OnToggleChanged(bool toggled);
    GtkWidget* m_item;
};

struct MenuItemData {
    RefPointer<Command>  command;
    MenuItemObserver*    observer;
    Window*              window;
    NativeWindow*        native;
    bool                 updating;   // setting the check mark activates a GtkCheckMenuItem
};

void MenuItemObserver::OnToggleChanged(bool toggled)
{
    MenuItemData* d = (MenuItemData*)g_object_get_data(G_OBJECT(m_item), "ldd-item");
    if (!d || !GTK_IS_CHECK_MENU_ITEM(m_item))
        return;
    d->updating = true;
    gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(m_item), toggled);
    d->updating = false;
}

void OnMenuItemActivate(GtkMenuItem* item, gpointer)
{
    MenuItemData* d = (MenuItemData*)g_object_get_data(G_OBJECT(item), "ldd-item");
    if (!d || d->updating)
        return;
    if (GTK_IS_CHECK_MENU_ITEM(item)) {
        // GTK has flipped the mark; the command decides (OnToggleChanged sets it back)
        d->updating = true;
        gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), d->command->IsToggled());
        d->updating = false;
    }
    if (d->native && d->native->view)
        MakeCurrent(d->native->view);
    RefPointer<Command> command = d->command;
    command->Do();
}

void OnMenuItemDestroy(GtkWidget* item, gpointer)
{
    MenuItemData* d = (MenuItemData*)g_object_get_data(G_OBJECT(item), "ldd-item");
    if (!d)
        return;
    g_object_set_data(G_OBJECT(item), "ldd-item", 0);
    if (d->observer) {
        d->observer->Detach();
        d->observer->Release();
    }
    delete d;
}

// Before a menu opens: each item enabled as validateMenuItem: decides on the Mac (not while the
// window is modal; the command's enable state).
void OnMenuShow(GtkWidget* menu, gpointer)
{
    GList* children = gtk_container_get_children(GTK_CONTAINER(menu));
    for (GList* l = children; l; l = l->next) {
        GtkWidget* item = (GtkWidget*)l->data;
        MenuItemData* d = (MenuItemData*)g_object_get_data(G_OBJECT(item), "ldd-item");
        if (!d)
            continue;
        bool enabled = !(d->native && d->native->modal) && d->command->GetEnable();
        gtk_widget_set_sensitive(item, enabled);
        if (GTK_IS_CHECK_MENU_ITEM(item)) {
            d->updating = true;
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(item), d->command->IsToggled());
            d->updating = false;
        }
    }
    g_list_free(children);
}

void BuildMenu(const Menu* menu, GtkWidget* shell, Window* window, NativeWindow* nw)
{
    const BasicArray<RefPointer<MenuItem> >& items = menu->GetItems();
    for (UInt32 i = 0; i < items.GetSize(); ++i) {
        const MenuItem* item = items[i].Get();
        if (item->GetSubMenu() && !item->GetTitle().IsEmpty()) {
            GtkWidget* entry = gtk_menu_item_new_with_label(UTF8(item->GetTitle()).c_str());
            GtkWidget* submenu = gtk_menu_new();
            g_signal_connect(submenu, "show", G_CALLBACK(OnMenuShow), 0);
            BuildMenu(item->GetSubMenu(), submenu, window, nw);
            gtk_menu_item_set_submenu(GTK_MENU_ITEM(entry), submenu);
            gtk_menu_shell_append(GTK_MENU_SHELL(shell), entry);
            continue;
        }
        if (!item->GetCommand() || item->GetTitle().IsEmpty()) {
            gtk_menu_shell_append(GTK_MENU_SHELL(shell), gtk_separator_menu_item_new());
            continue;
        }
        Command* command = item->GetCommand();
        std::string title = UTF8(item->GetTitle());
        GtkWidget* entry = command->IsToggleable() ? gtk_check_menu_item_new_with_label(title.c_str())
                                                   : gtk_menu_item_new_with_label(title.c_str());
        guint keyval = KeyvalOf(item->GetKey());
        if (keyval) {
            UInt32 m = item->GetModifiers();
            GdkModifierType mods = (GdkModifierType)(((m & kModifierControl) ? GDK_CONTROL_MASK : 0) |
                                                     ((m & kModifierShift) ? GDK_SHIFT_MASK : 0) |
                                                     ((m & kModifierAlt) ? GDK_MOD1_MASK : 0));
            GtkWidget* label = gtk_bin_get_child(GTK_BIN(entry));
            if (GTK_IS_ACCEL_LABEL(label))
                gtk_accel_label_set_accel(GTK_ACCEL_LABEL(label), keyval, mods);   // shown only
        }
        MenuItemData* d = new MenuItemData;
        d->command = command;
        d->window = window;
        d->native = nw;
        d->updating = false;
        d->observer = new MenuItemObserver(entry);
        d->observer->AddRef();
        d->observer->Attach(command);
        g_object_set_data(G_OBJECT(entry), "ldd-item", d);
        if (command->IsToggleable()) {
            d->updating = true;
            gtk_check_menu_item_set_active(GTK_CHECK_MENU_ITEM(entry), command->IsToggled());
            d->updating = false;
        }
        g_signal_connect(entry, "activate", G_CALLBACK(OnMenuItemActivate), 0);
        g_signal_connect(entry, "destroy", G_CALLBACK(OnMenuItemDestroy), 0);
        gtk_menu_shell_append(GTK_MENU_SHELL(shell), entry);
    }
}

void UpdateTitle(NativeWindow* nw)
{
    std::string title = UTF8(nw->title);
    if (nw->modified)
        title = "*" + title;
    gtk_window_set_title(GTK_WINDOW(nw->window), title.c_str());
}

} // namespace

// ---- shell -------------------------------------------------------------------------------------

void Initialize(int* argc, char*** argv)
{
    if (s_initialized)
        return;
    // LDD's renderer is fixed-function: GDK's legacy GL flag gives GtkGLArea a compatibility
    // profile (GLX and EGL)
    setenv("GDK_GL", "legacy", 0);
    g_set_prgname("ldd64");
    g_set_application_name("LEGO Digital Designer");
    gtk_init(argc, argv);
    s_initialized = true;
    // the window icon: ldd64.png or ldd64.ico next to the executable (LDD64's icon,
    // src/Products/LDD/Resources/ldd64.ico, copied there by the build)
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n > 0) {
        exe[n] = 0;
        gchar* dir = g_path_get_dirname(exe);
        static const char* const kIcons[] = {"ldd64.png", "ldd64.ico"};
        for (size_t i = 0; i < sizeof(kIcons) / sizeof(kIcons[0]); ++i) {
            gchar* path = g_build_filename(dir, kIcons[i], NULL);
            bool ok = g_file_test(path, G_FILE_TEST_EXISTS) && gtk_window_set_default_icon_from_file(path, 0);
            g_free(path);
            if (ok)
                break;
        }
        g_free(dir);
    }
}

void Pump()
{
    if (!s_initialized)
        return;
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
    if (s_current)
        MakeCurrent(s_current);
}

void* CreateWindow(Window* owner, const WindowStyle& style, bool menuBar)
{
    NativeWindow* nw = new NativeWindow;
    nw->owner = owner;
    nw->view = 0;
    nw->modified = false;
    nw->modal = false;
    nw->hidden = style.hidden;
    nw->title = style.title;
    nw->styleSize = style.size;
    nw->lastPressTime = 0;
    nw->lastPressButton = 0;
    nw->lastPressX = nw->lastPressY = 0.0;
    nw->buttonsDown = 0;
    nw->window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    g_object_ref_sink(nw->window);
    nw->box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_container_add(GTK_CONTAINER(nw->window), nw->box);
    // the menu bar shows once SetMenu gives it items (the Mac's menus are the application's;
    // `menuBar` is the Windows window's menu flag)
    (void)menuBar;
    nw->menuBar = gtk_menu_bar_new();
    gtk_widget_set_no_show_all(nw->menuBar, TRUE);
    gtk_box_pack_start(GTK_BOX(nw->box), nw->menuBar, FALSE, FALSE, 0);
    nw->overlay = gtk_overlay_new();
    gtk_box_pack_start(GTK_BOX(nw->box), nw->overlay, TRUE, TRUE, 0);
    g_signal_connect(nw->window, "delete-event", G_CALLBACK(OnDelete), nw);
    g_signal_connect(nw->window, "key-press-event", G_CALLBACK(OnKeyPress), nw);
    g_signal_connect(nw->window, "key-release-event", G_CALLBACK(OnKeyRelease), nw);
    g_signal_connect(nw->window, "focus-in-event", G_CALLBACK(OnFocus), nw);
    g_signal_connect(nw->window, "focus-out-event", G_CALLBACK(OnFocus), nw);
    g_signal_connect(nw->window, "configure-event", G_CALLBACK(OnConfigure), nw);
    g_signal_connect(nw->overlay, "size-allocate", G_CALLBACK(OnAllocate), nw);
    static GtkTargetEntry kUris = {(gchar*)"text/uri-list", 0, 0};
    gtk_drag_dest_set(nw->window, GTK_DEST_DEFAULT_ALL, &kUris, 1, GDK_ACTION_COPY);
    g_signal_connect(nw->window, "drag-data-received", G_CALLBACK(OnDragData), nw);
    s_windows.push_back(nw);
    UpdateTitle(nw);
    ApplyStyle(nw, style);
    gtk_widget_show_all(nw->box);
    return nw;
}

void DestroyWindow(void* native)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (!nw)
        return;
    for (size_t i = 0; i < s_windows.size(); ++i)
        if (s_windows[i] == nw) {
            s_windows.erase(s_windows.begin() + i);
            break;
        }
    if (nw->view)
        DestroyGLView(nw);
    gtk_widget_destroy(nw->window);
    g_object_unref(nw->window);
    delete nw;
}

void ApplyStyle(void* native, const WindowStyle& style)
{
    NativeWindow* nw = (NativeWindow*)native;
    GtkWindow* w = GTK_WINDOW(nw->window);
    gtk_window_set_decorated(w, style.flag29 || style.flag28 ? TRUE : FALSE);
    gtk_window_set_resizable(w, style.flag2a || style.flag28 ? TRUE : FALSE);
    if (style.minimumSize.x > 0.0f || style.minimumSize.y > 0.0f)
        gtk_widget_set_size_request(nw->overlay, (gint)style.minimumSize.x, (gint)style.minimumSize.y);
    if (style.flag28) {
        gtk_window_fullscreen(w);
        return;
    }
    gtk_window_unfullscreen(w);
    int width = style.size.x > 1.0f ? (int)style.size.x : 640;
    int height = style.size.y > 1.0f ? (int)style.size.y : 480;
    gint menu = 0;
    if (nw->menuBar && gtk_widget_get_visible(nw->menuBar))
        gtk_widget_get_preferred_height(nw->menuBar, 0, &menu);
    nw->styleSize = Vector2((Real32)width, (Real32)(height - menu > 1 ? height - menu : 1));
    if (gtk_widget_get_realized(nw->window))
        gtk_window_resize(w, width, height);
    else
        gtk_window_set_default_size(w, width, height);
    gtk_window_move(w, (gint)style.position.x, (gint)style.position.y);   // (X11 only)
}

void SetTitle(void* native, const String& title)
{
    NativeWindow* nw = (NativeWindow*)native;
    nw->title = title;
    UpdateTitle(nw);
}

void SetModified(void* native, bool modified)
{
    NativeWindow* nw = (NativeWindow*)native;
    nw->modified = modified;
    UpdateTitle(nw);
}

void SetVisible(void* native, bool visible)
{
    NativeWindow* nw = (NativeWindow*)native;
    nw->hidden = !visible;
    if (visible) {
        gtk_widget_show(nw->window);
        gtk_window_present(GTK_WINDOW(nw->window));
    } else {
        gtk_widget_hide(nw->window);
    }
}

Vector2 GetClientSize(void* native)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (!nw)
        return Vector2(0.0f, 0.0f);
    return nw->styleSize;
}

void Invalidate(void* native)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (!nw || !nw->view)
        return;
    nw->view->needsPaint = true;
    gtk_gl_area_queue_render(GTK_GL_AREA(nw->view->area));
}

void SetModal(void* native, bool modal)
{
    NativeWindow* nw = (NativeWindow*)native;
    nw->modal = modal;
    gtk_window_set_deletable(GTK_WINDOW(nw->window), modal ? FALSE : TRUE);
}

void SetMenu(void* native, const Menu* menu, Window* window)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (!nw || !nw->menuBar)
        return;
    GList* children = gtk_container_get_children(GTK_CONTAINER(nw->menuBar));
    for (GList* l = children; l; l = l->next)
        gtk_widget_destroy((GtkWidget*)l->data);
    g_list_free(children);
    if (menu && menu->GetItems().GetSize()) {
        BuildMenu(menu, nw->menuBar, window, nw);
        gtk_widget_set_no_show_all(nw->menuBar, FALSE);
        gtk_widget_show_all(nw->menuBar);
    } else {
        gtk_widget_hide(nw->menuBar);
    }
}

bool IsKeyPressed(UInt32 key)
{
    return s_pressedKeys.count(key) != 0;
}

void SetCursor(void* cursor)
{
    s_cursor = (GdkCursor*)cursor;
    for (size_t i = 0; i < s_windows.size(); ++i) {
        NativeWindow* nw = s_windows[i];
        if (nw->view) {
            GdkWindow* gw = gtk_widget_get_window(nw->view->area);
            if (gw)
                gdk_window_set_cursor(gw, s_cursor);
        }
    }
}

void* LoadSystemCursor(const char* name)
{
    GdkDisplay* display = gdk_display_get_default();
    return display ? gdk_cursor_new_from_name(display, name) : 0;
}

void* LoadResourceCursor(UInt16 id)
{
    GdkDisplay* display = gdk_display_get_default();
    if (!display)
        return 0;
    char exe[4096];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0)
        return 0;
    exe[n] = 0;
    gchar* dir = g_path_get_dirname(exe);
    char file[64];
    snprintf(file, sizeof(file), "cursor_%u.cur", (unsigned)id);
    gchar* path = g_build_filename(dir, "Cursors", file, NULL);
    g_free(dir);
    GdkPixbuf* image = gdk_pixbuf_new_from_file(path, 0);
    g_free(path);
    if (!image)
        return 0;
    const char* xs = gdk_pixbuf_get_option(image, "x_hot");
    const char* ys = gdk_pixbuf_get_option(image, "y_hot");
    GdkCursor* cursor = gdk_cursor_new_from_pixbuf(display, image, xs ? atoi(xs) : 0, ys ? atoi(ys) : 0);
    g_object_unref(image);
    return cursor;
}

void ReleaseCursor(void* cursor)
{
    if (cursor)
        g_object_unref((GdkCursor*)cursor);
}

Rect GetVirtualDesktopRect()
{
    GdkDisplay* display = gdk_display_get_default();
    if (!display)
        return Rect(0.0f, 0.0f, 0.0f, 0.0f);
    GdkRectangle all = {0, 0, 0, 0};
    int n = gdk_display_get_n_monitors(display);
    for (int i = 0; i < n; ++i) {
        GdkRectangle g;
        gdk_monitor_get_geometry(gdk_display_get_monitor(display, i), &g);
        if (i == 0)
            all = g;
        else
            gdk_rectangle_union(&all, &g, &all);
    }
    return Rect(Vector2((Real32)all.x, (Real32)all.y), Vector2((Real32)all.width, (Real32)all.height));
}

GtkWidget* GetGtkWindow(void* native)
{
    return native ? ((NativeWindow*)native)->window : 0;
}

GtkWidget* GetMainGtkWindow()
{
    for (size_t i = s_windows.size(); i-- > 0;)
        if (gtk_widget_get_visible(s_windows[i]->window))
            return s_windows[i]->window;
    return s_windows.empty() ? 0 : s_windows.back()->window;
}

bool CreateGLView(void* native, Window* owner)
{
    NativeWindow* nw = (NativeWindow*)native;
    (void)owner;
    if (!nw || nw->view)
        return nw != 0;
    _CGLContextObject* view = new _CGLContextObject;
    memset(view, 0, sizeof(*view));
    view->native = nw;
    view->area = gtk_gl_area_new();
    view->needsPaint = true;
    gtk_gl_area_set_auto_render(GTK_GL_AREA(view->area), FALSE);
    gtk_gl_area_set_has_alpha(GTK_GL_AREA(view->area), FALSE);
    gtk_widget_set_can_focus(view->area, TRUE);
    gtk_widget_add_events(view->area, GDK_POINTER_MOTION_MASK | GDK_BUTTON_PRESS_MASK | GDK_BUTTON_RELEASE_MASK |
                                          GDK_SCROLL_MASK | GDK_SMOOTH_SCROLL_MASK | GDK_LEAVE_NOTIFY_MASK |
                                          GDK_KEY_PRESS_MASK | GDK_KEY_RELEASE_MASK);
    g_signal_connect(view->area, "render", G_CALLBACK(OnRender), view);
    g_signal_connect(view->area, "resize", G_CALLBACK(OnAreaResize), view);
    g_signal_connect(view->area, "button-press-event", G_CALLBACK(OnButtonPress), nw);
    g_signal_connect(view->area, "button-release-event", G_CALLBACK(OnButtonRelease), nw);
    g_signal_connect(view->area, "motion-notify-event", G_CALLBACK(OnMotion), nw);
    g_signal_connect(view->area, "leave-notify-event", G_CALLBACK(OnLeave), nw);
    g_signal_connect(view->area, "scroll-event", G_CALLBACK(OnScroll), nw);
    gtk_container_add(GTK_CONTAINER(nw->overlay), view->area);
    gtk_widget_show(view->area);
    nw->view = view;
    gtk_widget_realize(nw->window);
    gtk_widget_realize(view->area);
    if (gtk_gl_area_get_error(GTK_GL_AREA(view->area)) || !gtk_widget_get_realized(view->area)) {
        GError* error = gtk_gl_area_get_error(GTK_GL_AREA(view->area));
        g_warning("LDD: no OpenGL context: %s", error ? error->message : "the view was not realized");
        DestroyGLView(nw);
        return false;
    }
    MakeCurrent(view);
    if (s_cursor) {
        GdkWindow* gw = gtk_widget_get_window(view->area);
        if (gw)
            gdk_window_set_cursor(gw, s_cursor);
    }
    gtk_widget_grab_focus(view->area);
    return true;
}

void DestroyGLView(void* native)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (!nw || !nw->view)
        return;
    _CGLContextObject* view = nw->view;
    if (gtk_widget_get_realized(view->area)) {
        gtk_gl_area_make_current(GTK_GL_AREA(view->area));
        if (view->framebuffer) {
            glDeleteFramebuffers(1, &view->framebuffer);
            glDeleteRenderbuffers(1, &view->color);
            glDeleteRenderbuffers(1, &view->depthStencil);
        }
    }
    if (s_current == view)
        s_current = 0;
    gtk_widget_destroy(view->area);
    nw->view = 0;
    delete view;
}

void AddControl(void* native, GtkWidget* widget)
{
    NativeWindow* nw = (NativeWindow*)native;
    gtk_widget_set_halign(widget, GTK_ALIGN_START);
    gtk_widget_set_valign(widget, GTK_ALIGN_START);
    gtk_overlay_add_overlay(GTK_OVERLAY(nw->overlay), widget);
}

void PlaceControl(GtkWidget* widget, const Rect& frame)
{
    gtk_widget_set_margin_start(widget, (gint)frame.GetMin().x);
    gtk_widget_set_margin_top(widget, (gint)frame.GetMin().y);
    gtk_widget_set_size_request(widget, (gint)frame.GetWidth(), (gint)frame.GetHeight());
}

void FocusControl(void* native, GtkWidget* widget, bool focus)
{
    NativeWindow* nw = (NativeWindow*)native;
    if (focus && widget)
        gtk_widget_grab_focus(widget);
    else if (nw && nw->view)
        gtk_widget_grab_focus(nw->view->area);
}

} // namespace LinuxShell
} // namespace LEGO

// ---- CGL (OpenGL/OpenGL.h) and the framebuffer redirection (OpenGL/gl.h) ------------------------

CGLContextObj CGLGetCurrentContext()
{
    return LEGO::LinuxShell::s_current;
}

CGLError CGLSetCurrentContext(CGLContextObj context)
{
    if (!context) {
        gdk_gl_context_clear_current();
        LEGO::LinuxShell::s_current = 0;
        return kCGLNoError;
    }
    LEGO::LinuxShell::MakeCurrent(context);
    return kCGLNoError;
}

// OpenGLRender::Present: the frame is shown by the area's next draw (inside the render signal the
// copy follows it).
CGLError CGLFlushDrawable(CGLContextObj context)
{
    if (!context)
        return kCGLBadContext;
    context->hasFrame = true;
    if (!context->inRender)
        gtk_gl_area_queue_render(GTK_GL_AREA(context->area));
    return kCGLNoError;
}

void LinuxGLBindFramebuffer(GLenum target, GLuint framebuffer)
{
    _CGLContextObject* view = LEGO::LinuxShell::s_current;
    if (view) {
        if (target == GL_FRAMEBUFFER || target == GL_DRAW_FRAMEBUFFER)
            view->bound = framebuffer;
        if (framebuffer == 0) {
            LEGO::LinuxShell::EnsureFramebuffer(view);
            framebuffer = view->framebuffer;
        }
    }
    glBindFramebuffer(target, framebuffer);
}

namespace {
bool WindowFramebufferBound(GLenum which)
{
    _CGLContextObject* view = LEGO::LinuxShell::s_current;
    if (!view || !view->framebuffer)
        return false;
    GLint bound = 0;
    glGetIntegerv(which, &bound);
    return (GLuint)bound == view->framebuffer;
}

GLenum WindowBuffer(GLenum mode)
{
    switch (mode) {
    case GL_BACK:
    case GL_FRONT:
    case GL_BACK_LEFT:
    case GL_FRONT_LEFT:
    case GL_FRONT_AND_BACK:
    case GL_LEFT:
        return GL_COLOR_ATTACHMENT0;
    default:
        return mode;
    }
}
} // namespace

void LinuxGLReadBuffer(GLenum mode)
{
    if (WindowFramebufferBound(GL_READ_FRAMEBUFFER_BINDING))
        mode = WindowBuffer(mode);
    glReadBuffer(mode);
}

void LinuxGLDrawBuffer(GLenum mode)
{
    if (WindowFramebufferBound(GL_DRAW_FRAMEBUFFER_BINDING))
        mode = WindowBuffer(mode);
    glDrawBuffer(mode);
}
#endif
