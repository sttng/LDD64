// LEGO Digital Designer — restored source
// LEGO::PrintJob — prints bitmaps page by page (building-instructions / model print of
// PrintCommand, TU 251).
// TU 119 "PrintJob.cpp": Mac 0x1d5fd0-0x1d67df (Carbon PMSession* + CGImage), Win
// 0x53b440-0x53b8ef (PrintDlgA / CreateDCA from the WIN.INI default device,
// StartDoc / StartPage, StretchDIBits). Mac system calls sit behind __OBJC__; HOST-ONLY builds
// of the Mac branch have no printer (the dialog reports "cancelled", no document starts).
//
// RTTI: vmi, virtual base Object. Vtables Mac 0x72173c {D1 0x1d6680, D0 0x1d66c0, OnFinalRelease
// 0x1d6750}, Object-in-PrintJob 0x721760 {thunks 0x1d66a0 0x1d6700 0x1d6770, AddRef, Release};
// Win 0xa2b16c (Object at +0x10) {sdd thunk 0x53b8f0 -> 0x53b4d0, OnFinalRelease thunk 0x53b900
// -> 0x53b8c0, AddRef, Release}. Sizes: Mac 0x60 (Object +0x58), Win 0x18 (Object +0x10).
// Method and member names inferred.
#pragma once
#include "LEGO/Types.h"
#include "Object.h"
#include "Geometry.h"

#if defined(_WIN32)
#include <windows.h>
#endif

namespace LEGO {

class String;
class Window;
class Bitmap;

class PrintJob : public virtual Object {
public:
    PrintJob();                                                      // Mac 0x1d5fd0  Win 0x53b440
    virtual ~PrintJob();                                             // Mac 0x1d6680/0x1d66c0 (thunks 0x1d66a0/0x1d6700)  Win 0x53b4d0

    // The system print dialog; `accepted`: the user pressed Print. False on an error (Win:
    // CommDlgExtendedError() != 0). Win: `parent` owns the dialog, the chosen printer's DC is kept.
    // Mac: `parent` unused; a print session is created first.
    bool ShowPrintDialog(Window* parent, bool& accepted);            // Mac 0x1d6030  Win 0x53b510
    // Starts the document `name` (Win: on the dialog's printer, else the WIN.INI default device).
    // NOTE (Win): the original returns nothing; this one returns whether the document started
    // (the state it sets).
    bool BeginDocument(const String& name);                          // Mac 0x1d6210  Win 0x53b5c0
    void EndDocument();                                              // Mac 0x1d62e0  Win 0x53b6d0
    void AbortDocument();                                            // Mac 0x1d6340  Win 0x53b700
    void BeginPage();                                                // Mac 0x1d63b0  Win 0x53b730
    void EndPage();                                                  // Mac 0x1d64a0  Win 0x53b750
    // Printable size in device units (Win HORZRES/VERTRES; Mac adjusted page rect, points).
    void GetPageSize(IntVector2& size);                              // Mac 0x1d64c0  Win 0x53b770
    // Draws `bitmap` into `destination` (device units, top-left origin). Win: the `source` rectangle,
    // StretchDIBits; only uncompressed true-colour bitmaps without alpha and with packed rows. Mac:
    // the whole bitmap as 24-bit RGB (`source` unused).
    bool DrawBitmap(const IntRect& destination, const Bitmap& bitmap, const IntRect& source); // Mac 0x1d6520  Win 0x53b7a0

    bool IsInDocument() const { return m_inDocument; }               // (accessors, names inferred)
    bool IsInPage() const { return m_inPage; }

protected:
    // Mac: nothing (Object's). Win: aborts an open document and deletes the DC.
    virtual void OnFinalRelease();                                   // Mac 0x1d6750 (thunk 0x1d6770)  Win 0x53b8c0 (thunk 0x53b900)

private:
    PrintJob(const PrintJob&);
    PrintJob& operator=(const PrintJob&);

#if !defined(_WIN32)
    struct PageRect { double top, left, bottom, right; };            // PMRect

    bool CreateSession();                                            // Mac 0x1d60d0
    void ReleaseSession();                                           // (inlined everywhere)
#endif

    bool     m_inDocument;      // Mac +0x04  Win +0x04 (after the vbptr)
    bool     m_inPage;          // Mac +0x05  Win +0x05
#if defined(_WIN32)
    HDC      m_dc;              // Win +0x08
#else
    void*    m_session;         // Mac +0x08 PMPrintSession
    void*    m_settings;        // Mac +0x0c PMPrintSettings
    void*    m_pageFormat;      // Mac +0x10 PMPageFormat
    void*    m_context;         // Mac +0x14 CGContextRef of the current page
    PageRect m_paperRect;       // Mac +0x18 adjusted paper rect
    PageRect m_pageRect;        // Mac +0x38 adjusted page rect
#endif
};

} // namespace LEGO
