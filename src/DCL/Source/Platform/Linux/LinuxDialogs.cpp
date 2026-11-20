// LDD64 Linux platform layer (not restored code): the dialogs of LinuxShell.h (alerts, file
// choosers) and opening URLs, on GTK 3.
#if defined(LDD_LINUX)
#include "LinuxShell.h"
#include <gtk/gtk.h>
#include <string>
#include <vector>

namespace LEGO {
namespace LinuxShell {

namespace {
std::string UTF8(const String& s)
{
    gchar* utf8 = g_utf16_to_utf8((const gunichar2*)s.GetBuffer(), (glong)s.GetLength(), 0, 0, 0);
    std::string r(utf8 ? utf8 : "");
    g_free(utf8);
    return r;
}

String FromUTF8(const char* utf8)
{
    glong n = 0;
    gunichar2* u = g_utf8_to_utf16(utf8 ? utf8 : "", -1, 0, &n, 0);
    String r((const UniChar16*)u, (UInt32)n);
    g_free(u);
    return r;
}

std::string Lower(const std::string& s)
{
    gchar* l = g_utf8_strdown(s.c_str(), -1);
    std::string r(l);
    g_free(l);
    return r;
}

std::string Upper(const std::string& s)
{
    gchar* u = g_utf8_strup(s.c_str(), -1);
    std::string r(u);
    g_free(u);
    return r;
}

bool HasExtension(const std::string& path, const std::string& extension)
{
    std::string tail = "." + Lower(extension);
    std::string base = Lower(path);
    return base.size() > tail.size() && base.compare(base.size() - tail.size(), tail.size(), tail) == 0;
}

// The dialog shows over the main window; LDD's own window keeps drawing meanwhile.
GtkWindow* Parent()
{
    GtkWidget* w = GetMainGtkWindow();
    return w ? GTK_WINDOW(w) : 0;
}
} // namespace

long Alert(const String* title, const String& message, const String* button1, const String* button2,
           const String* button3)
{
    std::string primary = title && !title->IsEmpty() ? UTF8(*title) : UTF8(message);
    std::string secondary = title && !title->IsEmpty() ? UTF8(message) : std::string();
    GtkWidget* dialog = gtk_message_dialog_new(Parent(), GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER, GTK_BUTTONS_NONE,
                                               "%s", primary.c_str());
    if (!secondary.empty())
        gtk_message_dialog_format_secondary_text(GTK_MESSAGE_DIALOG(dialog), "%s", secondary.c_str());
    gtk_window_set_title(GTK_WINDOW(dialog), g_get_application_name());
    // left to right as the Mac panel: other, alternate, default
    enum { kOther = 3, kAlternate = 2, kDefault = 1 };
    if (button3 && !button3->IsEmpty())
        gtk_dialog_add_button(GTK_DIALOG(dialog), UTF8(*button3).c_str(), kOther);
    if (button2 && !button2->IsEmpty())
        gtk_dialog_add_button(GTK_DIALOG(dialog), UTF8(*button2).c_str(), kAlternate);
    std::string defaultTitle = button1 && !button1->IsEmpty() ? UTF8(*button1) : std::string("OK");
    gtk_dialog_add_button(GTK_DIALOG(dialog), defaultTitle.c_str(), kDefault);
    gtk_dialog_set_default_response(GTK_DIALOG(dialog), kDefault);
    gint response = gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
    if (response == kAlternate)
        return 0;
    if (response == kOther)
        return -1;
    if (response == kDefault)
        return 1;
    // closed without a button: the alternate (Cancel) answer if there is one, else the default
    return button2 && !button2->IsEmpty() ? 0 : 1;
}

bool ChoosePath(ChooserMode mode, const String& title, const String& message, const String& folder,
                const String& name, const BasicArray<String>& names, const BasicArray<String>& extensions,
                UInt32* typeIndex, String& result)
{
    GtkFileChooserAction action = mode == kChooseSave ? GTK_FILE_CHOOSER_ACTION_SAVE
                                  : mode == kChooseFolder ? GTK_FILE_CHOOSER_ACTION_SELECT_FOLDER
                                                          : GTK_FILE_CHOOSER_ACTION_OPEN;
    std::string heading = !title.IsEmpty() ? UTF8(title) : !message.IsEmpty() ? UTF8(message) : std::string();
    const char* accept = mode == kChooseSave ? "_Save" : mode == kChooseFolder ? "_Choose" : "_Open";
    std::string acceptTitle = !title.IsEmpty() ? UTF8(title) : std::string(accept);
    GtkFileChooserNative* chooser = gtk_file_chooser_native_new(heading.empty() ? 0 : heading.c_str(), Parent(),
                                                                action, acceptTitle.c_str(), "_Cancel");
    GtkFileChooser* fc = GTK_FILE_CHOOSER(chooser);
    gtk_native_dialog_set_modal(GTK_NATIVE_DIALOG(chooser), TRUE);
    if (mode == kChooseSave)
        gtk_file_chooser_set_do_overwrite_confirmation(fc, TRUE);
    std::string start = UTF8(folder);
    if (!start.empty())
        gtk_file_chooser_set_current_folder(fc, start.c_str());
    if (mode == kChooseSave && !name.IsEmpty())
        gtk_file_chooser_set_current_name(fc, UTF8(name).c_str());
    std::vector<GtkFileFilter*> filters;
    std::vector<std::string> filterExtensions;
    if (mode != kChooseFolder) {
        // open: one filter of all the types (the Mac panel's allowed types); save: one per type
        GtkFileFilter* all = 0;
        for (UInt32 i = 0; i < extensions.GetSize(); ++i) {
            std::string ext = UTF8(extensions[i]);
            if (ext.empty())
                continue;
            std::string label = i < names.GetSize() ? UTF8(names[i]) : ext;
            if (mode == kChooseSave && label.empty())
                continue;
            GtkFileFilter* filter = 0;
            if (mode == kChooseSave) {
                filter = gtk_file_filter_new();
                gtk_file_filter_set_name(filter, (label + " (*." + ext + ")").c_str());
                filters.push_back(filter);
                filterExtensions.push_back(ext);
            } else {
                if (!all) {
                    all = gtk_file_filter_new();
                    filters.push_back(all);
                }
                filter = all;
            }
            gtk_file_filter_add_pattern(filter, ("*." + Lower(ext)).c_str());
            gtk_file_filter_add_pattern(filter, ("*." + Upper(ext)).c_str());
        }
        if (all) {
            std::string label;
            for (UInt32 i = 0; i < extensions.GetSize(); ++i)
                if (!extensions[i].IsEmpty())
                    label += std::string(label.empty() ? "" : ", ") + "*." + Lower(UTF8(extensions[i]));
            gtk_file_filter_set_name(all, label.c_str());
        }
        for (size_t i = 0; i < filters.size(); ++i)
            gtk_file_chooser_add_filter(fc, filters[i]);
        if (!filters.empty())
            gtk_file_chooser_set_filter(fc, filters[0]);   // (the Mac's pop-up starts at the first)
    }
    if (typeIndex && mode == kChooseSave)
        *typeIndex = 0;
    bool ok = gtk_native_dialog_run(GTK_NATIVE_DIALOG(chooser)) == GTK_RESPONSE_ACCEPT;
    if (ok) {
        gchar* file = gtk_file_chooser_get_filename(fc);
        if (!file) {
            ok = false;
        } else {
            std::string path(file);
            g_free(file);
            if (mode == kChooseSave && !filterExtensions.empty()) {
                GtkFileFilter* chosen = gtk_file_chooser_get_filter(fc);
                size_t index = 0;
                for (size_t i = 0; i < filters.size(); ++i)
                    if (filters[i] == chosen)
                        index = i;
                if (typeIndex)
                    *typeIndex = (UInt32)index;
                if (!HasExtension(path, filterExtensions[index]))
                    path += "." + Lower(filterExtensions[index]);
            }
            result = FromUTF8(path.c_str());
        }
    }
    g_object_unref(chooser);
    while (gtk_events_pending())
        gtk_main_iteration_do(FALSE);
    return ok;
}

bool OpenURI(const String& uri)
{
    std::string u = UTF8(uri);
    GError* error = 0;
    GtkWidget* parent = GetMainGtkWindow();
    gboolean ok = gtk_show_uri_on_window(parent ? GTK_WINDOW(parent) : 0, u.c_str(), GDK_CURRENT_TIME, &error);
    if (error) {
        g_warning("LDD: cannot open %s: %s", u.c_str(), error->message);
        g_error_free(error);
    }
    return ok != FALSE;
}

} // namespace LinuxShell
} // namespace LEGO
#endif
