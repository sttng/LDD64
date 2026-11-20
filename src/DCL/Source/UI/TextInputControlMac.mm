// LEGO Digital Designer — restored source
// LEGO::TextInputControl, Mac part (TU 325, Mac 0x2d80e0-0x2daadb): the NSTextField, its target /
// delegate LEGOTextInputControlTarget and the LEGOReal32 / Int / UIntNumberFormatter classes.
//
// Objective-C sits behind __OBJC__. LDD64 Linux (LDD_LINUX, not restored code): a GtkEntry
// (LinuxShell::AddControl) shown while the field has the keyboard focus, otherwise the text drawn
// in GL (m_textLayout), as on Windows; same notifications and number filters as the Mac's.
// HOST-ONLY (not restored code): the plain C++ test build keeps a HostTextField in m_field.
#if !defined(_WIN32)
#include "TextInputControl.h"
#include "ControlManager.h"
#include "Window.h"
#include "FontPlatformFactory.h"
#include "Render.h"

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>

// ===========================================================================================
// LEGOTextInputControlTarget — target (action textEntered:) and delegate of the NSTextField.
// Instance size 8 {isa, control +4}.
// ===========================================================================================
@interface LEGOTextInputControlTarget : NSObject <NSTextFieldDelegate> {   // (protocol: 10.6+ SDKs)
    LEGO::TextInputControl* control;
}
- (id)initWithTextInputControl:(LEGO::TextInputControl*)textInputControl;
- (void)textEntered:(id)sender;
- (void)controlTextDidChange:(NSNotification*)notification;
@end

@implementation LEGOTextInputControlTarget

// Mac 0x2d80e0
- (id)initWithTextInputControl:(LEGO::TextInputControl*)textInputControl
{
    self = [super init];
    if (self)
        control = textInputControl;
    return self;
}

// Mac 0x2d8120
- (void)textEntered:(id)sender
{
    (void)sender;
    control->NotifyTextEntered();
}

// Mac 0x2d83d0
- (void)controlTextDidChange:(NSNotification*)notification
{
    (void)notification;
    control->NotifyTextChanged();
}

@end

// ===========================================================================================
// Number formatters: isPartialStringValid accepts what may become a number while typing.
// ===========================================================================================

// Instance size 72 {NSNumberFormatter, decimalseperator +0x40, decimalcharacters +0x44}.
@interface LEGOReal32NumberFormatter : NSNumberFormatter {
    NSString*       decimalseperator;
    NSCharacterSet* decimalcharacters;
}
@end

@implementation LEGOReal32NumberFormatter

// Mac 0x2d8680
- (id)init
{
    self = [super init];
    if (self) {
        decimalseperator = [[NSLocale currentLocale] objectForKey:NSLocaleDecimalSeparator];
        [decimalseperator retain];
        decimalcharacters = [[NSCharacterSet characterSetWithCharactersInString:
            [decimalseperator stringByAppendingString:@"0123456789"]] retain];
        // NOTE: original quirk: the separator is released again here (and once more in
        // dealloc); it stays valid only through the locale's own reference.
        [decimalseperator release];
    }
    return self;
}

// Mac 0x2d8770
- (void)dealloc
{
    [decimalcharacters release];
    [decimalseperator release];
    [super dealloc];
}

// Mac 0x2d87d0: an optional leading '-', at most one decimal separator, digits.
- (BOOL)isPartialStringValid:(NSString*)partialString newEditingString:(NSString**)newString
            errorDescription:(NSString**)error
{
    (void)newString;
    (void)error;
    if ([partialString hasPrefix:@"-"]) {
        if ([partialString length] == 1)
            return YES;
        partialString = [partialString substringFromIndex:1];
    }
    NSRange first = [partialString rangeOfString:decimalseperator];
    NSRange last = [partialString rangeOfString:decimalseperator options:NSBackwardsSearch];
    if (first.location != last.location)
        return NO;
    NSScanner* scanner = [NSScanner localizedScannerWithString:partialString];
    [scanner scanCharactersFromSet:decimalcharacters intoString:NULL];
    return [scanner isAtEnd];
}

@end

@interface LEGOIntNumberFormatter : NSNumberFormatter
@end

@implementation LEGOIntNumberFormatter

// Mac 0x2d88c0: an optional leading '-', then digits.
- (BOOL)isPartialStringValid:(NSString*)partialString newEditingString:(NSString**)newString
            errorDescription:(NSString**)error
{
    (void)newString;
    (void)error;
    if ([partialString hasPrefix:@"-"]) {
        if ([partialString length] == 1)
            return YES;
        partialString = [partialString substringFromIndex:1];
    }
    NSScanner* scanner = [NSScanner localizedScannerWithString:partialString];
    [scanner scanCharactersFromSet:[NSCharacterSet decimalDigitCharacterSet] intoString:NULL];
    return [scanner isAtEnd];
}

@end

@interface LEGOUIntNumberFormatter : NSNumberFormatter
@end

@implementation LEGOUIntNumberFormatter

// Mac 0x2d8980: digits only.
- (BOOL)isPartialStringValid:(NSString*)partialString newEditingString:(NSString**)newString
            errorDescription:(NSString**)error
{
    (void)newString;
    (void)error;
    NSScanner* scanner = [NSScanner localizedScannerWithString:partialString];
    [scanner scanCharactersFromSet:[NSCharacterSet decimalDigitCharacterSet] intoString:NULL];
    return [scanner isAtEnd];
}

@end

// LDDView (TU 156) answers getNativeView with the view the native controls go into.
@interface NSView (LEGOTextInputControlNativeView)
- (NSView*)getNativeView;
@end

#define LEGO_FIELD ((NSTextField*)m_field)

#elif defined(LDD_LINUX) // ---------------------------------------------------------------------
#include "LinuxShell.h"
#include "LinuxText.h"
#include "TextLayout.h"
#include "ObjectFactory.h"
#include <gtk/gtk.h>
#include <langinfo.h>
#include <locale.h>
#include <math.h>
#include <string>

namespace {

// The NSTextField look of a borderless field: no frame, no padding beyond 2 points, white.
void InstallFieldStyle()
{
    static bool installed = false;
    if (installed)
        return;
    installed = true;
    GtkCssProvider* css = gtk_css_provider_new();
    gtk_css_provider_load_from_data(css,
        "entry.ldd-field { min-height: 0; padding: 0; border: none; border-radius: 0;"
        " box-shadow: none; background-color: #ffffff; color: #000000; }", -1, 0);
    gtk_style_context_add_provider_for_screen(gdk_screen_get_default(), GTK_STYLE_PROVIDER(css),
                                              GTK_STYLE_PROVIDER_PRIORITY_APPLICATION);
    g_object_unref(css);
}

std::string UTF8Of(const LEGO::String& s)
{
    gchar* utf8 = g_utf16_to_utf8((const gunichar2*)s.GetBuffer(), (glong)s.GetLength(), 0, 0, 0);
    std::string r(utf8 ? utf8 : "");
    g_free(utf8);
    return r;
}

LEGO::String StringOf(const char* utf8)
{
    glong n = 0;
    gunichar2* u = g_utf8_to_utf16(utf8 ? utf8 : "", -1, 0, &n, 0);
    LEGO::String r((const LEGO::UniChar16*)u, (LEGO::UInt32)n);
    g_free(u);
    return r;
}

// The user's decimal separator (NSLocale's; LC_NUMERIC itself stays "C").
std::string DecimalSeparator()
{
    static std::string separator;
    if (separator.empty()) {
        separator = ".";
        locale_t user = newlocale(LC_NUMERIC_MASK, "", (locale_t)0);
        if (user) {
            const char* radix = nl_langinfo_l(RADIXCHAR, user);
            if (radix && *radix)
                separator = radix;
            freelocale(user);
        }
    }
    return separator;
}

// isPartialStringValid: of the Mac's LEGOInt / UInt / Real32NumberFormatter.
bool IsPartialNumber(const std::string& text, LEGO::UInt32 type)
{
    using LEGO::TextInputControl;
    std::string s = text;
    if (type != TextInputControl::UIntNumber && !s.empty() && s[0] == '-') {
        if (s.size() == 1)
            return true;
        s = s.substr(1);
    }
    std::string separator = DecimalSeparator();
    if (type == TextInputControl::RealNumber) {
        size_t first = s.find(separator), last = s.rfind(separator);
        if (first != last)
            return false;
    }
    for (size_t i = 0; i < s.size();) {
        if (s[i] >= '0' && s[i] <= '9') {
            ++i;
            continue;
        }
        if (type == TextInputControl::RealNumber && s.compare(i, separator.size(), separator) == 0) {
            i += separator.size();
            continue;
        }
        return false;
    }
    return true;
}

void OnFieldChanged(GtkEditable*, gpointer data)
{
    LEGO::TextInputControl* control = (LEGO::TextInputControl*)data;
    control->SyncTextLayout();
    control->NotifyTextChanged();
}

void OnFieldActivate(GtkEntry*, gpointer data)
{
    LEGO::TextInputControl* control = (LEGO::TextInputControl*)data;
    control->NotifyTextEntered();
}

void OnFieldInsert(GtkEditable* editable, const gchar* text, gint length, gint* position, gpointer)
{
    LEGO::UInt32 type = (LEGO::UInt32)GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(editable), "ldd-number-type"));
    if (type == LEGO::TextInputControl::FreeText)
        return;
    std::string now = gtk_entry_get_text(GTK_ENTRY(editable));
    gint start = 0, end = 0;
    if (gtk_editable_get_selection_bounds(editable, &start, &end)) {
        glong a = g_utf8_offset_to_pointer(now.c_str(), start) - now.c_str();
        glong b = g_utf8_offset_to_pointer(now.c_str(), end) - now.c_str();
        now.erase((size_t)a, (size_t)(b - a));
    }
    glong at = g_utf8_offset_to_pointer(now.c_str(), *position) - now.c_str();
    if (at > (glong)now.size())
        at = (glong)now.size();
    now.insert((size_t)at, text, length < 0 ? strlen(text) : (size_t)length);
    if (!IsPartialNumber(now, type))
        g_signal_stop_emission_by_name(editable, "insert-text");
}

} // namespace

#define LEGO_FIELD ((GtkWidget*)m_field)

#else // HOST-ONLY ------------------------------------------------------------------------------

namespace {

// HOST-ONLY (not restored code): stands in for the NSTextField.
struct HostTextField {
    HostTextField() : hidden(false), editable(true), frame(0.0f, 0.0f, 10.0f, 10.0f) {}
    LEGO::String text;
    bool         hidden;
    bool         editable;
    LEGO::Rect   frame;
};

} // namespace

#define LEGO_FIELD ((HostTextField*)m_field)

#endif // __OBJC__

namespace LEGO {

// Mac 0x2d8c30
bool TextInputControl::Create()
{
    SetInputFlags(m_inputFlags | 3);
#if defined(__OBJC__)
    NSTextField* field = [[NSTextField alloc] initWithFrame:NSMakeRect(0, 0, 10, 10)];
    m_field = field;
    NSWindow* window = (NSWindow*)m_manager->GetWindow()->GetNativeWindow();
    [[[window contentView] getNativeView] addSubview:field];
    [field setHidden:!m_status.m_visible];
    [field setEditable:m_status.m_enabledInTree];
    [field setBordered:NO];
    LEGOTextInputControlTarget* target = [[LEGOTextInputControlTarget alloc] initWithTextInputControl:this];
    m_target = target;
    [field setTarget:target];
    [field setAction:@selector(textEntered:)];
    [field setDelegate:target];
#elif defined(LDD_LINUX)
    TextLayout::CreateArgs args;
    args.text = String();
    m_textLayout = m_manager->GetObjectFactory()->Create<TextLayoutRender>(args);
    if (!m_textLayout.Get())
        return false;
    InstallFieldStyle();
    GtkWidget* field = gtk_entry_new();
    g_object_ref_sink(field);
    m_field = field;
    gtk_style_context_add_class(gtk_widget_get_style_context(field), "ldd-field");
    gtk_entry_set_has_frame(GTK_ENTRY(field), FALSE);
    gtk_entry_set_width_chars(GTK_ENTRY(field), 1);
    LinuxShell::AddControl(m_manager->GetWindow()->GetNativeWindow(), field);
    LinuxShell::PlaceControl(field, Rect(Vector2(0.0f, 0.0f), Vector2(10.0f, 10.0f)));
    // shown while the control has the keyboard focus (OnKeyboardFocusGained), as the Windows edit
    gtk_widget_set_no_show_all(field, TRUE);
    gtk_widget_set_visible(field, FALSE);
    gtk_editable_set_editable(GTK_EDITABLE(field), m_status.m_enabledInTree ? TRUE : FALSE);
    g_signal_connect(field, "changed", G_CALLBACK(OnFieldChanged), this);
    g_signal_connect(field, "activate", G_CALLBACK(OnFieldActivate), this);
    g_signal_connect(field, "insert-text", G_CALLBACK(OnFieldInsert), 0);
#else
    HostTextField* field = new HostTextField;
    m_field = field;
    field->hidden = !m_status.m_visible;
    field->editable = m_status.m_enabledInTree;
#endif
    AttachToAncestors();
    return true;
}

// Mac 0x2d9540 (destructor part)
void TextInputControl::DestroyNative()
{
#if defined(__OBJC__)
    // NOTE: the original releases the target (the field's delegate and action target) before
    // the field, which keeps both as unretained pointers: removeFromSuperview (ending an edit)
    // can message the freed target. fixed: delegate and target cleared first.
    if (m_field) {
        [LEGO_FIELD setDelegate:nil];
        [LEGO_FIELD setTarget:nil];
    }
    if (m_target)
        [(id)m_target release];
    if (m_field) {
        [LEGO_FIELD removeFromSuperview];
        [LEGO_FIELD release];
        m_field = 0;
    }
    if (m_formatter)
        [(id)m_formatter release];
#elif defined(LDD_LINUX)
    if (m_field) {
        g_signal_handlers_disconnect_matched(LEGO_FIELD, G_SIGNAL_MATCH_DATA, 0, 0, 0, 0, this);
        gtk_widget_destroy(LEGO_FIELD);
        g_object_unref(LEGO_FIELD);
        m_field = 0;
    }
#else
    delete LEGO_FIELD;
    m_field = 0;
#endif
}

// Mac 0x2d8e00
bool TextInputControl::SetText(const String& text)
{
#if defined(__OBJC__)
    [LEGO_FIELD setStringValue:(NSString*)text.GetCFString()];
#elif defined(LDD_LINUX)
    if (LEGO_FIELD) {
        // (setStringValue: sends no controlTextDidChange:)
        g_signal_handlers_block_by_func(LEGO_FIELD, (gpointer)OnFieldChanged, this);
        g_signal_handlers_block_by_func(LEGO_FIELD, (gpointer)OnFieldInsert, 0);
        gtk_entry_set_text(GTK_ENTRY(LEGO_FIELD), UTF8Of(text).c_str());
        g_signal_handlers_unblock_by_func(LEGO_FIELD, (gpointer)OnFieldInsert, 0);
        g_signal_handlers_unblock_by_func(LEGO_FIELD, (gpointer)OnFieldChanged, this);
    }
    SyncTextLayout();
#else
    if (LEGO_FIELD)
        LEGO_FIELD->text = text;
#endif
    NotifyTextChanged();
    return true;
}

// Mac 0x2d90c0
String TextInputControl::GetText() const
{
#if defined(__OBJC__)
    return String((CFStringRef)[LEGO_FIELD stringValue]);
#elif defined(LDD_LINUX)
    return LEGO_FIELD ? StringOf(gtk_entry_get_text(GTK_ENTRY(LEGO_FIELD))) : String();
#else
    return LEGO_FIELD ? LEGO_FIELD->text : String();
#endif
}

// Mac 0x2d9100
bool TextInputControl::SetFont(Font* font)
{
    FontPlatform* platformFont = dynamic_cast<FontPlatform*>(font);
#if defined(__OBJC__)
    if (platformFont)
        [LEGO_FIELD setFont:(NSFont*)platformFont->GetNativeFont()];   // CTFontRef, toll-free
#elif defined(LDD_LINUX)
    if (platformFont && LEGO_FIELD) {
        const PangoFontDescription* d = (const PangoFontDescription*)LinuxText::GetFontDescription(
            (const LinuxText::Font*)platformFont->GetNativeFont());
        G_GNUC_BEGIN_IGNORE_DEPRECATIONS
        gtk_widget_override_font(LEGO_FIELD, d);
        G_GNUC_END_IGNORE_DEPRECATIONS
        g_object_set_data(G_OBJECT(LEGO_FIELD), "ldd-font", (gpointer)platformFont->GetNativeFont());
    }
    if (platformFont && m_textLayout.Get()) {
        m_textLayout->SetFont(platformFont);
        Invalidate(false);
    }
#endif
    return platformFont != 0;
}

// Mac 0x2d9170: lines * (ascender - descender + 4) of the field's font.
// NOTE: the return value is the SetSize result (Win 0x5147e5 returns it; the Mac caller ignores it).
bool TextInputControl::SetLineCount(UInt32 lines)
{
#if defined(__OBJC__)
    Real32 ascender = (Real32)[[LEGO_FIELD font] ascender];
    Real32 descender = (Real32)[[LEGO_FIELD font] descender];
#elif defined(LDD_LINUX)
    // the field's font (13 points without one): Pango's ascent and descent
    Real32 ascender = 12.0f;
    Real32 descender = -3.0f;
    const LinuxText::Font* font =
        LEGO_FIELD ? (const LinuxText::Font*)g_object_get_data(G_OBJECT(LEGO_FIELD), "ldd-font") : 0;
    PangoFontDescription* d = font ? pango_font_description_copy((const PangoFontDescription*)LinuxText::GetFontDescription(font))
                                   : pango_font_description_from_string("Sans");
    if (!font)
        pango_font_description_set_absolute_size(d, 13.0 * PANGO_SCALE);
    PangoContext* context = gtk_widget_get_pango_context(LEGO_FIELD);
    PangoFontMetrics* metrics = pango_context_get_metrics(context, d, 0);
    if (metrics) {
        ascender = (Real32)pango_font_metrics_get_ascent(metrics) / PANGO_SCALE;
        descender = -(Real32)pango_font_metrics_get_descent(metrics) / PANGO_SCALE;
        pango_font_metrics_unref(metrics);
    }
    pango_font_description_free(d);
#else
    // HOST-ONLY: metrics of a 13 pt system font
    Real32 ascender = 12.0f;
    Real32 descender = -3.0f;
#endif
    Vector2 size(m_size.x, (Real32)(double)lines * ((4.0f - descender) + ascender));
    return SetSize(size);
}

// Mac 0x2d9240
void TextInputControl::SetNumberType(NumberType type)
{
    if (m_numberType == (UInt32)type)
        return;
    m_numberType = type;
#if defined(__OBJC__)
    [(id)m_formatter release];
    if (type == FreeText) {
        m_formatter = 0;
        // NOTE: the field itself (NSControl setFormatter:), the number formatters go to its cell
        [LEGO_FIELD setFormatter:nil];
    } else {
        if (m_numberType == IntNumber) {
            NSNumberFormatter* f = [[LEGOIntNumberFormatter alloc] init];
            m_formatter = f;
            [f setNumberStyle:NSNumberFormatterNoStyle];
        }
        if (m_numberType == UIntNumber) {
            NSNumberFormatter* f = [[LEGOUIntNumberFormatter alloc] init];
            m_formatter = f;
            [f setNumberStyle:NSNumberFormatterNoStyle];
            [f setMinimum:[NSNumber numberWithInt:0]];
        }
        if (m_numberType == RealNumber) {
            NSNumberFormatter* f = [[LEGOReal32NumberFormatter alloc] init];
            m_formatter = f;
            [f setNumberStyle:NSNumberFormatterNoStyle];
        }
        [[LEGO_FIELD cell] setFormatter:(NSFormatter*)m_formatter];
    }
#elif defined(LDD_LINUX)
    if (LEGO_FIELD)
        g_object_set_data(G_OBJECT(LEGO_FIELD), "ldd-number-type", GUINT_TO_POINTER(m_numberType));
#endif
    SetText(String());
}

// Mac 0x2d9470
void TextInputControl::SetFractionDigits(UInt32 digits)
{
#if defined(__OBJC__)
    [(NSNumberFormatter*)m_formatter setMaximumFractionDigits:digits];
#elif defined(LDD_LINUX)
    if (LEGO_FIELD && m_numberType != FreeText)
        g_object_set_data(G_OBJECT(LEGO_FIELD), "ldd-fraction-digits", GUINT_TO_POINTER(digits));
#else
    (void)digits;
#endif
}

// Mac 0x2d94a0
Real32 TextInputControl::GetFloat() const
{
#if defined(__OBJC__)
    return [LEGO_FIELD floatValue];
#else
    // Linux / HOST-ONLY: NSString floatValue (leading number, else 0)
    Real32 value = 0.0f;
    UInt32 position = 0;
    String text = GetText();
    if (!text.Parse(value, position))
        value = 0.0f;
    return value;
#endif
}

// Mac 0x2d94d0
void TextInputControl::SetFloat(Real32 value)
{
#if defined(__OBJC__)
    [LEGO_FIELD setFloatValue:value];
#elif defined(LDD_LINUX)
    // the number as the field's formatter shows it (at most the fraction digits set, 0 for a number
    // field without them; NSNumberFormatterNoStyle), else as text; no change notification
    if (LEGO_FIELD) {
        std::string text;
        if (m_numberType == FreeText) {
            String s;
            s << value;
            text = UTF8Of(s);
        } else {
            unsigned digits = GPOINTER_TO_UINT(g_object_get_data(G_OBJECT(LEGO_FIELD), "ldd-fraction-digits"));
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%.*f", (int)digits, (double)value);
            text = buffer;
            if (text.find('.') != std::string::npos) {
                while (!text.empty() && text[text.size() - 1] == '0')
                    text.erase(text.size() - 1);
                if (!text.empty() && text[text.size() - 1] == '.')
                    text.erase(text.size() - 1);
                size_t dot = text.find('.');
                if (dot != std::string::npos)
                    text.replace(dot, 1, DecimalSeparator());
            }
            if (text == "-0")
                text = "0";
        }
        g_signal_handlers_block_by_func(LEGO_FIELD, (gpointer)OnFieldChanged, this);
        g_signal_handlers_block_by_func(LEGO_FIELD, (gpointer)OnFieldInsert, 0);
        gtk_entry_set_text(GTK_ENTRY(LEGO_FIELD), text.c_str());
        g_signal_handlers_unblock_by_func(LEGO_FIELD, (gpointer)OnFieldInsert, 0);
        g_signal_handlers_unblock_by_func(LEGO_FIELD, (gpointer)OnFieldChanged, this);
        SyncTextLayout();
    }
#else
    // HOST-ONLY: the field shows the number (setFloatValue sends no change notification)
    if (LEGO_FIELD) {
        String text;
        text << value;
        LEGO_FIELD->text = text;
    }
#endif
}

// Mac inlined (setFrame: with the absolute position and the size)
void TextInputControl::UpdateNativeFrame()
{
    Vector2 position = GetAbsolutePosition();
    Vector2 size(m_size);
#if defined(__OBJC__)
    [LEGO_FIELD setFrame:NSMakeRect(position.x, position.y, size.x, size.y)];
#elif defined(LDD_LINUX)
    if (LEGO_FIELD)
        LinuxShell::PlaceControl(LEGO_FIELD, Rect(position, size));
#else
    if (LEGO_FIELD)
        LEGO_FIELD->frame = Rect(position, size);
#endif
}

// Mac 0x2d9e00
void TextInputControl::OnShown()
{
#if defined(__OBJC__)
    [LEGO_FIELD setHidden:NO];
#elif defined(LDD_LINUX)
    // (Win: the edit window shows with the keyboard focus only)
    if (LEGO_FIELD && m_manager->GetKeyboardFocus() == this)
        gtk_widget_show(LEGO_FIELD);
#else
    if (LEGO_FIELD)
        LEGO_FIELD->hidden = false;
#endif
    Control::OnShown();
}

// Mac 0x2d9e30
void TextInputControl::OnHidden()
{
#if defined(__OBJC__)
    [LEGO_FIELD setHidden:YES];
#elif defined(LDD_LINUX)
    // as Win 0x514f58: the focus leaves (OnKeyboardFocusLost hides the entry)
    SetKeyboardFocus(false);
    if (LEGO_FIELD)
        gtk_widget_hide(LEGO_FIELD);
#else
    if (LEGO_FIELD)
        LEGO_FIELD->hidden = true;
#endif
    Control::OnHidden();
}

// Mac 0x2d9e60
void TextInputControl::OnEnabled()
{
#if defined(__OBJC__)
    [LEGO_FIELD setEditable:YES];
#elif defined(LDD_LINUX)
    if (LEGO_FIELD)
        gtk_editable_set_editable(GTK_EDITABLE(LEGO_FIELD), TRUE);
#else
    if (LEGO_FIELD)
        LEGO_FIELD->editable = true;
#endif
    Control::OnEnabled();
}

// Mac 0x2d9e90
void TextInputControl::OnDisabled()
{
#if defined(__OBJC__)
    [LEGO_FIELD setEditable:NO];
#elif defined(LDD_LINUX)
    if (LEGO_FIELD)
        gtk_editable_set_editable(GTK_EDITABLE(LEGO_FIELD), FALSE);
#else
    if (LEGO_FIELD)
        LEGO_FIELD->editable = false;
#endif
    Control::OnDisabled();
}

// Mac 0x2d9c40 (the NSTextField keeps its own focus)
void TextInputControl::OnKeyboardFocusGained(bool flag, Control* previous, UInt32 modifiers)
{
    Control::OnKeyboardFocusGained(flag, previous, modifiers);
#if defined(LDD_LINUX)
    // as Win 0x514e4d: the entry shows and takes the focus, a click puts the caret at the mouse
    if (LEGO_FIELD && m_status.m_visible) {
        gtk_widget_show(LEGO_FIELD);
        LinuxShell::FocusControl(m_manager->GetWindow()->GetNativeWindow(), LEGO_FIELD, true);
        if (flag) {
            Vector2 mouse = m_manager->GetMousePosition(this);
            GtkEntry* entry = GTK_ENTRY(LEGO_FIELD);
            PangoLayout* layout = gtk_entry_get_layout(entry);
            int index = 0, trailing = 0;
            pango_layout_xy_to_index(layout, (int)(mouse.x * PANGO_SCALE), 0, &index, &trailing);
            gint at = gtk_entry_layout_index_to_text_index(entry, index);
            const char* entryText = gtk_entry_get_text(entry);
            glong offset = g_utf8_pointer_to_offset(entryText, entryText + at) + trailing;
            gtk_editable_set_position(GTK_EDITABLE(entry), (gint)offset);
        }
    }
#endif
}

// Mac 0x2d9c50
void TextInputControl::OnKeyboardFocusLost(Control* next)
{
#if defined(LDD_LINUX)
    // as Win 0x514ee4: the entry hides, the GL view draws the text again
    if (LEGO_FIELD) {
        bool focused = gtk_widget_has_focus(LEGO_FIELD) != FALSE;
        gtk_widget_hide(LEGO_FIELD);
        if (focused)
            LinuxShell::FocusControl(m_manager->GetWindow()->GetNativeWindow(), 0, false);
    }
    SyncTextLayout();
    Invalidate(false);
#endif
    Control::OnKeyboardFocusLost(next);
}

#if defined(LDD_LINUX)
void TextInputControl::SyncTextLayout()
{
    if (!m_textLayout.Get())
        return;
    String text = GetText();
    if (text != m_textLayout->GetText()) {
        m_textLayout->SetText(text);
        Invalidate(false);
    }
}
#endif

// Mac 0x2d9ec0: the field draws itself; the GL side clears the area (transparent while
// enabled, the disabled colour otherwise).
void TextInputControl::Draw(ControlDrawArgs& args)
{
#if defined(LDD_LINUX)
    // as Win 0x514f74: white, the text in black (centred vertically as the GtkEntry shows it),
    // the disabled colour over a disabled field
    args.Setup(false);
    args.DrawRect(Rect(m_size), Color(1.0f, 1.0f, 1.0f, 1.0f), false);
    if (m_textLayout.Get()) {
        Vector2 textSize = m_textLayout->GetSize();
        Real32 top = (Real32)floor((m_size.y - textSize.y) * 0.5f);
        if (top < 0.0f)
            top = 0.0f;
        Rect textRect(Vector2(0.0f, top), textSize);
        Rect visible(textRect);
        visible &= args.GetLocalClip();
        Texture* texture = visible.IsValid() ? m_textLayout->GetTexture() : 0;
        if (texture)
            args.DrawTexture(textRect, texture, m_textLayout->GetTexCoords(), Color(0.0f, 0.0f, 0.0f, 1.0f), true);
    }
    if (!m_status.m_enabledInTree) {
        args.m_render->SetClearColor(m_disabledColor);
        args.m_render->Clear(1);
    }
#else
    args.Setup(false);
    Render* render = args.m_render;
    if (m_status.m_enabledInTree)
        render->SetClearColor(Color(0.0f, 0.0f, 0.0f, 0.0f));
    else
        render->SetClearColor(m_disabledColor);
    args.m_render->Clear(1);
#endif
    m_status.m_invalid = 0;
}

} // namespace LEGO

#undef LEGO_FIELD
#endif // !_WIN32
