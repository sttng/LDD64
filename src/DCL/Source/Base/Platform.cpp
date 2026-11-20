// LEGO Digital Designer — restored source
// LEGO::Platform, Win32 twin of Platform.mm (file name inferred) — Windows build only.
// Win 0x4127d0-0x415090. See Platform.h.
//
// Win 0x412b80 is the destructor of the static instance (not written out).
#if defined(_WIN32)
#include "Platform.h"
#include "Win32Unicode.h"
#include "StringConversion.h"
#include "Directory.h"
#include "Path.h"
#include "Window.h"

#include <commdlg.h>
#include <shellapi.h>
#include <shlobj.h>
#include <string.h>
#include <wctype.h>

namespace LEGO {

namespace {

inline LPCWSTR W(const String& s)
{
    return reinterpret_cast<LPCWSTR>(s.GetBuffer());
}

inline HWND OwnerOf(Window* window)
{
    return window ? window->GetHWND() : 0;
}

// Win 0x413680 (UTF-16 filter builder): "description\0*.extension\0" per type plus a closing
// "\0" (reserved 1000); the zeros are part of the string, giving the double "\0" the file
// dialogs need. Appended as one-character strings: String::Append(count, c) appends nothing for
// c == 0 (Win 0x402064).
String BuildFilterW(const Platform::FileTypeArray& types)
{
    static const UniChar16 kZero = 0;
    const String zero(&kZero, 1);
    String filter;
    for (UInt32 i = 0; i < types.GetSize(); ++i) {
        filter << types[i].first;
        filter << zero;
        filter << "*.";
        filter << types[i].second;
        filter << zero;
    }
    filter << zero;
    return filter;
}

// Win 0x413870: the same in the ANSI code page.
AString BuildFilterA(const Platform::FileTypeArray& types)
{
    AString filter;
    const char zero = 0;
    for (UInt32 i = 0; i < types.GetSize(); ++i) {
        AString description = EncodeStringCodePage(types[i].first, GetAnsiCodePage());
        AString extension = EncodeStringCodePage(types[i].second, GetAnsiCodePage());
        filter.Append(description.GetBuffer(), description.GetLength());
        filter.Append(&zero, 1);
        filter.Append("*.", 2);
        filter.Append(extension.GetBuffer(), extension.GetLength());
        filter.Append(&zero, 1);
    }
    filter.Append(&zero, 1);
    return filter;
}

// The proposed name copied into the MAX_PATH file buffer.
// NOTE: the original copies the whole name (memcpy of length + 1 characters) without a bound:
// a proposed name of MAX_PATH characters or more overflows the stack buffer. Fixed: cut at
// MAX_PATH - 1.
void CopyName(WCHAR* buffer, const String& name)
{
    UInt32 n = name.GetLength();
    if (n > MAX_PATH - 1)
        n = MAX_PATH - 1;
    memcpy(buffer, name.GetBuffer(), n * 2);
    buffer[n] = 0;
}

void CopyName(char* buffer, const String& name)
{
    AString a = EncodeStringCodePage(name, GetAnsiCodePage());
    UInt32 n = a.GetLength();
    if (n > MAX_PATH - 1)
        n = MAX_PATH - 1;
    memcpy(buffer, a.GetBuffer(), n);
    buffer[n] = 0;
}

// Win 0x414970: browse callback, expands the initial folder (lpData = native path).
int CALLBACK BrowseCallback(HWND window, UINT message, LPARAM /*lParam*/, LPARAM data)
{
    if (message == BFFM_INITIALIZED)
        ::SendMessageA(window, 0x46a /* BFFM_SETEXPANDED */, TRUE, data);
    return 0;
}

} // namespace

// ---------------------------------------------------------------------------------------------
// alerts
// ---------------------------------------------------------------------------------------------

// Win 0x4127d0
void Platform::ShowMessage(const String& message, const String* title, Window* parent)
{
    ::MessageBoxW(OwnerOf(parent), W(message), title ? W(*title) : 0,
                  MB_SETFOREGROUND | MB_TOPMOST | MB_TASKMODAL | MB_OK);   // 0x52000
}

// Win 0x412820
bool Platform::AskOkCancel(const String& message, const String* title, Window* parent)
{
    return ::MessageBoxW(OwnerOf(parent), W(message), title ? W(*title) : 0,
                         MB_SETFOREGROUND | MB_TOPMOST | MB_TASKMODAL | MB_ICONHAND | MB_OKCANCEL) // 0x52011
           == IDOK;
}

// ---------------------------------------------------------------------------------------------
// directories
// ---------------------------------------------------------------------------------------------

// Win 0x412870 (this ECX)
RefPointer<Directory> Platform::GetSystemDirectory(UInt32 kind, bool create)
{
    RefPointer<Directory>& slot = m_directories[kind][create ? 1 : 0];
    if (slot.IsNull())
        slot = CreateSystemDirectory(kind, create);
    return slot;
}

// Win 0x412970 (directory ECX, kind EBX; specialised for create = true, the only value passed)
void Platform::SetSystemDirectory(UInt32 kind, Directory* directory, bool create)
{
    RefPointer<Directory> keep = directory;
    m_directories[kind][create ? 1 : 0] = keep;
}

// Win 0x412a10 (the constructor of the static instance, called by GetInstance)
Platform::Platform()
    : m_isUnicode(false), m_module(0)
{
    OSVERSIONINFOA info;
    info.dwOSVersionInfoSize = sizeof(info);   // 0x94
    ::GetVersionExA(&info);
    m_osVersion = Version(info.dwMajorVersion, info.dwMinorVersion, 0, info.dwBuildNumber);
    m_servicePack = DecodeStringCodePage(info.szCSDVersion, (UInt32)strlen(info.szCSDVersion),
                                         GetAnsiCodePage());
    m_isUnicode = info.dwPlatformId == VER_PLATFORM_WIN32_NT;
    m_module = ::GetModuleHandleA(0);
}

// Win 0x412b80
Platform::~Platform()
{
}

// Win 0x412bf0 (this ECX)
String Platform::NativeToVFSPath(const String& nativePath) const
{
    String vfs;
    if (nativePath.GetLength() >= 2) {
        if (::iswalpha(nativePath.GetAt(0)) && nativePath.GetAt(1) == ':') {
            vfs.Append("file/");
            vfs << nativePath;
            vfs.Erase(6, 1);                       // "file/C:\x" -> "file/C\x"
        } else if (nativePath.GetAt(0) == '\\' && nativePath.GetAt(1) == '\\') {
            vfs.Append("network/");
            vfs << nativePath;
            vfs.Erase(8, 2);                       // "network/\\server" -> "network/server"
        } else {
            // relative: below the current directory
            vfs = PlatformGetCurrentDirectory();
            if (!vfs.IsEmpty()) {
                if (vfs.GetAt(vfs.GetLength() - 1) != '\\')
                    vfs << "\\";
                vfs << nativePath;
                return NativeToVFSPath(vfs);
            }
        }
        vfs.ReplaceAll(String("\\"), String("/"));
    }
    // NOTE: a native path shorter than two characters gives an empty VFS path
    return vfs;
}

namespace {
// Win 0x403aa0 -> _wcsnicmp(s, prefix, n) == 0 (case-insensitive, n characters)
inline bool StartsWithNoCase(const String& s, const char* prefix)
{
    const String p(prefix);
    return s.GetLength() >= p.GetLength() && s.Substring(0, p.GetLength()).EqualsNoCase(p);
}
} // namespace

// Win 0x412db0 (this ECX)
String Platform::VFSToNativePath(const String& vfsPath) const
{
    String native(vfsPath);
    if (StartsWithNoCase(vfsPath, "file/")) {
        native = vfsPath.Substring(5);
        // NOTE: the original throws LEGO::RangeException for "file/" (Insert past the end);
        // fixed: an empty rest stays empty.
        if (!native.IsEmpty())
            native.Insert(1, 1, ':');              // "C/x" -> "C:/x"
        if (native.GetLength() == 2)
            native.Append(1, '/');                 // "C:" -> "C:/"
    } else if (StartsWithNoCase(vfsPath, "network/")) {
        native = String("\\\\");
        native << vfsPath.Substring(8);
    }
    native.ReplaceAll(String("/"), String("\\"));
    return native;
}

// Win 0x413000 (kind ECX, create DL, this on the stack)
RefPointer<Directory> Platform::CreateSystemDirectory(UInt32 kind, bool create)
{
    String path;
    switch (kind) {
    case kAppDirectory:
    case kResourceDirectory: {
        // the directory of the executable
        if (!m_isUnicode) {
            char buffer[MAX_PATH];
            ::GetModuleFileNameA(m_module, buffer, MAX_PATH);
            path = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
        } else {
            WCHAR buffer[MAX_PATH];
            ::GetModuleFileNameW(m_module, buffer, MAX_PATH);
            path = String((const UniChar16*)buffer);
        }
        path.Erase(path.ReverseFind('\\'));
        break;
    }
    case kTempDirectory:
        if (!m_isUnicode) {
            const DWORD size = ::GetTempPathA(0, 0) + 1;
            char* buffer = new char[size];
            ::GetTempPathA(size, buffer);
            path = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
            delete[] buffer;
        } else {
            const DWORD size = ::GetTempPathW(0, 0) + 1;
            WCHAR* buffer = new WCHAR[size];
            const DWORD n = ::GetTempPathW(size, buffer);
            path = String((const UniChar16*)buffer, n);
            delete[] buffer;
        }
        break;
    case kAppDataDirectory:
    case kDocumentDirectory:
    case kFavoriteDirectory: {
        // `create` selects the per-user folder, otherwise the all-users one
        int folder = 0;
        if (kind == kAppDataDirectory)
            folder = create ? CSIDL_APPDATA : CSIDL_COMMON_APPDATA;           // 0x1a / 0x23
        else if (kind == kDocumentDirectory)
            folder = create ? CSIDL_PERSONAL : CSIDL_COMMON_DOCUMENTS;        // 0x05 / 0x2e
        else if (kind == kFavoriteDirectory)
            folder = create ? CSIDL_FAVORITES : CSIDL_COMMON_FAVORITES;       // 0x06 / 0x1f
        if (!m_isUnicode) {
            char buffer[MAX_PATH];
            if (::SHGetSpecialFolderPathA(0, buffer, folder, TRUE))
                path = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
        } else {
            WCHAR buffer[MAX_PATH];
            if (::SHGetSpecialFolderPathW(0, buffer, folder, TRUE))
                path = String((const UniChar16*)buffer);
        }
        break;
    }
    case kPictureDirectory:
    case kMusicDirectory:
    case kMovieDirectory: {
        int folder = 0;
        if (kind == kPictureDirectory)
            folder = create ? CSIDL_MYPICTURES : CSIDL_COMMON_PICTURES;       // 0x27 / 0x36
        else if (kind == kMusicDirectory)
            folder = create ? CSIDL_MYMUSIC : CSIDL_COMMON_MUSIC;             // 0x0d / 0x35
        else if (kind == kMovieDirectory)
            folder = create ? CSIDL_MYVIDEO : CSIDL_COMMON_VIDEO;             // 0x0e / 0x37
        if (!m_isUnicode) {
            char buffer[MAX_PATH];
            // NOTE: the ANSI path takes the folder when SHGetFolderPathA *fails* (result != S_OK)
            if (::SHGetFolderPathA(0, folder, 0, 0, buffer) != S_OK)
                path = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
        } else {
            WCHAR buffer[MAX_PATH];
            if (SUCCEEDED(::SHGetFolderPathW(0, folder, 0, 0, buffer)))
                path = String((const UniChar16*)buffer);
        }
        break;
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
    if (path.IsEmpty())
        return RefPointer<Directory>();
    if (path.GetAt(path.GetLength() - 1) == '\\')
        path.Erase(path.GetLength() - 1);
    path = NativeToVFSPath(path);
    return Directory::GetRoot()->GetDirectory(path, false);
}

// ---------------------------------------------------------------------------------------------
// dialogs
// ---------------------------------------------------------------------------------------------

// Win 0x413b40 (this EAX)
bool Platform::GetSavePath(Path& result, Directory* directory, const String& name,
                           const FileTypeArray& types, UInt32* typeIndex, Window* parent,
                           const String& title)
{
    result = Path();
    String initialDirectory;
    if (directory)
        initialDirectory = VFSToNativePath(directory->GetPath(true));
    String file;
    if (!m_isUnicode) {
        AString filter = BuildFilterA(types);
        char buffer[MAX_PATH];
        CopyName(buffer, name);
        AString initial = EncodeStringCodePage(initialDirectory, GetAnsiCodePage());
        AString caption = EncodeStringCodePage(title, GetAnsiCodePage());
        AString extension;
        if (types.GetSize() != 0)
            extension = EncodeStringCodePage(types[0].second, GetAnsiCodePage());
        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);             // 0x4c
        ofn.hwndOwner = OwnerOf(parent);
        ofn.lpstrFilter = filter.GetLength() > 1 ? filter.GetBuffer() : 0;
        ofn.lpstrFile = buffer;
        ofn.nFilterIndex = 0;
        ofn.nMaxFile = MAX_PATH;
        // NOTE: the original's ANSI path passes a stale (empty) string as the default
        // extension; fixed: the first type's extension, as on the Unicode path
        ofn.lpstrDefExt = types.GetSize() != 0 ? extension.GetBuffer() : 0;
        if (directory)
            ofn.lpstrInitialDir = initial.GetBuffer();
        if (!title.IsEmpty())
            ofn.lpstrTitle = caption.GetBuffer();
        ofn.Flags = 0x8802;   // OFN_NOREADONLYRETURN | OFN_PATHMUSTEXIST | OFN_OVERWRITEPROMPT
        if (!::GetSaveFileNameA(&ofn))
            return false;
        file = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
        *typeIndex = ofn.nFilterIndex - 1;
    } else {
        String filter = BuildFilterW(types);
        WCHAR buffer[MAX_PATH];
        CopyName(buffer, name);
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = OwnerOf(parent);
        ofn.lpstrFilter = filter.GetLength() > 1 ? W(filter) : 0;
        ofn.lpstrFile = buffer;
        ofn.nFilterIndex = 0;
        ofn.nMaxFile = MAX_PATH;
        ofn.lpstrDefExt = types.GetSize() != 0 ? W(types[0].second) : 0;
        if (directory)
            ofn.lpstrInitialDir = W(initialDirectory);
        if (!title.IsEmpty())
            ofn.lpstrTitle = W(title);
        ofn.Flags = 0x8802;
        if (!::GetSaveFileNameW(&ofn))
            return false;
        file = String((const UniChar16*)buffer);
        // NOTE: the selected type comes back; the incoming *typeIndex is not used on Win
        *typeIndex = ofn.nFilterIndex - 1;
    }
    result.Append(NativeToVFSPath(file));
    return true;
}

// Win 0x414310 (this in a register)
bool Platform::GetOpenPath(Path& result, Directory* directory, const String& name,
                           const FileTypeArray& types, Window* parent, const String& title)
{
    result = Path();
    String initialDirectory;
    if (directory)
        initialDirectory = VFSToNativePath(directory->GetPath(true));
    String file;
    if (!m_isUnicode) {
        AString filter = BuildFilterA(types);
        char buffer[MAX_PATH];
        CopyName(buffer, name);
        AString initial = EncodeStringCodePage(initialDirectory, GetAnsiCodePage());
        AString caption = EncodeStringCodePage(title, GetAnsiCodePage());
        OPENFILENAMEA ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = OwnerOf(parent);
        ofn.lpstrFilter = filter.GetLength() > 1 ? filter.GetBuffer() : 0;
        ofn.lpstrFile = buffer;
        ofn.nFilterIndex = 0;
        ofn.nMaxFile = MAX_PATH;
        if (directory)
            ofn.lpstrInitialDir = initial.GetBuffer();
        if (!title.IsEmpty())
            ofn.lpstrTitle = caption.GetBuffer();
        ofn.Flags = 0x1804;   // OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST | OFN_HIDEREADONLY
        if (!::GetOpenFileNameA(&ofn))
            return false;
        file = DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
    } else {
        String filter = BuildFilterW(types);
        WCHAR buffer[MAX_PATH];
        CopyName(buffer, name);
        OPENFILENAMEW ofn;
        memset(&ofn, 0, sizeof(ofn));
        ofn.lStructSize = sizeof(ofn);
        ofn.hwndOwner = OwnerOf(parent);
        ofn.lpstrFilter = filter.GetLength() > 1 ? W(filter) : 0;
        ofn.lpstrFile = buffer;
        ofn.nFilterIndex = 0;
        ofn.nMaxFile = MAX_PATH;
        if (directory)
            ofn.lpstrInitialDir = W(initialDirectory);
        if (!title.IsEmpty())
            ofn.lpstrTitle = W(title);
        ofn.Flags = 0x1804;
        if (!::GetOpenFileNameW(&ofn))
            return false;
        file = String((const UniChar16*)buffer);
    }
    result.Append(NativeToVFSPath(file));
    return true;
}

// Win 0x4149a0 (this ECX)
bool Platform::ChooseFolder(Path& result, const String& message, Directory* directory,
                            Window* parent)
{
    ::CoInitialize(0);
    IMalloc* allocator = 0;
    ::SHGetMalloc(&allocator);
    String initialDirectory;
    if (directory)
        initialDirectory = VFSToNativePath(directory->GetPath(true));
    String folder;
    if (!m_isUnicode) {
        AString title = EncodeStringCodePage(message, GetAnsiCodePage());
        char displayName[MAX_PATH];
        char path[MAX_PATH];
        BROWSEINFOA info;
        info.hwndOwner = OwnerOf(parent);
        info.pidlRoot = 0;
        info.pszDisplayName = displayName;
        info.lpszTitle = title.GetBuffer();
        info.ulFlags = 0x70;   // BIF_NEWDIALOGSTYLE | BIF_VALIDATE | BIF_EDITBOX
        info.lpfn = directory ? BrowseCallback : 0;
        info.lParam = (LPARAM)W(initialDirectory);
        info.iImage = 0;
        LPITEMIDLIST list = ::SHBrowseForFolderA(&info);
        if (!list)
            return false;   // NOTE: neither the allocator nor COM is released on cancel
        ::SHGetPathFromIDListA(list, path);
        folder = DecodeStringCodePage(path, (UInt32)strlen(path), GetAnsiCodePage());
    } else {
        WCHAR displayName[MAX_PATH];
        WCHAR path[MAX_PATH + 2];
        BROWSEINFOW info;
        info.hwndOwner = OwnerOf(parent);
        info.pidlRoot = 0;
        info.pszDisplayName = displayName;
        info.lpszTitle = W(message);
        info.ulFlags = 0x70;
        info.lpfn = directory ? BrowseCallback : 0;
        info.lParam = (LPARAM)W(initialDirectory);
        info.iImage = 0;
        LPITEMIDLIST list = ::SHBrowseForFolderW(&info);
        if (!list)
            return false;
        ::SHGetPathFromIDListW(list, path);
        folder = String((const UniChar16*)path);
    }
    // NOTE: the item list is never freed with the allocator
    allocator->Release();
    ::CoUninitialize();
    result = Path();
    result.Append(NativeToVFSPath(folder));
    return true;
}

// ---------------------------------------------------------------------------------------------
// shell
// ---------------------------------------------------------------------------------------------

namespace {

bool ShellOpen(const String& file)
{
    SHELLEXECUTEINFOW info;
    memset(&info, 0, sizeof(info));
    info.cbSize = sizeof(info);                    // 0x3c
    info.fMask = SEE_MASK_FLAG_NO_UI;              // 0x400
    info.hwnd = 0;
    info.lpVerb = L"open";
    info.lpFile = W(file);
    info.nShow = SW_SHOWNORMAL;
    return PlatformShellExecuteEx(&info);
}

} // namespace

// Win 0x414dd0
bool Platform::OpenURL(const String& url)
{
    ::SetForegroundWindow(::GetDesktopWindow());
    return ShellOpen(url);
}

// Win 0x414e60
bool Platform::OpenDocument(const Path& path)
{
    return OpenURL(GetInstance()->VFSToNativePath(path.GetPath(true)));
}

// Win 0x414f00 (the flag is not passed in the /LTCG build)
bool Platform::OpenWithApplication(const Path& path, bool /*deactivate*/)
{
    ::SetForegroundWindow(::GetDesktopWindow());
    return ShellOpen(GetInstance()->VFSToNativePath(path.GetPath(true)));
}

// Win 0x415020
Platform* Platform::GetInstance()
{
    static Platform s_instance;
    return &s_instance;
}

} // namespace LEGO
#endif // _WIN32
