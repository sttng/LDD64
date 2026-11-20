// LEGO Digital Designer — restored source
// TU 119 "PrintJob.cpp" (Mac 0x1d5fd0-0x1d67df) and its Windows twin (Win
// 0x53b440-0x53b8ef). Every function of both builds restored.
#include "PrintJob.h"
#include "LEGO/String.h"
#include "Bitmap.h"

#if defined(_WIN32)
#include <windows.h>
#include <commdlg.h>
#include <string.h>
#include "Win32Unicode.h"
#include "Window.h"
#elif defined(LDD_LINUX)
#include "LinuxShell.h"   // LDD64 Linux: GTK's print dialog and jobs (Platform/Linux/LinuxPrint.cpp)
#elif defined(__OBJC__)
#import <ApplicationServices/ApplicationServices.h>
#endif

namespace LEGO {

// Mac 0x1d5fd0 (C1), Win 0x53b440 (thiscall + most-derived flag)
PrintJob::PrintJob()
    : m_inDocument(false),
      m_inPage(false)
{
#if defined(_WIN32)
    m_dc = 0;
#else
    m_session = 0;
    m_settings = 0;
    m_pageFormat = 0;
    m_context = 0;
    m_paperRect.top = m_paperRect.left = m_paperRect.bottom = m_paperRect.right = 0;   // (not set by the original)
    m_pageRect = m_paperRect;
#endif
}

// Mac 0x1d6680 (D1), 0x1d66c0 (D0), thunks 0x1d66a0 / 0x1d6700; Win 0x53b4d0 (thunk 0x53b8f0)
PrintJob::~PrintJob()
{
}

// Mac 0x1d6750 (thunk 0x1d6770), Win 0x53b8c0 (thunk 0x53b900)
void PrintJob::OnFinalRelease()
{
#if defined(_WIN32)
    if (m_dc != 0) {
        AbortDoc(m_dc);
        DeleteDC(m_dc);
        m_dc = 0;
        m_inPage = false;
        m_inDocument = false;
    }
#else
    Object::OnFinalRelease();
#endif
}

#if !defined(_WIN32)
// Mac: PMRelease of format, settings, session (each nulled) — inlined in every error path.
void PrintJob::ReleaseSession()
{
#if defined(__OBJC__)
    if (m_pageFormat != 0) {
        PMRelease((PMObject)m_pageFormat);
        m_pageFormat = 0;
    }
    if (m_settings != 0) {
        PMRelease((PMObject)m_settings);
        m_settings = 0;
    }
    if (m_session != 0) {
        PMRelease((PMObject)m_session);
        m_session = 0;
    }
#elif defined(LDD_LINUX)
    if (m_session != 0) {
        LinuxShell::PrintRelease(m_session);
        m_session = 0;
    }
#endif
}

// Mac 0x1d60d0: session + default, validated print settings and page format.
bool PrintJob::CreateSession()
{
#if defined(__OBJC__)
    PMPrintSession session = 0;
    if (PMCreateSession(&session) == noErr) {
        m_session = session;
        Boolean changed = false;
        PMPrintSettings settings = 0;
        if (PMCreatePrintSettings(&settings) == noErr && (m_settings = settings, true) &&
            PMSessionDefaultPrintSettings(session, settings) == noErr) {
            OSStatus status = PMSessionValidatePrintSettings(session, settings, &changed);
            PMPageFormat format = 0;
            if (status == noErr && PMCreatePageFormat(&format) == noErr && (m_pageFormat = format, true) &&
                PMSessionDefaultPageFormat(session, format) == noErr &&
                PMSessionValidatePageFormat(session, format, &changed) == noErr)
                return true;
        }
    }
    ReleaseSession();
    return false;
#elif defined(LDD_LINUX)
    m_session = LinuxShell::PrintCreate();
    return m_session != 0;
#else
    return false;   // HOST-ONLY: no printing system
#endif
}
#endif

// Mac 0x1d6030, Win 0x53b510
bool PrintJob::ShowPrintDialog(Window* parent, bool& accepted)
{
#if defined(_WIN32)
    PRINTDLGA dialog;
    memset(&dialog, 0, sizeof(dialog));
    dialog.lStructSize = sizeof(dialog);                             // 0x42
    dialog.hwndOwner = parent != 0 ? parent->GetHWND() : 0;
    dialog.Flags = PD_RETURNDC | PD_NOSELECTION | PD_NOPAGENUMS | PD_USEDEVMODECOPIESANDCOLLATE; // 0x4010c
    dialog.nCopies = 1;
    if (!PrintDlgA(&dialog)) {
        accepted = false;
        return CommDlgExtendedError() == 0;
    }
    if (dialog.hDevMode != 0)
        GlobalFree(dialog.hDevMode);
    if (dialog.hDevNames != 0)
        GlobalFree(dialog.hDevNames);
    accepted = true;
    m_dc = dialog.hDC;
    return true;
#else
    (void)parent;
    if (!CreateSession()) {
#if !defined(__OBJC__)
        accepted = false;   // HOST-ONLY: "cancelled"
#endif
        return false;
    }
#if defined(__OBJC__)
    Boolean ok = false;
    OSStatus status = PMSessionPrintDialog((PMPrintSession)m_session, (PMPrintSettings)m_settings,
                                           (PMPageFormat)m_pageFormat, &ok);
    accepted = ok != 0;
    if (status == noErr)
        return true;
    ReleaseSession();
#elif defined(LDD_LINUX)
    if (LinuxShell::PrintDialog(m_session, accepted))
        return true;
    ReleaseSession();
#endif
    return false;
#endif
}

// Mac 0x1d6210, Win 0x53b5c0
bool PrintJob::BeginDocument(const String& name)
{
#if defined(_WIN32)
    if (m_dc == 0) {
        // the default printer: WIN.INI [windows] device=<device>,<driver>,<port>
        char device[0x1000];
        DWORD length = GetProfileStringA("windows", "device", "", device, sizeof(device));
        if (length > 0xffe)
            return m_inDocument;
        char* deviceName = strtok(device, ",");
        char* driver = strtok(0, ",");
        char* port = strtok(0, ",");
        m_dc = CreateDCA(driver, deviceName, port, 0);
        if (m_dc == 0)
            return m_inDocument;
    }
    SetBkMode(m_dc, TRANSPARENT);
    DOCINFOW info;
    info.cbSize = sizeof(info);                                      // 0x14
    info.lpszDocName = name.GetBuffer() != 0 ? (LPCWSTR)name.GetBuffer() : L"";
    info.lpszOutput = 0;
    info.lpszDatatype = 0;
    info.fwType = 0;
    if (PlatformStartDoc(m_dc, &info) > 0)
        m_inDocument = true;
    return m_inDocument;
#else
    if (m_session == 0 && !CreateSession())
        return false;
#if defined(__OBJC__)
    Boolean changed = false;
    if (PMPrintSettingsSetJobName((PMPrintSettings)m_settings, name.GetCFString()) == noErr &&
        PMSessionValidatePageFormat((PMPrintSession)m_session, (PMPageFormat)m_pageFormat, &changed) == noErr &&
        PMSessionBeginCGDocument((PMPrintSession)m_session, (PMPrintSettings)m_settings,
                                 (PMPageFormat)m_pageFormat) == noErr) {
        m_inDocument = true;
        return true;
    }
#elif defined(LDD_LINUX)
    if (LinuxShell::PrintBegin(m_session, name)) {
        m_inDocument = true;
        return true;
    }
#else
    (void)name;
#endif
    ReleaseSession();
    return false;
#endif
}

// Mac 0x1d62e0, Win 0x53b6d0
void PrintJob::EndDocument()
{
#if defined(_WIN32)
    EndDoc(m_dc);
    DeleteDC(m_dc);
    m_dc = 0;
#else
#if defined(__OBJC__)
    PMSessionEndDocument((PMPrintSession)m_session);
#elif defined(LDD_LINUX)
    LinuxShell::PrintEnd(m_session);
#endif
    ReleaseSession();
#endif
    m_inDocument = false;
}

// Mac 0x1d6340, Win 0x53b700
void PrintJob::AbortDocument()
{
#if defined(_WIN32)
    AbortDoc(m_dc);
    DeleteDC(m_dc);
    m_dc = 0;
#else
#if defined(__OBJC__)
    PMSessionSetError((PMPrintSession)m_session, kPMCancel);         // -30870 (0xffff876a)
#elif defined(LDD_LINUX)
    LinuxShell::PrintAbort(m_session);
#endif
    ReleaseSession();
#endif
    m_inPage = false;
    m_inDocument = false;
}

// Mac 0x1d63b0, Win 0x53b730
void PrintJob::BeginPage()
{
#if defined(_WIN32)
    ::StartPage(m_dc);
#elif defined(__OBJC__)
    PMSessionBeginPage((PMPrintSession)m_session, (PMPageFormat)m_pageFormat, 0);
    CGContextRef context = 0;
    PMSessionGetCGGraphicsContext((PMPrintSession)m_session, &context);
    m_context = context;
    PMGetAdjustedPaperRect((PMPageFormat)m_pageFormat, (PMRect*)&m_paperRect);
    PMGetAdjustedPageRect((PMPageFormat)m_pageFormat, (PMRect*)&m_pageRect);
    // origin at the top-left of the printable area
    CGContextTranslateCTM(context, (CGFloat)(float)-m_paperRect.left,
                          (CGFloat)(float)(m_paperRect.bottom - m_pageRect.bottom));
    CGContextClipToRect(context, CGRectMake(0, 0, (float)m_pageRect.right, (float)m_pageRect.bottom));
#elif defined(LDD_LINUX)
    LinuxShell::PrintBeginPage(m_session);
#endif
    m_inPage = true;
}

// Mac 0x1d64a0, Win 0x53b750
void PrintJob::EndPage()
{
#if defined(_WIN32)
    ::EndPage(m_dc);
#elif defined(__OBJC__)
    PMSessionEndPage((PMPrintSession)m_session);
#elif defined(LDD_LINUX)
    LinuxShell::PrintEndPage(m_session);
#endif
    m_inPage = false;
}

// Mac 0x1d64c0, Win 0x53b770
void PrintJob::GetPageSize(IntVector2& size)
{
#if defined(_WIN32)
    size.x = GetDeviceCaps(m_dc, HORZRES);
    size.y = GetDeviceCaps(m_dc, VERTRES);
#else
    PageRect page = { 0, 0, 0, 0 };
#if defined(__OBJC__)
    PMGetAdjustedPageRect((PMPageFormat)m_pageFormat, (PMRect*)&page);
#elif defined(LDD_LINUX)
    Int32 width = 0, height = 0;
    LinuxShell::PrintPageSize(m_session, width, height);
    page.right = width;
    page.bottom = height;
#endif
    size.x = (Int32)(page.right - page.left);
    size.y = (Int32)(page.bottom - page.top);
#endif
}

// Mac 0x1d6520, Win 0x53b7a0
bool PrintJob::DrawBitmap(const IntRect& destination, const Bitmap& bitmap, const IntRect& source)
{
#if defined(_WIN32)
    const UInt32 bits = bitmap.m_format & 0xff;
    const UInt32 rowBytes = bits < 8 ? (bitmap.m_width * bits + 7) >> 3 : (bits >> 3) * bitmap.m_width;
    if (rowBytes != (UInt32)bitmap.m_pitch || (Int32)bitmap.m_format < 0 ||
        (bitmap.m_format & 0xff00) == 0 || (bitmap.m_format & 0x40000000) != 0)
        return false;
    BITMAPINFO info;
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = (LONG)bitmap.m_width;
    info.bmiHeader.biHeight = -(LONG)bitmap.m_height;                // top-down
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = (WORD)bits;
    info.bmiHeader.biCompression = BI_RGB;
    info.bmiHeader.biSizeImage = 0;
    info.bmiHeader.biXPelsPerMeter = 5000;
    info.bmiHeader.biYPelsPerMeter = 5000;
    info.bmiHeader.biClrUsed = 0;
    info.bmiHeader.biClrImportant = 0;
    return StretchDIBits(m_dc, destination.m_left, destination.m_top,
                         destination.m_right - destination.m_left, destination.m_bottom - destination.m_top,
                         source.m_left, source.m_top, source.m_right - source.m_left, source.m_bottom - source.m_top,
                         bitmap.m_data, &info, DIB_RGB_COLORS, SRCCOPY) != (int)GDI_ERROR;
#else
    (void)source;
#if defined(__OBJC__)
    CGColorSpaceRef space = CGColorSpaceCreateWithName(kCGColorSpaceGenericRGB);
    CGDataProviderRef provider = CGDataProviderCreateWithData(0, bitmap.m_data,
                                                              bitmap.m_height * bitmap.m_pitch, 0);
    CGImageRef image = CGImageCreate(bitmap.m_width, bitmap.m_height, 8, 24, bitmap.m_pitch, space, 0,
                                     provider, 0, false, kCGRenderingIntentDefault);
    const float y = (float)m_pageRect.bottom - (float)destination.m_top - (float)destination.GetHeight();
    CGContextDrawImage((CGContextRef)m_context,
                       CGRectMake((float)destination.m_left, y, (float)destination.GetWidth(),
                                  (float)destination.GetHeight()),
                       image);
    CGImageRelease(image);
    CGDataProviderRelease(provider);
    CGColorSpaceRelease(space);
#elif defined(LDD_LINUX)
    return LinuxShell::PrintDrawBitmap(m_session, destination.m_left, destination.m_top, destination.GetWidth(),
                                       destination.GetHeight(), (const UInt8*)bitmap.m_data, bitmap.m_width,
                                       bitmap.m_height, bitmap.m_pitch, bitmap.m_format & 0xff);
#else
    (void)destination;
    (void)bitmap;   // HOST-ONLY: nothing to draw on
#endif
    return true;
#endif
}

} // namespace LEGO
