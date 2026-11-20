// LEGO Digital Designer — restored source
// Win32 A/W wrapper layer (file and all names inferred) — Windows build only. See Win32Unicode.h.
// Win 0x415140-0x417540 (+ the name helper 0x415090).
#if defined(_WIN32)
#include "Win32Unicode.h"
#include "Platform.h"
#include "StringConversion.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <wctype.h>

#if defined(_MSC_VER) || defined(__clang__)
#pragma comment(lib, "mpr")   // WNetGetResourceInformation (the original imports MPR.dll)
#endif

namespace LEGO {

namespace {

inline bool UseUnicode()
{
    return Platform::GetInstance()->IsUnicode();
}

inline LPCWSTR W(const String& s)
{
    return reinterpret_cast<LPCWSTR>(s.GetBuffer());   // never null ("" for an empty String)
}

inline AString A(const String& s)
{
    return EncodeStringCodePage(s, GetAnsiCodePage());
}

// Win 0x415090 (EDI = source, EBX = destination): an ANSI name converted into a WCHAR array of
// the same character count, zero terminated.
void CopyAnsiName(WCHAR* destination, const char* source)
{
    const UInt32 length = (UInt32)strlen(source);
    String s = DecodeStringCodePage(source, length, GetAnsiCodePage());
    UInt32 n = s.GetLength();
    if (n > length)
        n = length;
    if (n != 0)
        memcpy(destination, s.GetBuffer(), n * 2);
    // NOTE: terminates at the ANSI length, not at the converted length (as the original)
    destination[length] = 0;
}

void CopyFindData(WIN32_FIND_DATAW* data, const WIN32_FIND_DATAA& a)
{
    data->dwFileAttributes = a.dwFileAttributes;
    data->ftCreationTime = a.ftCreationTime;
    data->ftLastAccessTime = a.ftLastAccessTime;
    data->ftLastWriteTime = a.ftLastWriteTime;
    data->nFileSizeHigh = a.nFileSizeHigh;
    data->nFileSizeLow = a.nFileSizeLow;
    data->dwReserved0 = a.dwReserved0;
    data->dwReserved1 = a.dwReserved1;
    CopyAnsiName(data->cFileName, a.cFileName);
    CopyAnsiName(data->cAlternateFileName, a.cAlternateFileName);
}

// Converts an optional wide string field of a Win32 structure for the ...A call.
struct AnsiField {
    explicit AnsiField(LPCWSTR s) : m_null(s == 0)
    {
        if (s)
            m_value = A(String((const UniChar16*)s));
    }
    LPCSTR Get() const { return m_null ? 0 : m_value.GetBuffer(); }
    bool    m_null;
    AString m_value;
};

} // namespace

// (Win 0x433210 +4)
UInt32 GetAnsiCodePage()
{
    static UInt32 s_codePage = ::GetACP();
    return s_codePage;
}

// Win 0x415140 (path in ECX)
bool PlatformCreateDirectory(const String& path)
{
    if (!UseUnicode())
        return ::CreateDirectoryA(A(path).GetBuffer(), 0) != 0;
    return ::CreateDirectoryW(W(path), 0) != 0;
}

// Win 0x415210
HANDLE PlatformCreateFile(const String& path, DWORD access, DWORD share, DWORD creation, DWORD flags)
{
    if (!UseUnicode())
        return ::CreateFileA(A(path).GetBuffer(), access, share, 0, creation, flags, 0);
    return ::CreateFileW(W(path), access, share, 0, creation, flags, 0);
}

// Win 0x4152f0
HANDLE PlatformCreateMutex(LPSECURITY_ATTRIBUTES attributes, bool initialOwner, const String* name)
{
    if (!UseUnicode()) {
        AString a;
        if (name)
            a = A(*name);
        return ::CreateMutexA(attributes, initialOwner, name ? a.GetBuffer() : 0);
    }
    return ::CreateMutexW(attributes, initialOwner, name ? W(*name) : 0);
}

// Win 0x415400 (windowName in ECX, menu in EBX)
HWND PlatformCreateWindowEx(DWORD exStyle, const String& className, const String& windowName,
                            DWORD style, int x, int y, int width, int height, HWND parent,
                            HMENU menu, HINSTANCE instance, LPVOID parameter)
{
    if (!UseUnicode()) {
        AString name = A(windowName);
        AString cls = A(className);
        return ::CreateWindowExA(exStyle, cls.GetBuffer(), name.GetBuffer(), style, x, y, width,
                                 height, parent, menu, instance, parameter);
    }
    return ::CreateWindowExW(exStyle, W(className), W(windowName), style, x, y, width, height,
                             parent, menu, instance, parameter);
}

// Win 0x415580
bool PlatformDeleteFile(const String& path)
{
    if (!UseUnicode())
        return ::DeleteFileA(A(path).GetBuffer()) != 0;
    return ::DeleteFileW(W(path)) != 0;
}

// Win 0x415650 (pattern in ECX)
HANDLE PlatformFindFirstFile(const String& pattern, WIN32_FIND_DATAW* data)
{
    if (!UseUnicode()) {
        WIN32_FIND_DATAA a;
        HANDLE find = ::FindFirstFileA(A(pattern).GetBuffer(), &a);
        if (find != INVALID_HANDLE_VALUE)
            CopyFindData(data, a);
        return find;
    }
    return ::FindFirstFileW(W(pattern), data);
}

// Win 0x4157b0 (find in ECX, data in ESI)
bool PlatformFindNextFile(HANDLE find, WIN32_FIND_DATAW* data)
{
    if (UseUnicode())
        return ::FindNextFileW(find, data) != 0;
    WIN32_FIND_DATAA a;
    const bool found = ::FindNextFileA(find, &a) != 0;
    if (found)
        CopyFindData(data, a);
    return found;
}

// Win 0x4158a0
DWORD PlatformGetFileAttributes(const String& path)
{
    if (!UseUnicode())
        return ::GetFileAttributesA(A(path).GetBuffer());
    return ::GetFileAttributesW(W(path));
}

// Win 0x415960 (data in EDI)
bool PlatformGetFileAttributesEx(const String& path, WIN32_FILE_ATTRIBUTE_DATA* data)
{
    if (!UseUnicode())
        return ::GetFileAttributesExA(A(path).GetBuffer(), GetFileExInfoStandard, data) != 0;
    return ::GetFileAttributesExW(W(path), GetFileExInfoStandard, data) != 0;
}

// Win 0x415a30 (attributes in EDI)
bool PlatformSetFileAttributes(const String& path, DWORD attributes)
{
    if (!UseUnicode())
        return ::SetFileAttributesA(A(path).GetBuffer(), attributes) != 0;
    return ::SetFileAttributesW(W(path), attributes) != 0;
}

// Win 0x415b00 (letters in EDI)
void PlatformGetLogicalDriveLetters(String& letters)
{
    letters = String();
    if (!UseUnicode()) {
        char buffer[0x104 * 2];
        ::GetLogicalDriveStringsA(0x104, buffer);
        // NOTE: the ANSI loop steps 4 bytes per drive ("A:\" + NUL) over a WCHAR buffer of the
        // same size as the Unicode one; the bound is 0x104 bytes
        const char* p = buffer;
        for (Int32 i = 4; i < 0x105; i += 4, p += 4) {
            if (*p == 0)
                break;
            letters.Append(1, (UniChar16)(::toupper((int)*p) & 0xff));
        }
        return;
    }
    WCHAR buffer[0x104];
    ::GetLogicalDriveStringsW(0x104, buffer);
    const WCHAR* p = buffer;
    for (Int32 i = 4; i < 0x105; i += 4, p += 4) {
        if (*p == 0)
            break;
        letters.Append(1, (UniChar16)::towupper(*p));
    }
}

// Win 0x415c00 (window EDI, index ESI); 0x415c20 is the copy for index 0
// PORT64: pointer-sized. On Win32 LONG_PTR is LONG and Get/SetWindowLongPtrW/A are the
// Get/SetWindowLongW/A of the original; on Win64 the window long can hold a pointer.
LONG_PTR PlatformGetWindowLong(HWND window, int index)
{
    if (UseUnicode())
        return ::GetWindowLongPtrW(window, index);
    return ::GetWindowLongPtrA(window, index);
}

// Win 0x415c40 (class in ECX)
ATOM PlatformRegisterClassEx(const WNDCLASSEXW* windowClass)
{
    if (UseUnicode())
        return ::RegisterClassExW(windowClass);
    WNDCLASSEXA a;
    a.cbSize = sizeof(a);
    a.style = windowClass->style;
    a.lpfnWndProc = windowClass->lpfnWndProc;
    a.cbClsExtra = windowClass->cbClsExtra;
    a.cbWndExtra = windowClass->cbWndExtra;
    a.hInstance = windowClass->hInstance;
    a.hIcon = windowClass->hIcon;
    a.hCursor = windowClass->hCursor;
    a.hbrBackground = windowClass->hbrBackground;
    // NOTE: like the original, a menu / class name given as MAKEINTRESOURCE is treated as a
    // string (LDD always passes real names)
    AnsiField menu(windowClass->lpszMenuName);
    AnsiField className(windowClass->lpszClassName);
    a.lpszMenuName = menu.Get();
    a.lpszClassName = className.Get();
    a.hIconSm = windowClass->hIconSm;
    return ::RegisterClassExA(&a);
}

// Win 0x415e80
bool PlatformRemoveDirectory(const String& path)
{
    if (!UseUnicode())
        return ::RemoveDirectoryA(A(path).GetBuffer()) != 0;
    return ::RemoveDirectoryW(W(path)) != 0;
}

// Win 0x415f50 (window EBX, index EDI, value ESI); 0x415f70 is the copy for index 0
// PORT64: pointer-sized (see PlatformGetWindowLong).
LONG_PTR PlatformSetWindowLong(HWND window, int index, LONG_PTR value)
{
    if (UseUnicode())
        return ::SetWindowLongPtrW(window, index, value);
    return ::SetWindowLongPtrA(window, index, value);
}

// Win 0x415f90 (text in ECX, window in EDI)
bool PlatformSetWindowText(HWND window, const String& text)
{
    if (!UseUnicode())
        return ::SetWindowTextA(window, A(text).GetBuffer()) != 0;
    return ::SetWindowTextW(window, W(text)) != 0;
}

// Win 0x416060 (info in ESI)
bool PlatformShellExecuteEx(SHELLEXECUTEINFOW* info)
{
    if (UseUnicode())
        return ::ShellExecuteExW(info) != 0;
    SHELLEXECUTEINFOA a;
    a.cbSize = sizeof(a);
    a.fMask = info->fMask;
    a.hwnd = info->hwnd;
    AnsiField verb(info->lpVerb);
    AnsiField file(info->lpFile);
    AnsiField parameters(info->lpParameters);
    AnsiField directory(info->lpDirectory);
    a.lpVerb = verb.Get();
    a.lpFile = file.Get();
    a.lpParameters = parameters.Get();
    a.lpDirectory = directory.Get();
    a.nShow = info->nShow;
    a.hInstApp = info->hInstApp;
    a.lpIDList = info->lpIDList;
    AnsiField cls(info->lpClass);
    a.lpClass = cls.Get();
    a.dwHotKey = info->dwHotKey;
    a.hkeyClass = info->hkeyClass;
    a.hIcon = info->hIcon;   // union with hMonitor
    a.hProcess = info->hProcess;
    return ::ShellExecuteExA(&a) != 0;
}

// Win 0x4164f0
bool PlatformGetWindowText(HWND window, String& text)
{
    text = String();
    if (!UseUnicode()) {
        const int size = ::GetWindowTextLengthA(window) + 1;
        if (size == 0)
            return false;
        char* buffer = new char[size];
        const int n = ::GetWindowTextA(window, buffer, size);
        if (n != 0)
            text = DecodeStringCodePage(buffer, (UInt32)n, GetAnsiCodePage());
        delete[] buffer;
        return n != 0;
    }
    const int size = ::GetWindowTextLengthW(window) + 1;
    if (size == 0)
        return false;
    WCHAR* buffer = new WCHAR[size];
    const int n = ::GetWindowTextW(window, buffer, size);
    if (n != 0)
        text = String((const UniChar16*)buffer, (UInt32)n);
    delete[] buffer;
    return n != 0;
}

// Win 0x416720 (font in ECX)
HFONT PlatformCreateFontIndirect(const LOGFONTW* font)
{
    if (UseUnicode())
        return ::CreateFontIndirectW(font);
    LOGFONTA a;
    a.lfHeight = font->lfHeight;
    a.lfWidth = font->lfWidth;
    a.lfEscapement = font->lfEscapement;
    a.lfOrientation = font->lfOrientation;
    a.lfWeight = font->lfWeight;
    a.lfItalic = font->lfItalic;
    a.lfUnderline = font->lfUnderline;
    a.lfStrikeOut = font->lfStrikeOut;
    a.lfCharSet = font->lfCharSet;
    a.lfOutPrecision = font->lfOutPrecision;
    a.lfClipPrecision = font->lfClipPrecision;
    a.lfQuality = font->lfQuality;
    a.lfPitchAndFamily = font->lfPitchAndFamily;
    AString face = A(String((const UniChar16*)font->lfFaceName));
    // NOTE: unbounded copy into lfFaceName[32] (as the original's strcpy)
    strcpy(a.lfFaceName, face.GetBuffer());
    return ::CreateFontIndirectA(&a);
}

// Win 0x416880 (count ECX, text EDX, rect EBX)
int PlatformDrawText(HDC dc, const String& text, UInt32 count, RECT* rect, UINT format)
{
    const UInt32 length = text.GetLength();
    const UInt32 n = length < count ? length : count;
    if (!UseUnicode()) {
        AString a = EncodeStringCodePage(text, GetAnsiCodePage(), count);
        return ::DrawTextA(dc, a.GetBuffer(), (int)a.GetLength(), rect, format);
    }
    return ::DrawTextW(dc, W(text), (int)n, rect, format);
}

// Win 0x416980
bool PlatformSetCurrentDirectory(const String& path)
{
    if (!UseUnicode())
        return ::SetCurrentDirectoryA(A(path).GetBuffer()) != 0;
    return ::SetCurrentDirectoryW(W(path)) != 0;
}

// Win 0x416a50
String PlatformGetCurrentDirectory()
{
    if (!UseUnicode()) {
        char buffer[0x104];
        ::GetCurrentDirectoryA(0x104, buffer);
        return DecodeStringCodePage(buffer, (UInt32)strlen(buffer), GetAnsiCodePage());
    }
    const DWORD size = ::GetCurrentDirectoryW(0, 0);
    WCHAR* buffer = new WCHAR[size];
    String result;
    if (::GetCurrentDirectoryW(size, buffer) != 0)
        result = String((const UniChar16*)buffer);
    delete[] buffer;
    return result;
}

// Win 0x416bb0 (resource in ESI)
DWORD PlatformGetResourceInformation(NETRESOURCEW* resource, String& provider, String& remoteName,
                                     String& system)
{
    DWORD size = 0x408;
    void* buffer = malloc(size);
    DWORD result;
    if (!UseUnicode()) {
        NETRESOURCEA a;
        a.dwType = resource->dwType;
        a.dwScope = resource->dwScope;
        a.dwDisplayType = resource->dwDisplayType;
        a.dwUsage = resource->dwUsage;
        AString localName = A(String((const UniChar16*)resource->lpLocalName));
        AString remote = A(String((const UniChar16*)resource->lpRemoteName));
        AString comment = A(String((const UniChar16*)resource->lpComment));
        AString provider8 = A(String((const UniChar16*)resource->lpProvider));
        a.lpLocalName = resource->lpLocalName ? (LPSTR)localName.GetBuffer() : 0;
        a.lpRemoteName = resource->lpRemoteName ? (LPSTR)remote.GetBuffer() : 0;
        a.lpComment = resource->lpComment ? (LPSTR)comment.GetBuffer() : 0;
        a.lpProvider = resource->lpProvider ? (LPSTR)provider8.GetBuffer() : 0;
        LPSTR rest = 0;
        result = ::WNetGetResourceInformationA(&a, buffer, &size, &rest);
        while (result == ERROR_MORE_DATA) {
            buffer = realloc(buffer, size);
            result = ::WNetGetResourceInformationA(&a, buffer, &size, &rest);
        }
        if (result == NO_ERROR) {
            const NETRESOURCEA* r = (const NETRESOURCEA*)buffer;
            resource->dwScope = r->dwScope;
            resource->dwType = r->dwType;
            resource->dwDisplayType = r->dwDisplayType;
            resource->dwUsage = r->dwUsage;
            provider = DecodeStringCodePage(r->lpProvider, (UInt32)strlen(r->lpProvider), GetAnsiCodePage());
            remoteName = DecodeStringCodePage(r->lpRemoteName, (UInt32)strlen(r->lpRemoteName), GetAnsiCodePage());
            system = DecodeStringCodePage(rest, (UInt32)strlen(rest), GetAnsiCodePage());
        }
    } else {
        LPWSTR rest = 0;
        result = ::WNetGetResourceInformationW(resource, buffer, &size, &rest);
        while (result == ERROR_MORE_DATA) {
            buffer = realloc(buffer, size);
            result = ::WNetGetResourceInformationW(resource, buffer, &size, &rest);
        }
        if (result == NO_ERROR) {
            const NETRESOURCEW* r = (const NETRESOURCEW*)buffer;
            resource->dwScope = r->dwScope;
            resource->dwType = r->dwType;
            resource->dwDisplayType = r->dwDisplayType;
            resource->dwUsage = r->dwUsage;
            provider = String((const UniChar16*)r->lpProvider);
            remoteName = String((const UniChar16*)r->lpRemoteName);
            system = String((const UniChar16*)rest);
        }
    }
    free(buffer);
    resource->lpProvider = 0;
    resource->lpComment = 0;
    resource->lpRemoteName = 0;
    resource->lpLocalName = 0;
    return result;
}

// Win 0x417020
bool PlatformGetClassName(HWND window, String& className)
{
    className = String();
    if (!UseUnicode()) {
        char buffer[0x100];
        const int n = ::GetClassNameA(window, buffer, 0x100);
        if (n != 0)
            className = DecodeStringCodePage(buffer, (UInt32)n, GetAnsiCodePage());
        return n != 0;
    }
    WCHAR buffer[0x100];
    const int n = ::GetClassNameW(window, buffer, 0x100);
    if (n != 0)
        className = String((const UniChar16*)buffer, (UInt32)n);
    return n != 0;
}

// Win 0x417260
int PlatformStartDoc(HDC dc, const DOCINFOW* info)
{
    if (UseUnicode())
        return ::StartDocW(dc, info);
    DOCINFOA a;
    a.cbSize = sizeof(a);
    AnsiField name(info->lpszDocName);
    AnsiField output(info->lpszOutput);
    AnsiField type(info->lpszDatatype);
    a.lpszDocName = name.Get();
    a.lpszOutput = output.Get();
    a.lpszDatatype = type.Get();
    a.fwType = info->fwType;
    return ::StartDocA(dc, &a);
}

} // namespace LEGO
#endif // _WIN32
