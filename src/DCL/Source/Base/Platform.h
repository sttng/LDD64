// LEGO Digital Designer — restored source
// LEGO::Platform (name inferred: not polymorphic, no RTTI) — OS services singleton: cached
// system directories, native <-> VFS paths, alerts, file dialogs, URLs / documents, OS version.
// Mac: TU 010 "Platform.mm" (0xb380-0xdc2f, Cocoa). Win: "Platform.cpp" (Win 0x412630-0x415090,
// with ANSI paths for Windows 9x, see Win32Unicode.h). Instance: static in GetInstance (Mac
// 0x77cf8c, guard 0x77cff4, atexit 0xc880; Win 0xb44aa8, guard bit 0xb44c4c, atexit 0x9381f0).
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "LEGO/String.h"
#include "BasicArray.h"
#include "BasicMap.h"
#include "Version.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class Directory;
class Path;
class Window;

class Platform {
public:
    // Well-known directories; the numbers are the ones LDD passes (Preferences maps the keys
    // "appDirectory" … "assetsDirectory" to them in this order). (names inferred)
    enum DirectoryKind {
        kAppDirectory      = 0,   // Mac: bundle path without the .app (or argv[0] directory); Win: exe directory
        kTempDirectory     = 1,   // NSTemporaryDirectory / GetTempPath
        kAppDataDirectory  = 2,   // ~/Library/Application Support / CSIDL_(COMMON_)APPDATA
        kDocumentDirectory = 3,   // ~/Documents / CSIDL_(COMMON_)DOCUMENTS
        kFavoriteDirectory = 4,   // ~/Library/Favorites / CSIDL_(COMMON_)FAVORITES
        kPictureDirectory  = 5,   // ~/Pictures / CSIDL_(COMMON_)PICTURES
        kMusicDirectory    = 6,   // ~/Music / CSIDL_(COMMON_)MUSIC
        kMovieDirectory    = 7,   // ~/Movies / CSIDL_(COMMON_)VIDEO
        kResourceDirectory = 8,   // bundle resources (falls back to kAppDirectory); Win = kAppDirectory
        kAssetsDirectory   = 9,   // <resources>/Assets
        kDirectoryKindCount = 10
    };

    // One file type of a save / open dialog: first = description ("LEGO Model"), second =
    // extension without the dot ("lxf"). Mac element size 0x20 (two Strings), Win 8.
    typedef Pair<String, String>  FileType;                                              // (name inferred)
    typedef BasicArray<FileType>  FileTypeArray;                                         // RTTI BasicArray<Pair<String, String>>

    // The singleton (function-local static, constructed on first use). (name inferred)
    static Platform* GetInstance();                                                      // Mac 0xd7f0, Win 0x415020

    // Cached per (kind, create); created on first request. `create` makes missing directories
    // (and selects the per-user folder on Win, the user domain on Mac). (name inferred)
    RefPointer<Directory> GetSystemDirectory(UInt32 kind, bool create);                  // Mac 0xb990, Win 0x412870
    // Overrides the cached directory (Preferences "…Directory" keys). (name inferred)
    void SetSystemDirectory(UInt32 kind, Directory* directory, bool create);             // Mac 0xc1d0, Win 0x412970
    // Native file-system path -> VFS path. Mac: "file/…" (standardised, a symbolic link
    // resolved, made absolute); Win: "file/C/…" for "C:\…", "network/server/…" for
    // "\\server\…", relative paths below the current directory. (name inferred)
    String NativeToVFSPath(const String& nativePath) const;                              // Mac 0xc890, Win 0x412bf0
    // VFS path -> native path (inverse of NativeToVFSPath). (name inferred)
    String VFSToNativePath(const String& vfsPath) const;                                 // Mac 0xca90, Win 0x412db0

    // Operating-system version (Mac: Gestalt 'sysv' {10, minor, bugfix, 0}; Win: GetVersionEx
    // {major, minor, 0, build}). Read inline by the OpenGL caps init (Mac 0xf2335, Win
    // 0x4f23a9). (name inferred)
    const Version& GetOSVersion() const { return m_osVersion; }

    // ---- alerts (static: the Mac code does not use `this`) ----
    // Message panel with an OK button; `title` may be null; `parent` is only used on Win
    // (owner HWND). Mac: NSRunAlertPanel through RunAlertPaneleUpdateObserver. (name inferred)
    static void ShowMessage(const String& message, const String* title, Window* parent); // Mac 0xb4c0, Win 0x4127d0
    // OK / "Cancel" panel; true for OK. (name inferred)
    static bool AskOkCancel(const String& message, const String* title, Window* parent); // Mac 0xb700, Win 0x412820
#if !defined(_WIN32)
    // Panel with up to three buttons (null = none / default); returns 0 for the default
    // button, 1 for the alternate, 2 for the other one (Application::Alert, Mac 0x2980,
    // forwards its last five arguments; the Win Application::Alert uses MessageBoxW itself).
    // (name inferred)
    static UInt32 Alert(const String& message, const String* title, const String* button1,
                        const String* button2, const String* button3);               // Mac 0xb860
#endif

    // ---- dialogs (names inferred) ----
    // Save panel. `directory` = initial directory (may be null), `name` = proposed file name,
    // `types` = file types (Win filter "desc\0*.ext\0…"; Mac: one type -> allowed type, several
    // -> accessory popup LEGOGetPathAccessoryViewOwner), `typeIndex` = in/out selected type,
    // `parent` = window to attach a sheet to (Win: owner), `title` = panel title / prompt.
    // On OK the chosen file is appended to `result` (Win: `result` is reset first).
    bool GetSavePath(Path& result, Directory* directory, const String& name,
                     const FileTypeArray& types, UInt32* typeIndex, Window* parent,
                     const String& title);                                             // Mac 0xcb80, Win 0x413b40
    // Open panel (one file, files only); `types` filter by extension.
    bool GetOpenPath(Path& result, Directory* directory, const String& name,
                     const FileTypeArray& types, Window* parent, const String& title); // Mac 0xd000, Win 0x414310
    // Folder chooser (NSOpenPanel for directories / SHBrowseForFolder); `message` is the
    // panel message / browse title.
    bool ChooseFolder(Path& result, const String& message, Directory* directory,
                      Window* parent);                                                  // Mac 0xd3a0, Win 0x4149a0

    // ---- shell (names inferred) ----
    // Opens a URL in the browser (NSWorkspace openURL: / ShellExecuteEx "open").
    bool OpenURL(const String& url);                                                    // Mac 0xd630, Win 0x414dd0
    // Opens a document with its application (VFS path -> native file URL).
    bool OpenDocument(const Path& path);                                                // Mac 0xd6a0, Win 0x414e60
    // NSWorkspace openFile:withApplication:nil andDeactivate:`deactivate` (Win: ShellExecuteEx
    // "open", the flag is not passed).
    bool OpenWithApplication(const Path& path, bool deactivate);                        // Mac 0xd930, Win 0x414f00

    // The platform name the LDD web services get ("&platform=" of InitializeDesigner /
    // NewContent). (additive) Win 0x687006 (String "win" by value); Mac inlined
    // after GetInstance() ("mac", InitializeDesignerCaller Mac 0x1b4cd0). (name inferred)
    String GetWebPlatformName() const
    {
#if defined(_WIN32)
        return String("win");
#else
        return String("mac");
#endif
    }

#if defined(_WIN32)
    // Windows NT family: use the ...W API (Win reads +0x14 directly everywhere, e.g. the message
    // loops of Application and the wrappers of Win32Unicode.h). (name inferred)
    bool    IsUnicode() const { return m_isUnicode; }
    HMODULE GetModule() const { return m_module; }
    const String& GetServicePack() const { return m_servicePack; }
#endif

private:
    // Creates the directory of `kind` (NSSearchPathForDirectoriesInDomains, "Assets" …).
    RefPointer<Directory> CreateSystemDirectory(UInt32 kind, bool create);             // Mac 0xba60, Win 0x413000

    Platform();                                                                        // Mac 0xc250, Win 0x412a10
    ~Platform();                                                                       // Mac 0xc500 (atexit 0xc880), Win 0x412b80
    Platform(const Platform&);
    Platform& operator=(const Platform&);

    // ---- data ----
    Version               m_osVersion;           // +0x00
#if defined(_WIN32)
    String                m_servicePack;         // Win +0x10 szCSDVersion
    bool                  m_isUnicode;           // Win +0x14 dwPlatformId == VER_PLATFORM_WIN32_NT
    HMODULE               m_module;              // Win +0x18 GetModuleHandleA(0)
    RefPointer<Directory> m_directories[kDirectoryKindCount][2]; // Win +0x1c per (kind, create)
#else
    RefPointer<Directory> m_directories[kDirectoryKindCount][2]; // Mac +0x10 per (kind, create)
    void*                 m_accessoryViewOwner;  // Mac +0x60 LEGOGetPathAccessoryViewOwner* (lazy; released by the dtor)
    void*                 m_numberFormatter;     // Mac +0x64 CFNumberFormatterRef (created, never used here)
#endif
};

} // namespace LEGO
