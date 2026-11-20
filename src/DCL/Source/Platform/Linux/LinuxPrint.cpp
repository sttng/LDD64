// LDD64 Linux platform layer (not restored code): printing of LinuxShell.h on GTK's Unix print
// dialog and print jobs (CUPS), drawn with cairo.
#if defined(LDD_LINUX)
#include "LinuxShell.h"
#include <gtk/gtk.h>
#include <gtk/gtkunixprint.h>
#include <string>

namespace LEGO {
namespace LinuxShell {

namespace {
struct PrintSession {
    GtkPrinter*       printer;
    GtkPrintSettings* settings;
    GtkPageSetup*     pageSetup;
    GtkPrintJob*      job;
    cairo_t*          cr;
};

std::string UTF8(const String& s)
{
    gchar* utf8 = g_utf16_to_utf8((const gunichar2*)s.GetBuffer(), (glong)s.GetLength(), 0, 0, 0);
    std::string r(utf8 ? utf8 : "");
    g_free(utf8);
    return r;
}

void OnJobSent(GtkPrintJob*, gpointer, const GError* error)
{
    if (error)
        g_warning("LDD: printing failed: %s", error->message);
}

void Drop(PrintSession* s)
{
    if (s->cr) {
        cairo_destroy(s->cr);
        s->cr = 0;
    }
    if (s->job) {
        g_object_unref(s->job);
        s->job = 0;
    }
}
} // namespace

void* PrintCreate()
{
    PrintSession* s = new PrintSession;
    s->printer = 0;
    s->settings = gtk_print_settings_new();
    s->pageSetup = gtk_page_setup_new();
    s->job = 0;
    s->cr = 0;
    return s;
}

void PrintRelease(void* session)
{
    PrintSession* s = (PrintSession*)session;
    if (!s)
        return;
    Drop(s);
    if (s->printer)
        g_object_unref(s->printer);
    g_object_unref(s->settings);
    g_object_unref(s->pageSetup);
    delete s;
}

bool PrintDialog(void* session, bool& accepted)
{
    PrintSession* s = (PrintSession*)session;
    accepted = false;
    if (!s)
        return false;
    GtkWidget* parent = GetMainGtkWindow();
    GtkWidget* dialog = gtk_print_unix_dialog_new(0, parent ? GTK_WINDOW(parent) : 0);
    GtkPrintUnixDialog* d = GTK_PRINT_UNIX_DIALOG(dialog);
    gtk_print_unix_dialog_set_settings(d, s->settings);
    gtk_print_unix_dialog_set_page_setup(d, s->pageSetup);
    // (as the Windows dialog: no selection, no page ranges; copies by the printer)
    gtk_print_unix_dialog_set_manual_capabilities(d, (GtkPrintCapabilities)(GTK_PRINT_CAPABILITY_COPIES |
                                                                            GTK_PRINT_CAPABILITY_COLLATE |
                                                                            GTK_PRINT_CAPABILITY_GENERATE_PDF));
    gtk_print_unix_dialog_set_embed_page_setup(d, TRUE);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    if (response == GTK_RESPONSE_OK) {
        GtkPrinter* printer = gtk_print_unix_dialog_get_selected_printer(d);
        if (printer) {
            if (s->printer)
                g_object_unref(s->printer);
            s->printer = GTK_PRINTER(g_object_ref(printer));
            GtkPrintSettings* settings = gtk_print_unix_dialog_get_settings(d);
            g_object_unref(s->settings);
            s->settings = settings;
            GtkPageSetup* setup = gtk_print_unix_dialog_get_page_setup(d);
            g_object_unref(s->pageSetup);
            s->pageSetup = GTK_PAGE_SETUP(g_object_ref(setup));
            accepted = true;
        }
    }
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
    return true;
}

bool PrintBegin(void* session, const String& name)
{
    PrintSession* s = (PrintSession*)session;
    if (!s || !s->printer)
        return false;
    Drop(s);
    std::string title = UTF8(name);
    s->job = gtk_print_job_new(title.empty() ? "LEGO Digital Designer" : title.c_str(), s->printer, s->settings,
                               s->pageSetup);
    GError* error = 0;
    cairo_surface_t* surface = gtk_print_job_get_surface(s->job, &error);
    if (!surface) {
        if (error) {
            g_warning("LDD: cannot print: %s", error->message);
            g_error_free(error);
        }
        Drop(s);
        return false;
    }
    s->cr = cairo_create(surface);
    return true;
}

void PrintEnd(void* session)
{
    PrintSession* s = (PrintSession*)session;
    if (!s || !s->job)
        return;
    cairo_surface_finish(cairo_get_target(s->cr));
    cairo_destroy(s->cr);
    s->cr = 0;
    GtkPrintJob* job = s->job;
    s->job = 0;
    gtk_print_job_send(job, OnJobSent, 0, (GDestroyNotify)0);   // (the job keeps itself until sent)
    g_object_unref(job);
}

void PrintAbort(void* session)
{
    PrintSession* s = (PrintSession*)session;
    if (s)
        Drop(s);
}

void PrintBeginPage(void* session)
{
    PrintSession* s = (PrintSession*)session;
    if (!s || !s->cr)
        return;
    cairo_save(s->cr);
    // the origin at the top left of the printable area, clipped to it
    double left = gtk_page_setup_get_left_margin(s->pageSetup, GTK_UNIT_POINTS);
    double top = gtk_page_setup_get_top_margin(s->pageSetup, GTK_UNIT_POINTS);
    cairo_translate(s->cr, left, top);
    cairo_rectangle(s->cr, 0, 0, gtk_page_setup_get_page_width(s->pageSetup, GTK_UNIT_POINTS),
                    gtk_page_setup_get_page_height(s->pageSetup, GTK_UNIT_POINTS));
    cairo_clip(s->cr);
}

void PrintEndPage(void* session)
{
    PrintSession* s = (PrintSession*)session;
    if (!s || !s->cr)
        return;
    cairo_restore(s->cr);
    cairo_show_page(s->cr);
}

void PrintPageSize(void* session, Int32& width, Int32& height)
{
    PrintSession* s = (PrintSession*)session;
    width = height = 0;
    if (!s)
        return;
    width = (Int32)gtk_page_setup_get_page_width(s->pageSetup, GTK_UNIT_POINTS);
    height = (Int32)gtk_page_setup_get_page_height(s->pageSetup, GTK_UNIT_POINTS);
}

bool PrintDrawBitmap(void* session, Int32 x, Int32 y, Int32 width, Int32 height, const UInt8* data,
                     UInt32 bitmapWidth, UInt32 bitmapHeight, Int32 pitch, UInt32 bitsPerPixel)
{
    PrintSession* s = (PrintSession*)session;
    if (!s || !s->cr || !data || bitmapWidth == 0 || bitmapHeight == 0 || (bitsPerPixel != 24 && bitsPerPixel != 32))
        return false;
    cairo_surface_t* image = cairo_image_surface_create(CAIRO_FORMAT_RGB24, (int)bitmapWidth, (int)bitmapHeight);
    cairo_surface_flush(image);
    unsigned char* out = cairo_image_surface_get_data(image);
    int stride = cairo_image_surface_get_stride(image);
    for (UInt32 row = 0; row < bitmapHeight; ++row) {
        const UInt8* in = data + (ptrdiff_t)row * pitch;
        guint32* o = (guint32*)(out + (ptrdiff_t)row * stride);
        for (UInt32 col = 0; col < bitmapWidth; ++col) {
            UInt32 r, g, b;
            if (bitsPerPixel == 24) {
                r = in[col * 3];
                g = in[col * 3 + 1];
                b = in[col * 3 + 2];
            } else {
                b = in[col * 4];
                g = in[col * 4 + 1];
                r = in[col * 4 + 2];
            }
            o[col] = (r << 16) | (g << 8) | b;
        }
    }
    cairo_surface_mark_dirty(image);
    cairo_save(s->cr);
    cairo_translate(s->cr, x, y);
    cairo_scale(s->cr, (double)width / bitmapWidth, (double)height / bitmapHeight);
    cairo_set_source_surface(s->cr, image, 0, 0);
    cairo_paint(s->cr);
    cairo_restore(s->cr);
    cairo_surface_destroy(image);
    return true;
}

} // namespace LinuxShell
} // namespace LEGO
#endif
