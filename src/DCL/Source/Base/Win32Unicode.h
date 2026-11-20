// LEGO Digital Designer — restored source
// Win32 A/W wrapper layer (file and names inferred), Windows only: Win 0x415140-0x417540.
// Takes LEGO Strings, calls ...W on NT (Platform::IsUnicode, singleton +0x14), else
// converts for ...A (Windows 9x). Register arguments of the /LTCG build noted per function.
// Users: Win32Directory, Window, single-instance mutex, Platform, GDI text, PrintJob, TraceHook;
// Win32Directory.cpp keeps its own direct ...W helpers.
#pragma once
#if defined(_WIN32)
#include "LEGO/Types.h"
#include "LEGO/String.h"

#include <windows.h>
#include <shellapi.h>
#include <winnetwk.h>

namespace LEGO {

// The ANSI code page used for the ...A paths (Win 0x433210: function-local static
// {String, GetACP(), 1} of another TU; only its code page at +4 is read here).
UInt32 GetAnsiCodePage();                                                                 // (Win 0x433210 +4)

// CreateFile with no security attributes / template (access EBX, creation EDI, flags ECX).
HANDLE PlatformCreateFile(const String& path, DWORD access, DWORD share, DWORD creation,
                          DWORD flags);                                                   // Win 0x415210
bool   PlatformCreateDirectory(const String& path);                                       // Win 0x415140
// `name` may be null (unnamed mutex).
HANDLE PlatformCreateMutex(LPSECURITY_ATTRIBUTES attributes, bool initialOwner,
                           const String* name);                                           // Win 0x4152f0
HWND   PlatformCreateWindowEx(DWORD exStyle, const String& className, const String& windowName,
                              DWORD style, int x, int y, int width, int height, HWND parent,
                              HMENU menu, HINSTANCE instance, LPVOID parameter);          // Win 0x415400
bool   PlatformDeleteFile(const String& path);                                            // Win 0x415580
// ANSI path: the WIN32_FIND_DATAA is copied field by field, the names converted (0x415090).
HANDLE PlatformFindFirstFile(const String& pattern, WIN32_FIND_DATAW* data);              // Win 0x415650
bool   PlatformFindNextFile(HANDLE find, WIN32_FIND_DATAW* data);                         // Win 0x4157b0
DWORD  PlatformGetFileAttributes(const String& path);                                     // Win 0x4158a0
bool   PlatformGetFileAttributesEx(const String& path, WIN32_FILE_ATTRIBUTE_DATA* data);  // Win 0x415960
bool   PlatformSetFileAttributes(const String& path, DWORD attributes);                   // Win 0x415a30
// The drive letters of GetLogicalDriveStrings, upper-cased, one character per drive.
void   PlatformGetLogicalDriveLetters(String& letters);                                   // Win 0x415b00
LONG_PTR PlatformGetWindowLong(HWND window, int index);                                     // Win 0x415c00 (copy with index 0: 0x415c20)
ATOM   PlatformRegisterClassEx(const WNDCLASSEXW* windowClass);                           // Win 0x415c40
bool   PlatformRemoveDirectory(const String& path);                                       // Win 0x415e80
LONG_PTR PlatformSetWindowLong(HWND window, int index, LONG_PTR value);                         // Win 0x415f50 (copy with index 0: 0x415f70)
bool   PlatformSetWindowText(HWND window, const String& text);                            // Win 0x415f90
// The strings of `info` are converted for ShellExecuteExA; hInstApp / hProcess are not copied
// back on that path (as in the original).
bool   PlatformShellExecuteEx(SHELLEXECUTEINFOW* info);                                   // Win 0x416060
bool   PlatformGetWindowText(HWND window, String& text);                                  // Win 0x4164f0
// ANSI path: lfFaceName converted.
HFONT  PlatformCreateFontIndirect(const LOGFONTW* font);                                  // Win 0x416720
// Draws at most `count` characters of `text` (clipped to its length).
int    PlatformDrawText(HDC dc, const String& text, UInt32 count, RECT* rect, UINT format); // Win 0x416880
bool   PlatformSetCurrentDirectory(const String& path);                                   // Win 0x416980
String PlatformGetCurrentDirectory();                                                     // Win 0x416a50
// WNetGetResourceInformation on `resource` (whose strings are converted for the A call);
// on success the scope/type/display type/usage are copied back and `provider`, `remoteName`
// and `system` receive the result strings. The four string pointers of `resource` are reset
// to null on return.
DWORD  PlatformGetResourceInformation(NETRESOURCEW* resource, String& provider,
                                      String& remoteName, String& system);                // Win 0x416bb0
bool   PlatformGetClassName(HWND window, String& className);                              // Win 0x417020
int    PlatformStartDoc(HDC dc, const DOCINFOW* info);                                    // Win 0x417260

} // namespace LEGO
#endif // _WIN32
