// LEGO Digital Designer — restored source
// TU 010 "Platform.mm" (file name inferred) — Mac 0xb380-0xdc2f, Mac build only. See Platform.h.
//
// Link order: LEGORunAlertPanelHelper (0xb380, 0xb3c0), RunAlertPaneleUpdateObserver::ShowPanel
// 0xb400, Platform members, observer dtors 0xda10/0xda70 (thunks 0xdaf0/0xdb60), static init
// 0xdbf0; weak Path::GetPath 0xd850 (Path.h).
// LDD64 Linux (LDD_LINUX, not restored code): alerts, file choosers and URLs through the GTK shell
// (Platform/Linux/LinuxShell.h), the XDG user directories.
// Without __OBJC__ (the plain C++ test builds) the HOST-ONLY fallbacks
// apply: XDG-style directories, alerts on stderr (default button), dialogs answered from
// LDD_HOST_SAVE / LDD_HOST_OPEN / LDD_HOST_FOLDER (else cancel), POSIX path standardisation.
#if !defined(_WIN32)

#include "Platform.h"
#include "StringConversion.h"
#include "Directory.h"
#include "Path.h"
#include "Window.h"

#include <CoreFoundation/CoreFoundation.h>
#include <iostream>

#if defined(__OBJC__)
#import <Cocoa/Cocoa.h>
#include <CoreServices/CoreServices.h>   // Gestalt
#else
// HOST-ONLY / Linux
#if defined(LDD_LINUX)
#include "LinuxShell.h"
#include <glib.h>
#endif
#include <errno.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>
#include <string>
#include <vector>
#endif

namespace LEGO {

// =============================================================================================
// RunAlertPaneleUpdateObserver (sic) — one alert panel. RTTI N4LEGO28RunAlertPaneleUpdateObserverE
// (vmi, `virtual Object` base at +0x2c); Mac vtable 0x75f86c {D1 0xda10, D0 0xda70}, Object part
// 0x75f88c {0xdaf0, 0xdb60, Object slots}. sizeof 0x34. The layout is spelled out by the ivar
// encoding of LEGORunAlertPanelHelper.rapuo:
//   {^^? {String} ^{String} ^{String} ^{String} ^{String} i B ^^? i}
// =============================================================================================
class RunAlertPaneleUpdateObserver : public virtual Object {
public:
    // inline (Mac 0xb4c0 / 0xb700 / 0xb860)
    RunAlertPaneleUpdateObserver(const String& message, const String* title, const String* button1,
                                 const String* button2, const String* button3)
        : m_message(message), m_title(title), m_button1(button1), m_button2(button2),
          m_button3(button3), m_result(0), m_wasRunning(false)
    {
    }
    virtual ~RunAlertPaneleUpdateObserver();                          // Mac 0xda10 / 0xda70

    // Shows the panel on the main run loop; returns the button (0 default, 1 alternate,
    // 2 other), 1 when there is no NSApp. (name inferred)
    Int32 Run();                                                      // Mac 0xb5f0
    // NSRunAlertPanel(title, message, button1, button2, button3) -> m_result. (name inferred)
    void ShowPanel();                                                 // Mac 0xb400

    String        m_message;      // +0x04
    const String* m_title;        // +0x14 (may be null)
    const String* m_button1;      // +0x18
    const String* m_button2;      // +0x1c
    const String* m_button3;      // +0x20
    Int32         m_result;       // +0x24 (-1 while running)
    bool          m_wasRunning;   // +0x28 [NSApp isRunning] when Run started
};

} // namespace LEGO

#if defined(__OBJC__)
// ---------------------------------------------------------------------------------------------
// LEGORunAlertPanelHelper (NSObject, instance size 8): runs the panel from inside [NSApp run]
// when the application's run loop is not running yet, then stops it again.
// ---------------------------------------------------------------------------------------------
@interface LEGORunAlertPanelHelper : NSObject {
    LEGO::RunAlertPaneleUpdateObserver* rapuo;   // +4
}
- (id)initWithRunAlertPaneleUpdateObserver:(LEGO::RunAlertPaneleUpdateObserver*)observer;
- (void)ShowDialog:(id)sender;
@end

@implementation LEGORunAlertPanelHelper
// Mac 0xb380
- (id)initWithRunAlertPaneleUpdateObserver:(LEGO::RunAlertPaneleUpdateObserver*)observer
{
    self = [super init];
    if (self)
        rapuo = observer;
    return self;
}

// Mac 0xb3c0
- (void)ShowDialog:(id)sender
{
    rapuo->ShowPanel();
    [NSApp stop:self];
}
@end

// Classes of TU 267 ("GetPath sheets"); only the messages sent here.
// PORT64: the selected index is the caller's UInt32 (unsigned long on i386, unsigned int on
// 64-bit, where an unsigned long* would write 8 bytes into it).
#if defined(__LP64__)
typedef unsigned int LEGOTypeIndex;
#else
#define LEGOTypeIndex unsigned long   // (spelled out: clang encodes the typedef as unsigned int)
#endif
@interface LEGOGetPathAccessoryViewOwner : NSObject
- (void)setUpPanel:(NSSavePanel*)panel withFilterNames:(NSArray*)names types:(NSArray*)types
  andSelectedIndex:(LEGOTypeIndex*)index;
@end
@interface LEGOGetPathSheetDelegate : NSObject
+ (void)savePanelDidEnd:(NSSavePanel*)panel returnCode:(int)code contextInfo:(void*)info;
+ (void)openPanelDidEnd:(NSOpenPanel*)panel returnCode:(int)code contextInfo:(void*)info;
@end
#endif

namespace LEGO {

namespace {

#if defined(__OBJC__)
inline NSString* NS(const String& s)
{
    return (NSString*)s.GetCFString();
}
#else
// ---- HOST-ONLY: Foundation path semantics on POSIX ------------------------------------------
std::string Native(const String& s)
{
    AString utf8 = s.ToUTF8();
    return std::string(utf8.GetBuffer(), utf8.GetLength());
}

String FromNative(const std::string& s)
{
    return DecodeString(kTextEncodingUTF8, s.data(), (UInt32)s.size());
}

// -[NSString pathComponents]: "/" for a leading slash, empty components dropped, a trailing
// slash gives a last "/" component.
std::vector<std::string> PathComponents(const std::string& path)
{
    std::vector<std::string> components;
    if (path.empty())
        return components;
    if (path[0] == '/')
        components.push_back("/");
    size_t i = 0;
    while (i < path.size()) {
        size_t j = path.find('/', i);
        if (j == std::string::npos)
            j = path.size();
        if (j > i)
            components.push_back(path.substr(i, j - i));
        i = j + 1;
    }
    if (path.size() > 1 && path[path.size() - 1] == '/')
        components.push_back("/");
    return components;
}

// +[NSString pathWithComponents:]
std::string PathWithComponents(const std::vector<std::string>& components)
{
    std::string path;
    for (size_t i = 0; i < components.size(); ++i) {
        const std::string& c = components[i];
        if (c.empty())
            continue;
        if (c == "/") {
            if (path.empty())
                path = "/";
            continue;
        }
        if (!path.empty() && path[path.size() - 1] != '/')
            path += '/';
        path += c;
    }
    return path;
}

std::string HomeDirectory()
{
    const char* home = getenv("HOME");
    return home ? home : "/";
}

// -[NSString stringByStandardizingPath]: "~" expanded, "." removed, ".." resolved for
// absolute paths, trailing slash removed.
std::string StandardizePath(std::string path)
{
    if (!path.empty() && path[0] == '~' && (path.size() == 1 || path[1] == '/'))
        path = HomeDirectory() + path.substr(1);
    std::vector<std::string> in = PathComponents(path);
    std::vector<std::string> out;
    const bool absolute = !in.empty() && in[0] == "/";
    for (size_t i = 0; i < in.size(); ++i) {
        if (in[i] == "." || (in[i] == "/" && i != 0))
            continue;
        if (in[i] == ".." && absolute && out.size() > 1) {
            out.pop_back();
            continue;
        }
        out.push_back(in[i]);
    }
    return PathWithComponents(out);
}

std::string ExecutableDirectory()
{
    char buffer[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", buffer, sizeof buffer - 1);
    if (n <= 0)
        return ".";
    std::string path(buffer, (size_t)n);
    size_t cut = path.rfind('/');
    return cut == std::string::npos ? std::string(".") : path.substr(0, cut == 0 ? 1 : cut);
}

std::string DataHome()
{
    const char* xdg = getenv("XDG_DATA_HOME");
    if (xdg && *xdg)
        return xdg;
    return HomeDirectory() + "/.local/share";
}

// Linux: the XDG user directory of that kind (user-dirs.dirs: localised names); HOST-ONLY and
// without one: the home directory + `fallback`.
std::string UserDirectory(UInt32 kind, const char* fallback)
{
#if defined(LDD_LINUX)
    GUserDirectory which = G_USER_DIRECTORY_DOCUMENTS;
    switch (kind) {
    case Platform::kPictureDirectory: which = G_USER_DIRECTORY_PICTURES; break;
    case Platform::kMusicDirectory:   which = G_USER_DIRECTORY_MUSIC; break;
    case Platform::kMovieDirectory:   which = G_USER_DIRECTORY_VIDEOS; break;
    default:                          break;
    }
    const char* dir = g_get_user_special_dir(which);
    if (dir && *dir)
        return dir;
#else
    (void)kind;
#endif
    return HomeDirectory() + fallback;
}

#if defined(LDD_LINUX)
// The native directory of `directory` (empty without one) and the filter lists of `types`.
String NativeFolder(const Platform* platform, Directory* directory)
{
    return directory ? platform->VFSToNativePath(directory->GetPath(true)) : String();
}

void FilterLists(const Platform::FileTypeArray& types, BasicArray<String>& names, BasicArray<String>& extensions)
{
    for (UInt32 i = 0; i < types.GetSize(); ++i) {
        names.PushBack(types[i].first);
        extensions.PushBack(types[i].second);
    }
}
#endif
#endif

} // namespace

// =============================================================================================
// alerts
// =============================================================================================

// Mac 0xb400
void RunAlertPaneleUpdateObserver::ShowPanel()
{
#if defined(__OBJC__)
    NSString* message = NS(m_message);
    NSString* title = m_title ? NS(*m_title) : nil;
    NSString* button1 = m_button1 ? NS(*m_button1) : nil;
    NSString* button2 = m_button2 ? NS(*m_button2) : nil;
    NSString* button3 = m_button3 ? NS(*m_button3) : nil;
    // The message as an argument: a '%' in it is shown, not read as a format.
    const NSInteger r = NSRunAlertPanel(title, @"%@", button1, button2, button3, message);
#elif defined(LDD_LINUX)
    const long r = LinuxShell::Alert(m_title, m_message, m_button1, m_button2, m_button3);
#else
    // HOST-ONLY: the panel on stderr, answered with the default button
    fprintf(stderr, "ALERT %s%s%s\n", m_title ? Native(*m_title).c_str() : "",
            m_title ? ": " : "", Native(m_message).c_str());
    const long r = 1;   // NSAlertDefaultReturn
#endif
    if (r == 1)          // NSAlertDefaultReturn
        m_result = 0;
    else if (r == 0)     // NSAlertAlternateReturn
        m_result = 1;
    else                 // NSAlertOtherReturn
        m_result = 2;
}

// Mac 0xb5f0
Int32 RunAlertPaneleUpdateObserver::Run()
{
    m_result = -1;
#if defined(__OBJC__)
    if (NSApp == nil)
        return 1;
    m_wasRunning = [NSApp isRunning] != NO;
    if (!m_wasRunning) {
        // not inside the application's run loop yet: show the panel from a run-loop callback
        LEGORunAlertPanelHelper* helper =
            [[LEGORunAlertPanelHelper alloc] initWithRunAlertPaneleUpdateObserver:this];
        [[NSRunLoop currentRunLoop] performSelector:@selector(ShowDialog:) target:helper argument:nil
                                              order:0
                                              modes:[NSArray arrayWithObject:NSDefaultRunLoopMode]];
        [NSApp run];
        [helper release];
    } else {
        ShowPanel();
    }
#else
    // HOST-ONLY: there is no NSApp (the original returns 1 then); the panel goes to stderr
    m_wasRunning = false;
    ShowPanel();
#endif
    return m_result;
}

// Mac 0xb4c0 (`parent` unused on Mac)
void Platform::ShowMessage(const String& message, const String* title, Window* /*parent*/)
{
    RefPointer<RunAlertPaneleUpdateObserver> panel =
        new RunAlertPaneleUpdateObserver(message, title, 0, 0, 0);
    panel->Run();
}

// Mac 0xb700
bool Platform::AskOkCancel(const String& message, const String* title, Window* /*parent*/)
{
    String cancel("Cancel");
    RefPointer<RunAlertPaneleUpdateObserver> panel =
        new RunAlertPaneleUpdateObserver(message, title, 0, &cancel, 0);
    return panel->Run() == 0;
}

// Mac 0xb860
UInt32 Platform::Alert(const String& message, const String* title, const String* button1,
                       const String* button2, const String* button3)
{
    RefPointer<RunAlertPaneleUpdateObserver> panel =
        new RunAlertPaneleUpdateObserver(message, title, button1, button2, button3);
    return (UInt32)panel->Run();
}

// =============================================================================================
// directories
// =============================================================================================

// Mac 0xb990
RefPointer<Directory> Platform::GetSystemDirectory(UInt32 kind, bool create)
{
    RefPointer<Directory>& slot = m_directories[kind][create ? 1 : 0];
    if (slot.IsNull())
        slot = CreateSystemDirectory(kind, create);
    return slot;
}

// Mac 0xba60
RefPointer<Directory> Platform::CreateSystemDirectory(UInt32 kind, bool create)
{
    String path;
#if !defined(__OBJC__)
    (void)create;   // HOST-ONLY: user and local domains are the same directories
#endif
    switch (kind) {
    case kAppDirectory: {
#if defined(__OBJC__)
        NSBundle* bundle = [NSBundle mainBundle];
        if (bundle && [bundle bundlePath]) {
            NSString* p = [bundle bundlePath];
            if ([[p pathExtension] isEqualToString:@"app"])
                p = [p stringByDeletingLastPathComponent];
            path = String((CFStringRef)p);
            break;
        }
        path = String((CFStringRef)[[[[NSProcessInfo processInfo] arguments] objectAtIndex:0]
                                   stringByDeletingLastPathComponent]);
#else
        // HOST-ONLY: no bundle: the directory of the executable
        path = FromNative(ExecutableDirectory());
#endif
        break;
    }
    case kTempDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)NSTemporaryDirectory());
#else
    {
        const char* tmp = getenv("TMPDIR");   // HOST-ONLY
        path = FromNative(tmp && *tmp ? tmp : "/tmp/");
    }
#endif
        break;
    case kAppDataDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[[NSSearchPathForDirectoriesInDomains(
                   NSLibraryDirectory, create ? NSUserDomainMask : NSLocalDomainMask, YES)
                   objectAtIndex:0] stringByAppendingPathComponent:@"Application Support"]);
#else
        path = FromNative(DataHome());   // HOST-ONLY (~/Library/Application Support)
#endif
        break;
    case kDocumentDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[NSSearchPathForDirectoriesInDomains(
                   NSDocumentDirectory, create ? NSUserDomainMask : NSLocalDomainMask, YES)
                   objectAtIndex:0]);
#else
        path = FromNative(UserDirectory(kDocumentDirectory, "/Documents"));   // Linux / HOST-ONLY
#endif
        break;
    case kFavoriteDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[[NSSearchPathForDirectoriesInDomains(
                   NSLibraryDirectory, create ? NSUserDomainMask : NSLocalDomainMask, YES)
                   objectAtIndex:0] stringByAppendingPathComponent:@"Favorites"]);
#else
        path = FromNative(DataHome() + "/Favorites");   // HOST-ONLY (~/Library/Favorites)
#endif
        break;
    case kPictureDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[NSHomeDirectory() stringByAppendingPathComponent:@"Pictures"]);
#else
        path = FromNative(UserDirectory(kPictureDirectory, "/Pictures"));
#endif
        break;
    case kMusicDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[NSHomeDirectory() stringByAppendingPathComponent:@"Music"]);
#else
        path = FromNative(UserDirectory(kMusicDirectory, "/Music"));
#endif
        break;
    case kMovieDirectory:
#if defined(__OBJC__)
        path = String((CFStringRef)[NSHomeDirectory() stringByAppendingPathComponent:@"Movies"]);
#else
        path = FromNative(UserDirectory(kMovieDirectory, "/Movies"));
#endif
        break;
    case kResourceDirectory: {
#if defined(__OBJC__)
        NSBundle* bundle = [NSBundle mainBundle];
        if (bundle && [bundle resourcePath]) {
            String resources = NativeToVFSPath(String((CFStringRef)[bundle resourcePath]));
            RefPointer<Directory> directory = Directory::GetRoot()->GetDirectory(resources, false);
            if (!directory.IsNull())
                return directory;
        }
#endif
        // no bundle resources (HOST-ONLY: always): the application directory
        return GetSystemDirectory(kAppDirectory, true);
    }
    case kAssetsDirectory: {
        RefPointer<Directory> resources = GetSystemDirectory(kResourceDirectory, true);
        if (resources.IsNull())
            return RefPointer<Directory>();
        return resources->GetDirectory(String("Assets"), false);
    }
    default:
        break;
    }
    path = NativeToVFSPath(path);
    return Directory::GetRoot()->GetDirectory(path, true);
}

// Mac 0xc1d0
void Platform::SetSystemDirectory(UInt32 kind, Directory* directory, bool create)
{
    RefPointer<Directory> keep = directory;
    m_directories[kind][create ? 1 : 0] = keep;
}

// Mac 0xc250
Platform::Platform()
    : m_accessoryViewOwner(0), m_numberFormatter(0)
{
    UInt32 major = 0, minor = 0, bugfix = 0;
#if defined(__OBJC__)
    SInt32 bcd;
    if (Gestalt(gestaltSystemVersion, &bcd) == noErr) {   // 'sysv', e.g. 0x1068
#else
    // HOST-ONLY: the 10.6.8 of the reference machine
    UInt32 bcd = 0x1068;
    {
#endif
        // NOTE: divisions by 0xfff / 0xff / 0xf as in the original (a nibble 0xf of the minor
        // version would give 16)
        major = (bcd & 0xf000) / 0xfff * 10 + (bcd & 0xf00) / 0xff;
        minor = (bcd & 0xf0) / 0xf;
        bugfix = bcd & 0xf;
    }
    m_osVersion = Version(major, minor, bugfix, 0);
    m_numberFormatter = (void*)CFNumberFormatterCreate(0, 0, kCFNumberFormatterNoStyle);
}

// Mac 0xc500 (atexit 0xc880)
Platform::~Platform()
{
    CFRelease((CFNumberFormatterRef)m_numberFormatter);
#if defined(__OBJC__)
    [(id)m_accessoryViewOwner release];
#endif
    // m_directories released in reverse order
}

// Mac 0xc890
String Platform::NativeToVFSPath(const String& nativePath) const
{
#if defined(__OBJC__)
    NSString* path = [NS(nativePath) stringByStandardizingPath];
    NSFileManager* manager = [NSFileManager defaultManager];
    NSDictionary* attributes = [manager attributesOfItemAtPath:path error:nil];
    NSMutableArray* components;
    if (attributes && [[attributes objectForKey:NSFileType] isEqualToString:NSFileTypeSymbolicLink]) {
        // NOTE: the link target's components follow a "/" (an absolute target gives "file///…")
        path = [manager destinationOfSymbolicLinkAtPath:path error:nil];
        components = [NSMutableArray arrayWithObject:@"/"];
        [components addObjectsFromArray:[path pathComponents]];
    } else if ([path isAbsolutePath]) {
        components = [NSMutableArray arrayWithArray:[path pathComponents]];
    } else {
        components = [NSMutableArray arrayWithArray:[[manager currentDirectoryPath] pathComponents]];
        [components addObjectsFromArray:[path pathComponents]];
    }
    if ([components count] == 0)
        return String();
    if ([[components objectAtIndex:0] isEqualToString:@"/"])
        [components replaceObjectAtIndex:0 withObject:@"file"];
    return String((CFStringRef)[components componentsJoinedByString:@"/"]);
#else
    std::string path = StandardizePath(Native(nativePath));
    std::vector<std::string> components;
    struct stat info;
    if (!path.empty() && lstat(path.c_str(), &info) == 0 && S_ISLNK(info.st_mode)) {
        char target[PATH_MAX];
        ssize_t n = readlink(path.c_str(), target, sizeof target - 1);
        path.assign(target, n > 0 ? (size_t)n : 0);
        components.push_back("/");
        std::vector<std::string> more = PathComponents(path);
        components.insert(components.end(), more.begin(), more.end());
    } else if (!path.empty() && path[0] == '/') {
        components = PathComponents(path);
    } else {
        char cwd[PATH_MAX];
        components = PathComponents(getcwd(cwd, sizeof cwd) ? std::string(cwd) : std::string("/"));
        std::vector<std::string> more = PathComponents(path);
        components.insert(components.end(), more.begin(), more.end());
    }
    if (components.empty())
        return String();
    if (components[0] == "/")
        components[0] = "file";
    std::string joined;
    for (size_t i = 0; i < components.size(); ++i) {
        if (i)
            joined += '/';
        joined += components[i];
    }
    return FromNative(joined);
#endif
}

// Mac 0xca90
String Platform::VFSToNativePath(const String& vfsPath) const
{
#if defined(__OBJC__)
    NSMutableArray* components =
        [NSMutableArray arrayWithArray:[NS(vfsPath) componentsSeparatedByString:@"/"]];
    if ([components count] == 0)
        return String();
    if ([[components objectAtIndex:0] isEqualToString:@"file"])
        [components replaceObjectAtIndex:0 withObject:@"/"];
    return String((CFStringRef)[NSString pathWithComponents:components]);
#else
    std::string vfs = Native(vfsPath);
    std::vector<std::string> components;
    size_t i = 0;
    for (;;) {
        size_t j = vfs.find('/', i);
        components.push_back(vfs.substr(i, j == std::string::npos ? std::string::npos : j - i));
        if (j == std::string::npos)
            break;
        i = j + 1;
    }
    if (components[0] == "file")
        components[0] = "/";
    return FromNative(PathWithComponents(components));
#endif
}

// =============================================================================================
// dialogs
// =============================================================================================

#if defined(__OBJC__)
namespace {
// The initial directory of a panel as a new CFString (released by the caller), or null.
CFStringRef CreateDirectoryString(const Platform* platform, Directory* directory)
{
    if (!directory)
        return 0;
    String native = platform->VFSToNativePath(directory->GetPath(true));
    return CreateCFSubstring(native, 0, String::npos);
}

#if defined(__LP64__)
// PORT64: 64-bit builds use the panel API of Mac OS X 10.6 (directory URL, name field, completion
// handler, URL); the directory / file methods the 32-bit code calls are deprecated since 10.6.
// The panels run as before: without a parent modally, with one as a sheet plus a modal loop.
void SetUpPanel(NSSavePanel* panel, CFStringRef directory, NSString* file)
{
    if (directory)
        [panel setDirectoryURL:[NSURL fileURLWithPath:(NSString*)directory isDirectory:YES]];
    if (file)
        [panel setNameFieldStringValue:file];
}

BOOL RunPanel(NSSavePanel* panel, Window* parent)
{
    if (parent == 0)
        return [panel runModal] == NSModalResponseOK;
    __block BOOL ok = NO;
    [panel beginSheetModalForWindow:(NSWindow*)parent->GetNativeWindow()
                  completionHandler:^(NSInteger code) {
                      ok = code == NSModalResponseOK;
                      [NSApp stopModal];
                  }];
    [NSApp runModalForWindow:panel];
    return ok;
}

NSString* PanelPath(NSSavePanel* panel)
{
    return [[panel URL] path];
}
#endif
} // namespace
#endif

// Mac 0xcb80
bool Platform::GetSavePath(Path& result, Directory* directory, const String& name,
                           const FileTypeArray& types, UInt32* typeIndex, Window* parent,
                           const String& title)
{
#if defined(__OBJC__)
    BOOL ok = NO;
    NSSavePanel* panel = [NSSavePanel savePanel];
    [panel setCanSelectHiddenExtension:YES];
    CFStringRef initialDirectory = CreateDirectoryString(this, directory);
    NSString* file = name.IsEmpty() ? nil : NS(name);
    if (types.IsEmpty()) {
        [panel setAllowedFileTypes:nil];
    } else {
        NSMutableArray* names = [NSMutableArray arrayWithCapacity:types.GetSize()];
        NSMutableArray* extensions = [NSMutableArray arrayWithCapacity:types.GetSize()];
        for (UInt32 i = 0; i < types.GetSize(); ++i) {
            if (!types[i].first.IsEmpty() && !types[i].second.IsEmpty()) {
                [names addObject:NS(types[i].first)];
                [extensions addObject:NS(types[i].second)];
            }
        }
        if ([extensions count] == 0) {
            [panel setAllowedFileTypes:nil];
        } else if ([extensions count] == 1) {
            [panel setAllowedFileTypes:extensions];
        } else {
            if (!m_accessoryViewOwner)
                m_accessoryViewOwner = [[LEGOGetPathAccessoryViewOwner alloc] init];
            [(LEGOGetPathAccessoryViewOwner*)m_accessoryViewOwner setUpPanel:panel
                withFilterNames:names types:extensions andSelectedIndex:(LEGOTypeIndex*)typeIndex];
        }
    }
    if (!title.IsEmpty()) {
        [panel setTitle:NS(title)];
        [panel setPrompt:NS(title)];
    }
#if defined(__LP64__)
    SetUpPanel(panel, initialDirectory, file);
    ok = RunPanel(panel, parent);
    [panel setAccessoryView:nil];
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)PanelPath(panel))));
#else
    if (parent == 0) {
        ok = [panel runModalForDirectory:(NSString*)initialDirectory file:file] == NSOKButton;
    } else {
        [panel beginSheetForDirectory:(NSString*)initialDirectory file:file
                       modalForWindow:(NSWindow*)parent->GetNativeWindow()
                        modalDelegate:[LEGOGetPathSheetDelegate class]
                       didEndSelector:@selector(savePanelDidEnd:returnCode:contextInfo:)
                          contextInfo:&ok];
        [NSApp runModalForWindow:panel];
    }
    [panel setAccessoryView:nil];
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)[panel filename])));
#endif
    if (initialDirectory)
        CFRelease(initialDirectory);
    return ok != NO;
#elif defined(LDD_LINUX)
    (void)parent;
    BasicArray<String> names, extensions;
    FilterLists(types, names, extensions);
    String native;
    if (!LinuxShell::ChoosePath(LinuxShell::kChooseSave, title, String(), NativeFolder(this, directory), name,
                                names, extensions, typeIndex, native))
        return false;
    result.Append(NativeToVFSPath(native));
    return true;
#else
    // HOST-ONLY: LDD_HOST_SAVE=<native path> answers the panel, otherwise it is cancelled
    (void)directory; (void)name; (void)types; (void)typeIndex; (void)parent; (void)title;
    const char* answer = getenv("LDD_HOST_SAVE");
    if (!answer || !*answer)
        return false;
    result.Append(NativeToVFSPath(FromNative(answer)));
    return true;
#endif
}

// Mac 0xd000
bool Platform::GetOpenPath(Path& result, Directory* directory, const String& name,
                           const FileTypeArray& types, Window* parent, const String& title)
{
#if defined(__OBJC__)
    BOOL ok = NO;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    CFStringRef initialDirectory = CreateDirectoryString(this, directory);
    NSString* file = name.IsEmpty() ? nil : NS(name);
    NSMutableArray* extensions = nil;
    if (!types.IsEmpty()) {
        extensions = [NSMutableArray arrayWithCapacity:types.GetSize()];
        for (UInt32 i = 0; i < types.GetSize(); ++i)
            if (!types[i].second.IsEmpty())
                [extensions addObject:NS(types[i].second)];
    }
    [panel setCanChooseFiles:YES];
    [panel setCanChooseDirectories:NO];
    if (!title.IsEmpty()) {
        [panel setTitle:NS(title)];
        [panel setPrompt:NS(title)];
    }
#if defined(__LP64__)
    SetUpPanel(panel, initialDirectory, file);
    [panel setAllowedFileTypes:extensions];
    ok = RunPanel(panel, parent);
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)PanelPath(panel))));
#else
    if (parent == 0) {
        // NOTE: also without a parent the panel runs as a modeless "sheet" plus a modal loop
        [panel beginForDirectory:(NSString*)initialDirectory file:file types:extensions
                modelessDelegate:[LEGOGetPathSheetDelegate class]
                  didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                     contextInfo:&ok];
    } else {
        [panel beginSheetForDirectory:(NSString*)initialDirectory file:file types:extensions
                       modalForWindow:(NSWindow*)parent->GetNativeWindow()
                        modalDelegate:[LEGOGetPathSheetDelegate class]
                       didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                          contextInfo:&ok];
    }
    [NSApp runModalForWindow:panel];
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)[panel filename])));
#endif
    if (initialDirectory)
        CFRelease(initialDirectory);
    return ok != NO;
#elif defined(LDD_LINUX)
    (void)parent;
    BasicArray<String> names, extensions;
    FilterLists(types, names, extensions);
    String native;
    if (!LinuxShell::ChoosePath(LinuxShell::kChooseOpen, title, String(), NativeFolder(this, directory), name,
                                names, extensions, 0, native))
        return false;
    result.Append(NativeToVFSPath(native));
    return true;
#else
    // HOST-ONLY: LDD_HOST_OPEN=<native path> answers the panel, otherwise it is cancelled
    (void)directory; (void)name; (void)types; (void)parent; (void)title;
    const char* answer = getenv("LDD_HOST_OPEN");
    if (!answer || !*answer)
        return false;
    result.Append(NativeToVFSPath(FromNative(answer)));
    return true;
#endif
}

// Mac 0xd3a0
bool Platform::ChooseFolder(Path& result, const String& message, Directory* directory,
                            Window* parent)
{
#if defined(__OBJC__)
    BOOL ok = NO;
    NSOpenPanel* panel = [NSOpenPanel openPanel];
    [panel setAllowsMultipleSelection:NO];
    CFStringRef initialDirectory = CreateDirectoryString(this, directory);
    [panel setCanChooseFiles:NO];
    [panel setCanChooseDirectories:YES];
    if (!message.IsEmpty())
        [panel setMessage:NS(message)];
#if defined(__LP64__)
    SetUpPanel(panel, initialDirectory, nil);
    ok = RunPanel(panel, parent);
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)PanelPath(panel))));
#else
    if (parent == 0) {
        ok = [panel runModalForDirectory:(NSString*)initialDirectory file:nil] == NSOKButton;
    } else {
        [panel beginSheetForDirectory:(NSString*)initialDirectory file:nil
                       modalForWindow:(NSWindow*)parent->GetNativeWindow()
                        modalDelegate:[LEGOGetPathSheetDelegate class]
                       didEndSelector:@selector(openPanelDidEnd:returnCode:contextInfo:)
                          contextInfo:&ok];
        [NSApp runModalForWindow:panel];
    }
    if (ok)
        result.Append(NativeToVFSPath(String((CFStringRef)[panel filename])));
#endif
    if (initialDirectory)
        CFRelease(initialDirectory);
    return ok != NO;
#elif defined(LDD_LINUX)
    (void)parent;
    BasicArray<String> none;
    String native;
    if (!LinuxShell::ChoosePath(LinuxShell::kChooseFolder, String(), message, NativeFolder(this, directory),
                                String(), none, none, 0, native))
        return false;
    result.Append(NativeToVFSPath(native));
    return true;
#else
    // HOST-ONLY: LDD_HOST_FOLDER=<native path> answers the panel, otherwise it is cancelled
    (void)message; (void)directory; (void)parent;
    const char* answer = getenv("LDD_HOST_FOLDER");
    if (!answer || !*answer)
        return false;
    result.Append(NativeToVFSPath(FromNative(answer)));
    return true;
#endif
}

// =============================================================================================
// shell
// =============================================================================================

// Mac 0xd630
bool Platform::OpenURL(const String& url)
{
#if defined(__OBJC__)
    CGReleaseAllDisplays();
    return [[NSWorkspace sharedWorkspace] openURL:[NSURL URLWithString:NS(url)]] != NO;
#elif defined(LDD_LINUX)
    return LinuxShell::OpenURI(url);
#else
    fprintf(stderr, "OpenURL %s (HOST-ONLY: not opened)\n", Native(url).c_str());
    return false;
#endif
}

// Mac 0xd6a0
bool Platform::OpenDocument(const Path& path)
{
    String native = GetInstance()->VFSToNativePath(path.GetPath(true));
#if defined(__OBJC__)
    CGReleaseAllDisplays();
    CFStringRef file = CreateCFSubstring(native, 0, String::npos);
    const BOOL ok = [[NSWorkspace sharedWorkspace] openURL:[NSURL fileURLWithPath:(NSString*)file]];
    CFRelease(file);
    return ok != NO;
#elif defined(LDD_LINUX)
    gchar* uri = g_filename_to_uri(Native(native).c_str(), 0, 0);
    bool ok = uri && LinuxShell::OpenURI(String(uri));
    g_free(uri);
    return ok;
#else
    fprintf(stderr, "OpenDocument %s (HOST-ONLY: not opened)\n", Native(native).c_str());
    return false;
#endif
}

// Mac 0xd7f0
Platform* Platform::GetInstance()
{
    static Platform s_instance;
    return &s_instance;
}

// Mac 0xd930
bool Platform::OpenWithApplication(const Path& path, bool deactivate)
{
    String native = VFSToNativePath(path.GetPath(true));
#if defined(__OBJC__)
    CGReleaseAllDisplays();
    return [[NSWorkspace sharedWorkspace] openFile:NS(native) withApplication:nil
                                     andDeactivate:deactivate] != NO;
#else
    (void)deactivate;
    fprintf(stderr, "OpenWithApplication %s (HOST-ONLY: not opened)\n", Native(native).c_str());
    return false;
#endif
}

// Mac 0xda10 / 0xda70 (virtual-base thunks 0xdaf0 / 0xdb60)
RunAlertPaneleUpdateObserver::~RunAlertPaneleUpdateObserver()
{
}

} // namespace LEGO

#endif // !_WIN32
