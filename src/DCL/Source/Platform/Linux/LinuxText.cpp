// LDD64 Linux platform layer (not restored code): LinuxText.h on Pango / cairo.
#if defined(LDD_LINUX)
#include "LinuxText.h"
#include <gtk/gtk.h>
#include <pango/pangocairo.h>
#include <mutex>
#include <string>
#include <vector>

namespace LEGO {
namespace LinuxText {

namespace {
// Pango contexts and layouts are not thread-safe, and LDD lays text out on its worker threads too.
std::recursive_mutex& Lock()
{
    static std::recursive_mutex m;
    return m;
}

PangoContext* Context()
{
    static PangoContext* context = 0;
    if (!context) {
        PangoFontMap* map = pango_cairo_font_map_new();
        pango_cairo_font_map_set_resolution(PANGO_CAIRO_FONT_MAP(map), 96.0);
        context = pango_font_map_create_context(map);
        cairo_font_options_t* options = cairo_font_options_create();
        cairo_font_options_set_antialias(options, CAIRO_ANTIALIAS_GRAY);
        cairo_font_options_set_hint_style(options, CAIRO_HINT_STYLE_SLIGHT);
        cairo_font_options_set_hint_metrics(options, CAIRO_HINT_METRICS_OFF);
        pango_cairo_context_set_font_options(context, options);
        cairo_font_options_destroy(options);
        g_object_unref(map);
    }
    return context;
}

// The desktop's UI font family (GTK's gtk-font-name, e.g. "Ubuntu 11" -> "Ubuntu"); "Sans" without.
std::string Family()
{
    static std::string family;
    if (family.empty()) {
        family = "Sans";
        GtkSettings* settings = gtk_settings_get_default();   // 0 before gtk_init
        if (settings) {
            gchar* name = 0;
            g_object_get(settings, "gtk-font-name", &name, NULL);
            if (name) {
                PangoFontDescription* d = pango_font_description_from_string(name);
                const char* f = pango_font_description_get_family(d);
                if (f && *f)
                    family = f;
                pango_font_description_free(d);
                g_free(name);
            }
        }
    }
    return family;
}
} // namespace

struct Font {
    PangoFontDescription* description;
    UInt32 size;
};

struct Frame {
    PangoLayout* layout;
    UInt32 alignment;
    Real32 width;     // alignment width
};

Font* CreateFont(UInt32 size, bool bold, bool italic, UInt32 defaultSize)
{
    std::lock_guard<std::recursive_mutex> lock(Lock());
    Font* font = new Font;
    font->size = size ? size : defaultSize;
    font->description = pango_font_description_new();
    pango_font_description_set_family(font->description, Family().c_str());
    pango_font_description_set_absolute_size(font->description, (double)font->size * PANGO_SCALE);
    pango_font_description_set_weight(font->description, bold ? PANGO_WEIGHT_BOLD : PANGO_WEIGHT_NORMAL);
    pango_font_description_set_style(font->description, italic ? PANGO_STYLE_ITALIC : PANGO_STYLE_NORMAL);
    return font;
}

void DestroyFont(Font* font)
{
    if (!font)
        return;
    std::lock_guard<std::recursive_mutex> lock(Lock());
    pango_font_description_free(font->description);
    delete font;
}

UInt32 GetFontSize(const Font* font)
{
    return font ? font->size : 0;
}

const void* GetFontDescription(const Font* font)
{
    return font ? font->description : 0;
}

Frame* CreateFrame(const Font* font, const String& text, Real32 maxWidth, bool truncate,
                   UInt32 horizontalAlignment)
{
    std::lock_guard<std::recursive_mutex> lock(Lock());
    Frame* frame = new Frame;
    frame->alignment = horizontalAlignment;
    frame->layout = pango_layout_new(Context());
    if (font)
        pango_layout_set_font_description(frame->layout, font->description);
    gchar* utf8 = g_utf16_to_utf8((const gunichar2*)text.GetBuffer(), (glong)text.GetLength(), 0, 0, 0);
    pango_layout_set_text(frame->layout, utf8 ? utf8 : "", -1);
    g_free(utf8);
    if (maxWidth > 0.0f) {
        pango_layout_set_width(frame->layout, (int)(maxWidth * PANGO_SCALE));
        if (truncate) {
            pango_layout_set_ellipsize(frame->layout, PANGO_ELLIPSIZE_END);
            pango_layout_set_height(frame->layout, -1);   // one line per paragraph
        } else {
            pango_layout_set_wrap(frame->layout, PANGO_WRAP_WORD_CHAR);
        }
    }
    Real32 height;
    GetFrameSize(frame, frame->width, height);
    return frame;
}

void DestroyFrame(Frame* frame)
{
    if (!frame)
        return;
    std::lock_guard<std::recursive_mutex> lock(Lock());
    g_object_unref(frame->layout);
    delete frame;
}

void GetFrameSize(const Frame* frame, Real32& width, Real32& height)
{
    width = height = 0.0f;
    if (!frame)
        return;
    std::lock_guard<std::recursive_mutex> lock(Lock());
    PangoRectangle logical;
    pango_layout_get_extents(frame->layout, 0, &logical);
    // the widest line (the logical rectangle of a wrapped layout is as wide as its width)
    Real32 widest = 0.0f;
    PangoLayoutIter* it = pango_layout_get_iter(frame->layout);
    do {
        PangoRectangle line;
        pango_layout_iter_get_line_extents(it, 0, &line);
        PangoLayoutLine* l = pango_layout_iter_get_line_readonly(it);
        PangoRectangle own;
        pango_layout_line_get_extents(l, 0, &own);
        if ((Real32)own.width / PANGO_SCALE > widest)
            widest = (Real32)own.width / PANGO_SCALE;
    } while (pango_layout_iter_next_line(it));
    pango_layout_iter_free(it);
    width = widest;
    height = (Real32)logical.height / PANGO_SCALE;
}

void SetFrameWidth(Frame* frame, Real32 width)
{
    if (frame)
        frame->width = width;
}

void DrawFrame(const Frame* frame, UInt8* pixels, UInt32 width, UInt32 height, Int32 pitch,
               UInt32 bytesPerPixel, UInt32 channel, Int32 x, Int32 y)
{
    if (!frame || !pixels)
        return;
    std::lock_guard<std::recursive_mutex> lock(Lock());
    Real32 fw, fh;
    GetFrameSize(frame, fw, fh);
    int sw = (int)(frame->width > fw ? frame->width : fw) + 2;
    int sh = (int)fh + 2;
    if (sw <= 0 || sh <= 0)
        return;
    cairo_surface_t* surface = cairo_image_surface_create(CAIRO_FORMAT_A8, sw, sh);
    cairo_t* cr = cairo_create(surface);
    // each line placed by the alignment inside the frame width
    PangoLayoutIter* it = pango_layout_get_iter(frame->layout);
    do {
        PangoLayoutLine* line = pango_layout_iter_get_line_readonly(it);
        PangoRectangle own;
        pango_layout_line_get_extents(line, 0, &own);
        double lineWidth = (double)own.width / PANGO_SCALE;
        double dx = 0.0;
        if (frame->alignment == 1)
            dx = frame->width - lineWidth;
        else if (frame->alignment == 2)
            dx = (frame->width - lineWidth) * 0.5;
        int baseline = pango_layout_iter_get_baseline(it);
        cairo_move_to(cr, dx - (double)own.x / PANGO_SCALE, (double)baseline / PANGO_SCALE);
        pango_cairo_show_layout_line(cr, line);
    } while (pango_layout_iter_next_line(it));
    pango_layout_iter_free(it);
    cairo_destroy(cr);
    cairo_surface_flush(surface);
    const UInt8* coverage = cairo_image_surface_get_data(surface);
    int stride = cairo_image_surface_get_stride(surface);
    for (int row = 0; row < sh; ++row) {
        Int32 ty = y + row;
        if (ty < 0 || ty >= (Int32)height)
            continue;
        UInt8* out = pixels + (ptrdiff_t)ty * pitch;
        const UInt8* in = coverage + (ptrdiff_t)row * stride;
        for (int col = 0; col < sw; ++col) {
            Int32 tx = x + col;
            if (tx < 0 || tx >= (Int32)width || !in[col])
                continue;
            UInt8& p = out[(ptrdiff_t)tx * bytesPerPixel + channel];
            unsigned v = (unsigned)p + in[col];
            p = (UInt8)(v > 255 ? 255 : v);
        }
    }
    cairo_surface_destroy(surface);
}

} // namespace LinuxText
} // namespace LEGO
#endif
